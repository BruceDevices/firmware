# SubGHz V1 Sample Validation Matrix

Generated: `2026-05-23`

## Summary
- Decoder samples parsed: `47`
- Encoder samples parsed: `34`
- Decoder sample types: `raw=47`, `key=0`
- Unique expected decoder protocols: `46`
- Files missing from fixture directory: `0`
- Unresolved expected protocol macros: `0`
- Minimum battery target (>=10 samples): `PASS`
- CORE fixture coverage: `17/17`
- FULL fixture coverage: `46/56`
- CORE fixture completeness gate: `PASS`

## Decoder Samples (from Unleashed unit tests)
| Sample                    | Expected Protocol | Expected In CORE | Expected In FULL | Type | File |
| ------------------------- | ----------------- | ---------------- | ---------------- | ---- | ---- |
| came_atomo_raw.sub        | CAME Atomo        | yes              | yes              | raw  | ok   |
| came_raw.sub              | CAME              | yes              | yes              | raw  | ok   |
| came_twee_raw.sub         | CAME TWEE         | yes              | yes              | raw  | ok   |
| faac_slh_raw.sub          | Faac SLH          | yes              | yes              | raw  | ok   |
| gate_tx_raw.sub           | GateTX            | no               | yes              | raw  | ok   |
| hormann_hsm_raw.sub       | Hormann HSM       | no               | yes              | raw  | ok   |
| ido_117_111_raw.sub       | iDo 117/111       | no               | yes              | raw  | ok   |
| doorhan_raw.sub           | KeeLoq            | yes              | yes              | raw  | ok   |
| nero_radio_raw.sub        | Nero Radio        | no               | yes              | raw  | ok   |
| nero_sketch_raw.sub       | Nero Sketch       | no               | yes              | raw  | ok   |
| nice_flo_raw.sub          | Nice FLO          | yes              | yes              | raw  | ok   |
| nice_flor_s_raw.sub       | Nice FloR-S       | yes              | yes              | raw  | ok   |
| Princeton_raw.sub         | Princeton         | yes              | yes              | raw  | ok   |
| Somfy_keytis_raw.sub      | Somfy Keytis      | no               | yes              | raw  | ok   |
| somfy_telis_raw.sub       | Somfy Telis       | no               | yes              | raw  | ok   |
| linear_raw.sub            | Linear            | yes              | yes              | raw  | ok   |
| linear_delta3_raw.sub     | LinearDelta3      | yes              | yes              | raw  | ok   |
| megacode_raw.sub          | MegaCode          | yes              | yes              | raw  | ok   |
| security_pls_1_0_raw.sub  | Security+ 1.0     | yes              | yes              | raw  | ok   |
| security_pls_2_0_raw.sub  | Security+ 2.0     | yes              | yes              | raw  | ok   |
| holtek_raw.sub            | Holtek            | yes              | yes              | raw  | ok   |
| power_smart_raw.sub       | Power Smart       | yes              | yes              | raw  | ok   |
| marantec_raw.sub          | Marantec          | no               | yes              | raw  | ok   |
| bett_raw.sub              | BETT              | no               | yes              | raw  | ok   |
| doitrand_raw.sub          | Doitrand          | yes              | yes              | raw  | ok   |
| phoenix_v2_raw.sub        | Phoenix_V2        | no               | yes              | raw  | ok   |
| honeywell_wdb_raw.sub     | Honeywell         | no               | yes              | raw  | ok   |
| magellan_raw.sub          | Magellan          | no               | yes              | raw  | ok   |
| intertechno_v3_raw.sub    | Intertechno_V3    | no               | yes              | raw  | ok   |
| clemsa_raw.sub            | Clemsa            | no               | yes              | raw  | ok   |
| ansonic_raw.sub           | Ansonic           | no               | yes              | raw  | ok   |
| smc5326_raw.sub           | SMC5326           | no               | yes              | raw  | ok   |
| holtek_ht12x_raw.sub      | Holtek_HT12X      | yes              | yes              | raw  | ok   |
| dooya_raw.sub             | Dooya             | no               | yes              | raw  | ok   |
| alutech_at_4n_raw.sub     | Alutech AT-4N     | no               | yes              | raw  | ok   |
| nice_one_raw.sub          | Nice FloR-S       | yes              | yes              | raw  | ok   |
| kinggates_stylo4k_raw.sub | KingGates Stylo4k | no               | yes              | raw  | ok   |
| mastercode_raw.sub        | Mastercode        | no               | yes              | raw  | ok   |
| dickert_raw.sub           | Dickert_MAHS      | no               | yes              | raw  | ok   |
| legrand_raw.sub           | Legrand           | no               | yes              | raw  | ok   |
| marantec24_raw.sub        | Marantec24        | no               | yes              | raw  | ok   |
| roger_raw.sub             | Roger             | no               | yes              | raw  | ok   |
| feron_raw.sub             | Feron             | no               | yes              | raw  | ok   |
| gangqi_raw.sub            | GangQi            | no               | yes              | raw  | ok   |
| hollarm_raw.sub           | Hollarm           | no               | yes              | raw  | ok   |
| revers_rb2_raw.sub        | Revers_RB2        | no               | yes              | raw  | ok   |
| hay21_raw.sub             | Hay21             | no               | yes              | raw  | ok   |

