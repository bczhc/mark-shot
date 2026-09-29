#include "shot_window_module.h"

#include "annotation_filters/image_filter_effects.h"
#include "annotation_filters/spotlight_overlay.h"

#include <algorithm>

using namespace markshot::shot;

namespace filters = markshot::annotation_filters;

QImage ShotWindow::filteredRegion(QRect sourceRect, MosaicStyle style, qreal strength) const
{
    const QRect frameBounds(QPoint(0, 0), m_frozenFrame.size());
    sourceRect = sourceRect.normalized().intersected(frameBounds);
    if (sourceRect.isEmpty()) {
        return {};
    }

    switch (style) {
    case MosaicStyle::Pixelate:
        return mosaicImage(sourceRect, qRound(strength));
    case MosaicStyle::Blur: {
        // 1. 向外多取一圈像素再模糊，边缘能混入区域外的颜色，不会出现硬边
        const int radius = filters::blurRadiusForStrength(strength);
        const QRect padded = sourceRect.adjusted(-radius * 2, -radius * 2, radius * 2, radius * 2)
                                 .intersected(frameBounds);
        const QImage blurred = filters::gaussianBlurred(m_frozenFrame.copy(padded), radius);
        // 2. 裁回原区域
        return blurred.copy(sourceRect.translated(-padded.topLeft()));
    }
    case MosaicStyle::Grayscale:
        return filters::grayscaled(m_frozenFrame.copy(sourceRect));
    case MosaicStyle::Invert:
        return filters::inverted(m_frozenFrame.copy(sourceRect));
    case MosaicStyle::Brighten:
        return filters::brightened(m_frozenFrame.copy(sourceRect), filters::brightnessAmountForStrength(strength));
    }
    return {};
}

void ShotWindow::drawMosaic(QPainter &painter, const Annotation &annotation, bool widgetCoordinates) const
{
    const QRect sourceRect = annotation.rect.normalized().toAlignedRect()
                                 .intersected(QRect(QPoint(0, 0), m_frozenFrame.size()));
    if (sourceRect.isEmpty()) {
        return;
    }

    const QImage filtered = filteredRegion(sourceRect, annotation.mosaicStyle, annotation.width);
    if (filtered.isNull()) {
        return;
    }

    // 像素化保持硬边块，其他滤镜允许平滑缩放
    painter.save();
    painter.setRenderHint(QPainter::SmoothPixmapTransform, annotation.mosaicStyle != MosaicStyle::Pixelate);
    painter.drawImage(widgetCoordinates ? imageRectToWidget(sourceRect) : QRectF(sourceRect), filtered);
    painter.restore();
}

void ShotWindow::drawSpotlightOverlay(QPainter &painter, QRectF area, bool widgetCoordinates) const
{
    // 1. 收集全部聚光灯矩形（含正在绘制的草稿），遮罩暗度取最大值
    QVector<filters::SpotlightHole> holes;
    qreal opacity = 0.0;
    const qreal scale = annotationSizeScale(widgetCoordinates);
    auto collect = [&](const Annotation &annotation) {
        if (annotation.tool != Tool::Rectangle || annotation.rectangleStyle != RectangleStyle::Spotlight) {
            return;
        }
        filters::SpotlightHole hole;
        hole.rect = widgetCoordinates ? imageRectToWidget(annotation.rect) : annotation.rect.normalized();
        hole.cornerRadius = annotation.cornerRadius * scale;
        hole.rotationDegrees = annotation.rotationDegrees;
        holes.append(hole);
        opacity = std::max<qreal>(opacity, annotation.color.alphaF());
    };
    for (const Annotation &annotation : m_annotations) {
        collect(annotation);
    }
    if (widgetCoordinates && m_draft.has_value()) {
        collect(*m_draft);
    }
    if (holes.isEmpty()) {
        return;
    }

    // 2. 一次性绘制遮罩，多个聚光灯重叠时不会叠加变暗
    filters::paintSpotlightOverlay(painter, area, holes, opacity);
}
