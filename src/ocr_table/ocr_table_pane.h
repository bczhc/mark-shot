#pragma once

#include "ocr_table/ocr_table_layout.h"

#include <QFrame>

class QPushButton;
class QTableWidget;
class QTimer;

namespace markshot::ocr_table {

/**
 * OCR 结果窗口的「表格」页：可编辑的单元格网格，并按多种格式复制。
 */
class OcrTablePane final : public QFrame {
    Q_OBJECT

public:
    /**
     * 创建表格页。
     * @param table 推断出的表格。
     * @param parent 父控件。
     */
    explicit OcrTablePane(const OcrTable &table, QWidget *parent = nullptr);

    /**
     * 读取用户编辑后的表格内容。
     * @return 当前表格。
     */
    OcrTable currentTable() const;

    /**
     * 在复制按钮上显示结果，稍后恢复文案。
     * @param success 剪贴板写入是否成功。
     * @return 无返回值。
     */
    void showCopyFeedback(bool success);

signals:
    /**
     * 请求窗口通过剪贴板服务复制文本。
     * @param text 已按所选格式导出的文本。
     */
    void copyRequested(const QString &text);

private:
    QTableWidget *m_table = nullptr;
    QPushButton *m_copyButton = nullptr;
    QTimer *m_copyTimer = nullptr;
};

}  // namespace markshot::ocr_table
