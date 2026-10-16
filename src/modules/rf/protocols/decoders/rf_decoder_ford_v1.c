// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define FORD_PROTOCOL_V1_NAME "Ford V1"

typedef struct RfStreamProtocolDecoderFordV1 RfStreamProtocolDecoderFordV1;

extern const RfStreamCodec ford_protocol_v1;

void* rf_stream_protocol_decoder_ford_v1_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_ford_v1_free(void* context);
void rf_stream_protocol_decoder_ford_v1_reset(void* context);
void rf_stream_protocol_decoder_ford_v1_feed(void* context, bool level, uint32_t duration);
uint8_t rf_stream_protocol_decoder_ford_v1_get_hash_data(void* context);
RfStreamProtocolStatus rf_stream_protocol_decoder_ford_v1_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_ford_v1_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_ford_v1_get_string(void* context, RfText* output);

#if RF_STREAM_WITH_ENCODER
void* rf_stream_protocol_encoder_ford_v1_alloc(RfStreamEnvironment* environment);
RfStreamProtocolStatus
    rf_stream_protocol_encoder_ford_v1_deserialize(void* context, RfRecord* flipper_format);
extern const RfStreamProtocolEncoder rf_stream_protocol_ford_v1_encoder;
#endif




#include <string.h>

#define TAG "FordProtocolV1"

static const RfStreamBlockConst rf_stream_protocol_ford_v1_const = {
    .te_short = 65,
    .te_long = 130,
    .te_delta = 39,
    .min_count_bit_for_found = 136,
};

#define FORD_V1_DELTA_LONG        40U
#define FORD_V1_DELTA_DATASYNC    39U
#define FORD_V1_SILENCE_LONG_MULT 3U

#define FORD_V1_PREAMBLE_MIN 50
#define FORD_V1_DATA_BITS    136
#define FORD_V1_DATA_BYTES   17

typedef struct RfStreamProtocolDecoderFordV1 {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    uint32_t crc_calc;

    uint64_t data2;

    uint8_t raw_bytes[FORD_V1_DATA_BYTES + 1];
    uint8_t byte_count;

    RfManchesterState manchester_state;
    uint16_t preamble_count;

    uint8_t sync_event_idx;
    uint8_t sync_event_count;
    uint8_t sync_events[8];

    uint8_t encryption_supported;
} RfStreamProtocolDecoderFordV1;

typedef enum {
    FordV1DecoderStepReset = 0,
    FordV1DecoderStepPreamble = 1,
    FordV1DecoderStepSync = 2,
    FordV1DecoderStepData = 3,
} FordV1DecoderStep;

static const char* ford_v1_get_button_name(uint8_t btn);
static void ford_v1_decode_with_flag(uint8_t* raw, size_t len, uint8_t flag_byte);
static void ford_v1_decode(uint8_t* raw, size_t len);
static void ford_v1_encode_inverse_block(uint8_t block[9]);
static void ford_v1_encode_air_9bytes(const uint8_t* plain9, uint8_t* air9_out);
static bool ford_v1_plain_from_air(const uint8_t air9[9], uint8_t plain9_out[9]);
static void ford_v1_fields_from_plain(
    const uint8_t plain9[9],
    uint32_t* serial_out,
    uint8_t* btn_out,
    uint32_t* cnt_out);
static bool ford_v1_process_data(RfStreamProtocolDecoderFordV1* instance);
static bool ford_v1_try_last_byte_variants(RfStreamProtocolDecoderFordV1* instance);
static bool ford_v1_extract_plain_from_raw(
    const uint8_t* raw17_in,
    uint8_t* plain9_out,
    uint8_t* raw17_canonical_out_opt);
#if RF_STREAM_WITH_ENCODER
static void
    ford_v1_plain_apply_fields(uint8_t* plain9, uint32_t serial, uint8_t btn, uint32_t cnt);
static void ford_v1_encoder_rebuild_raw_from_plain(uint8_t* raw17, const uint8_t* plain9);
#endif

