// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define FORD_PROTOCOL_V0_NAME "Ford V0"

extern const RfStreamCodec ford_protocol_v0;

// Decoder functions
void* rf_stream_protocol_decoder_ford_v0_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_ford_v0_free(void* context);
void rf_stream_protocol_decoder_ford_v0_reset(void* context);
void rf_stream_protocol_decoder_ford_v0_feed(void* context, bool level, uint32_t duration);
RfStreamProtocolStatus rf_stream_protocol_decoder_ford_v0_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_ford_v0_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_ford_v0_get_string(void* context, RfText* output);

// Encoder functions
void* rf_stream_protocol_encoder_ford_v0_alloc(RfStreamEnvironment* environment);
RfStreamProtocolStatus
    rf_stream_protocol_encoder_ford_v0_deserialize(void* context, RfRecord* flipper_format);




#define TAG "FordProtocolV0"

// =============================================================================
// PROTOCOL CONSTANTS
// =============================================================================

// Uncomment to enable bit-level debug logging (WARNING: 80 log calls per signal)
// #define FORD_V0_DEBUG_BITS

static const RfStreamBlockConst rf_stream_protocol_ford_v0_const = {
    .te_short = 250,
    .te_long = 500,
    .te_delta = 100,
    .min_count_bit_for_found = 64,
};

#define FORD_V0_PREAMBLE_PAIRS  4
#define FORD_V0_GAP_US          3500
#define FORD_V0_TOTAL_BURSTS    6
#define FORD_V0_UPLOAD_CAPACITY (FORD_V0_TOTAL_BURSTS * 172U)
_Static_assert(
    FORD_V0_UPLOAD_CAPACITY <= RF_SHARED_UPLOAD_CAPACITY,
    "FORD_V0_UPLOAD_CAPACITY exceeds shared upload slab");

// =============================================================================
// CRC MATRIX
// Ford V0 uses matrix multiplication in GF(2) for CRC calculation
// =============================================================================

static const uint8_t ford_v0_crc_matrix[64] = {
    0xDA, 0xB5, 0x55, 0x6A, 0xAA, 0xAA, 0xAA, 0xD5, 0xB6, 0x6C, 0xCC, 0xD9, 0x99, 0x99, 0x99, 0xB3,
    0x71, 0xE3, 0xC3, 0xC7, 0x87, 0x87, 0x87, 0x8F, 0x0F, 0xE0, 0x3F, 0xC0, 0x7F, 0x80, 0x7F, 0x80,
    0x00, 0x1F, 0xFF, 0xC0, 0x00, 0x7F, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x23, 0x12, 0x94, 0x84, 0x35, 0xF4, 0x55, 0x84,
};

// =============================================================================
// STRUCT DEFINITIONS
// =============================================================================

typedef struct RfStreamProtocolDecoderFordV0 {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    RfManchesterState manchester_state;

    uint64_t data_low;
    uint64_t data_high;
    uint8_t bit_count;

    uint16_t header_count;

    uint64_t key1;
    uint16_t key2;
    uint32_t serial;
    uint8_t button;
    uint32_t count;
} RfStreamProtocolDecoderFordV0;
#if RF_STREAM_WITH_ENCODER
typedef struct RfStreamProtocolEncoderFordV0 {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;

    uint64_t key1;
    uint16_t key2;
    uint32_t serial;
    uint8_t button;
    uint32_t count;
    uint8_t checksum;
} RfStreamProtocolEncoderFordV0;
#endif
typedef enum {
    FordV0DecoderStepReset = 0,
    FordV0DecoderStepPreamble,
    FordV0DecoderStepPreambleCheck,
    FordV0DecoderStepGap,
    FordV0DecoderStepData,
} FordV0DecoderStep;

// =============================================================================
// FUNCTION PROTOTYPES
// =============================================================================

static void ford_v0_add_bit(RfStreamProtocolDecoderFordV0* instance, bool bit);
static void decode_ford_v0(
    uint64_t key1,
    uint16_t key2,
    uint32_t* serial,
    uint8_t* button,
    uint32_t* count);
#if RF_STREAM_WITH_ENCODER
static void encode_ford_v0(
    uint8_t header_byte,
    uint32_t serial,
    uint8_t button,
    uint32_t count,
    uint8_t checksum,
    uint64_t* key1);
