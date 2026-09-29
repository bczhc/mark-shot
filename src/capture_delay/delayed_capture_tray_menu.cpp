#include "capture_delay/delayed_capture_tray_menu.h"

#include "ui/i18n.h"

#include <QAction>
#include <QMenu>

#include <array>
#include <memory>
#include <utility>

namespace markshot::capture_delay {

QMenu *createDelayedCaptureMenu(DelayedCaptureMenuCallbacks callbacks, QWidget *parent)
{
    auto *menu = new QMenu(MS_TR("Delayed Capture"), parent);
    auto shared = std::make_shared<DelayedCaptureMenuCallbacks>(std::move(callbacks));

    // 1. 常用延时预设
    constexpr std::array<int, 3> kPresetSeconds = {3, 5, 10};
    for (const int seconds : kPresetSeconds) {
        menu->addAction(MS_TR("After %1 seconds").arg(seconds), menu, [shared, seconds] {
            if (shared->schedule) {
                shared->schedule(seconds * 1000);
            }
        });
    }

    // 2. 取消项只在存在待执行计划时可用
    menu->addSeparator();
    QAction *cancelAction = menu->addAction(MS_TR("Cancel Delayed Capture"), menu, [shared] {
        if (shared->cancel) {
            shared->cancel();
        }
    });
    cancelAction->setEnabled(false);
    QObject::connect(menu, &QMenu::aboutToShow, cancelAction, [shared, cancelAction] {
        cancelAction->setEnabled(shared->pending && shared->pending());
    });
    return menu;
}

}  // namespace markshot::capture_delay