const RfStreamProtocolDecoder rf_stream_protocol_ford_v1_decoder = {
    .alloc = rf_stream_protocol_decoder_ford_v1_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = rf_stream_protocol_decoder_ford_v1_feed,
    .reset = rf_stream_protocol_decoder_ford_v1_reset,
    .get_hash_data = rf_stream_protocol_decoder_ford_v1_get_hash_data,
    .serialize = rf_stream_protocol_decoder_ford_v1_serialize,
    .deserialize = rf_stream_protocol_decoder_ford_v1_deserialize,
    .get_string = rf_stream_protocol_decoder_ford_v1_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_ford_v1_encoder = {
    .alloc = rf_stream_protocol_encoder_ford_v1_alloc,
    .free = rf_stream_encoder_free,
    .deserialize = rf_stream_protocol_encoder_ford_v1_deserialize,
    .stop = rf_stream_encoder_stop,
    .yield = rf_stream_encoder_yield,
};
#else
const RfStreamProtocolEncoder rf_stream_protocol_ford_v1_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec ford_protocol_v1 = {
    .name = FORD_PROTOCOL_V1_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_ford_v1_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_ford_v1_encoder,
#else
    .encoder = NULL,
#endif
};

#define ford_v1_crc16(data, len) rf_stream_protocol_blocks_crc16((data), (len), 0x1021, 0x0000)

static const char* ford_v1_get_button_name(uint8_t btn) {
    switch(btn) {
    case 0:
        return "Sync";
    case 1:
        return "Lock";
    case 2:
        return "Unlock";
    case 4:
        return "Trunk";
    case 8:
        return "Panic";
    default:
        return "??";
    }
}

static void ford_v1_decode_with_flag(uint8_t* raw, size_t len, uint8_t flag_byte) {
    if(len < 9) return;

    if(flag_byte) {
        uint8_t xor_byte = raw[7];
        for(int i = 1; i < 7; i++) {
            raw[i] ^= xor_byte;
        }
    } else {
        uint8_t xor_byte = raw[6];
        for(int i = 1; i < 6; i++) {
            raw[i] ^= xor_byte;
        }
        raw[7] ^= xor_byte;
    }

    uint8_t b6 = raw[6];
    uint8_t b7 = raw[7];
    raw[6] = (b6 & 0xAA) | (b7 & 0x55);
    raw[7] = (b7 & 0xAA) | (b6 & 0x55);
}

static void ford_v1_decode(uint8_t* raw, size_t len) {
    if(len < 9) return;

    uint8_t endbyte = raw[8];
    uint8_t parity_any = (endbyte != 0) ? 1 : 0;
    uint8_t parity = 0;
    uint8_t tmp = endbyte;
    while(tmp) {
        parity ^= (tmp & 1);
        tmp >>= 1;
    }

    uint8_t flag_byte = parity_any ? parity : 0;
    ford_v1_decode_with_flag(raw, len, flag_byte);
}

static void ford_v1_encode_inverse_block(uint8_t block[9]) {
    uint8_t sum = 0;
    for(size_t i = 1; i <= 7; i++) {
        sum = (uint8_t)(sum + block[i]);
    }

    const uint8_t p6 = block[6];
    const uint8_t p7 = block[7];
    const uint8_t post6 = (uint8_t)((p6 & 0xAAU) | (p7 & 0x55U));
    const uint8_t post7 = (uint8_t)((p7 & 0xAAU) | (p6 & 0x55U));
    const uint8_t xorv = (uint8_t)(post6 ^ post7);

    uint8_t xor_byte;
    if((__builtin_popcount((unsigned int)sum) & 1) != 0) {
        block[6] = xorv;
        block[7] = post7;
        xor_byte = post7;
    } else {
        block[6] = post6;
        block[7] = xorv;
        xor_byte = post6;
    }

    for(size_t i = 1; i <= 5; i++) {
        block[i] ^= xor_byte;
    }
}

static void ford_v1_encode_air_9bytes(const uint8_t* plain9, uint8_t* air9_out) {
    uint8_t block[9];
    memcpy(block, plain9, 9);
    ford_v1_encode_inverse_block(block);
    memcpy(air9_out, block, 9);
}

static bool ford_v1_plain_from_air(const uint8_t air9[9], uint8_t plain9_out[9]) {
    for(uint8_t flag = 0; flag < 2; flag++) {
        uint8_t cand[9];
        memcpy(cand, air9, 9);
        ford_v1_decode_with_flag(cand, 9, flag);
        uint8_t reair[9];
        ford_v1_encode_air_9bytes(cand, reair);
        if(memcmp(reair, air9, 9) == 0) {
            memcpy(plain9_out, cand, 9);
            return true;
        }
    }
    return false;
}

static void ford_v1_fields_from_plain(
    const uint8_t plain9[9],
    uint32_t* serial_out,
    uint8_t* btn_out,
    uint32_t* cnt_out) {
    *serial_out = ((uint32_t)plain9[1] << 24) | ((uint32_t)plain9[2] << 16) |
                  ((uint32_t)plain9[3] << 8) | plain9[0];
    *btn_out = (plain9[5] >> 4) & 0x0F;
    *cnt_out = ((plain9[5] & 0x0F) << 16) | (plain9[6] << 8) | plain9[7];
}

static bool ford_v1_process_data(RfStreamProtocolDecoderFordV1* instance) {
    uint8_t* raw = instance->raw_bytes;
    uint8_t orig[FORD_V1_DATA_BYTES];
    memcpy(orig, raw, FORD_V1_DATA_BYTES);

    RF_STREAM_LOG_D(
        TAG,
        "process_data: raw=%02X %02X %02X %02X %02X %02X %02X %02X %02X",
        raw[0],
        raw[1],
        raw[2],
        raw[3],
        raw[4],
        raw[5],
        raw[6],
        raw[7],
        raw[8]);
    RF_STREAM_LOG_D(
        TAG,
        "process_data: raw[9..16]=%02X %02X %02X %02X %02X %02X %02X %02X",
        raw[9],
        raw[10],
        raw[11],
        raw[12],
        raw[13],
        raw[14],
        raw[15],
        raw[16]);

    uint16_t calc_crc = ford_v1_crc16(&raw[3], 12);
    uint16_t recv_crc = ((uint16_t)raw[15] << 8) | raw[16];

    RF_STREAM_LOG_D(TAG, "CRC check: calc=%04X recv=%04X", calc_crc, recv_crc);

    if(recv_crc != calc_crc) {
        RF_STREAM_LOG_D(TAG, "CRC mismatch, invert 17 bytes");
        memcpy(raw, orig, FORD_V1_DATA_BYTES);
        for(size_t i = 0; i < FORD_V1_DATA_BYTES; i++) {
            raw[i] = ~raw[i];
        }
        calc_crc = ford_v1_crc16(&raw[3], 12);
        recv_crc = ((uint16_t)raw[15] << 8) | raw[16];
        RF_STREAM_LOG_D(TAG, "CRC after invert(17): calc=%04X recv=%04X", calc_crc, recv_crc);
    }

    if(recv_crc != calc_crc) {
        RF_STREAM_LOG_D(TAG, "CRC FAIL after normal + invert17");
        return false;
    }

    RF_STREAM_LOG_D(TAG, "CRC OK, decoding payload");

    const uint8_t* const air9 = &raw[6];
    uint8_t decoded[9];
    bool strict_ok = false;
    bool rolling_ok = false;

    uint8_t decoded_b0[9];
    uint8_t decoded_b1[9];
    memcpy(decoded_b0, air9, 9);
    ford_v1_decode_with_flag(decoded_b0, 9, 0);
    memcpy(decoded_b1, air9, 9);
    ford_v1_decode_with_flag(decoded_b1, 9, 1);

    if((decoded_b0[3] == raw[5]) && (decoded_b0[4] == raw[6])) {
        memcpy(decoded, decoded_b0, 9);
        strict_ok = true;
        RF_STREAM_LOG_D(TAG, "Plain (cleartext) via strict branch 0");
    } else if((decoded_b1[3] == raw[5]) && (decoded_b1[4] == raw[6])) {
        memcpy(decoded, decoded_b1, 9);
        strict_ok = true;
        RF_STREAM_LOG_D(TAG, "Plain (cleartext) via strict branch 1");
    } else if(ford_v1_plain_from_air(air9, decoded)) {
        rolling_ok = true;
        RF_STREAM_LOG_D(TAG, "Plain (encrypted) via encode round-trip");
    } else {
        memcpy(decoded, air9, 9);
        ford_v1_decode(decoded, 9);
        RF_STREAM_LOG_W(
            TAG,
            "Descramble unresolved (b0[3]=%02X b1[3]=%02X raw[5]=%02X); header-only",
            decoded_b0[3],
            decoded_b1[3],
            raw[5]);
    }

    RF_STREAM_LOG_D(
        TAG,
        "Decoded: %02X %02X %02X %02X %02X %02X %02X %02X %02X",
        decoded[0],
        decoded[1],
        decoded[2],
        decoded[3],
        decoded[4],
        decoded[5],
        decoded[6],
        decoded[7],
        decoded[8]);

    uint16_t recalc_crc = ford_v1_crc16(&raw[3], 12);
    instance->crc_calc = recalc_crc;

    uint64_t key1 = 0;
    for(int i = 0; i < 7; i++) {
        key1 = (key1 << 8) | raw[0 + i];
    }

    uint64_t key2 = 0;
    for(int i = 0; i < 8; i++) {
        key2 = (key2 << 8) | raw[7 + i];
    }

    instance->generic.data = key1;
    instance->data2 = key2;
    instance->generic.data_count_bit = FORD_V1_DATA_BITS;

    if(strict_ok) {
        ford_v1_fields_from_plain(
            decoded, &instance->generic.serial, &instance->generic.btn, &instance->generic.cnt);
        instance->encryption_supported = 1;
    } else {
        instance->generic.serial = ((uint32_t)raw[3] << 24) | ((uint32_t)raw[4] << 16) |
                                   ((uint32_t)raw[5] << 8) | raw[6];
        instance->generic.btn = 0;
        instance->generic.cnt = 0;
        instance->encryption_supported = 0;
        (void)rolling_ok;
    }

    RF_STREAM_LOG_I(
        TAG,
        "DECODED OK: Sn=%08lX Btn=%02X Cnt=%05lX CRC=%04lX",
        (unsigned long)instance->generic.serial,
        instance->generic.btn,
        (unsigned long)instance->generic.cnt,
        (unsigned long)recalc_crc);

    if(instance->base.callback) {
        instance->base.callback(&instance->base, instance->base.context);
    }

    return true;
}

static bool ford_v1_try_last_byte_variants(RfStreamProtocolDecoderFordV1* instance) {
    if(instance->byte_count != 16U) {
        rf_stream_protocol_decoder_ford_v1_reset(instance);
        return false;
    }

    if((uint8_t)(instance->decoder.decode_count_bit + 0x7AU) > 1U) {
        rf_stream_protocol_decoder_ford_v1_reset(instance);
        return false;
    }

    const uint8_t shift = (uint8_t)(FORD_V1_DATA_BITS - instance->decoder.decode_count_bit);
    const uint8_t variants = (uint8_t)(1U << shift);
    uint8_t saved[16];
    memcpy(saved, instance->raw_bytes, sizeof(saved));
    const uint32_t partial = instance->decoder.decode_data;

    for(uint8_t variant = 0; variant < variants; variant++) {
        memcpy(instance->raw_bytes, saved, sizeof(saved));
        instance->raw_bytes[16] = (uint8_t)(((uint8_t)partial << shift) | variant);

        if(ford_v1_process_data(instance)) {
            rf_stream_protocol_decoder_ford_v1_reset(instance);
            return true;
        }
    }

    rf_stream_protocol_decoder_ford_v1_reset(instance);
    return false;
}

static bool ford_v1_extract_plain_from_raw(
    const uint8_t* raw17_in,
    uint8_t* plain9_out,
    uint8_t* raw17_canonical_out_opt) {
    uint8_t raw[FORD_V1_DATA_BYTES];
    memcpy(raw, raw17_in, FORD_V1_DATA_BYTES);

    uint16_t calc_crc = ford_v1_crc16(&raw[3], 12);
    uint16_t recv_crc = ((uint16_t)raw[15] << 8) | raw[16];

    if(recv_crc != calc_crc) {
        memcpy(raw, raw17_in, FORD_V1_DATA_BYTES);
        for(size_t i = 0; i < FORD_V1_DATA_BYTES; i++) {
            raw[i] = (uint8_t)~raw[i];
        }
        calc_crc = ford_v1_crc16(&raw[3], 12);
        recv_crc = ((uint16_t)raw[15] << 8) | raw[16];
    }

    if(recv_crc != calc_crc) {
        return false;
    }

    for(uint8_t flag = 0; flag < 2; flag++) {
        uint8_t cand[9];
        memcpy(cand, &raw[6], 9);
        ford_v1_decode_with_flag(cand, 9, flag);
        if((cand[3] == raw[5]) && (cand[4] == raw[6])) {
            memcpy(plain9_out, cand, 9);
            if(raw17_canonical_out_opt) {
                memcpy(raw17_canonical_out_opt, raw, FORD_V1_DATA_BYTES);
            }
            return true;
        }
    }

    if(ford_v1_plain_from_air(&raw[6], plain9_out)) {
        if(raw17_canonical_out_opt) {
            memcpy(raw17_canonical_out_opt, raw, FORD_V1_DATA_BYTES);
        }
        return true;
    }

    return false;
}

void* rf_stream_protocol_decoder_ford_v1_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolDecoderFordV1* instance = calloc(1, sizeof(RfStreamProtocolDecoderFordV1));
    instance->base.protocol = &ford_protocol_v1;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void rf_stream_protocol_decoder_ford_v1_reset(void* context) {
    RfStreamProtocolDecoderFordV1* instance = context;

    instance->decoder.parser_step = FordV1DecoderStepReset;
    instance->decoder.decode_data = 0;
    instance->decoder.decode_count_bit = 0;
    instance->byte_count = 0;
    memset(instance->raw_bytes, 0, sizeof(instance->raw_bytes));
    instance->preamble_count = 0;
    instance->sync_event_idx = 0;
    instance->sync_event_count = 0;
    memset(instance->sync_events, 0, sizeof(instance->sync_events));

    rf_manchester_advance(
        instance->manchester_state, RfManchesterEventReset, &instance->manchester_state, NULL);
}

void rf_stream_protocol_decoder_ford_v1_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderFordV1* instance = context;

    uint32_t te_short = rf_stream_protocol_ford_v1_const.te_short;
    uint32_t te_long = rf_stream_protocol_ford_v1_const.te_long;

    switch(instance->decoder.parser_step) {
    case FordV1DecoderStepReset:
        if(!level && (DURATION_DIFF(duration, te_long) < FORD_V1_DELTA_LONG)) {
            instance->decoder.parser_step = FordV1DecoderStepPreamble;
            instance->preamble_count = 1;
            instance->decoder.te_last = duration;
        }
        break;

    case FordV1DecoderStepPreamble:
        if(DURATION_DIFF(duration, te_long) < FORD_V1_DELTA_LONG) {
            instance->preamble_count++;
            instance->decoder.te_last = duration;
        } else if(DURATION_DIFF(duration, te_short) < FORD_V1_DELTA_DATASYNC) {
            if(instance->preamble_count >= FORD_V1_PREAMBLE_MIN) {
                instance->sync_event_idx = 0;
                instance->sync_event_count = 1;
                instance->sync_events[0] =
                    (uint8_t)(level ? RfManchesterEventShortHigh : RfManchesterEventShortLow);
                instance->decoder.parser_step = FordV1DecoderStepSync;
                RF_STREAM_LOG_D(TAG, "Preamble OK: %u pulses, entering Sync", instance->preamble_count);
            } else {
                instance->decoder.parser_step = FordV1DecoderStepReset;
            }
        } else {
            if(instance->preamble_count < FORD_V1_PREAMBLE_MIN) {
                instance->decoder.parser_step = FordV1DecoderStepReset;
            }
        }
        break;

    case FordV1DecoderStepSync: {
        uint8_t ev;
        bool is_short = false;

        if(DURATION_DIFF(duration, te_short) < FORD_V1_DELTA_DATASYNC) {
            ev = (uint8_t)(level ? RfManchesterEventShortHigh : RfManchesterEventShortLow);
            is_short = true;
        } else if(DURATION_DIFF(duration, te_long) < FORD_V1_DELTA_DATASYNC) {
            ev = (uint8_t)(level ? RfManchesterEventLongHigh : RfManchesterEventLongLow);
        } else {
            instance->decoder.parser_step = FordV1DecoderStepPreamble;
            break;
        }

        instance->sync_event_idx++;
        if(is_short) instance->sync_event_count++;

        if(instance->sync_event_idx < 8) {
            instance->sync_events[instance->sync_event_idx] = ev;
        }

        if(instance->sync_event_count > 2) {
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
            instance->byte_count = 0;
            memset(instance->raw_bytes, 0, sizeof(instance->raw_bytes));
            rf_manchester_advance(
                instance->manchester_state,
                RfManchesterEventReset,
                &instance->manchester_state,
                NULL);

            if(instance->sync_events[0] == (uint8_t)RfManchesterEventShortLow) {
                instance->manchester_state = RfManchesterStateMid0;
            }

            instance->decoder.parser_step = FordV1DecoderStepData;

            RF_STREAM_LOG_D(
                TAG, "Sync->Data: replaying %u buffered events", instance->sync_event_idx + 1);

            for(uint8_t i = 0; i <= instance->sync_event_idx && i < 8; i++) {
                bool data_bit;
                if(rf_manchester_advance(
                       instance->manchester_state,
                       (RfManchesterEvent)instance->sync_events[i],
                       &instance->manchester_state,
                       &data_bit)) {
                    instance->decoder.decode_data = (instance->decoder.decode_data << 1) |
                                                    (data_bit ? 1 : 0);
                    instance->decoder.decode_count_bit++;

                    if((instance->decoder.decode_count_bit & 7) == 0) {
                        uint8_t byte_val = (uint8_t)(instance->decoder.decode_data & 0xFF);
                        if(instance->byte_count < FORD_V1_DATA_BYTES) {
                            instance->raw_bytes[instance->byte_count] = byte_val;
                            instance->byte_count++;
                        }
                        instance->decoder.decode_data = 0;
                    }
                }
            }
            break;
        }

        if(instance->sync_event_idx >= 7) {
            instance->decoder.parser_step = FordV1DecoderStepPreamble;
        }
        break;
    }

    case FordV1DecoderStepData: {
        RfManchesterEvent event;

        if(DURATION_DIFF(duration, te_short) < FORD_V1_DELTA_DATASYNC) {
            event = level ? RfManchesterEventShortHigh : RfManchesterEventShortLow;
        } else if(DURATION_DIFF(duration, te_long) < FORD_V1_DELTA_DATASYNC) {
            event = level ? RfManchesterEventLongHigh : RfManchesterEventLongLow;
        } else {
            if(duration >= te_long * FORD_V1_SILENCE_LONG_MULT) {
                RF_STREAM_LOG_D(
                    TAG,
                    "Data idle gap: dur=%luus (>= %luus), bits=%u bytes=%u/%d — try partial last byte",
                    (unsigned long)duration,
                    (unsigned long)(te_long * FORD_V1_SILENCE_LONG_MULT),
                    instance->decoder.decode_count_bit,
                    instance->byte_count,
                    FORD_V1_DATA_BYTES);
            } else {
                RF_STREAM_LOG_D(
                    TAG,
                    "Data odd pulse: dur=%luus bits=%u bytes=%u/%d — try partial last byte",
                    (unsigned long)duration,
                    instance->decoder.decode_count_bit,
                    instance->byte_count,
                    FORD_V1_DATA_BYTES);
            }
            (void)ford_v1_try_last_byte_variants(instance);
            break;
        }

        bool data_bit;
        if(rf_manchester_advance(
               instance->manchester_state, event, &instance->manchester_state, &data_bit)) {
            instance->decoder.decode_data = (instance->decoder.decode_data << 1) |
                                            (data_bit ? 1 : 0);
            instance->decoder.decode_count_bit++;

            if((instance->decoder.decode_count_bit & 7) == 0) {
                uint8_t byte_val = (uint8_t)(instance->decoder.decode_data & 0xFF);
                if(instance->byte_count < FORD_V1_DATA_BYTES) {
                    instance->raw_bytes[instance->byte_count] = byte_val;
                    instance->byte_count++;
                    RF_STREAM_LOG_D(
                        TAG,
                        "Data byte[%u]=%02X (bits=%u)",
                        instance->byte_count - 1,
                        byte_val,
                        instance->decoder.decode_count_bit);
                }
                instance->decoder.decode_data = 0;

                if(instance->byte_count > 16) {
                    RF_STREAM_LOG_D(
                        TAG,
                        "Data complete: %u bytes, calling process_data",
                        instance->byte_count);
                    ford_v1_process_data(instance);
                    rf_stream_protocol_decoder_ford_v1_reset(instance);
                }
            }
        }

        instance->decoder.te_last = duration;
        break;
    }
    }
}