#endif
static bool ford_v0_process_data(RfStreamProtocolDecoderFordV0* instance);

// =============================================================================
// PROTOCOL INTERFACE DEFINITIONS
// =============================================================================

const RfStreamProtocolDecoder rf_stream_protocol_ford_v0_decoder = {
    .alloc = rf_stream_protocol_decoder_ford_v0_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = rf_stream_protocol_decoder_ford_v0_feed,
    .reset = rf_stream_protocol_decoder_ford_v0_reset,
    .get_hash_data = rf_stream_decoder_hash_blocks,
    .serialize = rf_stream_protocol_decoder_ford_v0_serialize,
    .deserialize = rf_stream_protocol_decoder_ford_v0_deserialize,
    .get_string = rf_stream_protocol_decoder_ford_v0_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_ford_v0_encoder = {
    .alloc = rf_stream_protocol_encoder_ford_v0_alloc,
    .free = rf_stream_encoder_free,
    .deserialize = rf_stream_protocol_encoder_ford_v0_deserialize,
    .stop = rf_stream_encoder_stop,
    .yield = rf_stream_encoder_yield,
};
#else
const RfStreamProtocolEncoder rf_stream_protocol_ford_v0_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec ford_protocol_v0 = {
    .name = FORD_PROTOCOL_V0_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_ford_v0_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_ford_v0_encoder,
#else
    .encoder = NULL,
#endif
};

// =============================================================================
// CHECKSUM CALCULATION
// =============================================================================
#if RF_STREAM_WITH_ENCODER
static uint8_t ford_v0_calculate_checksum(uint32_t serial, uint32_t count, uint8_t button) {
    return (uint8_t)((((count >> 24) & 0xFF) + ((count >> 16) & 0xFF) + ((count >> 8) & 0xFF) +
                      (count & 0xFF) + ((serial >> 24) & 0xFF) + ((serial >> 16) & 0xFF) +
                      ((serial >> 8) & 0xFF) + (serial & 0xFF) + (button << 3)) &
                     0xFF);
}
#endif
// =============================================================================
// CRC FUNCTIONS
// =============================================================================

static uint8_t ford_v0_calculate_crc(uint8_t* buf) {
    uint8_t crc = 0;

    for(int row = 0; row < 8; row++) {
        uint8_t xor_sum = 0;
        for(int col = 0; col < 8; col++) {
            xor_sum ^= (ford_v0_crc_matrix[row * 8 + col] & buf[col + 1]);
        }
        uint8_t parity = rf_stream_protocol_blocks_parity8(xor_sum);
        if(parity) {
            crc |= (1 << row);
        }
    }

    return crc;
}
#if RF_STREAM_WITH_ENCODER
static uint8_t ford_v0_calculate_crc_for_tx(uint64_t key1, uint8_t checksum) {
    uint8_t buf[16] = {0};

    for(int i = 0; i < 8; ++i) {
        buf[i] = (uint8_t)(key1 >> (56 - i * 8));
    }

    buf[8] = checksum;

    uint8_t crc = ford_v0_calculate_crc(buf);
    return crc ^ 0x80;
}
#endif
static bool ford_v0_verify_crc(uint64_t key1, uint16_t key2) {
    uint8_t buf[16] = {0};

    for(int i = 0; i < 8; ++i) {
        buf[i] = (uint8_t)(key1 >> (56 - i * 8));
    }

    buf[8] = (uint8_t)(key2 >> 8);

    uint8_t calculated_crc = ford_v0_calculate_crc(buf);
    uint8_t received_crc = (uint8_t)(key2 & 0xFF) ^ 0x80;

    return ((calculated_crc & 0x7F) == (received_crc & 0x7F));
}

// =============================================================================
// DECODE FUNCTION
// =============================================================================

static void decode_ford_v0(
    uint64_t key1,
    uint16_t key2,
    uint32_t* serial,
    uint8_t* button,
    uint32_t* count) {
    uint8_t buf[13] = {0};

    for(int i = 0; i < 8; ++i) {
        buf[i] = (uint8_t)(key1 >> (56 - i * 8));
    }

    buf[8] = (uint8_t)(key2 >> 8);
    buf[9] = (uint8_t)(key2 & 0xFF);

    uint8_t tmp = buf[8];
    uint8_t parity = 0;
    uint8_t parity_any = (tmp != 0);
    while(tmp) {
        parity ^= (tmp & 1);
        tmp >>= 1;
    }
    buf[11] = parity_any ? parity : 0;

    uint8_t xor_byte;
    uint8_t limit;
    if(buf[11]) {
        xor_byte = buf[7];
        limit = 7;
    } else {
        xor_byte = buf[6];
        limit = 6;
    }

    for(int idx = 1; idx < limit; ++idx) {
        buf[idx] ^= xor_byte;
    }

    if(buf[11] == 0) {
        buf[7] ^= xor_byte;
    }

    uint8_t orig_b7 = buf[7];
    buf[7] = (orig_b7 & 0xAA) | (buf[6] & 0x55);
    uint8_t mixed = (buf[6] & 0xAA) | (orig_b7 & 0x55);
    buf[12] = mixed;
    buf[6] = mixed;

    uint32_t serial_le = ((uint32_t)buf[1]) | ((uint32_t)buf[2] << 8) | ((uint32_t)buf[3] << 16) |
                         ((uint32_t)buf[4] << 24);

    *serial = ((serial_le & 0xFF) << 24) | (((serial_le >> 8) & 0xFF) << 16) |
              (((serial_le >> 16) & 0xFF) << 8) | ((serial_le >> 24) & 0xFF);

    *button = (buf[5] >> 3) & 0x0F;

    *count = ((buf[5] & 0x07) << 16) | (buf[6] << 8) | buf[7];
}

// =============================================================================
// ENCODE FUNCTION
// =============================================================================
#if RF_STREAM_WITH_ENCODER
static void encode_ford_v0(
    uint8_t header_byte,
    uint32_t serial,
    uint8_t button,
    uint32_t count,
    uint8_t checksum,
    uint64_t* key1) {
    if(!key1) {
        RF_STREAM_LOG_E(TAG, "encode_ford_v0: NULL key1 pointer");
        return;
    }

    uint8_t buf[8] = {0};

    buf[0] = header_byte;

    buf[1] = (serial >> 24) & 0xFF;
    buf[2] = (serial >> 16) & 0xFF;
    buf[3] = (serial >> 8) & 0xFF;
    buf[4] = serial & 0xFF;

    buf[5] = ((button & 0x0F) << 3) | ((count >> 16) & 0x0F);

    uint8_t count_mid = (count >> 8) & 0xFF;
    uint8_t count_low = count & 0xFF;

    uint8_t post_xor_6 = (count_mid & 0xAA) | (count_low & 0x55);
    uint8_t post_xor_7 = (count_low & 0xAA) | (count_mid & 0x55);

    uint8_t parity = 0;
    uint8_t tmp = checksum;
    while(tmp) {
        parity ^= (tmp & 1);
        tmp >>= 1;
    }
    bool parity_bit = (checksum != 0) ? (parity != 0) : false;

    if(parity_bit) {
        uint8_t xor_byte = post_xor_7;
        buf[1] ^= xor_byte;
        buf[2] ^= xor_byte;
        buf[3] ^= xor_byte;
        buf[4] ^= xor_byte;
        buf[5] ^= xor_byte;
        buf[6] = post_xor_6 ^ xor_byte;
        buf[7] = post_xor_7;
    } else {
        uint8_t xor_byte = post_xor_6;
        buf[1] ^= xor_byte;
        buf[2] ^= xor_byte;
        buf[3] ^= xor_byte;
        buf[4] ^= xor_byte;
        buf[5] ^= xor_byte;
        buf[6] = post_xor_6;
        buf[7] = post_xor_7 ^ xor_byte;
    }

    *key1 = 0;
    for(int i = 0; i < 8; i++) {
        *key1 = (*key1 << 8) | buf[i];
    }

    RF_STREAM_LOG_I(
        TAG,
        "Encode: Sn=%08lX Btn=%d Cnt=%05lX Checksum=%02X",
        (unsigned long)serial,
        button,
        (unsigned long)count,
        checksum);
    RF_STREAM_LOG_I(
        TAG,
        "Encode key1: %08lX%08lX",
        (unsigned long)(*key1 >> 32),
        (unsigned long)(*key1 & 0xFFFFFFFF));
}

// =============================================================================
// ENCODER IMPLEMENTATION
// =============================================================================

void* rf_stream_protocol_encoder_ford_v0_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolEncoderFordV0* instance = calloc(1, sizeof(RfStreamProtocolEncoderFordV0));
    if(!instance) {
        return NULL;
    }

    instance->base.protocol = &ford_protocol_v0;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 10;
    rf_stream_encoder_buffer_ensure(instance, FORD_V0_UPLOAD_CAPACITY);
    instance->encoder.is_running = false;
    instance->encoder.front = 0;

    instance->key1 = 0;
    instance->key2 = 0;
    instance->serial = 0;
    instance->button = 0;
    instance->count = 0;
    instance->checksum = 0;

    RF_STREAM_LOG_I(TAG, "Encoder allocated");
    return instance;
}

