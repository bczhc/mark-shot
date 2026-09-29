#include "capture_delay/capture_delay_option.h"
#include "capture_delay/capture_delay_scheduler.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

class CaptureDelayTest final : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证整数、小数与边界秒数都能换算成毫秒。
     * @return 无返回值。
     */
    void parsesValidSeconds()
    {
        QString error;
        QCOMPARE(markshot::capture_delay::parseCaptureDelaySeconds(QStringLiteral("3"), &error), 3000);
        QVERIFY(error.isEmpty());
        QCOMPARE(markshot::capture_delay::parseCaptureDelaySeconds(QStringLiteral(" 1.5 "), &error), 1500);
        QCOMPARE(markshot::capture_delay::parseCaptureDelaySeconds(QStringLiteral("0"), &error), 0);
        QCOMPARE(markshot::capture_delay::parseCaptureDelaySeconds(QStringLiteral("60"), &error), 60000);
    }

    /**
     * 验证非数字、负数、超上限与非有限值会被拒绝并给出错误信息。
     * @return 无返回值。
     */
    void rejectsInvalidSeconds()
    {
        const QStringList invalid = {QStringLiteral(""), QStringLiteral("abc"), QStringLiteral("-1"),
                                     QStringLiteral("60.5"), QStringLiteral("inf"), QStringLiteral("nan")};
        for (const QString &text : invalid) {
            QString error;
            QVERIFY2(!markshot::capture_delay::parseCaptureDelaySeconds(text, &error).has_value(),
                     qPrintable(text));
            QVERIFY(!error.isEmpty());
        }
    }

    /**
     * 验证计划到期后执行一次动作，并正确报告待执行状态。
     * @return 无返回值。
     */
    void firesOnceAfterDelay()
    {
        markshot::capture_delay::CaptureDelayScheduler scheduler;
        QSignalSpy pendingSpy(&scheduler, &markshot::capture_delay::CaptureDelayScheduler::pendingChanged);
        int fired = 0;
        scheduler.schedule(30, [&fired] { ++fired; });
        QVERIFY(scheduler.pending());
        QVERIFY(scheduler.remainingMs() <= 30);

        QTRY_COMPARE_WITH_TIMEOUT(fired, 1, 1000);
        QVERIFY(!scheduler.pending());
        QCOMPARE(scheduler.remainingMs(), 0);
        QCOMPARE(pendingSpy.count(), 2);
        QCOMPARE(pendingSpy.at(0).at(0).toBool(), true);
        QCOMPARE(pendingSpy.at(1).at(0).toBool(), false);
        QTest::qWait(50);
        QCOMPARE(fired, 1);
    }

    /**
     * 验证新请求替换尚未触发的旧请求，只执行最后一次。
     * @return 无返回值。
     */
    void newScheduleReplacesPending()
    {
        markshot::capture_delay::CaptureDelayScheduler scheduler;
        QSignalSpy pendingSpy(&scheduler, &markshot::capture_delay::CaptureDelayScheduler::pendingChanged);
        int first = 0;
        int second = 0;
        scheduler.schedule(20, [&first] { ++first; });
        scheduler.schedule(40, [&second] { ++second; });

        QTRY_COMPARE_WITH_TIMEOUT(second, 1, 1000);
        QTest::qWait(50);
        QCOMPARE(first, 0);
        // 替换不重复发出 pending=true
        QCOMPARE(pendingSpy.count(), 2);
    }

    /**
     * 验证取消后动作不再执行，没有计划时取消返回 false。
     * @return 无返回值。
     */
    void cancelPreventsFiring()
    {
        markshot::capture_delay::CaptureDelayScheduler scheduler;
        int fired = 0;
        QVERIFY(!scheduler.cancel());
        scheduler.schedule(20, [&fired] { ++fired; });
        QVERIFY(scheduler.cancel());
        QVERIFY(!scheduler.pending());
        QTest::qWait(60);
        QCOMPARE(fired, 0);
    }

    /**
     * 验证动作内部重新计划时，新计划保持有效。
     * @return 无返回值。
     */
    void actionCanReschedule()
    {
        markshot::capture_delay::CaptureDelayScheduler scheduler;
        int fired = 0;
        scheduler.schedule(0, [&scheduler, &fired] {
            ++fired;
            scheduler.schedule(10, [&fired] { ++fired; });
        });
        QTRY_COMPARE_WITH_TIMEOUT(fired, 2, 1000);
        QVERIFY(!scheduler.pending());
    }
};

QTEST_GUILESS_MAIN(CaptureDelayTest)

#include "capture_delay_test.moc"
