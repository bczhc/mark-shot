#include "ocr_result_window/ocr_result_window.h"

#include "app_config_store.h"
#include "clipboard_image.h"
#include "debug_log.h"
#include "ocr_result_window/ocr_result_window_config.h"
#include "ocr_result_window/ocr_result_window_geometry.h"
#include "ocr_result_window/ocr_text_pane.h"
#include "pinned_window_top.h"
#include "ui/i18n.h"

#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QTextEdit>
#include <QTimer>

namespace markshot::shot {

OcrResultWindow::OcrResultWindow(QString text,
                                 QScreen *targetScreen,
                                 QImage sourceImage,
                                 QVector<markshot::ocr::Token> tokens)
    : m_config(pinnedWindowConfig())
    , m_alwaysOnTop(ocrResultWindowAlwaysOnTopFromRoot(markshot::readAppConfigRoot()))
{
    // 1. 【OCR】【结果窗口】在独立窗口内组合原图、原文和译文
    setWindowTitle(MS_TR("OCR Result"));
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_StyledBackground);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setObjectName(QStringLiteral("ocrResultWindow"));
    initializeUi(text, std::move(sourceImage), tokens);
    applyTheme();

    // 2. 【OCR】【结果窗口放置】保留截图目标屏幕，限制初始尺寸并记录浮层几何
    QScreen *primaryScreen = QApplication::primaryScreen();
    QScreen *resolvedScreen = targetScreen ? targetScreen : primaryScreen;
    if (resolvedScreen) {
        setScreen(resolvedScreen);
    }
    const QRect targetGeometry = targetScreen ? targetScreen->availableGeometry() : QRect();
    const QRect primaryGeometry = primaryScreen ? primaryScreen->availableGeometry() : QRect();
    const OcrResultWindowPlacement placement = ocrResultWindowPlacement(targetGeometry, primaryGeometry);
    setMinimumSize(QSize(380, 340).boundedTo(placement.size));
    resize(placement.size);
    move(placement.topLeft);
    m_logicalGeometry = QRect(placement.topLeft, size());
    setProperty("markShotPinnedGeometry", m_logicalGeometry);
    applyPinnedWindowTopState(this, m_alwaysOnTop);
    markshot::debugLog("ocr", "【OCR】【结果窗口放置】target=%s actual=%s geometry=%d,%d %dx%d",
                       targetScreen ? targetScreen->name().toUtf8().constData() : "fallback",
                       screen() ? screen()->name().toUtf8().constData() : "none",
                       m_logicalGeometry.x(), m_logicalGeometry.y(), width(), height());
    m_sourcePane->editor()->setFocus(Qt::OtherFocusReason);
}

OcrResultWindow::~OcrResultWindow()
{
    cancelTranslation();
}

void OcrResultWindow::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (m_sourcePane && (event->type() == QEvent::ApplicationPaletteChange
                       || event->type() == QEvent::ThemeChange
                       || (event->type() == QEvent::ActivationChange && isActiveWindow()))) {
        applyTheme();
    }
}

void OcrResultWindow::copyResultText(const QString &text, bool translated)
{
    if (text.trimmed().isEmpty()) {
        return;
    }
    OcrTextPane *pane = translated ? m_translationPane : m_sourcePane;
    pane->showCopyFeedback(markshot::copyTextToClipboard(text));
}

void OcrResultWindow::showToast(const QString &text, int durationMs)
{
    m_statusLabel->setText(text);
    m_statusLabel->setToolTip(text);
    m_statusTimer->start(durationMs);
}

void OcrResultWindow::updateSourceState()
{
    if (!m_sourcePane || !m_translateButton) {
        return;
    }
    const QString source = m_sourcePane->text().trimmed();
    m_translateButton->setEnabled(m_translationTask || !source.isEmpty());
    if (m_translationToggle) {
        m_translationToggle->setEnabled(!source.isEmpty() || m_translationToggle->isChecked()
                                        || !m_translationPane->text().isEmpty());
    }

    // 1. 【OCR】【译文状态】只比较本次翻译的原文和目标语言，不覆盖用户继续编辑的内容
    if (!m_translationTask && !m_translationFailed && !m_translatedSource.isEmpty()) {
        const bool stale = source != m_translatedSource || currentTargetLanguage() != m_translatedTarget;
        m_translationPane->setNotice(stale
            ? MS_TR("Source or language changed. Translate again to update.")
            : QString());
    }
}

}
