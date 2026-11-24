#include "odid.h"

#include <string.h>

static const size_t ODID_MESSAGE_SIZE = 25;
static const size_t ODID_MAX_MESSAGES = 9;

static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
static uint16_t be16(const uint8_t *p) { return (uint16_t)(((uint16_t)p[0] << 8) | p[1]); }
static float altitude(uint16_t raw) { return raw * 0.5f - 1000.0f; }

// 20-byte id fields are null/space padded, not necessarily terminated.
static void copyId(char *dst, size_t dstSize, const uint8_t *src, size_t len) {
    memset(dst, 0, dstSize);
    size_t i = 0;
    for (; i < len && i + 1 < dstSize; i++) {
        const uint8_t c = src[i];
        if (c < 0x20 || c > 0x7e) break;
        dst[i] = (char)c;
    }
}

static void decodeMessage(OdidData &d, const uint8_t *m) {
    switch (m[0] >> 4) {
        case 0x0: // Basic ID
            d.uaType = m[1] & 0x0F;
            copyId(d.uasId, sizeof(d.uasId), m + 2, 20);
            break;
        case 0x1: { // Location / Vector
            d.status = (m[1] >> 4) & 0x0F;
            const bool speedMult = (m[1] & 0x01) != 0;
            const bool ew = (m[1] & 0x02) != 0;
            (void)ew; // heading, not shown: it is meaningless without a compass
            d.speedH = speedMult ? (m[3] * 0.75f + 255 * 0.25f) : (m[3] * 0.25f);
            d.speedV = ((int8_t)m[4]) * 0.5f;
            d.lat = (int32_t)be32(m + 5) / 1e7;
            d.lon = (int32_t)be32(m + 9) / 1e7;
            d.altGeo = altitude(be16(m + 15));
            d.height = altitude(be16(m + 17));
            break;
        }
        case 0x4: // System: where the pilot is standing
            d.opLat = (int32_t)be32(m + 2) / 1e7;
            d.opLon = (int32_t)be32(m + 6) / 1e7;
            break;
        case 0x5: // Operator ID
            copyId(d.opId, sizeof(d.opId), m + 2, 20);
            break;
        default: break; // Auth / Self-ID / unknown: nothing a screen can show
    }
}

bool odidDecodePack(const uint8_t *pack, size_t len, OdidData &out) {
    if (pack == nullptr || len < 3) return false;
    if ((pack[0] >> 4) != 0x0F) return false; // not a message pack
    if (pack[1] != ODID_MESSAGE_SIZE) return false;
    const size_t count = pack[2];
    if (count < 1 || count > ODID_MAX_MESSAGES) return false;
    if (3 + count * ODID_MESSAGE_SIZE > len) return false;

    OdidData d = {};
    d.valid = true;
    for (size_t i = 0; i < count; i++) decodeMessage(d, pack + 3 + i * ODID_MESSAGE_SIZE);
    // A pack with no Basic ID and no Location is not something to show.
    if (d.uasId[0] == 0 && d.lat == 0.0 && d.lon == 0.0) return false;
    out = d;
    return true;
}

bool odidDecodeServiceData(const uint8_t *sd, size_t len, OdidData &out) {
    if (sd == nullptr || len < 4) return false;
    size_t off = 0;
    if (sd[0] == 0x0D) off = 2; // application code + counter
    if (off >= len) return false;
    return odidDecodePack(sd + off, len - off, out);
}

bool odidDecodeBeaconVendorIe(const uint8_t *ie, size_t len, OdidData &out) {
    // ASD-STAN OUI FA:0B:BC, then the 0x0D message-pack application code.
    if (ie == nullptr || len < 7) return false;
    if (!(ie[0] == 0xFA && ie[1] == 0x0B && ie[2] == 0xBC)) return false;
    if (ie[3] != 0x0D) return false;
    return odidDecodePack(ie + 5, len - 5, out); // skip the counter as well
}

const char *odidUaTypeName(uint8_t t) {
    switch (t) {
        case 1: return "aeroplane";
        case 2: return "multirotor";
        case 3: return "gyroplane";
        case 4: return "hybrid lift";
        case 6: return "glider";
        case 7: return "kite";
        case 8: return "balloon";
        case 9: return "captive balloon";
        case 10: return "airship";
        case 12: return "rocket";
        case 14: return "obstacle";
        case 15: return "other";
        default: return "UAS";
    }
}

const char *odidStatusName(uint8_t s) {
    switch (s) {
        case 1: return "ground";
        case 2: return "AIRBORNE";
        case 3: return "EMERGENCY";
        case 4: return "RID FAILURE";
        default: return "undeclared";
    }
}
