#include "selection_aspect/selection_aspect_ratio.h"

#include <QtTest/QtTest>

using Handle = markshot::shot::types::SelectionDrag;

class SelectionAspectRatioTest final : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证按比例创建选区时向指针方向生长，并覆盖指针。
     * @return 无返回值。
     */
    void creationKeepsRatioTowardPointer()
    {
        const QSize image(1000, 1000);
        QRectF rect = markshot::selection_aspect::constrainedCreationRect(QPointF(100, 100), QPointF(260, 150),
                                                                        16.0 / 9.0, image);
        QCOMPARE(rect.topLeft(), QPointF(100, 100));
        QCOMPARE(rect.width(), 160.0);
        QCOMPARE(rect.height(), 90.0);

        // 向左上拖动
        rect = markshot::selection_aspect::constrainedCreationRect(QPointF(500, 500), QPointF(400, 450), 1.0, image);
        QCOMPARE(rect, QRectF(400, 400, 100, 100));
    }

    /**
     * 验证锚点靠近图像边缘时等比缩小，不越界也不破坏比例。
     * @return 无返回值。
     */
    void creationClampsInsideImage()
    {
        const QRectF rect = markshot::selection_aspect::constrainedCreationRect(QPointF(900, 100), QPointF(1200, 150),
                                                                              2.0, QSize(1000, 1000));
        QCOMPARE(rect.right(), 1000.0);
        QCOMPARE(rect.width(), 100.0);
        QCOMPARE(rect.height(), 50.0);
    }

    /**
     * 验证自由比例时返回普通矩形。
     * @return 无返回值。
     */
    void freeRatioReturnsPlainRect()
    {
        QCOMPARE(markshot::selection_aspect::constrainedCreationRect(QPointF(10, 10), QPointF(40, 90), 0.0, QSize(100, 100)),
                 QRectF(10, 10, 30, 80));
    }

    /**
     * 验证拖动角时以对角为锚点保持比例，拖动边时另一边按比例居中变化，移动不受影响。
     * @return 无返回值。
     */
    void adjustingHandlesKeepsRatio()
    {
        const QSize image(1000, 1000);
        const QRectF before(100, 100, 200, 100);

        const QRectF corner = markshot::selection_aspect::constrainedAdjustedRect(
            before, Handle::BottomRight, QRectF(100, 100, 400, 120), 2.0, image);
        QCOMPARE(corner, QRectF(100, 100, 400, 200));

        const QRectF right = markshot::selection_aspect::constrainedAdjustedRect(
            before, Handle::Right, QRectF(100, 100, 300, 100), 2.0, image);
        QCOMPARE(right.width(), 300.0);
        QCOMPARE(right.height(), 150.0);
        QCOMPARE(right.center().y(), before.center().y());

        const QRectF moved(150, 150, 200, 100);
        QCOMPARE(markshot::selection_aspect::constrainedAdjustedRect(before, Handle::Move, moved, 2.0, image), moved);
    }

    /**
     * 验证精确尺寸以左上角为基准，超出图像时向内平移。
     * @return 无返回值。
     */
    void resizeKeepsSelectionInsideImage()
    {
        QCOMPARE(markshot::selection_aspect::resizedSelection(QRectF(50, 50, 10, 10), QSize(200, 100), QSize(1000, 1000)),
                 QRectF(50, 50, 200, 100));
        QCOMPARE(markshot::selection_aspect::resizedSelection(QRectF(900, 900, 10, 10), QSize(200, 300), QSize(1000, 1000)),
                 QRectF(800, 700, 200, 300));
        QCOMPARE(markshot::selection_aspect::resizedSelection(QRectF(0, 0, 10, 10), QSize(5000, 5000), QSize(1000, 800)),
                 QRectF(0, 0, 1000, 800));
    }

    /**
     * 验证宽高联动换算。
     * @return 无返回值。
     */
    void linkedDimensions()
    {
        QCOMPARE(markshot::selection_aspect::heightForWidth(1920, 16.0 / 9.0), 1080);
        QCOMPARE(markshot::selection_aspect::widthForHeight(1080, 16.0 / 9.0), 1920);
        QCOMPARE(markshot::selection_aspect::heightForWidth(100, 0.0), 0);
    }
};

QTEST_GUILESS_MAIN(SelectionAspectRatioTest)

#include "selection_aspect_ratio_test.moc"