static void rf_stream_protocol_encoder_ford_v0_get_upload(RfStreamProtocolEncoderFordV0* instance) {
    size_t index = 0;

    uint64_t tx_key1 = ~instance->key1;
    uint16_t tx_key2 = ~instance->key2;

    RF_STREAM_LOG_I(
        TAG,
        "Building upload: key1=%08lX%08lX key2=%04X",
        (unsigned long)(instance->key1 >> 32),
        (unsigned long)(instance->key1 & 0xFFFFFFFF),
        instance->key2);

    uint32_t te_short = rf_stream_protocol_ford_v0_const.te_short;
    uint32_t te_long = rf_stream_protocol_ford_v0_const.te_long;

#define ADD_LEVEL(lvl, dur) \
    index = rf_stream_emit_merge(instance->encoder.upload, index, FORD_V0_UPLOAD_CAPACITY, (lvl), (dur))

    for(uint8_t burst = 0; burst < FORD_V0_TOTAL_BURSTS; burst++) {
        ADD_LEVEL(true, te_short);
        ADD_LEVEL(false, te_long);

        for(int i = 0; i < FORD_V0_PREAMBLE_PAIRS; i++) {
            ADD_LEVEL(true, te_long);
            ADD_LEVEL(false, te_long);
        }

        ADD_LEVEL(true, te_short);
        ADD_LEVEL(false, FORD_V0_GAP_US);

        bool first_bit = (tx_key1 >> 62) & 1;
        if(first_bit) {
            // Finish the implicit leading 1, then emit another 1. A single
            // long HIGH is invalid from RfManchesterStateMid1 and loses bit 62.
            ADD_LEVEL(true, te_short);
            ADD_LEVEL(false, te_short);
            ADD_LEVEL(true, te_short);
        } else {
            ADD_LEVEL(true, te_short);
            ADD_LEVEL(false, te_long);
        }

        bool prev_bit = first_bit;

        for(int bit = 61; bit >= 0; bit--) {
            bool curr_bit = (tx_key1 >> bit) & 1;

            if(!prev_bit && !curr_bit) {
                ADD_LEVEL(true, te_short);
                ADD_LEVEL(false, te_short);
            } else if(!prev_bit && curr_bit) {
                ADD_LEVEL(true, te_long);
            } else if(prev_bit && !curr_bit) {
                ADD_LEVEL(false, te_long);
            } else {
                ADD_LEVEL(false, te_short);
                ADD_LEVEL(true, te_short);
            }

            prev_bit = curr_bit;
        }

        for(int bit = 15; bit >= 0; bit--) {
            bool curr_bit = (tx_key2 >> bit) & 1;

            if(!prev_bit && !curr_bit) {
                ADD_LEVEL(true, te_short);
                ADD_LEVEL(false, te_short);
            } else if(!prev_bit && curr_bit) {
                ADD_LEVEL(true, te_long);
            } else if(prev_bit && !curr_bit) {
                ADD_LEVEL(false, te_long);
            } else {
                ADD_LEVEL(false, te_short);
                ADD_LEVEL(true, te_short);
            }

            prev_bit = curr_bit;
        }

        if(burst < FORD_V0_TOTAL_BURSTS - 1) {
            ADD_LEVEL(false, te_long * 100);
        }
    }

#undef ADD_LEVEL

    instance->encoder.size_upload = index;
    instance->encoder.front = 0;

    RF_STREAM_LOG_I(TAG, "Upload built: %d bursts, size=%zu", FORD_V0_TOTAL_BURSTS, index);
}

