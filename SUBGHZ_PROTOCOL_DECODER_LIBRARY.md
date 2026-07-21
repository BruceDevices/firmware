# Sub-GHz Protocol Decoder Library

## Masalah

Bruce Firmware saat ini hanya bisa men-decode ~16 protokol OOK pasif (Princeton, CAME, NICE, Holtek, dll) + KeeLoq via factor matching (`te`, `sync`, `zero`, `one`). Sinyal yang tidak cocok dengan factor-faktor ini hanya bisa dicapture sebagai RAW — tidak diidentifikasi, tidak di-decode.

Padahal CC1101 yang sama mampu menangkap **semua** protokol Sub-GHz di 300–928 MHz. Yang kurang adalah decoder library-nya.

## Tujuan

Menambahkan protocol decoder library yang komprehensif ke Bruce, mengadopsi decoder-decoder dari Flipper Zero protocol stack. Target: **59 protokol total** (Bruce saat ini ~17 + 12 generic = 29, tapi generic tidak berguna).

## Arsitektur

### Pendekatan: Hybrid Factor + Callback

```
                            ┌──────────────────────────┐
                            │    RMT RX (CC1101 GDO0)   │
                            │    durations[] (signed µs)│
                            └──────────┬───────────────┘
                                       │
                            ┌──────────▼───────────────┐
                            │   Signal Segmentation     │
                            │   (gap detection, noise   │
                            │    filtering, glitch      │
                            │    rejection)             │
                            └──────────┬───────────────┘
                                       │
                            ┌──────────▼───────────────┐
                            │   Protocol Scan Loop      │
                            │                           │
                            │  ┌─────────────────────┐  │
                            │  │ Factor Match (Cepat) │──│── OOK protokol sederhana
                            │  │ te/sync/zero/one    │  │   (Princeton, CAME, dll)
                            │  └─────────────────────┘  │
                            │                           │
                            │  ┌─────────────────────┐  │
                            │  │ Callback Decode      │──│── Protokol kompleks
                            │  │ (decode_cb per       │  │   (FAAC, Somfy, TPMS,
                            │  │  protokol)           │  │    SecPlus, Hormann)
                            │  └─────────────────────┘  │
                            └──────────┬───────────────┘
                                       │
                            ┌──────────▼───────────────┐
                            │   Post-Processing         │
                            │   - Rolling code unlock   │
                            │   - Manufacturer ident    │
                            │   - .sub serialization    │
                            └──────────────────────────┘
```

### Extension pada RfProtocolDef

```cpp
struct RfProtocolDef {
    const char *name;     // Nama protokol
    uint16_t te;          // Base pulse (µs)
    HighLow sync;         // Sync pulse factor
    HighLow zero;         // Bit 0 encoding
    HighLow one;          // Bit 1 encoding
    uint8_t bits;         // Panjang bit (0 = variable)
    bool inverted;        // Signal level inverted?
    uint8_t flags;        // RF_PF_HAS_SYNC, RF_PF_FIXED_LEN, dll

    // --- BARU: Callback untuk protokol kompleks ---
    bool (*decode)(const std::vector<int>& durations, RfCodes& out);
    bool (*encode)(const RfCodes& in, std::vector<int>& durations);
};
```

Jika `decode` == NULL → gunakan factor matching existing.  
Jika `decode` != NULL → panggil callback.  
Encoder juga sama.

## Protocol List Target

### Existing di Bruce (13 protokol OOK + KeeLoq + Chamberlain)
Tetap dipertahankan dengan factor matching.

### Baru dari Flipper Zero Port (41 protokol baru)

#### Simple OOK (factor-based — mudah ditambah)
| Protocol | Bits | te | Sync | Zero | One |
|----------|------|----|------|------|-----|
| Megacode | 48 | 400 | - | - | - |
| PowerSmart | 48 | 382 | - | - | - |
| Marantec | 24 | 450 | - | - | - |
| Marantec24 | 24 | 500 | - | - | - |
| Intertechno_V3 | 56 | 560 | - | - | - |
| SMC5326 | 48 | 400 | - | - | - |
| Linear_Delta3 | 42 | 500 | - | - | - |
| Dooya | 48 | 600 | - | - | - |
| Honeywell | 48 | 400 | - | - | - |
| Magellan | 56 | 300 | - | - | - |
| Nero_Sketch | 48 | 600 | - | - | - |
| Nero_Radio | 48 | 350 | - | - | - |
| IDo | 56 | 650 | - | - | - |
| Bett | 48 | 450 | - | - | - |
| Doitrand | 42 | 400 | - | - | - |
| Gangi | - | - | - | - | - |
| HollArm | - | - | - | - | - |
| Hay21 | - | - | - | - | - |
| Revers_RB2 | - | - | - | - | - |
| Feron | - | - | - | - | - |
| Roger | - | - | - | - | - |
| Elplast | - | - | - | - | - |
| Treadmill37 | - | - | - | - | - |
| Beninca_Arc | - | - | - | - | - |
| Jarolift | - | - | - | - | - |
| Ditec_Gol4 | - | - | - | - | - |
| KeyFinder | - | - | - | - | - |
| Kinggates_Stylo_4K | - | - | - | - | - |
| Legrand | - | - | - | - | - |
| Dickert_MAHS | - | - | - | - | - |
| Alutech_at_4N | - | - | - | - | - |

