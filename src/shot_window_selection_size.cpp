#include "shot_window_module.h"

#include "selection_aspect/selection_aspect_ratio.h"
#include "selection_aspect/selection_size_panel.h"

using namespace markshot::shot;

namespace aspect = markshot::selection_aspect;

namespace {

// 尺寸面板与选区之间的间距
constexpr int kSizePanelGap = 8;

}  // namespace

QRectF ShotWindow::creationSelectionRect(QPointF anchor, QPointF pointer, bool squareModifier) const
{
    // 锁定比例优先；未锁定时按住 Shift 拖出正方形
    const qreal ratio = m_selectionAspectRatio > 0.0 ? m_selectionAspectRatio : (squareModifier ? 1.0 : 0.0);
    if (ratio <= 0.0) {
        return normalizedRect(anchor, pointer);
    }
    return aspect::constrainedCreationRect(anchor, pointer, ratio, m_frozenFrame.size());
}

QRectF ShotWindow::aspectLockedSelection(QRectF before, SelectionDrag handle, QRectF adjusted) const
{
    if (m_selectionAspectRatio <= 0.0) {
        return adjusted;
    }
    return aspect::constrainedAdjustedRect(before, handle, adjusted, m_selectionAspectRatio, m_frozenFrame.size());
}

void ShotWindow::toggleSelectionSizePanel()
{
    // 1. 已显示时关闭并把键盘焦点还给截图窗口
    if (m_selectionSizePanel && m_selectionSizePanel->isVisible()) {
        m_selectionSizePanel->hide();
        setFocus(Qt::OtherFocusReason);
        return;
    }
    if (m_mode != Mode::Editing || m_fullscreenAnnotation || !hasUsableSelection()) {
        return;
    }

    // 2. 首次打开时创建面板并连接信号
    auto *panel = qobject_cast<aspect::SelectionSizePanel *>(m_selectionSizePanel);
    if (!panel) {
        panel = new aspect::SelectionSizePanel(m_toolbar ? m_toolbar->styleSheet() : QString(), this);
        connect(panel, &aspect::SelectionSizePanel::sizeApplied, this, [this](QSize size) {
            applySelectionSize(size);
            m_selectionSizePanel->hide();
            setFocus(Qt::OtherFocusReason);
        });
        connect(panel, &aspect::SelectionSizePanel::aspectRatioChanged, this, &ShotWindow::setSelectionAspectRatio);
        connect(panel, &aspect::SelectionSizePanel::closeRequested, this, [this] {
            m_selectionSizePanel->hide();
            setFocus(Qt::OtherFocusReason);
        });
        m_selectionSizePanel = panel;
    }

    // 3. 同步当前尺寸后显示，并让宽度输入框获得焦点
    const QRect selection = normalizedSelection().toAlignedRect();
    panel->setValues(selection.size(), m_frozenFrame.size(), m_selectionAspectRatio);
    panel->adjustSize();
    updateSelectionSizePanelGeometry();
    panel->show();
    panel->raise();
    panel->focusWidth();
}

void ShotWindow::updateSelectionSizePanelGeometry()
{
    if (!m_selectionSizePanel) {
        return;
    }
    const QRect widgetSelection = imageRectToWidget(normalizedSelection()).toAlignedRect();
    const QSize panelSize = m_selectionSizePanel->sizeHint();

    // 优先放在选区左下方；下方放不下时放到选区左上角内侧
    int x = widgetSelection.left();
    int y = widgetSelection.bottom() + kSizePanelGap;
    if (y + panelSize.height() > height()) {
        y = widgetSelection.top() + kSizePanelGap;
        x = widgetSelection.left() + kSizePanelGap;
    }
    x = std::clamp(x, 0, std::max(0, width() - panelSize.width()));
    y = std::clamp(y, 0, std::max(0, height() - panelSize.height()));
    m_selectionSizePanel->setGeometry(QRect(QPoint(x, y), panelSize));
}

void ShotWindow::setSelectionAspectRatio(qreal ratio)
{
    m_selectionAspectRatio = std::max<qreal>(0.0, ratio);
    if (m_selectionAspectRatio <= 0.0 || !hasUsableSelection()) {
        return;
    }

    // 以左上角为基准保持宽度按新比例改高度，放不下时再按高度收缩宽度
    const QRectF current = normalizedSelection();
    int width = qRound(current.width());
    int height = aspect::heightForWidth(width, m_selectionAspectRatio);
    if (height > m_frozenFrame.height()) {
        height = m_frozenFrame.height();
        width = aspect::widthForHeight(height, m_selectionAspectRatio);
    }
    applySelectionSize(QSize(width, height));
    if (auto *panel = qobject_cast<aspect::SelectionSizePanel *>(m_selectionSizePanel)) {
        panel->setValues(normalizedSelection().toAlignedRect().size(), m_frozenFrame.size(), m_selectionAspectRatio);
    }
}

void ShotWindow::applySelectionSize(QSize size)
{
    if (!hasUsableSelection() || size.isEmpty()) {
        return;
    }
    const QRectF resized = aspect::resizedSelection(normalizedSelection(), size, m_frozenFrame.size());
    if (resized.width() < kMinSelectionSize || resized.height() < kMinSelectionSize) {
        showToast(MS_TR("Selection is too small"), 1400);
        return;
    }
    m_selection = resized;
    refreshAdjustedSelection();
    updateSelectionSizePanelGeometry();
}
