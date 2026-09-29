#include "annotation_filters/image_filter_effects.h"
#include "annotation_filters/spotlight_overlay.h"

#include <QPainter>
#include <QtTest/QtTest>

namespace {

/**
 * 生成左黑右白的测试图，用于检查模糊是否跨越边界。
 * @return 测试图。
 */
QImage splitImage()
{
    QImage image(40, 10, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    QPainter painter(&image);
    painter.fillRect(QRect(20, 0, 20, 10), Qt::white);
    painter.end();
    return image;
}

}  // namespace

class AnnotationFiltersTest final : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证模糊会在黑白边界处产生中间灰度，远离边界的像素保持原色。
     * @return 无返回值。
     */
    void blurSoftensEdges()
    {
        const QImage blurred = markshot::annotation_filters::gaussianBlurred(splitImage(), 6);
        QCOMPARE(blurred.size(), QSize(40, 10));
        const int edge = qRed(blurred.pixel(20, 5));
        QVERIFY2(edge > 40 && edge < 215, qPrintable(QString::number(edge)));
        QVERIFY(qRed(blurred.pixel(0, 5)) < 20);
        QVERIFY(qRed(blurred.pixel(39, 5)) > 235);
    }

    /**
     * 验证半径小于 1 时不做处理。
     * @return 无返回值。
     */
    void blurWithZeroRadiusKeepsImage()
    {
        const QImage source = splitImage();
        QCOMPARE(markshot::annotation_filters::gaussianBlurred(source, 0), source);
    }

    /**
     * 验证灰度、反色、提亮的像素结果，并保留透明度。
     * @return 无返回值。
     */
    void colorFiltersTransformPixels()
    {
        QImage source(1, 1, QImage::Format_ARGB32);
        source.setPixel(0, 0, qRgba(200, 100, 50, 255));

        const QRgb gray = markshot::annotation_filters::grayscaled(source).pixel(0, 0);
        QCOMPARE(qRed(gray), qGreen(gray));
        QCOMPARE(qGreen(gray), qBlue(gray));

        const QRgb inverted = markshot::annotation_filters::inverted(source).pixel(0, 0);
        QCOMPARE(qRed(inverted), 55);
        QCOMPARE(qGreen(inverted), 155);
        QCOMPARE(qBlue(inverted), 205);

        const QRgb brighter = markshot::annotation_filters::brightened(source, 0.5).pixel(0, 0);
        QCOMPARE(qRed(brighter), 228);
        QCOMPARE(qBlue(brighter), 153);

        QImage transparent(1, 1, QImage::Format_ARGB32);
        transparent.setPixel(0, 0, qRgba(10, 10, 10, 0));
        QCOMPARE(qAlpha(markshot::annotation_filters::inverted(transparent).pixel(0, 0)), 0);
    }

    /**
     * 验证聚光灯遮罩挖空保留区域，重叠保留区域只挖一次，没有保留区域时不绘制。
     * @return 无返回值。
     */
    void spotlightPathExcludesHoles()
    {
        const QRectF area(0, 0, 100, 100);
        QVERIFY(markshot::annotation_filters::spotlightDimPath(area, {}).isEmpty());

        markshot::annotation_filters::SpotlightHole first;
        first.rect = QRectF(10, 10, 30, 30);
        markshot::annotation_filters::SpotlightHole second;
        second.rect = QRectF(25, 25, 30, 30);
        const QPainterPath dim = markshot::annotation_filters::spotlightDimPath(area, {first, second});
        QVERIFY(dim.contains(QPointF(90, 90)));
        QVERIFY(!dim.contains(QPointF(20, 20)));
        QVERIFY(!dim.contains(QPointF(30, 30)));
        QVERIFY(!dim.contains(QPointF(50, 50)));
    }

    /**
     * 验证遮罩实际绘制时保留区域像素不变，区域外被压暗。
     * @return 无返回值。
     */
    void spotlightPaintsOutsideOnly()
    {
        QImage image(60, 60, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);
        markshot::annotation_filters::SpotlightHole hole;
        hole.rect = QRectF(20, 20, 20, 20);
        QPainter painter(&image);
        markshot::annotation_filters::paintSpotlightOverlay(painter, QRectF(image.rect()), {hole}, 1.0);
        painter.end();
        QCOMPARE(image.pixel(30, 30), qRgb(255, 255, 255));
        QVERIFY(qRed(image.pixel(5, 5)) < 120);
    }
};

QTEST_GUILESS_MAIN(AnnotationFiltersTest)

#include "annotation_filters_test.moc"
