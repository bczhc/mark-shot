#include "selection_aspect/selection_size_panel.h"

#include "selection_aspect/selection_aspect_ratio.h"
#include "ui/i18n.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>

#include <cmath>

namespace markshot::selection_aspect {
namespace {

// 尺寸输入框样式，与属性面板的下拉框保持同一配色
const char *kSpinStyle =
    "QSpinBox {"
    " color: #E5E7EB;"
    " background: rgba(255, 255, 255, 16);"
    " border: 1px solid rgba(255, 255, 255, 24);"
    " border-radius: 7px;"
    " padding: 3px 6px;"
    "}"
    "QSpinBox:focus { border-color: rgba(45, 212, 191, 160); }";

/**
 * 创建宽或高输入框。
 * @param accessibleName 无障碍名称。
 * @param parent 父控件。
 * @return 输入框。
 */
QSpinBox *createDimensionSpin(const QString &accessibleName, QWidget *parent)
{
    auto *spin = new QSpinBox(parent);
    spin->setRange(1, 1);
    spin->setSuffix(QStringLiteral(" px"));
    spin->setAccessibleName(accessibleName);
    spin->setToolTip(accessibleName);
    spin->setFocusPolicy(Qt::StrongFocus);
    spin->setKeyboardTracking(true);
    spin->setMinimumWidth(spin->fontMetrics().horizontalAdvance(QStringLiteral("00000 px")) + 28);
    return spin;
}

}  // namespace

SelectionSizePanel::SelectionSizePanel(const QString &styleSheet, QWidget *parent)
    : QWidget(parent)
{
    // 1. 复用工具栏面板样式，保持截图界面紧凑一致
    setObjectName(QStringLiteral("annotationPropertyPanel"));
    setAttribute(Qt::WA_StyledBackground, true);
    setCursor(Qt::ArrowCursor);
    setStyleSheet(styleSheet + QString::fromLatin1(kSpinStyle));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(6);

    // 2. 宽 × 高 输入
    m_widthSpin = createDimensionSpin(MS_TR("Selection width"), this);
    m_heightSpin = createDimensionSpin(MS_TR("Selection height"), this);
    auto *times = new QLabel(QStringLiteral("×"), this);
    times->setObjectName(QStringLiteral("propertyValue"));
    layout->addWidget(m_widthSpin);
    layout->addWidget(times);
    layout->addWidget(m_heightSpin);

    // 3. 比例预设
    m_ratioCombo = new QComboBox(this);
    m_ratioCombo->setFocusPolicy(Qt::StrongFocus);
    m_ratioCombo->setToolTip(MS_TR("Lock aspect ratio"));
    m_ratioCombo->setAccessibleName(MS_TR("Lock aspect ratio"));
    for (const AspectRatioPreset &preset : aspectRatioPresets()) {
        m_ratioCombo->addItem(markshot::i18n::translate(preset.label), preset.ratio);
    }
    layout->addWidget(m_ratioCombo);

    // 4. 应用按钮
    m_applyButton = new QPushButton(MS_TR("Apply"), this);
    m_applyButton->setFocusPolicy(Qt::StrongFocus);
    m_applyButton->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_applyButton);

    connect(m_widthSpin, &QSpinBox::valueChanged, this, [this] { syncLinkedValue(true); });
    connect(m_heightSpin, &QSpinBox::valueChanged, this, [this] { syncLinkedValue(false); });
    connect(m_ratioCombo, QOverload<int>::of(&QComboBox::activated), this, [this] {
        syncLinkedValue(true);
        emit aspectRatioChanged(currentRatio());
    });
    connect(m_applyButton, &QPushButton::clicked, this, &SelectionSizePanel::applyValues);
}

void SelectionSizePanel::setValues(QSize size, QSize maximum, qreal ratio)
{
    m_syncing = true;
    const QSignalBlocker widthBlocker(m_widthSpin);
    const QSignalBlocker heightBlocker(m_heightSpin);
    const QSignalBlocker ratioBlocker(m_ratioCombo);
    m_widthSpin->setRange(1, std::max(1, maximum.width()));
    m_heightSpin->setRange(1, std::max(1, maximum.height()));
    m_widthSpin->setValue(size.width());
    m_heightSpin->setValue(size.height());

    // 1. 按比例值匹配预设，找不到时回到自由比例
    int ratioIndex = 0;
    for (int i = 0; i < m_ratioCombo->count(); ++i) {
        if (std::abs(m_ratioCombo->itemData(i).toDouble() - ratio) < 1e-6) {
            ratioIndex = i;
            break;
        }
    }
    m_ratioCombo->setCurrentIndex(ratioIndex);
    m_syncing = false;
}

void SelectionSizePanel::focusWidth()
{
    m_widthSpin->setFocus(Qt::OtherFocusReason);
    m_widthSpin->selectAll();
}

void SelectionSizePanel::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        applyValues();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        emit closeRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

qreal SelectionSizePanel::currentRatio() const
{
    return m_ratioCombo->currentData().toDouble();
}

void SelectionSizePanel::syncLinkedValue(bool widthChanged)
{
    const qreal ratio = currentRatio();
    if (m_syncing || ratio <= 0.0) {
        return;
    }
    m_syncing = true;
    if (widthChanged) {
        m_heightSpin->setValue(selection_aspect::heightForWidth(m_widthSpin->value(), ratio));
    } else {
        m_widthSpin->setValue(selection_aspect::widthForHeight(m_heightSpin->value(), ratio));
    }
    m_syncing = false;
}

void SelectionSizePanel::applyValues()
{
    // 编辑中的数字需要先提交，否则 Enter 时读到旧值
    m_widthSpin->interpretText();
    m_heightSpin->interpretText();
    emit sizeApplied(QSize(m_widthSpin->value(), m_heightSpin->value()));
}

}  // namespace markshot::selection_aspect
