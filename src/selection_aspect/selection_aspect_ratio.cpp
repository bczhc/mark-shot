#include "selection_aspect/selection_aspect_ratio.h"

#include <algorithm>
#include <cmath>

namespace markshot::selection_aspect {
namespace {

using Handle = markshot::shot::types::SelectionDrag;

// 与 adjustedSelectionRect 保持一致的最小边长
constexpr qreal kMinimumSide = 8.0;

/**
 * 返回角控制点对应的对角锚点与移动角。
 * @param handle 角控制点。
 * @param before 调整前选区。
 * @param adjusted 未约束的调整结果。
 * @param anchor 输出对角锚点。
 * @param corner 输出移动后的角点。
 * @return handle 是角控制点时返回 true。
 */
bool cornerPoints(Handle handle, QRectF before, QRectF adjusted, QPointF *anchor, QPointF *corner)
{
    switch (handle) {
    case Handle::TopLeft:
        *anchor = before.bottomRight();
        *corner = adjusted.topLeft();
        return true;
    case Handle::TopRight:
        *anchor = before.bottomLeft();
        *corner = adjusted.topRight();
        return true;
    case Handle::BottomLeft:
        *anchor = before.topRight();
        *corner = adjusted.bottomLeft();
        return true;
    case Handle::BottomRight:
        *anchor = before.topLeft();
        *corner = adjusted.bottomRight();
        return true;
    default:
        return false;
    }
}

}  // namespace

QVector<AspectRatioPreset> aspectRatioPresets()
{
    return {
        {QStringLiteral("Free"), 0.0},
        {QStringLiteral("1:1"), 1.0},
        {QStringLiteral("4:3"), 4.0 / 3.0},
        {QStringLiteral("3:2"), 3.0 / 2.0},
        {QStringLiteral("16:9"), 16.0 / 9.0},
        {QStringLiteral("21:9"), 21.0 / 9.0},
        {QStringLiteral("3:4"), 3.0 / 4.0},
        {QStringLiteral("9:16"), 9.0 / 16.0},
    };
}

QRectF constrainedCreationRect(QPointF anchor, QPointF pointer, qreal ratio, QSize imageSize)
{
    if (ratio <= 0.0) {
        return QRectF(anchor, pointer).normalized();
    }

    // 1. 取能覆盖指针的最小比例矩形
    const qreal signX = pointer.x() >= anchor.x() ? 1.0 : -1.0;
    const qreal signY = pointer.y() >= anchor.y() ? 1.0 : -1.0;
    qreal width = std::abs(pointer.x() - anchor.x());
    qreal height = std::abs(pointer.y() - anchor.y());
    width = std::max(width, height * ratio);
    height = width / ratio;

    // 2. 按锚点到图像边缘的可用空间等比缩小
    const qreal availableWidth = signX > 0 ? imageSize.width() - anchor.x() : anchor.x();
    const qreal availableHeight = signY > 0 ? imageSize.height() - anchor.y() : anchor.y();
    if (width > availableWidth) {
        width = std::max<qreal>(0.0, availableWidth);
        height = width / ratio;
    }
    if (height > availableHeight) {
        height = std::max<qreal>(0.0, availableHeight);
        width = height * ratio;
    }
    return QRectF(anchor, QPointF(anchor.x() + signX * width, anchor.y() + signY * height)).normalized();
}

QRectF constrainedAdjustedRect(QRectF before, Handle handle, QRectF adjusted, qreal ratio, QSize imageSize)
{
    if (ratio <= 0.0 || handle == Handle::None || handle == Handle::Move) {
        return adjusted;
    }

    // 1. 角控制点：以对角为锚点重新按比例生成
    QPointF anchor;
    QPointF corner;
    QRectF result;
    if (cornerPoints(handle, before, adjusted, &anchor, &corner)) {
        result = constrainedCreationRect(anchor, corner, ratio, imageSize);
    } else if (handle == Handle::Left || handle == Handle::Right) {
        // 2. 左右边：宽度跟随拖动，高度按比例并保持垂直居中
        qreal width = adjusted.width();
        qreal height = width / ratio;
        if (height > imageSize.height()) {
            height = imageSize.height();
            width = height * ratio;
        }
        const qreal left = handle == Handle::Right ? before.left() : before.right() - width;
        const qreal top = std::clamp(before.center().y() - height / 2.0, 0.0,
                                     std::max<qreal>(0.0, imageSize.height() - height));
        result = QRectF(left, top, width, height);
    } else if (handle == Handle::Top || handle == Handle::Bottom) {
        // 3. 上下边：高度跟随拖动，宽度按比例并保持水平居中
        qreal height = adjusted.height();
        qreal width = height * ratio;
        if (width > imageSize.width()) {
            width = imageSize.width();
            height = width / ratio;
        }
        const qreal top = handle == Handle::Bottom ? before.top() : before.bottom() - height;
        const qreal left = std::clamp(before.center().x() - width / 2.0, 0.0,
                                      std::max<qreal>(0.0, imageSize.width() - width));
        result = QRectF(left, top, width, height);
    } else {
        return adjusted;
    }

    // 4. 太小时保持原选区，避免比例约束把选区压扁
    if (result.width() < kMinimumSide || result.height() < kMinimumSide) {
        return before;
    }
    return result;
}

QRectF resizedSelection(QRectF current, QSize size, QSize imageSize)
{
    const qreal width = std::clamp<qreal>(size.width(), 1.0, std::max(1, imageSize.width()));
    const qreal height = std::clamp<qreal>(size.height(), 1.0, std::max(1, imageSize.height()));
    const QRectF normalized = current.normalized();
    const qreal left = std::clamp<qreal>(normalized.left(), 0.0, imageSize.width() - width);
    const qreal top = std::clamp<qreal>(normalized.top(), 0.0, imageSize.height() - height);
    return QRectF(left, top, width, height);
}

int heightForWidth(int width, qreal ratio)
{
    if (ratio <= 0.0) {
        return 0;
    }
    return std::max(1, static_cast<int>(std::lround(width / ratio)));
}

int widthForHeight(int height, qreal ratio)
{
    if (ratio <= 0.0) {
        return 0;
    }
    return std::max(1, static_cast<int>(std::lround(height * ratio)));
}

}  // namespace markshot::selection_aspect
