#pragma once

#include "ocr_result.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace markshot::ocr_table {

/**
 * 由 OCR 文字块推断出的表格。cells[row][column] 为单元格文本，缺失单元格为空串。
 */
struct OcrTable {
    QVector<QStringList> cells;

    /**
     * 表格行数。
     * @return 行数。
     */
    int rowCount() const { return cells.size(); }

    /**
     * 表格列数。
     * @return 列数，空表返回 0。
     */
    int columnCount() const { return cells.isEmpty() ? 0 : cells.first().size(); }
};

/**
 * 根据文字块位置推断表格结构。
 * 先按垂直重叠分行，再把同一行里间距较小的文字块合并为单元格，
 * 最后把各行单元格的水平范围投影合并为列。
 * @param tokens OCR 文字块，坐标为同一张图片的像素坐标。
 * @return 推断出的表格，文字块为空时返回空表。
 */
OcrTable inferTable(const QVector<markshot::ocr::Token> &tokens);

/**
 * 判断推断结果是否像一张表格：至少两行两列，且多数行占满一半以上的列。
 * @param table 推断出的表格。
 * @return 像表格时返回 true。
 */
bool looksLikeTable(const OcrTable &table);

/**
 * 导出为制表符分隔文本，可直接粘贴到电子表格。
 * @param table 表格。
 * @return TSV 文本。
 */
QString toTsv(const OcrTable &table);

/**
 * 导出为 CSV 文本，按 RFC 4180 转义引号、逗号与换行。
 * @param table 表格。
 * @return CSV 文本。
 */
QString toCsv(const OcrTable &table);

/**
 * 导出为 Markdown 表格，第一行作为表头。
 * @param table 表格。
 * @return Markdown 文本。
 */
QString toMarkdown(const OcrTable &table);

/**
 * 导出为 HTML 表格，单元格文本做 HTML 转义。
 * @param table 表格。
 * @return HTML 文本。
 */
QString toHtml(const OcrTable &table);

}  // namespace markshot::ocr_table