RfStreamProtocolStatus
    rf_stream_protocol_encoder_ford_v0_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderFordV0* instance = context;
    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    instance->encoder.is_running = false;
    instance->encoder.front = 0;

    do {
        if(rf_stream_verify_protocol_name(flipper_format, instance->base.protocol->name) !=
           RfStreamProtocolStatusOk) {
            break;
        }

        uint64_t original_key1 = 0;
        if(!rf_stream_flipper_read_hex_u64(flipper_format, FF_KEY, &original_key1)) {
            break;
        }
        uint8_t header_byte = (uint8_t)(original_key1 >> 56);

        uint32_t serial = UINT32_MAX;
        uint32_t btn = UINT32_MAX;
        uint32_t cnt = UINT32_MAX;
        // A full-width serial (0xFFFFFFFF) is data, not a missing-field sentinel.
        rf_record_rewind(flipper_format);
        if(!rf_record_read_uint32(flipper_format, FF_SERIAL, &serial, 1)) break;
        rf_record_rewind(flipper_format);
        if(!rf_record_read_uint32(flipper_format, FF_BTN, &btn, 1)) break;
        rf_record_rewind(flipper_format);
        if(!rf_record_read_uint32(flipper_format, FF_CNT, &cnt, 1)) break;
        if(btn > 0xFU || cnt > 0x7FFFFU || (header_byte & 0x80U)) break;

        instance->serial = serial;
        instance->button = (uint8_t)btn;
        instance->count = cnt;
        instance->generic.serial = instance->serial;
        instance->generic.btn = instance->button;
        instance->generic.cnt = instance->count;

        // Calculate Checksum from counter and button.
        instance->checksum =
            ford_v0_calculate_checksum(instance->serial, instance->count, instance->button);
        RF_STREAM_LOG_I(
            TAG,
            "Calculated Checksum: 0x%02X (from Cnt=0x%05lX, Btn=0x%02X)",
            instance->checksum,
            (unsigned long)instance->count,
            instance->button);

        encode_ford_v0(
            header_byte,
            instance->serial,
            instance->button,
            instance->count,
            instance->checksum,
            &instance->key1);

        instance->generic.data = instance->key1;
        instance->generic.data_count_bit = 64;

        uint8_t calculated_crc = ford_v0_calculate_crc_for_tx(instance->key1, instance->checksum);
        instance->key2 = ((uint16_t)instance->checksum << 8) | calculated_crc;

        RF_STREAM_LOG_I(
            TAG,
            "Final key2: 0x%04X (Checksum=0x%02X, CRC=0x%02X)",
            instance->key2,
            instance->checksum,
            calculated_crc);

        instance->encoder.repeat = rf_stream_encoder_read_repeat(flipper_format, 10);

        rf_stream_protocol_encoder_ford_v0_get_upload(instance);

        uint8_t updated_key[8];
        rf_stream_u64_to_bytes_be(instance->key1, updated_key);
        if(!rf_record_insert_or_update_hex(flipper_format, FF_KEY, updated_key, 8)) break;

        if(instance->encoder.size_upload == 0) {
            RF_STREAM_LOG_E(TAG, "Upload build failed");
            break;
        }

        //Update the PSF file, since we have overwritten the COUNTER and BUTTON
        //This makes the file's nummers wrong, and fails tests. It wasnt causing a TX bug, but manual tests failed.
        rf_record_rewind(flipper_format);
        uint32_t temp = calculated_crc;
        rf_record_insert_or_update_uint32(flipper_format, "CRC", &temp, 1);
        temp = instance->checksum;
        rf_record_insert_or_update_uint32(flipper_format, "Checksum", &temp, 1);

        instance->encoder.is_running = true;

        RF_STREAM_LOG_I(
            TAG,
            "Encoder ready: size=%zu repeat=%u",
            instance->encoder.size_upload,
            instance->encoder.repeat);

        ret = RfStreamProtocolStatusOk;
    } while(false);

    RF_STREAM_LOG_I(TAG, "Encoder deserialize finished, status=%d", ret);
    return ret;
}

