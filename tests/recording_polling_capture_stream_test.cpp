#include "recording/recording_polling_capture_stream.h"

#include "screen_capture.h"

#include <QEventLoop>
#include <QPointer>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest/QtTest>

#include <functional>
#include <memory>

namespace {

// 测试替身在嵌套事件循环里执行的动作，模拟门户请求期间录制被停止
std::function<void()> g_duringCapture;
int g_captureCalls = 0;

}  // namespace

/**
 * 测试替身：替代平台抓帧，在嵌套事件循环中执行预设动作后返回一帧。
 * @param request 抓帧请求。
 * @return 固定尺寸的测试帧。
 */
CaptureResult captureScreenFrame(const CaptureRequest &request)
{
    ++g_captureCalls;
    if (g_duringCapture) {
        QEventLoop loop;
        QTimer::singleShot(0, &loop, [&loop] {
            if (g_duringCapture) {
                g_duringCapture();
            }
            loop.quit();
        });
        loop.exec();
    }
    CaptureResult result;
    result.image = QImage(4, 4, QImage::Format_ARGB32_Premultiplied);
    result.image.fill(Qt::black);
    result.sourceGeometry = request.sourceGeometry;
    return result;
}

class RecordingPollingCaptureStreamTest final : public QObject {
    Q_OBJECT

private slots:
    /**
     * 注册录制帧信号参数类型。
     * @return 无返回值。
     */
    void initTestCase()
    {
        qRegisterMetaType<markshot::recording::RecordingFrameSample>();
    }

    /**
     * 重置测试替身状态。
     * @return 无返回值。
     */
    void init()
    {
        g_duringCapture = nullptr;
        g_captureCalls = 0;
    }

    /**
     * 验证抓帧嵌套事件循环中销毁采集流后，返回路径不再访问已释放对象。
     * @return 无返回值。
     */
    void destroyedDuringCaptureDoesNotEmit()
    {
        markshot::recording::RecordingOptions options;
        options.mode = markshot::recording::RecordingMode::Gif;
        options.fps = 10;
        options.captureGeometry = QRect(0, 0, 4, 4);

        auto *stream = new markshot::recording::RecordingPollingCaptureStream(options);
        QPointer<markshot::recording::RecordingPollingCaptureStream> guard(stream);
        int frames = 0;
        connect(stream,
                &markshot::recording::RecordingCaptureStream::frameReady,
                this,
                [&frames](const markshot::recording::RecordingFrameSample &) { ++frames; });
        g_duringCapture = [stream] {
            stream->stop();
            delete stream;
        };

        QString error;
        QVERIFY(stream->start(&error));
        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 1000);
        QCOMPARE(g_captureCalls, 1);
        QCOMPARE(frames, 0);
    }

    /**
     * 验证抓帧期间仅停止采集流时，不再发出迟到的帧。
     * @return 无返回值。
     */
    void stoppedDuringCaptureDoesNotEmit()
    {
        markshot::recording::RecordingOptions options;
        options.mode = markshot::recording::RecordingMode::Gif;
        options.fps = 10;
        options.captureGeometry = QRect(0, 0, 4, 4);

        markshot::recording::RecordingPollingCaptureStream stream(options);
        QSignalSpy frameSpy(&stream, &markshot::recording::RecordingCaptureStream::frameReady);
        g_duringCapture = [&stream] { stream.stop(); };

        QString error;
        QVERIFY(stream.start(&error));
        QTRY_COMPARE_WITH_TIMEOUT(g_captureCalls, 1, 1000);
        QTest::qWait(50);
        QCOMPARE(frameSpy.count(), 0);
        QCOMPARE(g_captureCalls, 1);
    }

    /**
     * 验证接收方在帧信号中销毁采集流时，发射之后不再调度下一帧。
     * @return 无返回值。
     */
    void destroyedByFrameReceiverStopsScheduling()
    {
        markshot::recording::RecordingOptions options;
        options.mode = markshot::recording::RecordingMode::Gif;
        options.fps = 10;
        options.captureGeometry = QRect(0, 0, 4, 4);

        auto *stream = new markshot::recording::RecordingPollingCaptureStream(options);
        QPointer<markshot::recording::RecordingPollingCaptureStream> guard(stream);
        connect(stream,
                &markshot::recording::RecordingCaptureStream::frameReady,
                this,
                [stream](const markshot::recording::RecordingFrameSample &) {
                    stream->stop();
                    delete stream;
                });

        QString error;
        QVERIFY(stream->start(&error));
        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 1000);
        QTest::qWait(50);
        QCOMPARE(g_captureCalls, 1);
    }
};

QTEST_GUILESS_MAIN(RecordingPollingCaptureStreamTest)

#include "recording_polling_capture_stream_test.moc"
