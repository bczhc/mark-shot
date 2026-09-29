#include "ocr_table/ocr_table_layout.h"

#include <algorithm>

namespace markshot::ocr_table {
namespace {

using markshot::ocr::Token;

// 同行判断：两个文字块垂直重叠至少占较矮一方高度的比例
constexpr qreal kRowOverlapRatio = 0.5;
// 同一单元格内相邻文字块的最大间距，按行高的倍数计
constexpr qreal kCellGapFactor = 1.0;

/**
 * 一行中合并后的单元格。
 */
struct RowCell {
    qreal left = 0.0;
    qreal right = 0.0;
    QRectF lastRect;
    QString text;
};

/**
 * 表格列在水平方向上的范围。
 */
struct ColumnBand {
    qreal left = 0.0;
    qreal right = 0.0;
};

/**
 * 计算两个区间的重叠长度。
 * @return 重叠长度，不重叠时为 0。
 */
qreal overlapLength(qreal leftA, qreal rightA, qreal leftB, qreal rightB)
{
    return std::max<qreal>(0.0, std::min(rightA, rightB) - std::max(leftA, leftB));
}

/**
 * 按垂直重叠把文字块分组为行，行内按从左到右排序。
 * @param tokens 文字块。
 * @return 行列表。
 */
QVector<QVector<Token>> groupRows(QVector<Token> tokens)
{
    std::sort(tokens.begin(), tokens.end(), [](const Token &a, const Token &b) {
        return a.imageRect.center().y() < b.imageRect.center().y();
    });

    QVector<QVector<Token>> rows;
    qreal rowTop = 0.0;
    qreal rowBottom = 0.0;
    for (const Token &token : tokens) {
        const QRectF rect = token.imageRect;
        // 1. 与当前行的垂直范围重叠足够多时并入当前行
        if (!rows.isEmpty()) {
            const qreal rowHeight = rowBottom - rowTop;
            const qreal overlap = overlapLength(rowTop, rowBottom, rect.top(), rect.bottom());
            if (overlap >= kRowOverlapRatio * std::min(rowHeight, rect.height())) {
                rows.last().append(token);
                rowTop = std::min(rowTop, rect.top());
                rowBottom = std::max(rowBottom, rect.bottom());
                continue;
            }
        }
        // 2. 否则开启新行
        rows.append({token});
        rowTop = rect.top();
        rowBottom = rect.bottom();
    }

    for (QVector<Token> &row : rows) {
        std::sort(row.begin(), row.end(), [](const Token &a, const Token &b) {
            return a.imageRect.left() < b.imageRect.left();
        });
    }
    return rows;
}

/**
 * 把一行中间距较小的文字块合并为单元格。
 * @param row 已按从左到右排序的文字块。
 * @return 单元格列表。
 */
QVector<RowCell> mergeRowCells(const QVector<Token> &row)
{
    // 1. 以行内文字块高度的中位数作为间距基准
    QVector<qreal> heights;
    heights.reserve(row.size());
    for (const Token &token : row) {
        heights.append(token.imageRect.height());
    }
    std::sort(heights.begin(), heights.end());
    const qreal medianHeight = heights.isEmpty() ? 0.0 : heights.at(heights.size() / 2);
    const qreal maxGap = medianHeight * kCellGapFactor;

    // 2. 相邻文字块间距小于阈值时合并，文字按 OCR 规则决定是否补空格
    QVector<RowCell> cells;
    for (const Token &token : row) {
        const QRectF rect = token.imageRect;
        if (!cells.isEmpty() && rect.left() - cells.last().right <= maxGap) {
            RowCell &cell = cells.last();
            if (markshot::ocr::shouldInsertSpace(cell.text, token.text, cell.lastRect, rect)) {
                cell.text += QLatin1Char(' ');
            }
            cell.text += token.text;
            cell.right = std::max(cell.right, rect.right());
            cell.lastRect = rect;
            continue;
        }
        cells.append({rect.left(), rect.right(), rect, token.text});
    }
    return cells;
}

/**
 * 由多单元格行的水平范围投影出列。单单元格行（标题等）不参与，避免吞并所有列。
 * @param rows 各行单元格。
 * @return 从左到右的列范围。
 */
QVector<ColumnBand> buildColumnBands(const QVector<QVector<RowCell>> &rows)
{
    QVector<ColumnBand> intervals;
    for (const QVector<RowCell> &row : rows) {
        if (row.size() < 2) {
            continue;
        }
        for (const RowCell &cell : row) {
            intervals.append({cell.left, cell.right});
        }
    }
    if (intervals.isEmpty()) {
        for (const QVector<RowCell> &row : rows) {
            for (const RowCell &cell : row) {
                intervals.append({cell.left, cell.right});
            }
        }
    }
    std::sort(intervals.begin(), intervals.end(), [](const ColumnBand &a, const ColumnBand &b) {
        return a.left < b.left;
    });

    // 重叠区间合并为同一列
    QVector<ColumnBand> bands;
    for (const ColumnBand &interval : intervals) {
        if (!bands.isEmpty() && interval.left <= bands.last().right) {
            bands.last().right = std::max(bands.last().right, interval.right);
            continue;
        }
        bands.append(interval);
    }
    return bands;
}

/**
 * 找到与单元格重叠最多的列；都不重叠时取中心最近的列。
 * @param cell 单元格。
 * @param bands 列范围。
 * @return 列下标。
 */
int columnForCell(const RowCell &cell, const QVector<ColumnBand> &bands)
{
    int best = 0;
    qreal bestOverlap = -1.0;
    qreal bestDistance = 0.0;
    const qreal center = (cell.left + cell.right) / 2.0;
    for (int i = 0; i < bands.size(); ++i) {
        const qreal overlap = overlapLength(cell.left, cell.right, bands[i].left, bands[i].right);
        const qreal distance = std::abs(center - (bands[i].left + bands[i].right) / 2.0);
        if (overlap > bestOverlap || (qFuzzyIsNull(overlap) && qFuzzyIsNull(bestOverlap) && distance < bestDistance)) {
            best = i;
            bestOverlap = overlap;
            bestDistance = distance;
        }
    }
    return best;
}

/**
 * 处理 CSV 字段转义。
 * @param value 单元格文本。
 * @return 转义后的字段。
 */
QString csvField(QString value)
{
    if (!value.contains(QLatin1Char(',')) && !value.contains(QLatin1Char('"'))
        && !value.contains(QLatin1Char('\n')) && !value.contains(QLatin1Char('\r'))) {
        return value;
    }
    value.replace(QStringLiteral("\""), QStringLiteral("\"\""));
    return QLatin1Char('"') + value + QLatin1Char('"');
}

/**
 * 处理 Markdown 单元格转义：竖线转义，换行替换为空格。
 * @param value 单元格文本。
 * @return 转义后的文本。
 */
QString markdownCell(QString value)
{
    value.replace(QLatin1Char('|'), QStringLiteral("\\|"));
    value.replace(QLatin1Char('\n'), QLatin1Char(' '));
    return value;
}

}  // namespace

OcrTable inferTable(const QVector<Token> &tokens)
{
    // 1. 丢弃空文本和无效位置
    QVector<Token> valid;
    valid.reserve(tokens.size());
    for (const Token &token : tokens) {
        if (!token.text.trimmed().isEmpty() && token.imageRect.isValid() && !token.imageRect.isEmpty()) {
            valid.append(token);
        }
    }
    if (valid.isEmpty()) {
        return {};
    }

    // 2. 分行并合并单元格
    QVector<QVector<RowCell>> rows;
    for (const QVector<Token> &row : groupRows(valid)) {
        rows.append(mergeRowCells(row));
    }

    // 3. 投影出列并把单元格放入网格，同列多个单元格用空格拼接
    const QVector<ColumnBand> bands = buildColumnBands(rows);
    OcrTable table;
    table.cells.reserve(rows.size());
    for (const QVector<RowCell> &row : rows) {
        QStringList line;
        for (int i = 0; i < bands.size(); ++i) {
            line.append(QString());
        }
        for (const RowCell &cell : row) {
            QString &target = line[columnForCell(cell, bands)];
            target = target.isEmpty() ? cell.text : target + QLatin1Char(' ') + cell.text;
        }
        table.cells.append(line);
    }
    return table;
}

bool looksLikeTable(const OcrTable &table)
{
    if (table.rowCount() < 2 || table.columnCount() < 2) {
        return false;
    }
    const int halfColumns = (table.columnCount() + 1) / 2;
    int denseRows = 0;
    for (const QStringList &row : table.cells) {
        const int filled = static_cast<int>(std::count_if(row.begin(), row.end(), [](const QString &cell) {
            return !cell.isEmpty();
        }));
        if (filled >= 2 && filled >= halfColumns) {
            ++denseRows;
        }
    }
    return denseRows >= std::max(2, (table.rowCount() + 1) / 2);
}

QString toTsv(const OcrTable &table)
{
    QStringList lines;
    for (const QStringList &row : table.cells) {
        QStringList fields;
        for (QString cell : row) {
            cell.replace(QLatin1Char('\t'), QLatin1Char(' '));
            cell.replace(QLatin1Char('\n'), QLatin1Char(' '));
            fields.append(cell);
        }
        lines.append(fields.join(QLatin1Char('\t')));
    }
    return lines.join(QLatin1Char('\n'));
}

QString toCsv(const OcrTable &table)
{
    QStringList lines;
    for (const QStringList &row : table.cells) {
        QStringList fields;
        for (const QString &cell : row) {
            fields.append(csvField(cell));
        }
        lines.append(fields.join(QLatin1Char(',')));
    }
    return lines.join(QStringLiteral("\r\n"));
}

QString toMarkdown(const OcrTable &table)
{
    if (table.rowCount() == 0 || table.columnCount() == 0) {
        return {};
    }
    auto formatRow = [](const QStringList &row) {
        QStringList cells;
        for (const QString &cell : row) {
            cells.append(markdownCell(cell));
        }
        return QStringLiteral("| ") + cells.join(QStringLiteral(" | ")) + QStringLiteral(" |");
    };

    QStringList lines;
    lines.append(formatRow(table.cells.first()));
    QStringList separator;
    for (int i = 0; i < table.columnCount(); ++i) {
        separator.append(QStringLiteral("---"));
    }
    lines.append(QStringLiteral("| ") + separator.join(QStringLiteral(" | ")) + QStringLiteral(" |"));
    for (int row = 1; row < table.rowCount(); ++row) {
        lines.append(formatRow(table.cells.at(row)));
    }
    return lines.join(QLatin1Char('\n'));
}

QString toHtml(const OcrTable &table)
{
    QString html = QStringLiteral("<table>\n");
    for (const QStringList &row : table.cells) {
        html += QStringLiteral("  <tr>");
        for (const QString &cell : row) {
            html += QStringLiteral("<td>") + cell.toHtmlEscaped() + QStringLiteral("</td>");
        }
        html += QStringLiteral("</tr>\n");
    }
    html += QStringLiteral("</table>");
    return html;
}

}  // namespace markshot::ocr_table
