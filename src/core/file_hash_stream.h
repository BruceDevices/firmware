#pragma once

#include <stddef.h>
#include <stdint.h>

// Feed exact binary bytes to a hash without allocating a file-sized buffer.
// The caller opens the file at position zero and initializes/finalizes the hash.
template <typename FileType, typename HashType, typename YieldType>
bool feedFileToHash(FileType &file, HashType &hash, YieldType pause) {
    const size_t expectedSize = file.size();
    size_t remaining = expectedSize;
    uint8_t buffer[512];
    while (remaining > 0) {
        const size_t wanted = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        const int received = file.read(buffer, wanted);
        if (received <= 0 || size_t(received) > wanted) return false;
        hash.add(buffer, size_t(received));
        remaining -= size_t(received);
        pause();
    }
    // A truncated or extended file must not produce a successful partial hash.
    return file.size() == expectedSize;
}
