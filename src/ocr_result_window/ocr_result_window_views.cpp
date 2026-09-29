#include "ocr_result_window/ocr_result_window.h"

#include "clipboard_image.h"
#include "ocr_result_window/ocr_source_preview.h"
#include "ocr_table/ocr_table_pane.h"
#include "ocr_result_window/ocr_text_pane.h"
#include "ui/i18n.h"
#include "ui/theme.h"

#include <QBoxLayout>
#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabBar>
#include <QTextEdit>
#include <QTimer>

#include <utility>

namespace markshot::shot {
namespace {

/// @brief 读取编辑器两个方向的滚动位置
/// @param editor 需要保留视区的编辑器
/// @return 水平与垂直滚动值
QPoint scrollOffset(const QTextEdit *editor)
{
    return {editor->horizontalScrollBar()->value(), editor->verticalScrollBar()->value()};
}

/// @brief 页面恢复后还原编辑器滚动位置
/// @param editor 需要恢复视区的编辑器
/// @param offset 保存的水平与垂直滚动值
/// @return 无返回值
void restoreScrollOffset(QTextEdit *editor, QPoint offset)
{
    editor->horizontalScrollBar()->setValue(offset.x());
    editor->verticalScrollBar()->setValue(offset.y());
}

}

QWidget *OcrResultWindow::createContentViews(const QString &text,
                                             QImage sourceImage,
                                             const QVector<markshot::ocr::Token> &tokens)
{
    m_contentViews = new QStackedWidget(this);
    m_contentViews->setObjectName(QStringLiteral("ocrContentViews"));

    // 1. 【OCR】【文本页面】外层滚动仅用于极小窗口，编辑器占满当前页面
    auto *scroll = new QScrollArea(m_contentViews);
    scroll->setObjectName(QStringLiteral("ocrContentScroll"));
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *content = new QWidget(scroll);
    auto *body = new QVBoxLayout(content);
    body->setContentsMargins(0, 0, 0, 0);
    body->setSizeConstraint(QLayout::SetMinAndMaxSize);
    m_splitter = new QSplitter(Qt::Horizontal, content);
    m_splitter->setObjectName(QStringLiteral("ocrTextSplitter"));
    m_splitter->setHandleWidth(8);
    m_splitter->setChildrenCollapsible(false);
    m_sourcePane = new OcrTextPane(MS_TR("Recognized"), MS_TR("OCR text appears here"), m_splitter);
    m_sourcePane->setObjectName(QStringLiteral("ocrSourcePane"));
    m_sourcePane->editor()->setObjectName(QStringLiteral("ocrEditor"));
    m_sourcePane->setText(text);
    m_translationPane = new OcrTextPane(MS_TR("Translated text"), MS_TR("Translation appears here"), m_splitter);
    m_translationPane->setObjectName(QStringLiteral("ocrTranslationPane"));
    m_translationPane->editor()->setObjectName(QStringLiteral("ocrTranslationEditor"));
    m_splitter->addWidget(m_sourcePane);
    m_splitter->addWidget(m_translationPane);
    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 2);
    m_translationPane->hide();
    body->addWidget(m_splitter, 1);
    scroll->setWidget(content);
    content->setAutoFillBackground(false);
    scroll->viewport()->setAutoFillBackground(false);
    m_contentViews->addWidget(scroll);

    // 2. 【OCR】【表格页面】文字块能排成表格时追加表格页，识别结果不像表格时不打扰
    const markshot::ocr_table::OcrTable table = markshot::ocr_table::inferTable(tokens);
    if (markshot::ocr_table::looksLikeTable(table)) {
        m_tablePane = new markshot::ocr_table::OcrTablePane(table, m_contentViews);
        m_tablePane->setObjectName(QStringLiteral("ocrTablePane"));
        connect(m_tablePane, &markshot::ocr_table::OcrTablePane::copyRequested, this, [this](const QString &value) {
            m_tablePane->showCopyFeedback(markshot::copyTextToClipboard(value));
        });
        m_contentViews->addWidget(m_tablePane);
    }

    // 3. 【OCR】【原图页面】原图与文本互斥显示，页面切换不销毁编辑器
    if (!sourceImage.isNull()) {
        m_contentViews->addWidget(new OcrSourcePreview(std::move(sourceImage), m_contentViews));
    }
    connect(m_sourcePane, &OcrTextPane::copyRequested, this,
            [this](const QString &value) { copyResultText(value, false); });
    connect(m_translationPane, &OcrTextPane::copyRequested, this,
            [this](const QString &value) { copyResultText(value, true); });
    connect(m_sourcePane->editor(), &QTextEdit::textChanged, this, &OcrResultWindow::updateSourceState);
    m_textViewFocus = m_sourcePane->editor();
    connect(qApp, &QApplication::focusChanged, this, [this, scroll](QWidget *, QWidget *current) {
        if (current && scroll->isAncestorOf(current)) {
            m_textViewFocus = current;
        }
    });
    return m_contentViews;
}