Note: Simple OOK di atas perlu dianalisis timing-nya lebih lanjut. Beberapa mungkin punya sync pattern atau encoding yang berbeda.

#### Rolling Code / Kompleks (callback-based)
| Protocol | Tipe | Notes |
|----------|------|-------|
| FAAC_SLH | Rolling code | Proprietary FAAC algorithm |
| NiceFlor_S | Rolling code | Nice FLO-R (flor) |
| SecPlus_v1 | Rolling code | Chamberlain Security+ v1 |
| SecPlus_v2 | Rolling code | Chamberlain Security+ v2 |
| Hormann | Rolling code | HSM rolling code |
| Somfy_Telis | Rolling code | Somfy RTS |
| Somfy_Keytis | Rolling code | Somfy RTS |
| CAME_Twee | Rolling code | CAME Twin protocol |
| CAME_Atomo | Rolling code | CAME Atomo |
| Honeywell_WDB | Roll/timed | Honeywell wireless doorbell |
| Chamb_Code | Fixed | Chamberlain 9-bit (Bruce sudah punya) |

#### TPMS (callback-based — Manchester/PWM decoding)
| Protocol | Notes |
|----------|-------|
| Schrader_GG4 | Tire pressure sensor |
| Ford | Ford TPMS |
| Renault | Renault TPMS |
| Citroen | Citroen TPMS |
| PMV107J | Toyota TPMS |

## Dampak Positif

1. **Dari 13 → ~55 protokol yang otomatis terdeteksi** — sinyal yang tadinya cuma RAW sekarang kebaca namanya, termasuk rolling code.
2. **Tidak perlu ganti hardware** — CC1101 yang sudah dipakai Bruce cukup. Decoder adalah pure software.
3. **.sub file compatibility dengan Flipper Zero** — format file yang sama bisa dipertukarkan.
4. **Protocol scanner jadi lebih cerdas** — bukan cuma nampilin "RAW signal found", tapi "Princeton 24bit key=0x123456 ditemukan".
5. **Transmit dengan parameter yang benar** — protokol yang terdeteksi bisa ditransmit ulang dengan encoding yang tepat, bukan sekedar RAW replay.

## File yang Akan Dimodifikasi

| File | Perubahan |
|------|-----------|
| `src/modules/rf/protocols/rf_protocol.h` | Tambah field decode/encode callback di RfProtocolDef |
| `src/modules/rf/protocols/rf_decoder.h/.cpp` | Tambah dispatch logic untuk callback decoder |
| `src/modules/rf/protocols/rf_encoder.h/.cpp` | Tambah dispatch logic untuk callback encoder |
| `src/modules/rf/protocols/rf_registry.h/.cpp` | Tambah entry untuk semua protokol baru |
| `src/modules/rf/rf_scan.cpp` | Wire callback decoder ke pipeline scan |
| `src/modules/rf/rf_send.cpp` | Wire callback encoder ke send pipeline |
| `src/modules/rf/structs.h` | Tambah field baru di RfCodes untuk protokol spesifik |
| `src/modules/rf/save.cpp` | Update .sub serializer untuk protokol baru |

### File Protokol Baru
`src/modules/rf/protocols/decoders/`
Setiap protokol complex mendapat file sendiri.

## Implementation Plan

### Phase 1: Arsitektur
- [ ] Tambah field callback di RfProtocolDef
- [ ] Update rf_decode_ook() untuk coba callback
- [ ] Update sendRfCommand() untuk encoder callback
- [ ] Update rf_registry dengan protokol baru (simple OOK dulu)

### Phase 2: Simple OOK Decoders
- [ ] Tambah semua OOK protokol yang cocok dengan factor model
- [ ] Uji dengan sinyal real

### Phase 3: Rolling Code Decoders
- [ ] Buat decoder FAAC SLH
- [ ] Buat decoder SecPlus v1/v2
- [ ] Buat decoder Somfy RTS
- [ ] Buat decoder Hormann
- [ ] Buat decoder NiceFlor_S
- [ ] Uji dengan sinyal real

### Phase 4: TPMS Decoders
- [ ] Buat decoder Schrader GG4
- [ ] Buat decoder Ford
- [ ] Buat decoder Renault
- [ ] Buat decoder Citroen
- [ ] Buat decoder PMV107J (Toyota)
- [ ] Uji dengan sinyal real
