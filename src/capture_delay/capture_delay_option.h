#pragma once

#include <QString>

#include <optional>

namespace markshot::capture_delay {

// 延时截图允许的最长等待时间
inline constexpr int kMaxCaptureDelayMs = 60 * 1000;

/**
 * 解析命令行中的延时秒数。
 * @param text 参数文本，支持整数或小数秒，例如 "3"、"1.5"。
 * @param error 解析失败时写入错误信息。
 * @return 成功时返回毫秒数，范围 0 至 kMaxCaptureDelayMs；失败返回空。
 */
std::optional<int> parseCaptureDelaySeconds(const QString &text, QString *error);

}  // namespace markshot::capture_delay
