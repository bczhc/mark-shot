#pragma once

#include <QString>

namespace markshot::recording {

/**
 * 录制启动占位。门户选择在嵌套事件循环里返回前，会话对象还没公布，
 * 需要先占住启动权，避免同一次等待里再打开第二个选择弹窗。
 */
class RecordingStartGate {
public:
    /**
     * 进入启动临界区。
     * @param error 已被占用时写入错误信息。
     * @return 成功占位时返回 true。
     */
    bool tryEnter(QString *error);

    /**
     * 离开启动临界区。
     * @return 无返回值。
     */
    void leave();

    /**
     * 当前是否处于启动临界区。
     * @return 已占位时返回 true。
     */
    bool active() const;

private:
    bool m_active = false;
};

}  // namespace markshot::recording