QWidget *OcrResultWindow::createTranslationActions()
{
    // 1. 【OCR】【翻译操作】语言输入与执行按钮同行排列
    m_translationActions = new QWidget(this);
    m_translationActions->setObjectName(QStringLiteral("ocrTranslationActions"));
    auto *actions = new QHBoxLayout(m_translationActions);
    actions->setContentsMargins(0, 0, 0, 0);
    actions->setSpacing(8);
    m_languageLabel = new QLabel(MS_TR("Target Language"), m_translationActions);
    m_languageLabel->setProperty("role", QStringLiteral("muted"));
    m_languageLabel->setFont(markshot::theme::uiFont(9));
    actions->addWidget(m_languageLabel);
    m_targetLanguageCombo = new QComboBox(m_translationActions);
    setupTargetLanguageCombo();
    m_languageLabel->setBuddy(m_targetLanguageCombo);
    actions->addWidget(m_targetLanguageCombo, 1);
    m_translateButton = new QPushButton(MS_TR("Translate"), m_translationActions);
    m_translateButton->setObjectName(QStringLiteral("ocrTranslateButton"));
    m_translateButton->setProperty("role", QStringLiteral("primary"));
    m_translateButton->setFont(markshot::theme::uiFont(10, QFont::DemiBold));
    m_translateButton->setMinimumWidth(92);
    m_translateButton->setToolTip(MS_TR("Translate") + QStringLiteral(" (Ctrl+Enter)"));
    // 2. 【OCR】【翻译任务】同一入口支持开始与取消，保留已有文本
    connect(m_translateButton, &QPushButton::clicked, this, [this] {
        if (m_translationTask) {
            cancelTranslation();
            m_translationPane->setNotice(MS_TR("Translation canceled"));
        } else {
            startTranslation();
        }
    });
    actions->addWidget(m_translateButton);
    m_translationActions->hide();
    return m_translationActions;
}

void OcrResultWindow::setContentView(int index)
{
    if (index < 0 || index >= m_contentViews->count() || index == m_contentViews->currentIndex()) {
        return;
    }
    // 1. 【OCR】【页面切换】离开文本页时保存视区与焦点，文本和撤销历史由编辑器保留
    if (m_contentViews->currentIndex() == 0) {
        m_sourceScrollOffset = scrollOffset(m_sourcePane->editor());
        m_translationScrollOffset = scrollOffset(m_translationPane->editor());
        m_textPaneSizes = m_splitter->sizes();
    }
    const QSignalBlocker blocker(m_viewTabs);
    m_viewTabs->setCurrentIndex(index);
    m_contentViews->setCurrentIndex(index);
    const bool textView = index == 0;
    m_translationToggle->setVisible(textView);
    m_translationActions->setVisible(textView && m_translationToggle->isChecked());
    if (!textView) {
        m_contentViews->currentWidget()->setFocus(Qt::OtherFocusReason);
        return;
    }

    // 2. 【OCR】【页面恢复】布局完成后恢复分栏和滚动，避免隐藏期间尺寸变化挤走阅读位置
    QTimer::singleShot(0, this, [this] {
        if (m_contentViews->currentIndex() != 0) {
            return;
        }
        if (!m_textPaneSizes.isEmpty()) {
            m_splitter->setSizes(m_textPaneSizes);
        }
        QWidget *focus = m_textViewFocus && m_contentViews->isAncestorOf(m_textViewFocus)
            ? m_textViewFocus.data() : m_sourcePane->editor();
        focus->setFocus(Qt::OtherFocusReason);
        restoreScrollOffset(m_sourcePane->editor(), m_sourceScrollOffset);
        restoreScrollOffset(m_translationPane->editor(), m_translationScrollOffset);
    });
}

void OcrResultWindow::updateResponsiveLayout()
{
    if (!m_splitter) {
        return;
    }
    const Qt::Orientation orientation = width() >= 620 ? Qt::Horizontal : Qt::Vertical;
    if (m_splitter->orientation() != orientation) {
        m_splitter->setOrientation(orientation);
        distributeTextPaneSpace();
    }
    m_languageLabel->setVisible(width() >= 480);
    m_titleIcon->setVisible(width() >= 500);
}

void OcrResultWindow::distributeTextPaneSpace()
{
    const int extent = m_splitter->orientation() == Qt::Horizontal
        ? m_splitter->width() : m_splitter->height();
    m_splitter->setSizes({extent * 3 / 5, extent * 2 / 5});
}

void OcrResultWindow::showTranslationPane()
{
    // 1. 【OCR】【译文显示】先返回文本页面，再显示语言与任务操作
    setContentView(0);
    const QSignalBlocker blocker(m_translationToggle);
    m_translationToggle->setChecked(true);
    m_translationToggle->setToolTip(MS_TR("Hide translation"));
    m_translationActions->show();
    if (m_translationPane->isHidden()) {
        // 2. 【OCR】【译文分栏】首次展开按新布局分配空间，不恢复收起时的零宽分栏
        m_textPaneSizes.clear();
        m_translationPane->show();
        updateResponsiveLayout();
        distributeTextPaneSpace();
    }
}

void OcrResultWindow::toggleTranslationPane(bool visible)
{
    if (visible) {
        showTranslationPane();
        if (m_translationPane->text().isEmpty()) {
            startTranslation();
        }
        return;
    }
    cancelTranslation();
    m_translationPane->hide();
    m_translationActions->hide();
    m_translationToggle->setToolTip(MS_TR("Translate") + QStringLiteral(" (Ctrl+Enter)"));
}

}
