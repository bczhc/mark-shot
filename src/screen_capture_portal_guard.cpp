#include "screen_capture_portal_guard.h"

#include <atomic>

namespace markshot {
namespace {

std::atomic<int> g_interactiveScreenCastDepth{0};

}  // namespace

bool tryAcquireInteractiveScreenCast(QString *error)
{
    int expected = 0;
    if (!g_interactiveScreenCastDepth.compare_exchange_strong(expected, 1)) {
        if (error) {
            *error = QStringLiteral("ScreenCast portal request is already in progress");
        }
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

void releaseInteractiveScreenCast()
{
    g_interactiveScreenCastDepth.store(0);
}

bool interactiveScreenCastInProgress()
{
    return g_interactiveScreenCastDepth.load() != 0;
}

}  // namespace markshot
