#pragma once

#include "ocr_result.h"
#include "shot_window_internal.h"

#include <QImage>
#include <QPointer>
#include <QWidget>

class QComboBox;
class QLabel;
class QMouseEvent;
class QPushButton;
class QScreen;
class QSplitter;
class QStackedWidget;
class QTabBar;
class QTimer;

namespace markshot::providers {
class ProviderTask;
struct TaskResult;
}

namespace markshot::ocr_table {
class OcrTablePane;
}

namespace markshot::shot {

class OcrTextPane;

/// @brief 组合原图、识别原文与译文，并协调独立置顶和翻译任务
class OcrResultWindow final : public QWidget {
public:
    /// @brief 创建 OCR 结果窗口
    /// @param text 初始识别原文
    /// @param targetScreen 截图所在屏幕，缺失时回退到主屏幕
    /// @param sourceImage 本次识别使用的原图，缺失时隐藏预览入口
    /// @param tokens 带位置的识别文字块，能组成表格时显示表格页
    explicit OcrResultWindow(QString text,
                             QScreen *targetScreen = nullptr,
                             QImage sourceImage = {},
                             QVector<markshot::ocr::Token> tokens = {});

    /// @brief 取消正在运行的翻译任务并释放临时文件
    ~OcrResultWindow() override;

protected:
    /// @brief 捕获中断或隐藏时清理窗口拖动，重新进入时恢复悬停光标
    /// @param event 窗口生命周期或指针事件
    /// @return Qt 事件处理结果
    bool event(QEvent *event) override;
    /// @brief 绘制透明窗口的主题背景与圆角边框
    /// @param event 绘制事件
    /// @return 无返回值
    void paintEvent(QPaintEvent *event) override;
    /// @brief 显示后重试依赖桌面扩展的置顶请求
    /// @param event 显示事件
    /// @return 无返回值
    void showEvent(QShowEvent *event) override;
    /// @brief 同步布局并提交应用内发起的 layer-shell 尺寸变化
    /// @param event 尺寸变更事件
    /// @return 无返回值
    void resizeEvent(QResizeEvent *event) override;
    /// @brief 激活窗口或系统主题改变后同步界面主题
    /// @param event 窗口状态事件
    /// @return 无返回值
    void changeEvent(QEvent *event) override;
    /// @brief 处理标题栏拖动的起始事件
    /// @param event 鼠标事件
    /// @return 无返回值
    void mousePressEvent(QMouseEvent *event) override;
    /// @brief 更新正在进行的窗口拖动
    /// @param event 鼠标事件
    /// @return 无返回值
    void mouseMoveEvent(QMouseEvent *event) override;
    /// @brief 结束窗口拖动
    /// @param event 鼠标事件
    /// @return 无返回值
    void mouseReleaseEvent(QMouseEvent *event) override;
    /// @brief 接收标题栏空白区域的拖动事件
    /// @param watched 事件目标
    /// @param event Qt 事件
    /// @return 已处理时返回 true
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    /// @brief 构建独立文本区、原图预览和窗口操作栏
    /// @param text 初始识别文本
    /// @param sourceImage 原始截图
    /// @return 无返回值
    /// @param tokens 带位置的识别文字块
    void initializeUi(const QString &text, QImage sourceImage, const QVector<markshot::ocr::Token> &tokens);
    /// @brief 创建互斥的文本与原图页面，文本页面保留原文和译文编辑器
    /// @param text 初始识别文本
    /// @param sourceImage 原始截图
    /// @return 承载页面的堆叠控件
    /// @param tokens 带位置的识别文字块，能组成表格时追加表格页
    QWidget *createContentViews(const QString &text, QImage sourceImage, const QVector<markshot::ocr::Token> &tokens);
    /// @brief 创建位于编辑区域上方的目标语言与翻译操作栏
    /// @return 翻译操作栏控件
    QWidget *createTranslationActions();
    /// @brief 切换文本或原图页面并保留编辑焦点、分栏和滚动位置
    /// @param index 页面下标，0 为文本，其余依次为表格与原图
    /// @return 无返回值
    void setContentView(int index);
    /// @brief 应用当前明暗主题并更新图标配色
    /// @return 无返回值
    void applyTheme();
    /// @brief 按可用宽度切换文本区排列方向
    /// @return 无返回值
    void updateResponsiveLayout();
    /// @brief 根据实际可用空间为原文和译文分配默认比例
    /// @return 无返回值
    void distributeTextPaneSpace();
    /// @brief 同步翻译入口及旧译文提示
    /// @return 无返回值
    void updateSourceState();
    /// @brief 展开译文区并为两个文本区分配空间
    /// @return 无返回值
    void showTranslationPane();
    /// @brief 按需展开翻译任务，收起时取消正在进行的请求并保留已有译文
    /// @param visible 是否显示翻译区
    /// @return 无返回值
    void toggleTranslationPane(bool visible);
    /// @brief 通过应用剪贴板服务复制文本并显示结果
    /// @param text 待复制的完整文本
    /// @param translated 是否为译文
    /// @return 无返回值
    void copyResultText(const QString &text, bool translated);
    /// @brief 在底部状态栏显示短提示，重复操作重置计时
    /// @param text 提示内容
    /// @param durationMs 显示时间，单位为毫秒
    /// @return 无返回值
    void showToast(const QString &text, int durationMs = 2500);

