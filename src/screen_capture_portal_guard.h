#pragma once

#include <QString>

namespace markshot {

/**
 * 占用交互式 ScreenCast 门户请求。
 * 同一进程里已有选择弹窗在等待时，后续请求应直接失败，而不是再弹一次。
 * @param error 占用失败时写入错误信息。
 * @return 成功占用时返回 true。
 */
bool tryAcquireInteractiveScreenCast(QString *error);

/**
 * 释放交互式 ScreenCast 门户请求占用。
 * @return 无返回值。
 */
void releaseInteractiveScreenCast();

/**
 * 当前是否有交互式 ScreenCast 门户请求正在等待用户选择。
 * @return 正在等待时返回 true。
 */
bool interactiveScreenCastInProgress();

}  // namespace markshot