uint8_t rf_stream_protocol_decoder_ford_v1_get_hash_data(void* context) {
    RfStreamProtocolDecoderFordV1* instance = context;

    uint32_t crc = instance->crc_calc;
    uint8_t hash = (uint8_t)(crc ^ (crc >> 8));

    for(int i = 0; i < 8; i++) {
        hash ^= instance->raw_bytes[i + 5];
    }

    return hash;
}

static void ford_v1_key3_bytes_from_crc16(uint16_t crc16, uint8_t key3_out[4]) {
    key3_out[0] = 0;
    key3_out[1] = 0;
    key3_out[2] = (uint8_t)((crc16 >> 8) & 0xFFU);
    key3_out[3] = (uint8_t)(crc16 & 0xFFU);
}

static uint16_t ford_v1_crc16_from_key3_bytes(const uint8_t key3[4]) {
    uint32_t crc = ((uint32_t)key3[0] << 24) | ((uint32_t)key3[1] << 16) |
                   ((uint32_t)key3[2] << 8) | (uint32_t)key3[3];
    if((crc & 0xFFFFU) == 0U && (crc >> 16) != 0U) {
        return __builtin_bswap16((uint16_t)(crc >> 16));
    }
    return (uint16_t)(crc & 0xFFFFU);
}

static uint64_t ford_v1_u64_from_be_key8(const uint8_t key8[8]) {
    uint64_t v = 0;
    for(int i = 0; i < 8; i++) {
        v = (v << 8) | (uint64_t)key8[i];
    }
    return v;
}

