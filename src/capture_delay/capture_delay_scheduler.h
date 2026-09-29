#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

#include <functional>

namespace markshot::capture_delay {

/**
 * 管理单个待执行的延时截图。
 * 同一时刻只保留一个计划，新的请求会替换尚未触发的旧请求。
 */
class CaptureDelayScheduler final : public QObject {
    Q_OBJECT

public:
    using Action = std::function<void()>;

    /**
     * 创建延时截图调度器。
     * @param parent 父对象。
     */
    explicit CaptureDelayScheduler(QObject *parent = nullptr);

    /**
     * 计划在指定延时后执行截图动作。
     * @param delayMs 延时毫秒数，小于等于 0 时在下一轮事件循环执行。
     * @param action 到期后执行的动作。
     * @return 无返回值。
     */
    void schedule(int delayMs, Action action);

    /**
     * 取消尚未触发的延时截图。
     * @return 存在待执行计划并已取消时返回 true。
     */
    bool cancel();

    /**
     * 是否存在待执行的延时截图。
     * @return 存在时返回 true。
     */
    bool pending() const;

    /**
     * 读取距离触发的剩余时间。
     * @return 剩余毫秒数，没有计划时返回 0。
     */
    int remainingMs() const;

signals:
    /**
     * 待执行状态变化时发出，供托盘等界面刷新。
     * @param pending 当前是否存在待执行计划。
     */
    void pendingChanged(bool pending);

private:
    /**
     * 计时到期后执行动作并清空计划。
     * @return 无返回值。
     */
    void fire();

    QTimer m_timer;
    QElapsedTimer m_elapsed;
    int m_delayMs = 0;
    Action m_action;
};

}  // namespace markshot::capture_delay
