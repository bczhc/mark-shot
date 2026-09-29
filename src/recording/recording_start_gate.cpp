#include "recording/recording_start_gate.h"

namespace markshot::recording {

bool RecordingStartGate::tryEnter(QString *error)
{
    if (m_active) {
        if (error) {
            *error = QStringLiteral("recording is already active");
        }
        return false;
    }
    m_active = true;
    if (error) {
        error->clear();
    }
    return true;
}

void RecordingStartGate::leave()
{
    m_active = false;
}

bool RecordingStartGate::active() const
{
    return m_active;
}

}  // namespace markshot::recording
