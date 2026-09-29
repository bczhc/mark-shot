#pragma once

#include <QSize>
#include <QWidget>

class QComboBox;
class QPushButton;
class QSpinBox;

namespace markshot::selection_aspect {

/**
 * 截图选区尺寸面板：输入精确宽高并选择锁定比例。
 * 数值单位为截图图像像素，与选区尺寸提示一致。
 */
class SelectionSizePanel final : public QWidget {
    Q_OBJECT

public:
    /**
     * 创建尺寸面板。
     * @param styleSheet 与工具栏一致的样式表。
     * @param parent 父控件，通常是截图窗口。
     */
    explicit SelectionSizePanel(const QString &styleSheet, QWidget *parent = nullptr);

    /**
     * 同步当前选区尺寸与可输入上限，不触发信号。
     * @param size 当前选区尺寸。
     * @param maximum 图像尺寸，作为输入上限。
     * @param ratio 当前锁定比例，0 表示自由。
     * @return 无返回值。
     */
    void setValues(QSize size, QSize maximum, qreal ratio);

    /**
     * 让宽度输入框获得焦点并全选，方便直接输入。
     * @return 无返回值。
     */
    void focusWidth();

signals:
    /**
     * 用户确认新的尺寸。
     * @param size 目标尺寸（图像像素）。
     */
    void sizeApplied(QSize size);

    /**
     * 用户切换锁定比例。
     * @param ratio 宽高比，0 表示自由比例。
     */
    void aspectRatioChanged(qreal ratio);

    /**
     * 用户按 Esc 请求关闭面板。
     */
    void closeRequested();

protected:
    /**
     * 处理 Enter 确认与 Esc 关闭。
     * @param event 键盘事件。
     * @return 无返回值。
     */
    void keyPressEvent(QKeyEvent *event) override;

private:
    /**
     * 当前选中的锁定比例。
     * @return 宽高比，0 表示自由比例。
     */
    qreal currentRatio() const;

    /**
     * 锁定比例时按改动的一侧联动另一侧。
     * @param widthChanged true 表示宽度被修改。
     * @return 无返回值。
     */
    void syncLinkedValue(bool widthChanged);

    /**
     * 发出 sizeApplied 信号。
     * @return 无返回值。
     */
    void applyValues();

    QSpinBox *m_widthSpin = nullptr;
    QSpinBox *m_heightSpin = nullptr;
    QComboBox *m_ratioCombo = nullptr;
    QPushButton *m_applyButton = nullptr;
    bool m_syncing = false;
};

}  // namespace markshot::selection_aspect