static uint64_t ford_v1_u64_legacy_halves_bswap(const uint8_t key8[8]) {
    uint64_t stored = ford_v1_u64_from_be_key8(key8);
    return ((uint64_t)__builtin_bswap32((uint32_t)(stored >> 32)) << 32) |
           (uint64_t)__builtin_bswap32((uint32_t)(stored & 0xFFFFFFFFU));
}

static void
    ford_v1_raw14_from_internal_keys(uint64_t key1, uint64_t data2, uint8_t raw14_out[14]) {
    for(int i = 0; i < 7; i++) {
        raw14_out[i] = (uint8_t)((key1 >> (48 - i * 8)) & 0xFFU);
    }
    for(int i = 0; i < 8; i++) {
        raw14_out[7 + i] = (uint8_t)((data2 >> (56 - i * 8)) & 0xFFU);
    }
}

static bool ford_v1_keys_file_to_canonical_raw(
    const uint8_t key1b[8],
    const uint8_t key2b[8],
    uint16_t crc16,
    uint64_t* key1_out,
    uint64_t* key2_out,
    uint8_t raw17[FORD_V1_DATA_BYTES],
    uint8_t plain9[9]) {
    for(unsigned attempt = 0; attempt < 2; attempt++) {
        uint64_t k1 = (attempt == 0) ? ford_v1_u64_from_be_key8(key1b) :
                                       ford_v1_u64_legacy_halves_bswap(key1b);
        uint64_t k2 = (attempt == 0) ? ford_v1_u64_from_be_key8(key2b) :
                                       ford_v1_u64_legacy_halves_bswap(key2b);
        *key1_out = k1;
        *key2_out = k2;
        ford_v1_raw14_from_internal_keys(k1, k2, raw17);
        raw17[15] = (uint8_t)((crc16 >> 8) & 0xFFU);
        raw17[16] = (uint8_t)(crc16 & 0xFFU);
        if(ford_v1_extract_plain_from_raw(raw17, plain9, raw17)) {
            return true;
        }
    }
    *key1_out = ford_v1_u64_from_be_key8(key1b);
    *key2_out = ford_v1_u64_from_be_key8(key2b);
    ford_v1_raw14_from_internal_keys(*key1_out, *key2_out, raw17);
    raw17[15] = (uint8_t)((crc16 >> 8) & 0xFFU);
    raw17[16] = (uint8_t)(crc16 & 0xFFU);
    return false;
}