#endif
// =============================================================================
// DECODER IMPLEMENTATION
// =============================================================================

static void ford_v0_add_bit(RfStreamProtocolDecoderFordV0* instance, bool bit) {
#ifdef FORD_V0_DEBUG_BITS
    RF_STREAM_LOG_D(TAG, "Bit %d: %d", instance->bit_count, bit);
#endif

    uint32_t low = (uint32_t)instance->data_low;
    instance->data_low = (instance->data_low << 1) | (bit ? 1 : 0);
    instance->data_high = (instance->data_high << 1) | ((low >> 31) & 1);
    instance->bit_count++;
}

static bool ford_v0_process_data(RfStreamProtocolDecoderFordV0* instance) {
    if(instance->bit_count == 64) {
        uint64_t combined = ((uint64_t)instance->data_high << 32) | instance->data_low;
        instance->key1 = ~combined;
        instance->data_low = 0;
        instance->data_high = 0;
        return false;
    }

    if(instance->bit_count == 80) {
        uint16_t key2_raw = (uint16_t)(instance->data_low & 0xFFFF);
        uint16_t key2 = ~key2_raw;

        decode_ford_v0(
            instance->key1, key2, &instance->serial, &instance->button, &instance->count);

        instance->key2 = key2;
        return true;
    }

    return false;
}

