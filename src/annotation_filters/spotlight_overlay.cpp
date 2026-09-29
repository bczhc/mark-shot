#include "annotation_filters/spotlight_overlay.h"

#include <QPainter>
#include <QTransform>

#include <algorithm>

namespace markshot::annotation_filters {
namespace {

/**
 * 生成单个保留区域的路径，含圆角与绕中心旋转。
 * @param hole 保留区域。
 * @return 保留区域路径。
 */
QPainterPath holePath(const SpotlightHole &hole)
{
    const QRectF rect = hole.rect.normalized();
    QPainterPath path;
    if (hole.cornerRadius > 0.0) {
        path.addRoundedRect(rect, hole.cornerRadius, hole.cornerRadius);
    } else {
        path.addRect(rect);
    }
    if (qFuzzyIsNull(hole.rotationDegrees)) {
        return path;
    }
    QTransform transform;
    transform.translate(rect.center().x(), rect.center().y());
    transform.rotate(hole.rotationDegrees);
    transform.translate(-rect.center().x(), -rect.center().y());
    return transform.map(path);
}

}  // namespace

QPainterPath spotlightDimPath(const QRectF &area, const QVector<SpotlightHole> &holes)
{
    if (area.isEmpty() || holes.isEmpty()) {
        return {};
    }

    // 1. 先合并保留区域，重叠部分只挖空一次
    QPainterPath united;
    united.setFillRule(Qt::WindingFill);
    for (const SpotlightHole &hole : holes) {
        if (!hole.rect.normalized().isEmpty()) {
            united = united.united(holePath(hole));
        }
    }

    // 2. 外框减去并集得到遮罩
    QPainterPath dim;
    dim.addRect(area);
    return united.isEmpty() ? dim : dim.subtracted(united);
}

QColor spotlightDimColor(qreal opacity)
{
    const qreal clamped = std::clamp<qreal>(opacity, 0.0, 1.0);
    return QColor(0, 0, 0, qRound(kSpotlightMaxDimAlpha * clamped));
}

void paintSpotlightOverlay(QPainter &painter,
                           const QRectF &area,
                           const QVector<SpotlightHole> &holes,
                           qreal opacity)
{
    const QPainterPath dim = spotlightDimPath(area, holes);
    if (dim.isEmpty()) {
        return;
    }
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setClipRect(area, Qt::IntersectClip);
    painter.fillPath(dim, spotlightDimColor(opacity));
    painter.restore();
}

}  // namespace markshot::annotation_filters