RfStreamProtocolStatus rf_stream_protocol_decoder_ford_v1_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderFordV1* instance = context;

    RfStreamProtocolStatus ret =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);

    if(ret == RfStreamProtocolStatusOk) {
        uint32_t ser_w = instance->generic.serial;
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_uint32(flipper_format, FF_SERIAL, &ser_w, 1);
        uint32_t btn_w = instance->generic.btn;
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_uint32(flipper_format, FF_BTN, &btn_w, 1);
        uint32_t cnt_w = instance->generic.cnt;
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_uint32(flipper_format, FF_CNT, &cnt_w, 1);

        uint8_t key1_bytes[8];
        for(int i = 0; i < 8; i++) {
            key1_bytes[i] = (uint8_t)(instance->generic.data >> (56 - i * 8));
        }
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_hex(flipper_format, FF_KEY, key1_bytes, 8);

        uint8_t key2_bytes[8];
        for(int i = 0; i < 8; i++) {
            key2_bytes[i] = (uint8_t)(instance->data2 >> (56 - i * 8));
        }
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_hex(flipper_format, "Key_2", key2_bytes, 8);

        uint8_t key3_bytes[4];
        ford_v1_key3_bytes_from_crc16((uint16_t)(instance->crc_calc & 0xFFFFU), key3_bytes);
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_hex(flipper_format, "Key_3", key3_bytes, 4);
    }

    return ret;
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_ford_v1_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderFordV1* instance = context;

    RfStreamProtocolStatus ret = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, rf_stream_protocol_ford_v1_const.min_count_bit_for_found);

    if(ret == RfStreamProtocolStatusOk) {
        rf_record_rewind(flipper_format);
        uint8_t key1_bytes[8] = {0};
        if(!rf_record_read_hex(flipper_format, FF_KEY, key1_bytes, 8)) {
            return RfStreamProtocolStatusErrorParserKey;
        }

        rf_record_rewind(flipper_format);
        uint8_t key2_bytes[8] = {0};
        if(!rf_record_read_hex(flipper_format, "Key_2", key2_bytes, 8)) {
            return RfStreamProtocolStatusErrorParserOthers;
        }

        rf_record_rewind(flipper_format);
        uint8_t key3_bytes[4] = {0};
        if(rf_record_read_hex(flipper_format, "Key_3", key3_bytes, 4)) {
            instance->crc_calc = ford_v1_crc16_from_key3_bytes(key3_bytes);
        }

        uint8_t raw[FORD_V1_DATA_BYTES];
        uint8_t plain9_tmp[9];
        uint64_t k1 = 0;
        uint64_t k2 = 0;
        uint16_t crc16 = (uint16_t)(instance->crc_calc & 0xFFFFU);
        bool extract_ok = ford_v1_keys_file_to_canonical_raw(
            key1_bytes, key2_bytes, crc16, &k1, &k2, raw, plain9_tmp);
        instance->generic.data = k1;
        instance->data2 = k2;

        instance->encryption_supported =
            (extract_ok && (plain9_tmp[3] == raw[5]) && (plain9_tmp[4] == raw[6])) ? 1 : 0;

        memcpy(instance->raw_bytes, raw, FORD_V1_DATA_BYTES);

        uint32_t serial = UINT32_MAX;
        uint32_t btn = UINT32_MAX;
        uint32_t cnt = UINT32_MAX;
        rf_stream_encoder_read_fields(flipper_format, &serial, &btn, &cnt, NULL);
        if(serial == UINT32_MAX || btn == UINT32_MAX || cnt == UINT32_MAX) {
            return RfStreamProtocolStatusErrorParserOthers;
        }
        instance->generic.serial = serial;
        instance->generic.btn = (uint8_t)btn;
        instance->generic.cnt = cnt;
    }

    return ret;
}