void* rf_stream_protocol_decoder_ford_v0_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolDecoderFordV0* instance = calloc(1, sizeof(RfStreamProtocolDecoderFordV0));
    if(!instance) {
        return NULL;
    }
    instance->base.protocol = &ford_protocol_v0;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void rf_stream_protocol_decoder_ford_v0_reset(void* context) {
    RfStreamProtocolDecoderFordV0* instance = context;

    instance->decoder.parser_step = FordV0DecoderStepReset;
    instance->decoder.te_last = 0;
    instance->manchester_state = RfManchesterStateMid1;
    instance->data_low = 0;
    instance->data_high = 0;
    instance->bit_count = 0;
    instance->header_count = 0;
    instance->key1 = 0;
    instance->key2 = 0;
    instance->serial = 0;
    instance->button = 0;
    instance->count = 0;
}

void rf_stream_protocol_decoder_ford_v0_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderFordV0* instance = context;

    uint32_t te_short = rf_stream_protocol_ford_v0_const.te_short;
    uint32_t te_long = rf_stream_protocol_ford_v0_const.te_long;
    uint32_t te_delta = rf_stream_protocol_ford_v0_const.te_delta;
    uint32_t gap_threshold = 3500;

    switch(instance->decoder.parser_step) {
    case FordV0DecoderStepReset:
        if(level && (DURATION_DIFF(duration, te_short) < te_delta)) {
            instance->data_low = 0;
            instance->data_high = 0;
            instance->decoder.parser_step = FordV0DecoderStepPreamble;
            instance->decoder.te_last = duration;
            instance->header_count = 0;
            instance->bit_count = 0;
            rf_manchester_advance(
                instance->manchester_state,
                RfManchesterEventReset,
                &instance->manchester_state,
                NULL);
        }
        break;

    case FordV0DecoderStepPreamble:
        if(!level) {
            if(DURATION_DIFF(duration, te_long) < te_delta) {
                instance->decoder.te_last = duration;
                instance->decoder.parser_step = FordV0DecoderStepPreambleCheck;
            } else {
                instance->decoder.parser_step = FordV0DecoderStepReset;
            }
        }
        break;

    case FordV0DecoderStepPreambleCheck:
        if(level) {
            if(DURATION_DIFF(duration, te_long) < te_delta) {
                instance->header_count++;
                instance->decoder.te_last = duration;
                instance->decoder.parser_step = FordV0DecoderStepPreamble;
            } else if(DURATION_DIFF(duration, te_short) < te_delta) {
                instance->decoder.parser_step = FordV0DecoderStepGap;
            } else {
                instance->decoder.parser_step = FordV0DecoderStepReset;
            }
        }
        break;

    case FordV0DecoderStepGap:
        if(!level && (DURATION_DIFF(duration, gap_threshold) < 250)) {
            instance->data_low = 1;
            instance->data_high = 0;
            instance->bit_count = 1;
            instance->decoder.parser_step = FordV0DecoderStepData;
        } else if(!level && duration > gap_threshold + 250) {
            instance->decoder.parser_step = FordV0DecoderStepReset;
        }
        break;

    case FordV0DecoderStepData: {
        RfManchesterEvent event =
            rf_stream_manchester_event(duration, level, &rf_stream_protocol_ford_v0_const);
        if(event == RfManchesterEventReset) {
            instance->decoder.parser_step = FordV0DecoderStepReset;
            break;
        }

        bool data_bit;
        if(rf_manchester_advance(
               instance->manchester_state, event, &instance->manchester_state, &data_bit)) {
            ford_v0_add_bit(instance, data_bit);

            if(ford_v0_process_data(instance)) {
                instance->generic.data = instance->key1;
                instance->generic.data_count_bit = 64;
                instance->generic.serial = instance->serial;
                instance->generic.btn = instance->button;
                instance->generic.cnt = instance->count;

                if(instance->base.callback) {
                    instance->base.callback(&instance->base, instance->base.context);
                }

                instance->data_low = 0;
                instance->data_high = 0;
                instance->bit_count = 0;
                instance->decoder.parser_step = FordV0DecoderStepReset;
            }
        }

        instance->decoder.te_last = duration;
        break;
    }
    }
}