    /// @brief 判断窗口局部坐标是否命中标题栏操作按钮
    /// @param windowPoint 窗口局部坐标
    /// @return 命中按钮时返回 true
    bool titleControlContains(QPoint windowPoint) const;
    /// @brief 开始拖动 OCR 窗口
    /// @param event 鼠标事件
    /// @return 成功开始时返回 true
    bool beginWindowDrag(QMouseEvent *event);
    /// @brief 更新 OCR 窗口拖动位置
    /// @param event 鼠标事件
    /// @return 当前正在拖动时返回 true
    bool updateWindowDrag(QMouseEvent *event);
    /// @brief 结束 OCR 窗口拖动
    /// @param event 鼠标事件
    /// @return 成功结束时返回 true
    bool finishWindowDrag(QMouseEvent *event);
    /// @brief 清理拖动状态、鼠标捕获和标题栏光标
    /// @return 无返回值
    void resetWindowDrag();
    /// @brief 切换并保存 OCR 窗口置顶状态
    /// @param alwaysOnTop 是否保持置顶
    /// @return 无返回值
    void setAlwaysOnTop(bool alwaysOnTop);
    /// @brief 切换窗口协议角色并恢复位置与拖动状态
    /// @return 无返回值
    void recreateWindowSurface();

    /// @brief 初始化可输入自定义语言的目标语言下拉框
    /// @return 无返回值
    void setupTargetLanguageCombo();
    /// @brief 同步目标语言下拉框的显示值
    /// @param targetLanguage 翻译器使用的语言名称
    /// @return 无返回值
    void setTargetLanguageComboValue(const QString &targetLanguage);
    /// @brief 读取并规范化当前目标语言
    /// @return 翻译器使用的语言名称
    QString currentTargetLanguage() const;
    /// @brief 持久化当前目标语言，失败时恢复旧值
    /// @return 无返回值
    void applyTargetLanguageFromCombo();
    /// @brief 使用当前原文启动翻译任务
    /// @return 无返回值
    void startTranslation();
    /// @brief 将成功结果写入译文区，失败时保留原文并显示原因
    /// @param task 已完成的任务
    /// @param result 翻译任务结果
    /// @return 无返回值
    void finishTranslation(markshot::providers::ProviderTask *task,
                           const markshot::providers::TaskResult &result);
    /// @brief 取消翻译并清理临时文件
    /// @return 无返回值
    void cancelTranslation();
    /// @brief 清理已经结束的任务及临时文件
    /// @param task 已结束任务
    /// @return 无返回值
    void finishTranslationCleanup(markshot::providers::ProviderTask *task);
    /// @brief 恢复翻译按钮、编辑器和语言下拉框状态
    /// @return 无返回值
    void resetTranslationUi();

    QWidget *m_titleBar = nullptr;
    QWidget *m_translationActions = nullptr;
    QPushButton *m_translationToggle = nullptr;
    QPushButton *m_moreButton = nullptr;
    QLabel *m_titleIcon = nullptr;
    QLabel *m_languageLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTimer *m_statusTimer = nullptr;
    QSplitter *m_splitter = nullptr;
    QStackedWidget *m_contentViews = nullptr;
    QTabBar *m_viewTabs = nullptr;
    QPointer<QWidget> m_textViewFocus;
    QPoint m_sourceScrollOffset;
    QPoint m_translationScrollOffset;
    QList<int> m_textPaneSizes;
    OcrTextPane *m_sourcePane = nullptr;
    OcrTextPane *m_translationPane = nullptr;
    markshot::ocr_table::OcrTablePane *m_tablePane = nullptr;
    QPushButton *m_translateButton = nullptr;
    QComboBox *m_targetLanguageCombo = nullptr;
    QPushButton *m_pinButton = nullptr;
    QPushButton *m_closeButton = nullptr;
    markshot::providers::ProviderTask *m_translationTask = nullptr;
    QString m_translationInputPath;
    QString m_pendingSource;
    QString m_pendingTarget;
    QString m_translatedSource;
    QString m_translatedTarget;
    QPoint m_dragOffset;
    QRect m_logicalGeometry;
    PinnedWindowConfig m_config;
    bool m_alwaysOnTop = false;
    bool m_dragging = false;
    bool m_translationFailed = false;
};

}
