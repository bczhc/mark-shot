#include "ocr_table/ocr_table_layout.h"

#include <QtTest/QtTest>

namespace {

/**
 * 构造测试用文字块。
 * @param text 文本。
 * @param x 左边。
 * @param y 上边。
 * @param width 宽度。
 * @param height 高度。
 * @return 文字块。
 */
markshot::ocr::Token token(const QString &text, qreal x, qreal y, qreal width = 60, qreal height = 20)
{
    markshot::ocr::Token result;
    result.text = text;
    result.imageRect = QRectF(x, y, width, height);
    return result;
}

}  // namespace

class OcrTableLayoutTest final : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证规整的三行三列文字块还原为 3x3 表格，且顺序与位置无关。
     * @return 无返回值。
     */
    void infersRegularGrid()
    {
        const QVector<markshot::ocr::Token> tokens = {
            token(QStringLiteral("C3"), 300, 82), token(QStringLiteral("A1"), 10, 10),
            token(QStringLiteral("B1"), 150, 12), token(QStringLiteral("C1"), 300, 9),
            token(QStringLiteral("A2"), 10, 46), token(QStringLiteral("B2"), 150, 45),
            token(QStringLiteral("C2"), 300, 47), token(QStringLiteral("A3"), 10, 80),
            token(QStringLiteral("B3"), 150, 81),
        };
        const markshot::ocr_table::OcrTable table = markshot::ocr_table::inferTable(tokens);
        QCOMPARE(table.rowCount(), 3);
        QCOMPARE(table.columnCount(), 3);
        QCOMPARE(table.cells.at(0), QStringList({QStringLiteral("A1"), QStringLiteral("B1"), QStringLiteral("C1")}));
        QCOMPARE(table.cells.at(2), QStringList({QStringLiteral("A3"), QStringLiteral("B3"), QStringLiteral("C3")}));
        QVERIFY(markshot::ocr_table::looksLikeTable(table));
    }

    /**
     * 验证同一单元格内的多个单词会被合并，而不是拆成多列。
     * @return 无返回值。
     */
    void mergesWordsInsideCell()
    {
        const QVector<markshot::ocr::Token> tokens = {
            token(QStringLiteral("First"), 10, 10, 40), token(QStringLiteral("Name"), 58, 10, 40),
            token(QStringLiteral("Age"), 200, 10, 40),
            token(QStringLiteral("Alice"), 10, 40, 40), token(QStringLiteral("30"), 200, 40, 30),
        };
        const markshot::ocr_table::OcrTable table = markshot::ocr_table::inferTable(tokens);
        QCOMPARE(table.columnCount(), 2);
        QCOMPARE(table.cells.at(0).at(0), QStringLiteral("First Name"));
        QCOMPARE(table.cells.at(1).at(1), QStringLiteral("30"));
    }

    /**
     * 验证缺失的单元格留空，跨列标题不会把所有列合并成一列。
     * @return 无返回值。
     */
    void keepsEmptyCellsAndIgnoresSpanningTitle()
    {
        const QVector<markshot::ocr::Token> tokens = {
            token(QStringLiteral("Quarterly report"), 10, 0, 380),
            token(QStringLiteral("Q1"), 10, 40), token(QStringLiteral("10"), 150, 40), token(QStringLiteral("20"), 300, 40),
            token(QStringLiteral("Q2"), 10, 80), token(QStringLiteral("30"), 300, 80),
        };
        const markshot::ocr_table::OcrTable table = markshot::ocr_table::inferTable(tokens);
        QCOMPARE(table.rowCount(), 3);
        QCOMPARE(table.columnCount(), 3);
        QCOMPARE(table.cells.at(2).at(1), QString());
        QCOMPARE(table.cells.at(2).at(2), QStringLiteral("30"));
    }

    /**
     * 验证普通段落文本不会被误判为表格。
     * @return 无返回值。
     */
    void plainParagraphIsNotTable()
    {
        const QVector<markshot::ocr::Token> tokens = {
            token(QStringLiteral("This is a normal line of text"), 10, 10, 400),
            token(QStringLiteral("and another line below it"), 10, 40, 380),
            token(QStringLiteral("with no columns at all"), 10, 70, 360),
        };
        QVERIFY(!markshot::ocr_table::looksLikeTable(markshot::ocr_table::inferTable(tokens)));
        QVERIFY(!markshot::ocr_table::looksLikeTable(markshot::ocr_table::inferTable({})));
    }

    /**
     * 验证各导出格式的分隔符与转义规则。
     * @return 无返回值。
     */
    void exportsEscapeSpecialCharacters()
    {
        markshot::ocr_table::OcrTable table;
        table.cells = {
            {QStringLiteral("Name"), QStringLiteral("Note")},
            {QStringLiteral("a|b"), QStringLiteral("say \"hi\", <ok>")},
        };
        QCOMPARE(markshot::ocr_table::toTsv(table), QStringLiteral("Name\tNote\na|b\tsay \"hi\", <ok>"));
        QCOMPARE(markshot::ocr_table::toCsv(table),
                 QStringLiteral("Name,Note\r\na|b,\"say \"\"hi\"\", <ok>\""));
        QCOMPARE(markshot::ocr_table::toMarkdown(table),
                 QStringLiteral("| Name | Note |\n| --- | --- |\n| a\\|b | say \"hi\", <ok> |"));
        QVERIFY(markshot::ocr_table::toHtml(table).contains(QStringLiteral("<td>say &quot;hi&quot;, &lt;ok&gt;</td>")));
    }
};

QTEST_GUILESS_MAIN(OcrTableLayoutTest)

#include "ocr_table_layout_test.moc"
