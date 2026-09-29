#pragma once

#include <QImage>

namespace markshot::annotation_filters {

/**
 * 对图像做近似高斯模糊（三次盒式模糊）。
 * @param source 源图像，任意格式。
 * @param radius 模糊半径，单位为像素，小于 1 时返回原图副本。
 * @return ARGB32_Premultiplied 格式的模糊结果。
 */
QImage gaussianBlurred(const QImage &source, int radius);

/**
 * 把图像转为灰度，保留透明度。
 * @param source 源图像。
 * @return ARGB32_Premultiplied 格式的灰度图。
 */
QImage grayscaled(const QImage &source);

/**
 * 对图像 RGB 通道反色，保留透明度。
 * @param source 源图像。
 * @return ARGB32_Premultiplied 格式的反色图。
 */
QImage inverted(const QImage &source);

/**
 * 按比例提亮图像，每个通道向白色靠近。
 * @param source 源图像。
 * @param amount 提亮比例，0 表示不变，1 表示全白，超出范围会被截断。
 * @return ARGB32_Premultiplied 格式的提亮图。
 */
QImage brightened(const QImage &source, qreal amount);

/**
 * 把工具强度（滚轮/滑块数值）换算为模糊半径。
 * @param strength 工具强度，与马赛克块大小共用同一数值。
 * @return 模糊半径，范围 1 至 48。
 */
int blurRadiusForStrength(qreal strength);

/**
 * 把工具强度换算为提亮比例。
 * @param strength 工具强度。
 * @return 提亮比例，范围 0.05 至 0.9。
 */
qreal brightnessAmountForStrength(qreal strength);

}  // namespace markshot::annotation_filters