## Encoder Samples (from Unleashed unit tests)
| Sample               | Protocol Field | Type | File |
| -------------------- | -------------- | ---- | ---- |
| princeton.sub        | Princeton      | key  | ok   |
| came.sub             | CAME           | key  | ok   |
| came_twee.sub        | CAME TWEE      | key  | ok   |
| gate_tx.sub          | GateTX         | key  | ok   |
| nice_flo.sub         | Nice FLO       | key  | ok   |
| doorhan.sub          | KeeLoq         | key  | ok   |
| linear.sub           | Linear         | key  | ok   |
| linear_delta3.sub    | LinearDelta3   | key  | ok   |
| megacode.sub         | MegaCode       | key  | ok   |
| holtek.sub           | Holtek         | key  | ok   |
| security_pls_1_0.sub | Security+ 1.0  | key  | ok   |
| security_pls_2_0.sub | Security+ 2.0  | key  | ok   |
| power_smart.sub      | Power Smart    | key  | ok   |
| marantec.sub         | Marantec       | key  | ok   |
| bett.sub             | BETT           | key  | ok   |
| doitrand.sub         | Doitrand       | key  | ok   |
| phoenix_v2.sub       | Phoenix_V2     | key  | ok   |
| honeywell_wdb.sub    | Honeywell      | key  | ok   |
| magellan.sub         | Magellan       | key  | ok   |
| intertechno_v3.sub   | Intertechno_V3 | key  | ok   |
| clemsa.sub           | Clemsa         | key  | ok   |
| ansonic.sub          | Ansonic        | key  | ok   |
| smc5326.sub          | SMC5326        | key  | ok   |
| holtek_ht12x.sub     | Holtek_HT12X   | key  | ok   |
| dooya.sub            | Dooya          | key  | ok   |
| mastercode.sub       | Mastercode     | key  | ok   |
| dickert_mahs.sub     | Dickert_MAHS   | key  | ok   |
| legrand.sub          | Legrand        | key  | ok   |
| feron.sub            | Feron          | key  | ok   |
| gangqi.sub           | GangQi         | key  | ok   |
| hollarm.sub          | Hollarm        | key  | ok   |
| revers_rb2.sub       | Revers_RB2     | key  | ok   |
| roger.sub            | Roger          | key  | ok   |
| marantec24.sub       | Marantec24     | key  | ok   |

## Expected Decoder Protocols

- `Alutech AT-4N`
- `Ansonic`
- `BETT`
- `CAME`
- `CAME Atomo`
- `CAME TWEE`
- `Clemsa`
- `Dickert_MAHS`
- `Doitrand`
- `Dooya`
- `Faac SLH`
- `Feron`
- `GangQi`
- `GateTX`
- `Hay21`
- `Hollarm`
- `Holtek`
- `Holtek_HT12X`
- `Honeywell`
- `Hormann HSM`
- `Intertechno_V3`
- `KeeLoq`
- `KingGates Stylo4k`
- `Legrand`
- `Linear`
- `LinearDelta3`
- `Magellan`
- `Marantec`
- `Marantec24`
- `Mastercode`
- `MegaCode`
- `Nero Radio`
- `Nero Sketch`
- `Nice FLO`
- `Nice FloR-S`
- `Phoenix_V2`
- `Power Smart`
- `Princeton`
- `Revers_RB2`
- `Roger`
- `SMC5326`
- `Security+ 1.0`
- `Security+ 2.0`
- `Somfy Keytis`
- `Somfy Telis`
- `iDo 117/111`

## Coverage vs Runtime Registry

- CORE covered by decoder fixtures: `17/17`
- FULL covered by decoder fixtures: `46/56`

### FULL protocols without decoder fixture

- `Allstar Firefly`
- `Beninca ARC`
- `Cham_Code`
- `Ditec GOL4`
- `Elplast`
- `Honeywell Sec`
- `Jarolift`
- `KeyFinder`
- `Nord ICE`
- `Treadmill37`
