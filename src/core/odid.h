#ifndef __ODID_H
#define __ODID_H

#include <stddef.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// ASTM F3411 / ASD-STAN Remote ID (Open Drone ID) decoder - shared by the BLE
// detector (service data on 0xFFFA) and the Wi-Fi airspace page (beacon or NAN
// vendor IEs).
//
// The layout is the reference one (opendroneid-core-c, `*_encoded` structs: bit
// fields LSb first, every multi-byte field big-endian):
//
//   message pack  [proto:4|type:4 = 0xF?][single message size = 25][count]
//                 [message * count]
//   Basic ID      [0x0?][UAType:4|IDType:4][UAS ID, 20 bytes]
//   Location      [0x1?][SpeedMult:1 EWDirection:1 HeightType:1 rsvd:1 Status:4]
//                 [direction u8][speedH u8][speedV s8][lat s32][lon s32]
//                 [altBaro u16][altGeo u16][height u16][acc..][timestamp u16]..
//   System        [0x4?][OpLocType:2 ClassType:3 rsvd:3][opLat s32][opLon s32]..
//   Operator ID   [0x5?][type u8][operator id, 20 bytes]
//
// lat/lon are int32 degrees * 1e7; altitudes are uint16 in 0.5 m steps with a
// -1000 m offset. Only what a screen can honestly show is decoded: identity,
// aircraft type, status, position, altitude, speed and the operator's position.
// ---------------------------------------------------------------------------

struct OdidData {
    bool valid;
    char uasId[21]; // Basic ID, null padded
    char opId[21];  // Operator ID, null padded
    uint8_t uaType; // 0 none, 1 aeroplane, 2 helicopter/multirotor, ...
    uint8_t status; // 0 undeclared, 1 ground, 2 airborne, 3 emergency, 4 failure
    double lat, lon;     // drone position
    double opLat, opLon; // operator (pilot) position
    float altGeo;        // metres above WGS84
    float height;        // metres above take-off or ground (HeightType)
    float speedH, speedV;
};

/// Decodes a message pack that starts at its 0xF? header byte. `len` is the
/// number of bytes available; anything that is not a complete, well-formed pack
/// is rejected rather than half-read.
bool odidDecodePack(const uint8_t *pack, size_t len, OdidData &out);

/// BLE service data for 0xFFFA: [0x0D application code][counter][pack]. The
/// application code is optional in the wild, so it is skipped when present.
bool odidDecodeServiceData(const uint8_t *sd, size_t len, OdidData &out);

/// Beacon vendor IE: [OUI 3][0x0D][counter][pack]. Returns true when the OUI is
/// the ODID one (ASD-STAN FA:0B:BC) and the payload really decoded.
bool odidDecodeBeaconVendorIe(const uint8_t *ie, size_t len, OdidData &out);

/// Human name for the Basic ID's UA type.
const char *odidUaTypeName(uint8_t uaType);

/// Human name for the Location message's operational status.
const char *odidStatusName(uint8_t status);

#endif
