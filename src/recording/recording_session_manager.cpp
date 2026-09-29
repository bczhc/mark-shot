#include "recording/recording_session_manager.h"

#include "notifications/app_notifications.h"
#include "recording/recording_controller.h"
#include "recording/recording_display_source.h"

namespace markshot::recording {

RecordingSessionManager &RecordingSessionManager::instance()
{
    static RecordingSessionManager manager;
    return manager;
}

RecordingSessionManager::RecordingSessionManager(QObject *parent)
    : QObject(parent)
{
}

bool RecordingSessionManager::start(const RecordingOptions &options, QObject *parent, QString *error)
{
    if (error) {
        error->clear();
    }
    if (m_controller) {
        if (error) {
            *error = QStringLiteral("recording is already active");
        }
        return false;
    }
    // 门户 SelectSources 在嵌套事件循环里阻塞。先占住启动权，
    // 避免这段等待里再次 start() 又弹出一个选择窗口。
    if (!m_startGate.tryEnter(error)) {
        return false;
    }
    struct StartGateLeave {
        RecordingStartGate *gate;
        ~StartGateLeave()
        {
            gate->leave();
        }
    };
    [[maybe_unused]] StartGateLeave leaveGate{&m_startGate};

    RecordingOptions resolved = options;
    bindRegionRecordingDisplay(&resolved);

    auto *controller = new RecordingController(parent ? parent : this);
    connect(controller, &RecordingController::statusChanged, this, &RecordingSessionManager::statusChanged);
    connect(controller,
            &RecordingController::finished,
            this,
            [this, controller](bool ok, const QString &outputPath, const QString &message) {
                if (m_controller == controller) {
                    m_controller = nullptr;
                }
                emit statusChanged();
                if (ok) {
                    markshot::notifications::notifyRecordingSaved(outputPath);
                } else {
                    markshot::notifications::notifyRecordingFailed(message);
                }
                emit recordingFinished(ok, outputPath, message);
            });
    if (!controller->start(resolved, error)) {
        controller->deleteLater();
        return false;
    }
    // 采集在 start() 内部同步失败时会走 finished，此时不能再把会话接上
    if (!controller->status().active) {
        return false;
    }

    m_controller = controller;
    emit statusChanged();
    markshot::notifications::notifyRecordingStarted(resolved);
    return true;
}

bool RecordingSessionManager::stop(QString *error)
{
    if (error) {
        error->clear();
    }
    if (!m_controller) {
        if (error) {
            *error = QStringLiteral("no active recording");
        }
        return false;
    }
    m_controller->requestStop();
    return true;
}

bool RecordingSessionManager::setPaused(bool paused, QString *error)
{
    if (error) {
        error->clear();
    }
    if (!m_controller) {
        if (error) {
            *error = QStringLiteral("no active recording");
        }
        return false;
    }
    if (!m_controller->setPaused(paused)) {
        if (error) {
            *error = paused ? QStringLiteral("recording is already paused")
                            : QStringLiteral("recording is not paused");
        }
        return false;
    }
    emit statusChanged();
    return true;
}

bool RecordingSessionManager::togglePause(QString *error)
{
    if (error) {
        error->clear();
    }
    if (!m_controller) {
        if (error) {
            *error = QStringLiteral("no active recording");
        }
        return false;
    }
    return setPaused(!m_controller->isPaused(), error);
}

RecordingStatus RecordingSessionManager::status() const
{
    return m_controller ? m_controller->status() : RecordingStatus();
}

}  // namespace markshot::recording
