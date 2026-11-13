// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef __BLE_GLASSES_H__
#define __BLE_GLASSES_H__

// Scans for camera-equipped smart glasses (Meta Ray-Ban, Oakley Meta, Snap
// Spectacles, and similar) by matching their BLE advertisement fingerprints:
// Bluetooth SIG manufacturer (company) IDs, the "META_RB_GLASS" manufacturer
// data string, known product names, and fixed service UUIDs.
//
// Fingerprints sourced from public research (ZuckOff, yj_nearbyglasses):
//   - 0x01AB  Meta Platforms, Inc. (formerly Facebook)
//   - 0x058E  Meta Platforms Technologies, LLC
//   - 0x0D53  Luxottica Group S.p.A (manufactures Meta Ray-Bans/Oakley)
//   - 0x03C2  Snapchat, Inc. (Snap Spectacles)
//   - 0x05D6  Zhuhai Jieli Technology Co (Rogbird VisionPro / Rollme VistaView)
//   - 0xFEAA  Google/Eddystone service UUID (seen on Meta frames)
//   - 7905FFF0-B5CE-4E99-A40F-4B1E122D00D0  HeyCyan smart-glasses SDK UUID
//
// Detection is heuristic: a match means "camera-capable hardware of a known
// maker is in BLE range", NOT that recording is happening. False positives
// are possible (e.g. a VR headset from the same vendor).
void ble_glasses_detector();

#endif
