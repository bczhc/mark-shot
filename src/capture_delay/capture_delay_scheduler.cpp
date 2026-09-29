#include "capture_delay/capture_delay_scheduler.h"

#include "debug_log.h"

#include <algorithm>
#include <utility>

namespace markshot::capture_delay {

CaptureDelayScheduler::CaptureDelayScheduler(QObject *parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &CaptureDelayScheduler::fire);
}

void CaptureDelayScheduler::schedule(int delayMs, Action action)
{
    // 1. 新请求替换旧计划，保证同一时刻只有一个截图等待触发
    const bool wasPending = pending();
    m_timer.stop();
    m_action = std::move(action);
    m_delayMs = std::max(0, delayMs);
    m_elapsed.restart();

    // 2. 启动计时，并在待执行状态从无到有时通知界面
    m_timer.start(m_delayMs);
    markshot::debugLog("capture-session",
                       "【截图会话】【延时截图】scheduled delay_ms=%d replaced=%d",
                       m_delayMs,
                       wasPending ? 1 : 0);
    if (!wasPending) {
        emit pendingChanged(true);
    }
}

bool CaptureDelayScheduler::cancel()
{
    if (!pending()) {
        return false;
    }
    m_timer.stop();
    m_action = nullptr;
    markshot::debugLog("capture-session", "【截图会话】【延时截图】cancelled");
    emit pendingChanged(false);
    return true;
}

bool CaptureDelayScheduler::pending() const
{
    return m_timer.isActive();
}

int CaptureDelayScheduler::remainingMs() const
{
    if (!pending() || !m_elapsed.isValid()) {
        return 0;
    }
    return static_cast<int>(std::max<qint64>(0, m_delayMs - m_elapsed.elapsed()));
}

void CaptureDelayScheduler::fire()
{
    // 1. 先取出动作再清空，动作内部可以安全地重新计划
    Action action = std::move(m_action);
    m_action = nullptr;
    markshot::debugLog("capture-session", "【截图会话】【延时截图】fired");
    emit pendingChanged(false);

    // 2. 执行截图动作
    if (action) {
        action();
    }
}

}  // namespace markshot::capture_delay
