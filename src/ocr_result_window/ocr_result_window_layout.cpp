#include "ocr_result_window/ocr_result_window.h"

#include "app_config_store.h"
#include "ocr_result_window/ocr_result_window_style.h"
#include "ocr_result_window/ocr_text_pane.h"
#include "pinned_window_top.h"
#include "settings/settings_design_tokens.h"
#include "ui/i18n.h"
#include "ui/interface_theme_config.h"
#include "ui/theme.h"
#include "ui/window_resize_grip.h"

#include <QBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QShortcut>
#include <QStyleOption>
#include <QTabBar>
#include <QTextEdit>
#include <QTimer>

namespace markshot::shot {

void OcrResultWindow::initializeUi(const QString &text,
                                   QImage sourceImage,
                                   const QVector<markshot::ocr::Token> &tokens)
{
    setFont(markshot::theme::uiFont(10));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 8);
    layout->setSpacing(6);

    // 1. 【OCR】【顶部操作】页面入口和翻译按钮共用标题栏，保留主要空间给内容
    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName(QStringLiteral("ocrTitleBar"));
    m_titleBar->setCursor(Qt::OpenHandCursor);
    auto *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(6);
    m_titleIcon = new QLabel(m_titleBar);
    m_titleIcon->setFixedSize(22, 22);
    m_titleIcon->setAccessibleName(MS_TR("Text Recognition"));
    m_titleIcon->setAttribute(Qt::WA_TransparentForMouseEvents);
    titleLayout->addWidget(m_titleIcon);
    m_viewTabs = new QTabBar(m_titleBar);
    m_viewTabs->setObjectName(QStringLiteral("ocrViewTabs"));
    m_viewTabs->setFont(markshot::theme::uiFont(9));
    m_viewTabs->setExpanding(false);
    m_viewTabs->setDrawBase(false);
    m_viewTabs->setFocusPolicy(Qt::StrongFocus);
    m_viewTabs->addTab(MS_TR("Text"));
    titleLayout->addWidget(m_viewTabs);
    titleLayout->addStretch(1);
    m_translationToggle = new QPushButton(MS_TR("Translate"), m_titleBar);
    m_translationToggle->setObjectName(QStringLiteral("ocrTranslationToggle"));
    m_translationToggle->setProperty("role", QStringLiteral("quiet"));
    m_translationToggle->setCheckable(true);
    m_translationToggle->setFont(markshot::theme::uiFont(9));
    m_translationToggle->setToolTip(MS_TR("Translate") + QStringLiteral(" (Ctrl+Enter)"));
    titleLayout->addWidget(m_translationToggle);
    m_moreButton = new QPushButton(m_titleBar);
    m_moreButton->setObjectName(QStringLiteral("ocrMoreButton"));
    m_moreButton->setAccessibleName(MS_TR("More actions"));
    m_moreButton->setToolTip(MS_TR("More actions"));
    m_pinButton = new QPushButton(m_titleBar);
    m_pinButton->setObjectName(QStringLiteral("ocrPinButton"));
    m_pinButton->setCheckable(true);
    m_pinButton->setChecked(m_alwaysOnTop);
    m_pinButton->setAccessibleName(MS_TR("Always on Top"));
    m_pinButton->setToolTip(m_alwaysOnTop ? MS_TR("Always on Top: On") : MS_TR("Always on Top: Off"));
    m_closeButton = new QPushButton(m_titleBar);
    m_closeButton->setObjectName(QStringLiteral("ocrCloseButton"));
    m_closeButton->setAccessibleName(MS_TR("Close"));
    m_closeButton->setToolTip(MS_TR("Close") + QStringLiteral(" (Esc)"));
    for (QPushButton *button : {m_moreButton, m_pinButton, m_closeButton}) {
        button->setProperty("role", QStringLiteral("icon"));
        button->setFixedSize(30, 30);
        button->setIconSize(QSize(18, 18));
        button->setCursor(Qt::PointingHandCursor);
        titleLayout->addWidget(button);
    }
    auto *moreMenu = new QMenu(m_moreButton);
    auto *shortcuts = moreMenu->addMenu(MS_TR("Keyboard shortcuts"));
    for (const QString &entry : {MS_TR("Translate") + QStringLiteral("  Ctrl+Enter"),
                                 MS_TR("Copy all text") + QStringLiteral("  Ctrl+Shift+C"),
                                 MS_TR("Undo edit") + QStringLiteral("  Ctrl+Z")}) {
        shortcuts->addAction(entry)->setEnabled(false);
    }
    m_moreButton->setMenu(moreMenu);
    connect(m_pinButton, &QPushButton::toggled, this, &OcrResultWindow::setAlwaysOnTop);
    connect(m_closeButton, &QPushButton::clicked, this, &QWidget::close);
    m_titleBar->installEventFilter(this);
    layout->addWidget(m_titleBar);

