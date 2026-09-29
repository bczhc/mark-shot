#pragma once

#include <QColor>
#include <QPainterPath>
#include <QRectF>
#include <QVector>

class QPainter;

namespace markshot::annotation_filters {

/**
 * 聚光灯保留区域，坐标与调用方绘制坐标系一致。
 */
struct SpotlightHole {
    QRectF rect;
    qreal cornerRadius = 0.0;
    qreal rotationDegrees = 0.0;
};

// 聚光灯遮罩在不透明度 100% 时的暗度
inline constexpr int kSpotlightMaxDimAlpha = 170;

/**
 * 生成聚光灯遮罩路径：外框减去全部保留区域的并集。
 * @param area 需要压暗的范围，通常是截图选区。
 * @param holes 保留区域，重叠区域不会被重复压暗。
 * @return 遮罩路径，没有保留区域时返回空路径。
 */
QPainterPath spotlightDimPath(const QRectF &area, const QVector<SpotlightHole> &holes);

/**
 * 按不透明度计算遮罩颜色。
 * @param opacity 标注不透明度，范围 0 至 1。
 * @return 半透明黑色。
 */
QColor spotlightDimColor(qreal opacity);

/**
 * 在 area 内绘制聚光灯遮罩。
 * @param painter 绘制器。
 * @param area 需要压暗的范围。
 * @param holes 保留区域。
 * @param opacity 遮罩不透明度，多个聚光灯时取最大值。
 * @return 无返回值。
 */
void paintSpotlightOverlay(QPainter &painter,
                           const QRectF &area,
                           const QVector<SpotlightHole> &holes,
                           qreal opacity);

}  // namespace markshot::annotation_filters
