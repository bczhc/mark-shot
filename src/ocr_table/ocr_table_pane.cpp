#include "ocr_table/ocr_table_pane.h"

#include "ui/i18n.h"
#include "ui/theme.h"

#include <QBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>

#include <functional>

namespace markshot::ocr_table {

OcrTablePane::OcrTablePane(const OcrTable &table, QWidget *parent)
    : QFrame(parent)
{
    setProperty("ocrPane", true);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 2, 0, 0);
    layout->setSpacing(6);

    // 1. 【OCR】【表格】标题行：行列数与复制入口
    auto *header = new QHBoxLayout;
    header->setSpacing(6);
    auto *title = new QLabel(MS_TR("Table"), this);
    title->setProperty("role", QStringLiteral("sectionTitle"));
    title->setFont(markshot::theme::uiFont(10, QFont::DemiBold));
    header->addWidget(title);
    auto *summary = new QLabel(MS_TR("%1 rows × %2 columns").arg(table.rowCount()).arg(table.columnCount()), this);
    summary->setProperty("role", QStringLiteral("muted"));
    summary->setFont(markshot::theme::uiFont(9));
    header->addWidget(summary, 1);
    m_copyButton = new QPushButton(MS_TR("Copy"), this);
    m_copyButton->setObjectName(QStringLiteral("ocrTableCopyButton"));
    m_copyButton->setProperty("role", QStringLiteral("quiet"));
    m_copyButton->setFont(markshot::theme::uiFont(10));
    m_copyButton->setMinimumWidth(68);
    m_copyButton->setToolTip(MS_TR("Copy table"));
    m_copyButton->setAccessibleName(MS_TR("Copy table"));
    header->addWidget(m_copyButton);
    layout->addLayout(header);

    // 2. 【OCR】【表格】可编辑网格，识别错误可以直接修正后再复制
    m_table = new QTableWidget(table.rowCount(), table.columnCount(), this);
    m_table->setObjectName(QStringLiteral("ocrTable"));
    m_table->setFont(markshot::theme::uiFont(10));
    m_table->setAccessibleName(MS_TR("Recognized table"));
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setDefaultSectionSize(m_table->fontMetrics().height() + 10);
    for (int row = 0; row < table.rowCount(); ++row) {
        for (int column = 0; column < table.columnCount(); ++column) {
            m_table->setItem(row, column, new QTableWidgetItem(table.cells.at(row).at(column)));
        }
    }
    m_table->resizeColumnsToContents();
    layout->addWidget(m_table, 1);

    // 3. 【OCR】【表格】复制菜单：默认 TSV，可粘贴到电子表格
    auto *menu = new QMenu(m_copyButton);
    using Exporter = std::function<QString(const OcrTable &)>;
    const QList<QPair<QString, Exporter>> formats = {
        {MS_TR("Copy as spreadsheet (TSV)"), toTsv},
        {MS_TR("Copy as Markdown"), toMarkdown},
        {MS_TR("Copy as CSV"), toCsv},
        {MS_TR("Copy as HTML"), toHtml},
    };
    for (const auto &format : formats) {
        const Exporter exporter = format.second;
        menu->addAction(format.first, this, [this, exporter] {
            emit copyRequested(exporter(currentTable()));
        });
    }
    m_copyButton->setMenu(menu);

    m_copyTimer = new QTimer(this);
    m_copyTimer->setSingleShot(true);
    connect(m_copyTimer, &QTimer::timeout, this, [this] { m_copyButton->setText(MS_TR("Copy")); });
}

OcrTable OcrTablePane::currentTable() const
{
    OcrTable table;
    table.cells.reserve(m_table->rowCount());
    for (int row = 0; row < m_table->rowCount(); ++row) {
        QStringList line;
        for (int column = 0; column < m_table->columnCount(); ++column) {
            const QTableWidgetItem *item = m_table->item(row, column);
            line.append(item ? item->text() : QString());
        }
        table.cells.append(line);
    }
    return table;
}

void OcrTablePane::showCopyFeedback(bool success)
{
    m_copyButton->setText(success ? MS_TR("Copied") : MS_TR("Copy failed"));
    m_copyTimer->start(1600);
}

}  // namespace markshot::ocr_table