void rf_stream_protocol_decoder_ford_v1_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderFordV1* instance = context;

    uint64_t key1 = instance->generic.data;
    uint64_t key2 = instance->data2;
    uint32_t crc = instance->crc_calc;
    uint16_t crc16 = crc & 0xFFFF;

    if(instance->encryption_supported) {
        const char* btn_name = ford_v1_get_button_name(instance->generic.btn);

        uint16_t calc_crc = crc16;

        bool crc_ok;
        uint8_t raw[FORD_V1_DATA_BYTES];
        ford_v1_raw14_from_internal_keys(key1, key2, raw);
        raw[15] = (uint8_t)((crc >> 8) & 0xFF);
        raw[16] = (uint8_t)(crc & 0xFF);

        uint16_t check_crc = ford_v1_crc16(&raw[3], 12);
        crc_ok = (check_crc == calc_crc);

        rf_text_cat_printf(
            output,
            "%s %dbit\r\n"
            "%014llX%06llX\r\n"
            "%010llX%04lX\r\n"
            "Sn:%08lX Bt:%01X [%s]\r\n"
            "Cnt:%05lX CRC:%04lX [%s]\r\n",
            instance->generic.protocol_name,
            instance->generic.data_count_bit,
            (unsigned long long)key1,
            (unsigned long long)(key2 >> 40),
            (unsigned long long)(key2 & 0xFFFFFFFFFFULL),
            (unsigned long)crc16,
            (unsigned long)instance->generic.serial,
            instance->generic.btn,
            btn_name,
            (unsigned long)instance->generic.cnt,
            (unsigned long)crc16,
            crc_ok ? "OK" : "ERR");
    } else {
        uint8_t raw[FORD_V1_DATA_BYTES];
        ford_v1_raw14_from_internal_keys(key1, key2, raw);
        raw[15] = (uint8_t)((crc >> 8) & 0xFF);
        raw[16] = (uint8_t)(crc & 0xFF);

        uint16_t check_crc = ford_v1_crc16(&raw[3], 12);
        bool crc_ok = (check_crc == crc16);

        uint32_t device_id = ((uint32_t)raw[3] << 24) | ((uint32_t)raw[4] << 16) |
                             ((uint32_t)raw[5] << 8) | raw[6];

        rf_text_cat_printf(
            output,
            "%s %dbit\r\n"
            "%014llX%06llX\r\n"
            "%010llX%04lX\r\n"
            "Sn:%08lX\r\n"
            "CRC:%04lX [%s]\r\n"
            "Encryption not supported !\r\n",
            instance->generic.protocol_name,
            instance->generic.data_count_bit,
            (unsigned long long)key1,
            (unsigned long long)(key2 >> 40),
            (unsigned long long)(key2 & 0xFFFFFFFFFFULL),
            (unsigned long)crc16,
            (unsigned long)device_id,
            (unsigned long)crc16,
            crc_ok ? "OK" : "ERR");
    }
}

#if RF_STREAM_WITH_ENCODER

