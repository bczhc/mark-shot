#pragma once

#include "shot_window_types.h"

#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QVector>

namespace markshot::selection_aspect {

/**
 * 选区比例预设。ratio 为宽除以高，0 表示自由比例；label 为未翻译的显示文本。
 */
struct AspectRatioPreset {
    QString label;
    qreal ratio = 0.0;
};

/**
 * 返回尺寸面板提供的比例预设，第一项为自由比例。
 * @return 比例预设列表。
 */
QVector<AspectRatioPreset> aspectRatioPresets();

/**
 * 按固定比例从锚点向指针方向创建选区，并保证不超出图像。
 * @param anchor 拖动起点（图像坐标）。
 * @param pointer 当前指针（图像坐标）。
 * @param ratio 宽高比，必须大于 0。
 * @param imageSize 图像尺寸。
 * @return 满足比例的选区；ratio 无效时返回锚点与指针构成的普通矩形。
 */
QRectF constrainedCreationRect(QPointF anchor, QPointF pointer, qreal ratio, QSize imageSize);

/**
 * 按固定比例调整已有选区的边或角。移动整个选区时不做比例约束。
 * @param before 调整前的选区。
 * @param handle 拖动的边、角或整个选区。
 * @param adjusted 未约束时的调整结果，用于读取拖动后的边位置。
 * @param ratio 宽高比，必须大于 0。
 * @param imageSize 图像尺寸。
 * @return 满足比例的选区；无法满足最小尺寸时返回 before。
 */
QRectF constrainedAdjustedRect(QRectF before,
                               markshot::shot::types::SelectionDrag handle,
                               QRectF adjusted,
                               qreal ratio,
                               QSize imageSize);

/**
 * 以当前选区左上角为基准设置精确尺寸，超出图像时向左上平移，仍放不下则缩小。
 * @param current 当前选区。
 * @param size 目标尺寸（图像像素）。
 * @param imageSize 图像尺寸。
 * @return 新选区。
 */
QRectF resizedSelection(QRectF current, QSize size, QSize imageSize);

/**
 * 按比例由宽度推算高度。
 * @param width 宽度。
 * @param ratio 宽高比，小于等于 0 时返回 0。
 * @return 高度，至少为 1。
 */
int heightForWidth(int width, qreal ratio);

/**
 * 按比例由高度推算宽度。
 * @param height 高度。
 * @param ratio 宽高比，小于等于 0 时返回 0。
 * @return 宽度，至少为 1。
 */
int widthForHeight(int height, qreal ratio);

}  // namespace markshot::selection_aspect
