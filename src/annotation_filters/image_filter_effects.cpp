#include "annotation_filters/image_filter_effects.h"

#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace markshot::annotation_filters {
namespace {

/**
 * 按标准差计算三次盒式模糊的盒宽，使叠加结果接近高斯分布。
 * @param sigma 目标高斯标准差。
 * @return 三次模糊各自使用的半径。
 */
std::array<int, 3> boxRadiiForGauss(qreal sigma)
{
    constexpr int passes = 3;
    const qreal idealWidth = std::sqrt((12.0 * sigma * sigma / passes) + 1.0);
    int lower = static_cast<int>(std::floor(idealWidth));
    if (lower % 2 == 0) {
        --lower;
    }
    lower = std::max(1, lower);
    const int upper = lower + 2;
    const qreal idealCount = (12.0 * sigma * sigma - passes * lower * lower - 4.0 * passes * lower - 3.0 * passes)
        / (-4.0 * lower - 4.0);
    const int lowerCount = static_cast<int>(std::lround(idealCount));

    std::array<int, 3> radii{};
    for (int i = 0; i < passes; ++i) {
        radii[static_cast<size_t>(i)] = ((i < lowerCount ? lower : upper) - 1) / 2;
    }
    return radii;
}

/**
 * 沿一个方向做一次滑动窗口盒式模糊，边界像素按夹取处理。
 * @param source 输入像素。
 * @param target 输出像素。
 * @param width 图像宽度。
 * @param height 图像高度。
 * @param radius 盒半径。
 * @param horizontal true 为水平方向，false 为垂直方向。
 * @return 无返回值。
 */
void boxBlurPass(const std::vector<QRgb> &source,
                 std::vector<QRgb> &target,
                 int width,
                 int height,
                 int radius,
                 bool horizontal)
{
    const int lineCount = horizontal ? height : width;
    const int lineLength = horizontal ? width : height;
    const int windowSize = radius * 2 + 1;
    auto indexOf = [width, horizontal](int line, int position) {
        return horizontal ? line * width + position : position * width + line;
    };

    for (int line = 0; line < lineCount; ++line) {
        // 1. 初始化窗口，越界位置取边缘像素
        std::array<qint64, 4> sum{};
        for (int offset = -radius; offset <= radius; ++offset) {
            const QRgb pixel = source[static_cast<size_t>(indexOf(line, std::clamp(offset, 0, lineLength - 1)))];
            sum[0] += qAlpha(pixel);
            sum[1] += qRed(pixel);
            sum[2] += qGreen(pixel);
            sum[3] += qBlue(pixel);
        }

        // 2. 滑动窗口，每次加入右侧像素并移出左侧像素
        for (int position = 0; position < lineLength; ++position) {
            target[static_cast<size_t>(indexOf(line, position))] =
                qRgba(static_cast<int>(sum[1] / windowSize),
                      static_cast<int>(sum[2] / windowSize),
                      static_cast<int>(sum[3] / windowSize),
                      static_cast<int>(sum[0] / windowSize));
            const QRgb incoming =
                source[static_cast<size_t>(indexOf(line, std::min(position + radius + 1, lineLength - 1)))];
            const QRgb outgoing = source[static_cast<size_t>(indexOf(line, std::max(position - radius, 0)))];
            sum[0] += qAlpha(incoming) - qAlpha(outgoing);
            sum[1] += qRed(incoming) - qRed(outgoing);
            sum[2] += qGreen(incoming) - qGreen(outgoing);
            sum[3] += qBlue(incoming) - qBlue(outgoing);
        }
    }
}

/**
 * 逐像素改写未预乘的 ARGB 图像，再转回预乘格式。
 * @param source 源图像。
 * @param transform 像素变换函数。
 * @return ARGB32_Premultiplied 格式结果。
 */
template <typename Transform>
QImage mapPixels(const QImage &source, Transform transform)
{
    if (source.isNull()) {
        return {};
    }
    QImage image = source.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        auto *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            line[x] = transform(line[x]);
        }
    }
    return image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

}  // namespace

QImage gaussianBlurred(const QImage &source, int radius)
{
    if (source.isNull()) {
        return {};
    }
    QImage image = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    if (radius < 1 || image.width() < 2 || image.height() < 2) {
        return image;
    }

    // 1. 预乘格式下做平均，透明边缘不会渗出黑边
    const int width = image.width();
    const int height = image.height();
    std::vector<QRgb> buffer(static_cast<size_t>(width) * static_cast<size_t>(height));
    for (int y = 0; y < height; ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        std::copy(line, line + width, buffer.begin() + static_cast<std::ptrdiff_t>(y) * width);
    }

    // 2. 三次水平 + 垂直盒式模糊逼近高斯
    std::vector<QRgb> scratch(buffer.size());
    for (const int boxRadius : boxRadiiForGauss(std::max<qreal>(1.0, radius / 2.0))) {
        if (boxRadius < 1) {
            continue;
        }
        boxBlurPass(buffer, scratch, width, height, boxRadius, true);
        boxBlurPass(scratch, buffer, width, height, boxRadius, false);
    }

    // 3. 写回图像
    for (int y = 0; y < height; ++y) {
        auto *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        std::copy(buffer.begin() + static_cast<std::ptrdiff_t>(y) * width,
                  buffer.begin() + static_cast<std::ptrdiff_t>(y + 1) * width,
                  line);
    }
    return image;
}

QImage grayscaled(const QImage &source)
{
    return mapPixels(source, [](QRgb pixel) {
        const int gray = qGray(pixel);
        return qRgba(gray, gray, gray, qAlpha(pixel));
    });
}

QImage inverted(const QImage &source)
{
    return mapPixels(source, [](QRgb pixel) {
        return qRgba(255 - qRed(pixel), 255 - qGreen(pixel), 255 - qBlue(pixel), qAlpha(pixel));
    });
}

QImage brightened(const QImage &source, qreal amount)
{
    const qreal clamped = std::clamp<qreal>(amount, 0.0, 1.0);
    auto lift = [clamped](int channel) {
        return std::clamp(static_cast<int>(std::lround(channel + (255 - channel) * clamped)), 0, 255);
    };
    return mapPixels(source, [&lift](QRgb pixel) {
        return qRgba(lift(qRed(pixel)), lift(qGreen(pixel)), lift(qBlue(pixel)), qAlpha(pixel));
    });
}

int blurRadiusForStrength(qreal strength)
{
    return std::clamp(static_cast<int>(std::lround(strength)), 1, 48);
}

qreal brightnessAmountForStrength(qreal strength)
{
    return std::clamp<qreal>(strength / 60.0, 0.05, 0.9);
}

}  // namespace markshot::annotation_filters