#define FORD_V1_ENC_BURST_COUNT    6U
#define FORD_V1_ENC_PREAMBLE_PAIRS 400U
#define FORD_V1_ENC_BURST_LD_COUNT \
    ((FORD_V1_ENC_PREAMBLE_PAIRS * 2U) + 2U + (FORD_V1_DATA_BYTES * 16U) + 1U)
_Static_assert(
    FORD_V1_ENC_BURST_LD_COUNT <= RF_SHARED_UPLOAD_CAPACITY,
    "FORD_V1_ENC_BURST_LD_COUNT exceeds shared upload slab");
#define FORD_V1_ENC_SYNC_SHORT_US 65U
#define FORD_V1_ENC_SYNC_LONG_US  130U
#define FORD_V1_ENC_GAP_REPEAT_US 50000U
#define FORD_V1_ENC_GAP_LAST_US   260U

#if FORD_V1_ENC_BURST_LD_COUNT != 0x433U
#error Ford V1 encoder burst layout constants out of sync
#endif

static const uint8_t ford_v1_encoder_burst_pkt4_vals[6] = {0x08, 0x00, 0x10, 0x08, 0x00, 0x10};

typedef struct RfStreamProtocolEncoderFordV1 {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;
    uint64_t data2;
    uint16_t crc_calc;
    uint8_t plain9[9];
    uint8_t raw_tx[FORD_V1_DATA_BYTES];
    uint8_t encryption_supported;
    uint8_t plain_valid;
    uint8_t burst_idx;
} RfStreamProtocolEncoderFordV1;

static void
    ford_v1_plain_apply_fields(uint8_t* plain9, uint32_t serial, uint8_t btn, uint32_t cnt) {
    uint8_t chk = (uint8_t)(plain9[8] - plain9[6] - plain9[7] - plain9[5]);
    plain9[0] = (uint8_t)(serial & 0xFFU);
    plain9[1] = (uint8_t)((serial >> 24) & 0xFFU);
    plain9[2] = (uint8_t)((serial >> 16) & 0xFFU);
    plain9[3] = (uint8_t)((serial >> 8) & 0xFFU);
    plain9[5] = (uint8_t)(((btn & 0x0FU) << 4) | ((cnt >> 16) & 0x0FU));
    plain9[6] = (uint8_t)((cnt >> 8) & 0xFFU);
    plain9[7] = (uint8_t)(cnt & 0xFFU);
    plain9[8] = (uint8_t)(chk + plain9[7] + plain9[6] + plain9[5]);
}

static void ford_v1_encoder_rebuild_raw_from_plain(uint8_t* raw17, const uint8_t* plain9) {
    uint8_t air9[9];
    ford_v1_encode_air_9bytes(plain9, air9);
    memcpy(&raw17[6], air9, 9);
    uint16_t c = ford_v1_crc16(&raw17[3], 12);
    raw17[15] = (uint8_t)(c >> 8);
    raw17[16] = (uint8_t)(c & 0xFF);
}

static void ford_v1_encoder_keys_from_raw(RfStreamProtocolEncoderFordV1* instance) {
    uint8_t* raw = instance->raw_tx;
    uint64_t key1 = 0;
    for(int i = 0; i < 7; i++) {
        key1 = (key1 << 8) | raw[i];
    }
    uint64_t key2 = 0;
    for(int i = 0; i < 8; i++) {
        key2 = (key2 << 8) | raw[7 + i];
    }
    instance->generic.data = key1;
    instance->data2 = key2;
    instance->crc_calc = (uint16_t)(((uint16_t)raw[15] << 8) | raw[16]);
}

__attribute__((weak)) bool rf_stream_block_generic_global_counter_override_get(uint32_t* cnt_p) {
    UNUSED(cnt_p);
    return false;
}

static uint32_t ford_v1_encoder_preset_hop_read_stub(void) {
    return 0;
}

static void ford_v1_encoder_apply_counter_cap(uint32_t* cnt_p, uint32_t cap) {
    uint32_t ee = ford_v1_encoder_preset_hop_read_stub();
    if(ee == 0x80000001u) {
        uint32_t u3 = *cnt_p;
        uint32_t r2 = u3 + 1u;
        if(r2 > cap) {
            *cnt_p = 0;
            return;
        }
        if(u3 == 0u) {
            *cnt_p = r2;
            return;
        }
        uint32_t capm1 = cap - 1u;
        if(u3 == capm1) {
            *cnt_p = capm1;
            return;
        }
        *cnt_p = r2;
        return;
    }
    if(rf_stream_block_generic_global_counter_override_get(cnt_p)) {
        return;
    }
    uint32_t sum = ee + *cnt_p;
    if(sum <= cap) {
        *cnt_p = sum;
        return;
    }
    *cnt_p = 0;
}

static void ford_v1_encoder_patch_key1_low_bits(RfStreamProtocolEncoderFordV1* instance) {
    uint64_t k = instance->generic.data;
    uint32_t lo = (uint32_t)(k & 0xFFFFFFFFULL);
    lo = (lo & 0xFF00FFFFU) | 0x80000U;
    instance->generic.data = (k & 0xFFFFFFFF00000000ULL) | (uint64_t)lo;
}

