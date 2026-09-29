#include "windows_tray_controller.h"

#include "config_value.h"
#include "window_detection.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include <optional>

namespace markshot {
namespace {

/// @brief Applies system tray configurations from a JSON object.
/// @param object The JSON object containing tray configuration values.
/// @param config Pointer to the configuration structure to update.
void applyTrayConfig(const QJsonObject &object, WindowsTrayController::Config *config)
{
    if (!config || object.isEmpty()) {
        return;
    }

    if (const std::optional<bool> enabled = config::boolValue(object.value(QStringLiteral("enabled")))) {
        config->autoStart = *enabled;
    }
    if (const std::optional<bool> autoStart = config::boolValue(object.value(QStringLiteral("autoStart")))) {
        config->autoStart = *autoStart;
    }
    if (const std::optional<bool> startInTray = config::boolValue(object.value(QStringLiteral("startInTray")))) {
        config->autoStart = *startInTray;
    }
    if (const std::optional<bool> hotkeysEnabled = config::boolValue(object.value(QStringLiteral("hotkeysEnabled")))) {
        config->hotkeysEnabled = *hotkeysEnabled;
    }
    if (const std::optional<bool> hotkeyEnabled = config::boolValue(object.value(QStringLiteral("hotkeyEnabled")))) {
        config->hotkeysEnabled = *hotkeyEnabled;
    }
}

/// @brief Applies hotkey-related configurations from a JSON object.
/// @param object The JSON object containing hotkey configuration values.
/// @param config Pointer to the configuration structure to update.
void applyHotkeyConfig(const QJsonObject &object, WindowsTrayController::Config *config)
{
    if (!config || object.isEmpty()) {
        return;
    }

    if (const std::optional<bool> enabled = config::boolValue(object.value(QStringLiteral("enabled")))) {
        config->hotkeysEnabled = *enabled;
    }
    for (const QString &key : {QStringLiteral("capture"),
                               QStringLiteral("screenshot"),
                               QStringLiteral("shot"),
                               QStringLiteral("captureHotkey"),
                               QStringLiteral("hotkey")}) {
        if (const std::optional<QKeySequence> sequence = config::keySequenceValue(object.value(key))) {
            config->captureHotkey = *sequence;
            break;
        }
    }
    for (const QString &key : {QStringLiteral("stopRecording"),
                               QStringLiteral("stopRecordingHotkey"),
                               QStringLiteral("recordingStop")}) {
        if (const std::optional<QKeySequence> sequence = config::keySequenceValue(object.value(key))) {
            config->stopRecordingHotkey = *sequence;
            break;
        }
    }
    for (const QString &key : {QStringLiteral("pauseRecording"),
                               QStringLiteral("pauseRecordingHotkey"),
                               QStringLiteral("recordingPause")}) {
        if (const std::optional<QKeySequence> sequence = config::keySequenceValue(object.value(key))) {
            config->pauseRecordingHotkey = *sequence;
            break;
        }
    }
    for (const QString &key : {QStringLiteral("fullscreen"),
                               QStringLiteral("fullScreen"),
                               QStringLiteral("fullscreenCapture"),
                               QStringLiteral("fullscreenHotkey")}) {
        if (const std::optional<QKeySequence> sequence = config::keySequenceValue(object.value(key))) {
            config->fullscreenHotkey = *sequence;
            break;
        }
    }
}

/// @brief Applies general Windows-specific configurations from a JSON object.
/// @param object The JSON object containing configuration values.
/// @param config Pointer to the configuration structure to update.
void applyWindowsConfig(const QJsonObject &object, WindowsTrayController::Config *config)
{
    if (!config || object.isEmpty()) {
        return;
    }

    applyTrayConfig(config::firstNonEmptyObjectValue(object, {QStringLiteral("tray"), QStringLiteral("systemTray")}), config);
    applyHotkeyConfig(config::firstNonEmptyObjectValue(object, {QStringLiteral("hotkeys"), QStringLiteral("globalHotkeys")}), config);

    if (const std::optional<bool> trayEnabled = config::boolValue(object.value(QStringLiteral("trayEnabled")))) {
        config->autoStart = *trayEnabled;
    }
    if (const std::optional<bool> startInTray = config::boolValue(object.value(QStringLiteral("startInTray")))) {
        config->autoStart = *startInTray;
    }
    if (const std::optional<bool> hotkeysEnabled = config::boolValue(object.value(QStringLiteral("hotkeysEnabled")))) {
        config->hotkeysEnabled = *hotkeysEnabled;
    }
    if (const std::optional<QKeySequence> hotkey = config::keySequenceValue(object.value(QStringLiteral("hotkey")))) {
        config->captureHotkey = *hotkey;
    }
    if (const std::optional<QKeySequence> captureHotkey = config::keySequenceValue(object.value(QStringLiteral("captureHotkey")))) {
        config->captureHotkey = *captureHotkey;
    }
    if (const std::optional<QKeySequence> fullscreenHotkey = config::keySequenceValue(object.value(QStringLiteral("fullscreenHotkey")))) {
        config->fullscreenHotkey = *fullscreenHotkey;
    }
}

}  // namespace

/**
 * 从应用配置文件读取托盘与全局快捷键配置。
 * @return 托盘配置，文件缺失或解析失败时返回默认值。
 */
WindowsTrayController::Config WindowsTrayController::readConfig()
{
    Config config;

    QFile file(markshot::appConfigPath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return config;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return config;
    }

    const QJsonObject root = document.object();
    applyTrayConfig(config::firstNonEmptyObjectValue(root, {QStringLiteral("tray"), QStringLiteral("systemTray")}), &config);
    applyHotkeyConfig(config::firstNonEmptyObjectValue(root, {QStringLiteral("globalHotkeys"), QStringLiteral("windowsHotkeys")}), &config);
    applyWindowsConfig(config::objectValue(root, QStringLiteral("windows")), &config);
    return config;
}

}  // namespace markshot