RfStreamProtocolStatus rf_stream_protocol_decoder_ford_v0_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderFordV0* instance = context;

    RfStreamProtocolStatus ret =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);

    if(ret == RfStreamProtocolStatusOk) {
        uint32_t temp = (instance->key2 >> 8) & 0xFF;
        rf_record_write_uint32(flipper_format, "Checksum", &temp, 1);

        temp = instance->key2 & 0xFF;
        rf_record_write_uint32(flipper_format, "CRC", &temp, 1);

        rf_stream_serialize_fields(
            flipper_format,
            RF_FIELD_SERIAL | RF_FIELD_BTN | RF_FIELD_CNT,
            instance->serial,
            instance->button,
            instance->count,
            0);
    }

    return ret;
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_ford_v0_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderFordV0* instance = context;

    RfStreamProtocolStatus ret = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, rf_stream_protocol_ford_v0_const.min_count_bit_for_found);

    if(ret == RfStreamProtocolStatusOk) {
        instance->key1 = instance->generic.data;

        rf_record_rewind(flipper_format);

        uint32_t checksum_temp = 0;
        uint32_t crc_temp = 0;
        rf_record_read_uint32(flipper_format, "Checksum", &checksum_temp, 1);
        rf_record_read_uint32(flipper_format, "CRC", &crc_temp, 1);
        instance->key2 = ((checksum_temp & 0xFF) << 8) | (crc_temp & 0xFF);

        uint32_t serial = instance->serial;
        uint32_t btn = instance->button;
        uint32_t cnt = instance->count;
        rf_stream_encoder_read_fields(flipper_format, &serial, &btn, &cnt, NULL);
        instance->serial = serial;
        instance->button = (uint8_t)btn;
        instance->count = cnt;
        instance->generic.serial = instance->serial;
        instance->generic.btn = instance->button;
        instance->generic.cnt = instance->count;
    }

    return ret;
}

void rf_stream_protocol_decoder_ford_v0_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderFordV0* instance = context;

    uint32_t code_found_hi = (uint32_t)(instance->key1 >> 32);
    uint32_t code_found_lo = (uint32_t)(instance->key1 & 0xFFFFFFFF);

    bool crc_ok = ford_v0_verify_crc(instance->key1, instance->key2);

    const char* button_name = "??";
    if(instance->button == 0x01)
        button_name = "Panic";
    else if(instance->button == 0x02)
        button_name = "Lock";
    else if(instance->button == 0x04)
        button_name = "Unlock";
    else if(instance->button == 0x08)
        button_name = "Boot";

    rf_text_cat_printf(
        output,
        "%s %dbit CRC:%s\r\n"
        "Key1: %08lX%08lX\r\n"
        "Key2: %04X"
        "  Sn: %08lX\r\n"
        "Cnt: %05lX"
        "  Checksum: %02X\r\n"
        "Btn: %02X - %s"
        "  CRC: %02X",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        crc_ok ? "OK" : "BAD",
        (unsigned long)code_found_hi,
        (unsigned long)code_found_lo,
        instance->key2,
        (unsigned long)instance->serial,

        (unsigned long)instance->count,
        (instance->key2 >> 8) & 0xFF,
        instance->button,
        button_name,
        instance->key2 & 0xFF);
}

#endif