    // 2. 【OCR】【内容页面】翻译操作位于内容上方，原图与文本使用互斥页面
    layout->addWidget(createTranslationActions());
    const bool hasSourceImage = !sourceImage.isNull();
    layout->addWidget(createContentViews(text, std::move(sourceImage), tokens), 1);
    // 标签顺序与页面堆叠顺序一致：文本、表格（可选）、原图（可选）
    if (m_tablePane) {
        m_viewTabs->addTab(MS_TR("Table"));
    }
    if (hasSourceImage) {
        m_viewTabs->addTab(MS_TR("Source image"));
    }
    connect(m_viewTabs, &QTabBar::currentChanged, this, &OcrResultWindow::setContentView);
    connect(m_translationToggle, &QPushButton::toggled, this, &OcrResultWindow::toggleTranslationPane);

    // 3. 【OCR】【操作反馈】底部仅保留轻量状态与尺寸入口
    auto *footer = new QHBoxLayout;
    footer->setContentsMargins(0, 0, 0, 0);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("ocrStatus"));
    m_statusLabel->setProperty("role", QStringLiteral("muted"));
    m_statusLabel->setFont(markshot::theme::uiFont(9));
    m_statusLabel->setTextFormat(Qt::PlainText);
    m_statusLabel->setWordWrap(true);
    footer->addWidget(m_statusLabel, 1);
    auto *grip = new markshot::ui::WindowResizeGrip(this, [this] {
        return pinnedWindowHasLayerShellTop(this);
    });
    grip->setObjectName(QStringLiteral("ocrResizeGrip"));
    grip->setFixedSize(18, 18);
    grip->setToolTip(MS_TR("Resize window"));
    footer->addWidget(grip, 0, Qt::AlignBottom);
    layout->addLayout(footer);
    m_statusTimer = new QTimer(this);
    m_statusTimer->setSingleShot(true);
    connect(m_statusTimer, &QTimer::timeout, this, [this] {
        m_statusLabel->clear();
        m_statusLabel->setToolTip(QString());
    });

    // 4. 【OCR】【键盘操作】保留编辑器复制语义及完整文本复制、翻译快捷键
    auto *closeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(closeShortcut, &QShortcut::activated, this, &QWidget::close);
    auto *translateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
    translateShortcut->setKeys({QKeySequence(Qt::CTRL | Qt::Key_Return), QKeySequence(Qt::CTRL | Qt::Key_Enter)});
    connect(translateShortcut, &QShortcut::activated, this, &OcrResultWindow::startTranslation);
    auto *copyShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C), this);
    connect(copyShortcut, &QShortcut::activated, this, [this] {
        const bool translated = m_translationPane->editor()->hasFocus();
        copyResultText(translated ? m_translationPane->text() : m_sourcePane->text(), translated);
    });
    updateSourceState();
}

void OcrResultWindow::applyTheme()
{
    const auto mode = markshot::ui::effectiveUiThemeMode(
        markshot::ui::uiThemeModeFromConfigRoot(markshot::readAppConfigRoot()));
    QPalette colors = markshot::settings::tokens::settingsPalette(mode);
    const QColor muted = mode == markshot::ui::UiThemeMode::Light
        ? QColor(100, 116, 139) : markshot::settings::tokens::kTextSecondary;
    colors.setColor(QPalette::PlaceholderText, muted);
    colors.setColor(QPalette::Disabled, QPalette::Text, muted);
    colors.setColor(QPalette::Disabled, QPalette::ButtonText, muted);
    setPalette(colors);
    setStyleSheet(ocrWindowStyleSheet(colors));
    const QColor ink = colors.color(QPalette::ButtonText);
    const QColor accent = colors.color(QPalette::Highlight);
    m_titleIcon->setPixmap(ocrActionIcon(ShotWindow::Action::OcrCopy, accent).pixmap(QSize(22, 22), devicePixelRatioF()));
    m_pinButton->setIcon(ocrActionIcon(ShotWindow::Action::Pin, ink, accent));
    m_closeButton->setIcon(ocrActionIcon(ShotWindow::Action::Cancel, ink));
    m_moreButton->setIcon(QIcon(mode == markshot::ui::UiThemeMode::Light
        ? QStringLiteral(":/icons/more-light.svg") : QStringLiteral(":/icons/more.svg")));
    m_moreButton->menu()->setStyleSheet(ocrMenuStyleSheet(colors));
    m_sourcePane->setPalette(colors);
    m_translationPane->setPalette(colors);
    m_sourcePane->refreshTheme();
    m_translationPane->refreshTheme();
}

void OcrResultWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    // 1. 【OCR】【窗口绘制】显式绘制自定义窗口的样式背景，避免透明区域透出合成器底色
    QStyleOption option;
    option.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
}

}
