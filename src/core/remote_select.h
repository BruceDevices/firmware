#ifndef BRUCE_REMOTE_SELECT_H
#define BRUCE_REMOTE_SELECT_H

#include <stdint.h>

// Protected by the existing recursive input mutex. Generation and duration are
// published together, so a menu cannot observe an ID without its payload.
struct RemoteSelectState {
    uint32_t generation = 0;
    bool longPress = false;
    bool legacyEvent = false;
};
extern RemoteSelectState remoteSelectState;
extern volatile bool SelPress, SerialCmdPress, AnyKeyPress;
void inputLock();
void inputUnlock();

// commandGeneration is owned by one synchronous nav callback. Its zero value
// starts a command; repeats retain the ID for legacy consumers. An older HTTP
// callback cannot republish after a newer command supersedes it.
inline void publishRemoteSelect(uint32_t durationMs, uint32_t &commandGeneration) {
    inputLock();
    if (commandGeneration == 0) {
        if (++remoteSelectState.generation == 0) ++remoteSelectState.generation;
        commandGeneration = remoteSelectState.generation;
        remoteSelectState.longPress = durationMs >= 500;
    }
    if (commandGeneration == remoteSelectState.generation) {
        remoteSelectState.legacyEvent = true;
        AnyKeyPress = SerialCmdPress = SelPress = true;
    }
    inputUnlock();
}

inline RemoteSelectState readRemoteSelectState() {
    inputLock();
    const RemoteSelectState state = remoteSelectState;
    inputUnlock();
    return state;
}

class RemoteMenuSelect {
public:
    explicit RemoteMenuSelect(uint32_t entryGeneration) : seen(entryGeneration) {}

    bool consume(const RemoteSelectState &state, bool &longPress) {
        if (state.generation == seen) return false;
        seen = state.generation;
        longPress = state.longPress;
        return true;
    }

private:
    uint32_t seen;
};

#endif