static void ford_v1_encoder_build_burst(RfStreamProtocolEncoderFordV1* instance, uint8_t burst_idx) {
    LevelDuration* const upload = instance->encoder.upload;
    uint8_t pkt[FORD_V1_DATA_BYTES];
    memcpy(pkt, instance->raw_tx, FORD_V1_DATA_BYTES);

    pkt[4] = ford_v1_encoder_burst_pkt4_vals[burst_idx];
    uint16_t crcw = ford_v1_crc16(&pkt[3], 12);
    pkt[15] = (uint8_t)(crcw >> 8);
    pkt[16] = (uint8_t)(crcw & 0xFFU);

    size_t index = 0;
    for(size_t i = 0; i < FORD_V1_ENC_PREAMBLE_PAIRS; i++) {
        upload[index++] = level_duration_make(true, FORD_V1_ENC_SYNC_LONG_US);
        upload[index++] = level_duration_make(false, FORD_V1_ENC_SYNC_LONG_US);
    }
    upload[index++] = level_duration_make(true, FORD_V1_ENC_SYNC_LONG_US);
    upload[index++] = level_duration_make(false, FORD_V1_ENC_SYNC_SHORT_US);

    for(size_t by = 0; by < FORD_V1_DATA_BYTES; by++) {
        uint8_t b = pkt[by];
        for(int bit_i = 7; bit_i >= 0; bit_i--) {
            bool bit = ((b >> bit_i) & 1U) != 0U;
            upload[index++] = level_duration_make(bit, FORD_V1_ENC_SYNC_SHORT_US);
            upload[index++] = level_duration_make(!bit, FORD_V1_ENC_SYNC_SHORT_US);
        }
    }

    upload[index++] = level_duration_make(
        false,
        (burst_idx + 1U == FORD_V1_ENC_BURST_COUNT) ? FORD_V1_ENC_GAP_LAST_US :
                                                      FORD_V1_ENC_GAP_REPEAT_US);
    instance->encoder.size_upload = FORD_V1_ENC_BURST_LD_COUNT;
    instance->encoder.front = 0;
}

void* rf_stream_protocol_encoder_ford_v1_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolEncoderFordV1* instance = calloc(1, sizeof(RfStreamProtocolEncoderFordV1));
    instance->base.protocol = &ford_protocol_v1;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->encoder.repeat = 10;
    instance->encoder.front = 0;
    instance->encoder.is_running = false;
    instance->burst_idx = 0;
    rf_stream_encoder_buffer_ensure(instance, FORD_V1_ENC_BURST_LD_COUNT);
    return instance;
}

RfStreamProtocolStatus
    rf_stream_protocol_encoder_ford_v1_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderFordV1* instance = context;
    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    instance->encoder.is_running = false;
    instance->encoder.front = 0;
    instance->encoder.repeat = 10;
    instance->plain_valid = 0;
    instance->burst_idx = 0;

    do {
        rf_record_rewind(flipper_format);
        if(rf_stream_verify_protocol_name(flipper_format, instance->base.protocol->name) !=
           RfStreamProtocolStatusOk) {
            break;
        }

        rf_record_rewind(flipper_format);
        uint8_t key1_bytes[8] = {0};
        if(!rf_record_read_hex(flipper_format, FF_KEY, key1_bytes, 8)) {
            break;
        }
        rf_record_rewind(flipper_format);
        uint8_t key2_bytes[8] = {0};
        if(!rf_record_read_hex(flipper_format, "Key_2", key2_bytes, 8)) {
            break;
        }
        rf_record_rewind(flipper_format);
        uint8_t key3_bytes[4] = {0};
        if(!rf_record_read_hex(flipper_format, "Key_3", key3_bytes, 4)) {
            break;
        }

        uint16_t crc16 = ford_v1_crc16_from_key3_bytes(key3_bytes);
        instance->crc_calc = crc16;

        bool keys_ok = ford_v1_keys_file_to_canonical_raw(
            key1_bytes,
            key2_bytes,
            crc16,
            &instance->generic.data,
            &instance->data2,
            instance->raw_tx,
            instance->plain9);

        instance->plain_valid = keys_ok ? 1U : 0U;
        instance->encryption_supported = instance->plain_valid;
        if(keys_ok) {
            ford_v1_encoder_keys_from_raw(instance);
            instance->crc_calc =
                (uint16_t)(((uint16_t)instance->raw_tx[15] << 8) | instance->raw_tx[16]);
        }

        if(!instance->encryption_supported || !instance->plain_valid) {
            break;
        }

        rf_record_rewind(flipper_format);
        RfStreamProtocolStatus g = rf_stream_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            rf_stream_protocol_ford_v1_const.min_count_bit_for_found);
        if(g != RfStreamProtocolStatusOk) {
            ret = g;
            break;
        }

        ford_v1_encoder_keys_from_raw(instance);

        {
            uint32_t serial = UINT32_MAX;
            uint32_t btn = UINT32_MAX;
            uint32_t cnt = UINT32_MAX;
            rf_stream_encoder_read_fields(flipper_format, &serial, &btn, &cnt, NULL);
            if(serial == UINT32_MAX || btn == UINT32_MAX || cnt == UINT32_MAX) break;
            instance->generic.serial = serial;
            instance->generic.btn = (uint8_t)(btn & 0x0FU);
            instance->generic.cnt = cnt & 0xFFFFFU;
        }

        {
            uint8_t btn_rf = (uint8_t)(instance->generic.btn & 0x0FU);

            ford_v1_encoder_apply_counter_cap(&instance->generic.cnt, 0xFFFFFU);

            uint8_t work[9];
            memcpy(work, instance->plain9, 9);
            ford_v1_plain_apply_fields(
                work, instance->generic.serial, btn_rf, instance->generic.cnt & 0xFFFFFU);
            memcpy(instance->plain9, work, 9);
            ford_v1_encoder_rebuild_raw_from_plain(instance->raw_tx, work);
            ford_v1_encoder_keys_from_raw(instance);
        }

        ford_v1_encoder_patch_key1_low_bits(instance);

        instance->encoder.repeat = rf_stream_encoder_read_repeat(flipper_format, 10);

        ford_v1_encoder_build_burst(instance, instance->burst_idx);

        {
            rf_record_rewind(flipper_format);
            rf_record_insert_or_update_uint32(
                flipper_format, FF_SERIAL, &instance->generic.serial, 1);
            uint32_t btn_store = instance->generic.btn;
            rf_record_rewind(flipper_format);
            rf_record_insert_or_update_uint32(flipper_format, FF_BTN, &btn_store, 1);
            uint32_t cnt_store = instance->generic.cnt;
            rf_record_rewind(flipper_format);
            rf_record_insert_or_update_uint32(flipper_format, FF_CNT, &cnt_store, 1);
        }

        instance->encoder.is_running = true;
        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
}

#endif // RF_STREAM_WITH_ENCODER

#endif
