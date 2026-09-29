#include "capture_delay/capture_delay_option.h"

#include <cmath>

namespace markshot::capture_delay {

std::optional<int> parseCaptureDelaySeconds(const QString &text, QString *error)
{
    if (error) {
        error->clear();
    }

    // 1. 数值格式校验，拒绝空串、非数字与 NaN/Inf
    bool ok = false;
    const double seconds = text.trimmed().toDouble(&ok);
    if (!ok || !std::isfinite(seconds)) {
        if (error) {
            *error = QStringLiteral("delay must be a number of seconds");
        }
        return std::nullopt;
    }

    // 2. 范围校验，上限避免误输入后长时间无响应
    if (seconds < 0.0 || seconds * 1000.0 > kMaxCaptureDelayMs) {
        if (error) {
            *error = QStringLiteral("delay must be between 0 and %1 seconds")
                         .arg(kMaxCaptureDelayMs / 1000);
        }
        return std::nullopt;
    }
    return static_cast<int>(std::lround(seconds * 1000.0));
}

}  // namespace markshot::capture_delay
