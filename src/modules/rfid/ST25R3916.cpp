#include "ST25R3916.h"
#if !defined(LITE_VERSION)

// ST25R3916 via RFAL fork (lewisxhe/ST25R3916-fork + NFC-RFAL-fork).
// Supported: SPI mode on boards with ST25R wiring such as lilygo-t-lora-pager and reaper.
// I2C mode uses the fork constructor RfalRfST25R3916Class(&Wire, irqPin).
// Card emulation/listen mode is not wired in this driver yet.
// ISO15693 writes are not implemented.

#include "core/sd_functions.h"
#include "modules/rfid/apdu.h"
#include <esp_random.h>
#include <globals.h>

#define ST25R_DEBUG 1
#if ST25R_DEBUG
#define ST25R_LOG(fmt, ...) Serial.printf("[ST25R] " fmt "\n", ##__VA_ARGS__)
#else
#define ST25R_LOG(fmt, ...)                                                                                  \
    do {                                                                                                     \
    } while (0)
#endif

static const char *_stateStr(rfalNfcState st) {
    switch (st) {
        case RFAL_NFC_STATE_NOTINIT: return "NOTINIT";
        case RFAL_NFC_STATE_IDLE: return "IDLE";
        case RFAL_NFC_STATE_START_DISCOVERY: return "START_DISCOVERY";
        case RFAL_NFC_STATE_WAKEUP_MODE: return "WAKEUP_MODE";
        case RFAL_NFC_STATE_POLL_TECHDETECT: return "POLL_TECHDETECT";
        case RFAL_NFC_STATE_POLL_COLAVOIDANCE: return "POLL_COLAVOIDANCE";
        case RFAL_NFC_STATE_POLL_SELECT: return "POLL_SELECT";
        case RFAL_NFC_STATE_POLL_ACTIVATION: return "POLL_ACTIVATION";
        case RFAL_NFC_STATE_LISTEN_TECHDETECT: return "LISTEN_TECHDETECT";
        case RFAL_NFC_STATE_ACTIVATED: return "ACTIVATED";
        case RFAL_NFC_STATE_DATAEXCHANGE: return "DATAEXCHANGE";
        case RFAL_NFC_STATE_DEACTIVATION: return "DEACTIVATION";
        default: return "UNKNOWN";
    }
}

static void _deselectSharedSpiDevices() {
    /*
    #if defined(TFT_CS) && TFT_CS >= 0
        digitalWrite(TFT_CS, HIGH);
    #endif
    #if defined(SDCARD_CS) && SDCARD_CS >= 0
        digitalWrite(SDCARD_CS, HIGH);
    #endif
    #if defined(LORA_CS) && LORA_CS >= 0
        digitalWrite(LORA_CS, HIGH);
    #endif
    #if defined(NFC_CS) && NFC_CS >= 0
        digitalWrite(NFC_CS, HIGH);
    #endif
    */
}

static void _setNfcPower(bool enabled) {
#if defined(IO_EXP_NFC) && IO_EXP_NFC >= 0
    ioExpander.setPinDirection(IO_EXP_NFC, OUTPUT);
    ioExpander.turnPinOnOff(IO_EXP_NFC, enabled ? HIGH : LOW);
#endif
}

namespace {
int st25HexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool st25ParseHexBytesAfterColon(const String &line, std::vector<uint8_t> &bytes) {
    bytes.clear();
    int colon = line.indexOf(':');
    if (colon < 0) return false;

    int hi = -1;
    for (int i = colon + 1; i < line.length(); i++) {
        int v = st25HexNibble(line.charAt(i));
        if (v < 0) continue;
        if (hi < 0) {
            hi = v;
        } else {
            bytes.push_back(static_cast<uint8_t>((hi << 4) | v));
            hi = -1;
        }
    }
    return !bytes.empty() && hi < 0;
}

bool st25ExtractNdefMessageFromPageDump(const String &dump, std::vector<uint8_t> &ndefOut) {
    ndefOut.clear();
    if (dump.length() == 0) return false;

    std::vector<uint8_t> userData;
    std::vector<uint8_t> lineBytes;
    int pos = 0;

    while (pos < dump.length()) {
        int nl = dump.indexOf('\n', pos);
        if (nl < 0) nl = dump.length();

        String line = dump.substring(pos, nl);
        line.trim();
        pos = nl + 1;

        if (!line.startsWith("Page ")) continue;

        int colon = line.indexOf(':');
        if (colon < 0) continue;

        int page = line.substring(5, colon).toInt();
        if (page < 4) continue;

        if (!st25ParseHexBytesAfterColon(line, lineBytes)) continue;
        if (lineBytes.size() < 4) continue;
        userData.insert(userData.end(), lineBytes.begin(), lineBytes.begin() + 4);
    }

    size_t i = 0;
    while (i < userData.size()) {
        uint8_t tlv = userData[i++];

        if (tlv == 0x00) continue;
        if (tlv == 0xFE) break;
        if (i >= userData.size()) return false;

        uint32_t len = userData[i++];
        if (len == 0xFF) {
            if (i + 1 >= userData.size()) return false;
            len = (static_cast<uint32_t>(userData[i]) << 8) | userData[i + 1];
            i += 2;
        }

        if (i + len > userData.size()) return false;

        if (tlv == 0x03) {
            ndefOut.assign(userData.begin() + i, userData.begin() + i + len);
            return !ndefOut.empty();
        }

        i += len;
    }

    return false;
}

bool st25BuildNdefMessageFromStruct(const RFIDInterface::NdefMessage &src, std::vector<uint8_t> &ndefOut) {
    ndefOut.clear();
    if (src.messageSize == 0 || src.payloadSize == 0) return false;

    ndefOut.reserve(4 + src.payloadSize);
    ndefOut.push_back(src.header);
    ndefOut.push_back(src.tnf);
    ndefOut.push_back(src.payloadSize);
    ndefOut.push_back(src.payloadType);
    ndefOut.insert(ndefOut.end(), src.payload, src.payload + src.payloadSize);

    return ndefOut.size() == src.messageSize;
}
} // namespace

void ST25R3916::_logOpControl(const char *where) {
    if (!_hw) return;

    uint8_t op = 0;
    uint8_t id = 0;
    uint8_t io1 = 0;
    uint8_t io2 = 0;
    _hw->st25r3916ReadRegister(ST25R3916_REG_OP_CONTROL, &op);
    _hw->st25r3916ReadRegister(ST25R3916_REG_IC_IDENTITY, &id);
    _hw->st25r3916ReadRegister(ST25R3916_REG_IO_CONF1, &io1);
    _hw->st25r3916ReadRegister(ST25R3916_REG_IO_CONF2, &io2);
    ST25R_LOG(
        "%s OP_CONTROL=0x%02X en=%d rx=%d tx=%d IC_ID=0x%02X IO1=0x%02X IO2=0x%02X",
        where,
        op,
        (op & ST25R3916_REG_OP_CONTROL_en) ? 1 : 0,
        (op & ST25R3916_REG_OP_CONTROL_rx_en) ? 1 : 0,
        (op & ST25R3916_REG_OP_CONTROL_tx_en) ? 1 : 0,
        id,
        io1,
        io2
    );
}

void ST25R3916::_probeField(const char *where) {
    if (!_hw) return;

    auto err = _hw->rfalFieldOnAndStartGT();
    ST25R_LOG("%s rfalFieldOnAndStartGT -> %d", where, err);
    _logOpControl("after manual field on");
    _hw->rfalFieldOff();
    _logOpControl("after manual field off");
}

ST25R3916::ST25R3916(CONNECTION_TYPE connection_type) : _connection_type(connection_type) {}

ST25R3916::~ST25R3916() {
    stopDiscovery();
    _setNfcPower(false);
    delete _nfc;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
    // RFAL class is deleted through its concrete type, but the fork lacks a virtual destructor.
    delete _hw;
#pragma GCC diagnostic pop
}

bool ST25R3916::_initSPI() {
    _deselectSharedSpiDevices();

    int cs = (int)bruceConfigPins.ST25R_bus.cs;
    int irq = (int)bruceConfigPins.ST25R_bus.io0;
    ST25R_LOG(
        "_initSPI: CS=%d IRQ=%d MOSI=%d MISO=%d SCK=%d",
        cs,
        irq,
        (int)bruceConfigPins.ST25R_bus.mosi,
        (int)bruceConfigPins.ST25R_bus.miso,
        (int)bruceConfigPins.ST25R_bus.sck
    );

    if (bruceConfigPins.ST25R_bus.mosi == (gpio_num_t)TFT_MOSI &&
        bruceConfigPins.ST25R_bus.mosi != GPIO_NUM_NC) {
#if TFT_MOSI >= 0
        ST25R_LOG("_initSPI: sharing TFT SPI");
        _spi = &tft.getSPIinstance();
#endif
    } else if (bruceConfigPins.ST25R_bus.mosi == bruceConfigPins.SDCARD_bus.mosi) {
        ST25R_LOG("_initSPI: sharing SDCARD SPI");
        _spi = &sdcardSPI;
    } else {
        ST25R_LOG("_initSPI: creating dedicated HSPI");
        static SPIClass st25rSPI(HSPI);
        static bool spiStarted = false;
        if (!spiStarted) {
            st25rSPI.begin(
                (int)bruceConfigPins.ST25R_bus.sck,
                (int)bruceConfigPins.ST25R_bus.miso,
                (int)bruceConfigPins.ST25R_bus.mosi
            );
            spiStarted = true;
        }
        _spi = &st25rSPI;
    }

    _hw = new RfalRfST25R3916Class(_spi, cs, irq);
    ST25R_LOG("_initSPI: _hw=%s", _hw ? "ok" : "NULL");
    return _hw != nullptr;
}

bool ST25R3916::_initI2C() {
    Wire.begin(bruceConfigPins.i2c_bus.sda, bruceConfigPins.i2c_bus.scl);
    _hw = new RfalRfST25R3916Class(&Wire, GPIO_NUM_NC);
    return _hw != nullptr;
}

bool ST25R3916::begin() {
    _setNfcPower(true);
    delay(5);
    _deselectSharedSpiDevices();
    ST25R_LOG("begin: mode=%s", _connection_type == SPI_MODE ? "SPI" : "I2C");
    bool ok = (_connection_type == SPI_MODE) ? _initSPI() : _initI2C();
    if (!ok) {
        ST25R_LOG("begin: hw init FAILED");
        return false;
    }

    _nfc = new RfalNfcClass(_hw);
    uint16_t err = _nfc->rfalNfcInitialize();
    ST25R_LOG("begin: rfalNfcInitialize -> %d (%s)", err, err == ST_ERR_NONE ? "OK" : "FAIL");
    _logOpControl("after init");
    _hw->st25r3916OscOn();
    _logOpControl("after forced osc on");
    return (err == ST_ERR_NONE);
}

static void _notifyCb(rfalNfcState st) { /* ST25R_LOG("notifyCb: %s(%d)", _stateStr(st), (int)st); */ }

bool ST25R3916::_startDiscovery() {
    _deselectSharedSpiDevices();

    rfalNfcDiscoverParam params;
    memset(&params, 0, sizeof(params));
    params.compMode = RFAL_COMPLIANCE_MODE_NFC;
    params.devLimit = 1;
    params.nfcfBR = RFAL_BR_212;
    params.ap2pBR = RFAL_BR_424;
    params.techs2Find =
        RFAL_NFC_POLL_TECH_A | RFAL_NFC_POLL_TECH_B | RFAL_NFC_POLL_TECH_V | RFAL_NFC_POLL_TECH_F;
    params.GBLen = RFAL_NFCDEP_GB_MAX_LEN;
    params.notifyCb = _notifyCb;
    params.totalDuration = 1000U;
    params.wakeupEnabled = false;
    params.wakeupConfigDefault = true;

    uint16_t err = _nfc->rfalNfcDiscover(&params);
    ST25R_LOG("_startDiscovery: rfalNfcDiscover -> %d (%s)", err, err == ST_ERR_NONE ? "OK" : "FAIL");
    _hw->st25r3916OscOn();
    _logOpControl("after discover");
    return (err == ST_ERR_NONE);
}

void ST25R3916::stopDiscovery() {
    if (_discoveryStarted && _nfc) {
        _nfc->rfalNfcDeactivate(false);
        _discoveryStarted = false;
    }
}

String ST25R3916::_getNfcaTypeName(uint8_t sak) {
    if (sak == 0x08 || sak == 0x88) return "MIFARE Classic 1K";
    if (sak == 0x18) return "MIFARE Classic 4K";
    if (sak == 0x09) return "MIFARE Mini";
    if (sak == 0x00) return "MIFARE Ultralight";
    if (sak == 0x20) return "ISO 14443-4";
    return "ISO 14443A";
}

void ST25R3916::_parseDevice(rfalNfcDevice *dev) {
    uid.size = dev->nfcidLen;
    memcpy(uid.uidByte, dev->nfcid, uid.size);

    printableUID.uid = "";
    for (int i = 0; i < uid.size; i++) {
        char buf[3];
        sprintf(buf, "%02X", uid.uidByte[i]);
        printableUID.uid += buf;
        if (i < uid.size - 1) printableUID.uid += " ";
    }

    uint8_t bcc = 0;
    for (int i = 0; i < uid.size; i++) bcc ^= uid.uidByte[i];
    char bccBuf[3];
    sprintf(bccBuf, "%02X", bcc);
    printableUID.bcc = String(bccBuf);

    switch (dev->type) {
        case RFAL_NFC_LISTEN_TYPE_NFCA: {
            rfalNfcaListenDevice *nfca = &dev->dev.nfca;
            uid.sak = nfca->selRes.sak;
            char sakBuf[3];
            sprintf(sakBuf, "%02X", uid.sak);
            printableUID.sak = String(sakBuf);

            uid.atqaByte[0] = nfca->sensRes.anticollisionInfo;
            uid.atqaByte[1] = nfca->sensRes.platformInfo;
            // ATQA em ordem de transmissão (byte baixo primeiro), igual ao PN532/Flipper
            // (ex.: NTAG => "44 00").
            char atqaBuf[6];
            sprintf(atqaBuf, "%02X %02X", uid.atqaByte[0], uid.atqaByte[1]);
            printableUID.atqa = String(atqaBuf);

            printableUID.picc_type = _getNfcaTypeName(uid.sak);
            break;
        }
        case RFAL_NFC_LISTEN_TYPE_NFCB:
            printableUID.picc_type = "ISO14443B";
            printableUID.sak = "--";
            printableUID.atqa = "--";
            break;
        case RFAL_NFC_LISTEN_TYPE_NFCV:
            printableUID.picc_type = "ISO15693";
            printableUID.sak = "--";
            printableUID.atqa = "--";
            break;
        default:
            printableUID.picc_type = "Unknown";
            printableUID.sak = "--";
            printableUID.atqa = "--";
            break;
    }
}

int ST25R3916::_readDataBlocks(rfalNfcDevice *dev) {
    strAllPages = "";
    dataPages = 0;
    totalPages = 0;
    pageReadSuccess = false;

    // ISO-DEP (ISO14443-4) — T4T / DESFire / EMV (NFC-A SAK=0x20 ou NFC-B).
    if (dev->rfInterface == RFAL_NFC_INTERFACE_ISODEP) { return _readIsoDep(dev); }

    switch (dev->type) {
        case RFAL_NFC_LISTEN_TYPE_NFCB:
            _parseNfcB(dev);
            pageReadStatus = SUCCESS;
            pageReadSuccess = true;
            return SUCCESS;
        case RFAL_NFC_LISTEN_TYPE_NFCV: return _readNfcV(dev);
        case RFAL_NFC_LISTEN_TYPE_NFCF: return _readFeliCa(dev);
        default: break;
    }

    // MIFARE Classic (SAK 0x08/0x88 = 1K, 0x18 = 4K, 0x09 = Mini)
    if (dev->type == RFAL_NFC_LISTEN_TYPE_NFCA && isMifareClassicSak(uid.sak)) {
        return _readMifareClassic(dev);
    }

    if (dev->type != RFAL_NFC_LISTEN_TYPE_NFCA || uid.sak != 0x00) {
        pageReadStatus = FAILURE;
        return FAILURE;
    }

    // Detect NTAG size from CC page (page 3)
    uint8_t ccBuf[16] = {0};
    uint16_t rcvLen = 0;
    auto ccErr = _nfc->rfalT2TPollerRead(3, ccBuf, sizeof(ccBuf), &rcvLen);
    if (ccErr != ST_ERR_NONE || rcvLen < 4) {
        pageReadStatus = FAILURE;
        return FAILURE;
    }
    if (_ntagPagesHint > 0) {
        // GET_VERSION is authoritative (also covers blank/unformatted tags)
        totalPages = _ntagPagesHint;
    } else {
        switch (ccBuf[2]) {
            case 0x12: totalPages = 45; break;  // NTAG213
            case 0x3E: totalPages = 135; break; // NTAG215
            case 0x6D: totalPages = 231; break; // NTAG216
            default: totalPages = 64; break;    // MF Ultralight
        }
    }

    for (int page = 0; page < totalPages; page++) {
        uint8_t pageData[16] = {0};
        uint16_t len = 0;
        auto err = _nfc->rfalT2TPollerRead(page, pageData, sizeof(pageData), &len);
        if (err != ST_ERR_NONE || len < 4) {
            pageReadStatus = FAILURE;
            return FAILURE;
        }
        char line[32];
        sprintf(
            line, "Page %d: %02X %02X %02X %02X", page, pageData[0], pageData[1], pageData[2], pageData[3]
        );
        strAllPages += String(line) + "\n";
        dataPages++;
    }

    pageReadStatus = SUCCESS;
    pageReadSuccess = true;
    return SUCCESS;
}

void ST25R3916::_parseLoadedData() {
    String strUID = printableUID.uid;
    strUID.trim();
    strUID.replace(" ", "");
    uid.size = strUID.length() / 2;
    if (uid.size > sizeof(uid.uidByte)) uid.size = sizeof(uid.uidByte);
    for (size_t i = 0; i < uid.size; i++) {
        uid.uidByte[i] = strtoul(strUID.substring(i * 2, i * 2 + 2).c_str(), NULL, 16);
    }

    uint8_t bcc = 0;
    for (int i = 0; i < uid.size; i++) bcc ^= uid.uidByte[i];
    char bccBuf[3];
    sprintf(bccBuf, "%02X", bcc);
    printableUID.bcc = String(bccBuf);

    printableUID.sak.trim();
    uid.sak = strtoul(printableUID.sak.c_str(), NULL, 16);

    String strAtqa = printableUID.atqa;
    strAtqa.trim();
    strAtqa.replace(" ", "");
    uid.atqaByte[0] = 0;
    uid.atqaByte[1] = 0;
    if (strAtqa.length() >= 4) {
        // ATQA em ordem de transmissão (byte baixo primeiro), igual ao PN532/Flipper.
        uid.atqaByte[0] = strtoul(strAtqa.substring(0, 2).c_str(), NULL, 16);
        uid.atqaByte[1] = strtoul(strAtqa.substring(2, 4).c_str(), NULL, 16);
    }
}

bool ST25R3916::_isUltralightUserPage(int page) const {
    if (page < 4) return false;
    if (totalPages <= 0) return true;
    return page < (totalPages - 5);
}

bool ST25R3916::_buildLoadedNdefMessage(std::vector<uint8_t> &ndefOut) {
    if (st25ExtractNdefMessageFromPageDump(strAllPages, ndefOut)) return true;
    if (st25BuildNdefMessageFromStruct(ndefMessage, ndefOut)) return true;

    std::vector<uint8_t> uriPayload = Ndef::urlNdefAbbrv("https://bruce.computer");
    ndefOut = Ndef::newMessage(uriPayload);
    return !ndefOut.empty();
}

int ST25R3916::read(int cardBaudRate) {
    pageReadStatus = FAILURE;
    pageReadSuccess = false;
    if (!_nfc) return FAILURE;
    _deselectSharedSpiDevices();

    if (!_discoveryStarted) {
        ST25R_LOG("starting discovery");
        if (!_startDiscovery()) {
            ST25R_LOG("_startDiscovery FAILED");
            return FAILURE;
        }
        _discoveryStarted = true;
    }

    _deselectSharedSpiDevices();
    _hw->st25r3916OscOn();
    _nfc->rfalNfcWorker();

    rfalNfcState state = _nfc->rfalNfcGetState();
    ST25R_LOG("-> %s(%d)", _stateStr(state), (int)state);

    if (state != RFAL_NFC_STATE_ACTIVATED) { return TAG_NOT_PRESENT; }

    rfalNfcDevice *dev;
    _nfc->rfalNfcGetActiveDevice(&dev);
    ST25R_LOG("TAG! type=%d uid_len=%d", (int)dev->type, dev->nfcidLen);
    _parseDevice(dev);
    ST25R_LOG(
        "uid=%s type=%s sak=%s atqa=%s",
        printableUID.uid.c_str(),
        printableUID.picc_type.c_str(),
        printableUID.sak.c_str(),
        printableUID.atqa.c_str()
    );
    ntagVariant = "";
    ntagHasVersion = false;
    ntagHasSignature = false;
    ntagHasCounters = false;
    _ntagPagesHint = 0;
    bool isNfcaT2T = (dev->type == RFAL_NFC_LISTEN_TYPE_NFCA && uid.sak == 0x00);

    // Read data blocks FIRST. GET_VERSION (0x60) is unsupported by non-EV1
    // MIFARE Ultralight and puts the PICC into HALT, which would make every
    // subsequent READ time out. _readDataBlocks detects the size from the CC
    // page when _ntagPagesHint is 0, so the GET_VERSION hint is not required.
    int blkResult = _readDataBlocks(dev);
    ST25R_LOG("_readDataBlocks -> %d pages=%d", blkResult, dataPages);

    if (isNfcaT2T) {
        // Variant/signature/counter are NTAG21x extras; running them after the
        // data read means a HALT from an unsupported command no longer hurts.
        String variant = _getNtagVariant();
        if (variant.length() > 0) {
            ntagVariant = variant;
            printableUID.picc_type = variant;
        }
        _readNtagSignature();
        _readNtagCounters();
    }

    _nfc->rfalNfcDeactivate(false);
    _discoveryStarted = false;
    return SUCCESS;
}

bool ST25R3916::_pollForTag(rfalNfcDevice **dev, uint32_t timeoutMs) {
    if (_discoveryStarted) {
        _nfc->rfalNfcDeactivate(false);
        _discoveryStarted = false;
    }
    _deselectSharedSpiDevices();

    rfalNfcDiscoverParam params;
    memset(&params, 0, sizeof(params));
    params.compMode = RFAL_COMPLIANCE_MODE_NFC;
    params.devLimit = 1;
    params.nfcfBR = RFAL_BR_212;
    params.ap2pBR = RFAL_BR_424;
    params.techs2Find = RFAL_NFC_POLL_TECH_A;
    params.GBLen = RFAL_NFCDEP_GB_MAX_LEN;
    params.notifyCb = _notifyCb;
    params.totalDuration = timeoutMs;
    params.wakeupEnabled = false;
    params.wakeupConfigDefault = true;

    if (_nfc->rfalNfcDiscover(&params) != ST_ERR_NONE) return false;
    _hw->st25r3916OscOn();

    uint32_t deadline = millis() + timeoutMs;
    while (millis() < deadline) {
        _nfc->rfalNfcWorker();
        if (_nfc->rfalNfcGetState() == RFAL_NFC_STATE_ACTIVATED) {
            _nfc->rfalNfcGetActiveDevice(dev);
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return false;
}

bool ST25R3916::_writeT2TPage(uint8_t page, const uint8_t data[4], bool verify) {
    auto err = _nfc->rfalT2TPollerWrite(page, const_cast<uint8_t *>(data));
    if (err != ST_ERR_NONE) {
        ST25R_LOG("_writeT2TPage: page %u write failed err=%d", page, (int)err);
        return false;
    }

    delay(12);

    if (!verify) return true;

    uint8_t rx[16] = {0};
    uint16_t rxLen = 0;
    err = _nfc->rfalT2TPollerRead(page, rx, sizeof(rx), &rxLen);
    if (err != ST_ERR_NONE || rxLen < 4) {
        ST25R_LOG("_writeT2TPage: page %u verify read failed err=%d len=%u", page, (int)err, rxLen);
        return false;
    }

    if (memcmp(rx, data, 4) != 0) {
        ST25R_LOG(
            "_writeT2TPage: page %u verify mismatch got=%02X %02X %02X %02X expected=%02X %02X %02X %02X",
            page,
            rx[0],
            rx[1],
            rx[2],
            rx[3],
            data[0],
            data[1],
            data[2],
            data[3]
        );
        return false;
    }

    return true;
}

int ST25R3916::_writeUltralight(rfalNfcDevice *dev) {
    String pages = strAllPages;
    int written = 0;
    int pos = 0;

    while (pos < (int)pages.length()) {
        int nl = pages.indexOf('\n', pos);
        if (nl < 0) nl = pages.length();
        String line = pages.substring(pos, nl);
        pos = nl + 1;
        if (line.length() == 0) continue;

        int pageNum = -1;
        unsigned int b0 = 0, b1 = 0, b2 = 0, b3 = 0;
        if (sscanf(line.c_str(), "Page %d: %02X %02X %02X %02X", &pageNum, &b0, &b1, &b2, &b3) == 5) {
            if (!_isUltralightUserPage(pageNum)) continue;
            uint8_t data[4] = {(uint8_t)b0, (uint8_t)b1, (uint8_t)b2, (uint8_t)b3};
            if (!_writeT2TPage((uint8_t)pageNum, data)) {
                _nfc->rfalNfcDeactivate(false);
                return FAILURE;
            }
            written++;
        }
    }

    ST25R_LOG("_writeUltralight: wrote %d pages", written);
    _nfc->rfalNfcDeactivate(false);
    return SUCCESS;
}

int ST25R3916::_eraseUltralight(rfalNfcDevice *dev) {
    uint8_t zeros[4] = {0x00, 0x00, 0x00, 0x00};
    for (int page = 4; page < totalPages; page++) {
        if (!_isUltralightUserPage(page)) continue;
        if (!_writeT2TPage((uint8_t)page, zeros)) {
            _nfc->rfalNfcDeactivate(false);
            return FAILURE;
        }
    }
    ST25R_LOG("_eraseUltralight: erased pages 4..%d", totalPages - 1);
    _nfc->rfalNfcDeactivate(false);
    return SUCCESS;
}

int ST25R3916::_writeNdefBlocks(rfalNfcDevice *dev) {
    uint8_t ndefPayload[180] = {0};
    size_t idx = 0;

    if (ndefMessage.messageSize == 0 || ndefMessage.payloadSize == 0) return FAILURE;
    if ((size_t)ndefMessage.messageSize + 3 > sizeof(ndefPayload)) return FAILURE;

    ndefPayload[idx++] = ndefMessage.begin;
    ndefPayload[idx++] = ndefMessage.messageSize;
    ndefPayload[idx++] = ndefMessage.header;
    ndefPayload[idx++] = ndefMessage.tnf;
    ndefPayload[idx++] = ndefMessage.payloadSize;
    ndefPayload[idx++] = ndefMessage.payloadType;

    for (uint8_t i = 0; i < ndefMessage.payloadSize && idx < sizeof(ndefPayload); i++) {
        ndefPayload[idx++] = ndefMessage.payload[i];
    }

    if (idx >= sizeof(ndefPayload)) return FAILURE;
    ndefPayload[idx++] = ndefMessage.end;

    int maxBytes = (totalPages > 0 ? (totalPages - 5 - 4) : 36) * 4;
    if (maxBytes <= 0 || (int)idx > maxBytes) {
        ST25R_LOG("_writeNdefBlocks: payload too large len=%d max=%d", (int)idx, maxBytes);
        return FAILURE;
    }

    int pagesWritten = 0;
    for (int page = 4; _isUltralightUserPage(page); page++) {
        int base = (page - 4) * 4;
        if (base >= (int)idx) break;

        uint8_t data[4] = {0x00, 0x00, 0x00, 0x00};
        for (int b = 0; b < 4 && base + b < (int)idx; b++) data[b] = ndefPayload[base + b];

        if (!_writeT2TPage((uint8_t)page, data)) { return FAILURE; }
        pagesWritten++;
    }

    uint8_t zeros[4] = {0x00, 0x00, 0x00, 0x00};
    for (int page = 4 + pagesWritten; _isUltralightUserPage(page); page++) {
        if (!_writeT2TPage((uint8_t)page, zeros)) { return FAILURE; }
    }

    ST25R_LOG("_writeNdefBlocks: wrote %d bytes", (int)idx);
    return SUCCESS;
}

bool ST25R3916::_writeMagicGen2UID(rfalNfcDevice *dev) {
    if (uid.size == 7) {
        uint8_t bcc0 = 0x88 ^ uid.uidByte[0] ^ uid.uidByte[1] ^ uid.uidByte[2];
        uint8_t bcc1 = uid.uidByte[3] ^ uid.uidByte[4] ^ uid.uidByte[5] ^ uid.uidByte[6];
        uint8_t page0[4] = {uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], bcc0};
        uint8_t page1[4] = {uid.uidByte[3], uid.uidByte[4], uid.uidByte[5], uid.uidByte[6]};
        uint8_t page2[4] = {bcc1, 0x00, 0x00, 0x00};
        if (_writeT2TPage(0, page0) && _writeT2TPage(1, page1) && _writeT2TPage(2, page2)) {
            ST25R_LOG("_writeMagicGen2UID: 7-byte UID written to pages 0-2");
            return true;
        }
    } else if (uid.size == 4) {
        uint8_t bcc = uid.uidByte[0] ^ uid.uidByte[1] ^ uid.uidByte[2] ^ uid.uidByte[3];
        uint8_t page0[4] = {uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], uid.uidByte[3]};
        uint8_t page1[4] = {bcc, uid.sak, uid.atqaByte[1], uid.atqaByte[0]};
        if (_writeT2TPage(0, page0) && _writeT2TPage(1, page1)) {
            ST25R_LOG("_writeMagicGen2UID: 4-byte UID written to pages 0-1");
            return true;
        }
    }
    return false;
}

bool ST25R3916::_writeMagicGen1UID(rfalNfcDevice *dev) {
    // Magic Gen1 backdoor: send 0x40 (7 bits, no CRC, no parity), then 0x43
    uint8_t cmd40 = 0x40;
    uint8_t rxBuf[4] = {0};
    uint16_t actLen = 0;

    rfalTransceiveContext ctx;
    ctx.txBuf = &cmd40;
    ctx.txBufLen = 7;
    ctx.rxBuf = rxBuf;
    ctx.rxBufLen = 32;
    ctx.rxRcvdLen = &actLen;
    ctx.flags = (uint32_t)RFAL_TXRX_FLAGS_CRC_TX_MANUAL | (uint32_t)RFAL_TXRX_FLAGS_PAR_TX_NONE |
                (uint32_t)RFAL_TXRX_FLAGS_CRC_RX_KEEP | (uint32_t)RFAL_TXRX_FLAGS_PAR_RX_KEEP;
    ctx.fwt = rfalConv64fcTo1fc(5000);

    if (_hw->rfalStartTransceive(&ctx) != ST_ERR_NONE) return false;
    uint32_t t0 = millis();
    while (millis() - t0 < 50) {
        _hw->rfalWorker();
        if (_hw->rfalGetTransceiveState() == RFAL_TXRX_STATE_IDLE) break;
    }
    if (_hw->rfalGetTransceiveStatus() != ST_ERR_NONE || rxBuf[0] != 0x0A) {
        ST25R_LOG("_writeMagicGen1UID: 0x40 no ACK (got 0x%02X)", rxBuf[0]);
        return false;
    }

    uint8_t cmd43 = 0x43;
    memset(rxBuf, 0, sizeof(rxBuf));
    actLen = 0;
    ctx.txBuf = &cmd43;
    ctx.txBufLen = 8;
    ctx.rxBuf = rxBuf;
    ctx.rxBufLen = 32;
    ctx.rxRcvdLen = &actLen;
    ctx.flags = (uint32_t)RFAL_TXRX_FLAGS_CRC_TX_MANUAL | (uint32_t)RFAL_TXRX_FLAGS_PAR_TX_NONE |
                (uint32_t)RFAL_TXRX_FLAGS_CRC_RX_KEEP | (uint32_t)RFAL_TXRX_FLAGS_PAR_RX_KEEP;
    ctx.fwt = rfalConv64fcTo1fc(5000);

    if (_hw->rfalStartTransceive(&ctx) != ST_ERR_NONE) return false;
    t0 = millis();
    while (millis() - t0 < 50) {
        _hw->rfalWorker();
        if (_hw->rfalGetTransceiveState() == RFAL_TXRX_STATE_IDLE) break;
    }
    if (_hw->rfalGetTransceiveStatus() != ST_ERR_NONE || rxBuf[0] != 0x0A) {
        ST25R_LOG("_writeMagicGen1UID: 0x43 no ACK (got 0x%02X)", rxBuf[0]);
        return false;
    }

    // Build block 0: UID (4 bytes) + BCC + SAK + ATQA (2) + filler (8)
    uint8_t block0[16] = {0};
    uint8_t bcc = 0;
    int uidLen = uid.size < 4 ? uid.size : 4;
    for (int i = 0; i < uidLen; i++) {
        block0[i] = uid.uidByte[i];
        bcc ^= uid.uidByte[i];
    }
    block0[4] = bcc;
    block0[5] = uid.sak;
    block0[6] = uid.atqaByte[1];
    block0[7] = uid.atqaByte[0];

    // Send MIFARE WRITE block 0 command (with CRC)
    uint8_t writeCmd[2] = {0xA0, 0x00};
    memset(rxBuf, 0, sizeof(rxBuf));
    actLen = 0;
    auto err = _hw->rfalTransceiveBlockingTxRx(
        writeCmd, sizeof(writeCmd), rxBuf, sizeof(rxBuf), &actLen, RFAL_TXRX_FLAGS_DEFAULT, RFAL_FWT_NONE
    );
    if (err != ST_ERR_NONE || rxBuf[0] != 0x0A) {
        ST25R_LOG("_writeMagicGen1UID: WRITE cmd no ACK err=%d", (int)err);
        return false;
    }

    // Send block data (with CRC)
    memset(rxBuf, 0, sizeof(rxBuf));
    actLen = 0;
    err = _hw->rfalTransceiveBlockingTxRx(
        block0, sizeof(block0), rxBuf, sizeof(rxBuf), &actLen, RFAL_TXRX_FLAGS_DEFAULT, RFAL_FWT_NONE
    );
    if (err != ST_ERR_NONE) {
        ST25R_LOG("_writeMagicGen1UID: data send failed err=%d", (int)err);
        return false;
    }

    ST25R_LOG("_writeMagicGen1UID: block 0 written successfully");
    return true;
}

int ST25R3916::write(int cardBaudRate) {
    if (!_nfc) return FAILURE;
    _deselectSharedSpiDevices();

    rfalNfcDevice *dev;
    if (!_pollForTag(&dev)) return TAG_NOT_PRESENT;

    if (dev->type != RFAL_NFC_LISTEN_TYPE_NFCA) {
        _nfc->rfalNfcDeactivate(false);
        return TAG_NOT_MATCH;
    }

    // MIFARE Classic: write the loaded dump back into a genuine MIFARE Classic card.
    if (mfcLoaded && isMifareClassicSak(dev->dev.nfca.selRes.sak)) {
        _parseDevice(dev);
        return _writeMifareClassic(dev);
    }

    bool storedIsUltralight = printableUID.picc_type.indexOf("Ultralight") >= 0;
    bool presentIsUltralight = (dev->dev.nfca.selRes.sak == 0x00);

    if (!storedIsUltralight || !presentIsUltralight) {
        _nfc->rfalNfcDeactivate(false);
        return TAG_NOT_MATCH;
    }

    return _writeUltralight(dev);
}

int ST25R3916::clone() {
    if (!_nfc) return FAILURE;
    _deselectSharedSpiDevices();

    rfalNfcDevice *dev = nullptr;
    if (!_pollForTag(&dev)) return TAG_NOT_PRESENT;

    if (dev->type != RFAL_NFC_LISTEN_TYPE_NFCA) {
        _nfc->rfalNfcDeactivate(false);
        return TAG_NOT_MATCH;
    }

    // MIFARE Classic dump → clone full content into a Magic Gen1 card.
    if (mfcLoaded) {
        int r = _writeMifareClassicMagic(dev);
        _nfc->rfalNfcDeactivate(false);
        return r;
    }

    if (_writeMagicGen2UID(dev)) {
        _nfc->rfalNfcDeactivate(false);
        return SUCCESS;
    }

    _nfc->rfalNfcDeactivate(false);
    delay(50);

    dev = nullptr;
    if (!_pollForTag(&dev)) return TAG_NOT_PRESENT;

    if (dev->type != RFAL_NFC_LISTEN_TYPE_NFCA) {
        _nfc->rfalNfcDeactivate(false);
        return TAG_NOT_MATCH;
    }

    if (_writeMagicGen1UID(dev)) {
        _nfc->rfalNfcDeactivate(false);
        return SUCCESS;
    }

    _nfc->rfalNfcDeactivate(false);
    return FAILURE;
}

int ST25R3916::erase() {
    if (!_nfc) return FAILURE;
    _deselectSharedSpiDevices();

    rfalNfcDevice *dev;
    if (!_pollForTag(&dev)) return TAG_NOT_PRESENT;

    if (dev->type == RFAL_NFC_LISTEN_TYPE_NFCA && dev->dev.nfca.selRes.sak == 0x00) {
        return _eraseUltralight(dev);
    }

    _nfc->rfalNfcDeactivate(false);
    return NOT_IMPLEMENTED;
}

int ST25R3916::write_ndef() {
    if (!_nfc) return FAILURE;
    _deselectSharedSpiDevices();

    rfalNfcDevice *dev;
    if (!_pollForTag(&dev)) return TAG_NOT_PRESENT;

    if (dev->type != RFAL_NFC_LISTEN_TYPE_NFCA || dev->dev.nfca.selRes.sak != 0x00) {
        _nfc->rfalNfcDeactivate(false);
        return TAG_NOT_MATCH;
    }

    if (totalPages <= 0) {
        _parseDevice(dev);
        if (_readDataBlocks(dev) != SUCCESS) {
            _nfc->rfalNfcDeactivate(false);
            return FAILURE;
        }
    }

    int result = _writeNdefBlocks(dev);
    _nfc->rfalNfcDeactivate(false);
    return result;
}

// ---------------------------------------------------------------------------
// Emulação de tag NFC-A (modo passive target / listen)
//
// O wrapper RFAL de alto nível (rfalListenStart) é stub (ST_ERR_NOTSUPP) neste
// fork. Porém o ST25R3916 faz a anti-colisão NFC-A em hardware no modo
// "Passive Target" (MODE.targ + om_targ_nfca): basta carregar UID/ATQA/SAK na
// PT Memory e o chip responde sozinho SENS_RES/anticolisão/SELECT. A sequência
// de registradores abaixo segue o furi_hal_nfc do Flipper (mesmo CI).
// ---------------------------------------------------------------------------

// Converte strAllPages ("Page N: XX XX XX XX") em _emuPages. Retorna nº de páginas.
int ST25R3916::_buildEmuPages() {
    _emuPageCount = 0;
    memset(_emuPages, 0, sizeof(_emuPages));
    int pos = 0;
    while (pos < strAllPages.length() && _emuPageCount < 256) {
        int nl = strAllPages.indexOf('\n', pos);
        if (nl < 0) nl = strAllPages.length();
        String line = strAllPages.substring(pos, nl);
        line.trim();
        pos = nl + 1;
        if (!line.startsWith("Page ")) continue;
        int colon = line.indexOf(':');
        if (colon < 0) continue;
        int page = line.substring(5, colon).toInt();
        if (page < 0 || page >= 256) continue;
        std::vector<uint8_t> bytes;
        if (!st25ParseHexBytesAfterColon(line, bytes) || bytes.size() < 4) continue;
        memcpy(_emuPages[page], bytes.data(), 4);
        if (page + 1 > _emuPageCount) _emuPageCount = page + 1;
    }
    return _emuPageCount;
}

bool ST25R3916::_setupListenMode(const uint8_t *uidBuf, uint8_t uidLen, const uint8_t *atqa, uint8_t sak) {
    _deselectSharedSpiDevices();
    _hw->st25r3916OscOn();

    // Configura modo + front-end analógico para listen NFC-A (caminho testado do fork).
    auto mres = _hw->rfalSetMode(RFAL_MODE_LISTEN_NFCA, RFAL_BR_106, RFAL_BR_106);
    if (mres != ST_ERR_NONE) {
        ST25R_LOG("emulate: rfalSetMode(LISTEN_NFCA) -> %d", (int)mres);
        return false;
    }

    // Passive target: receptor ligado, detector de campo externo automático (sem TX próprio).
    _hw->st25r3916WriteRegister(
        ST25R3916_REG_OP_CONTROL,
        ST25R3916_REG_OP_CONTROL_en | ST25R3916_REG_OP_CONTROL_rx_en | ST25R3916_REG_OP_CONTROL_en_fd_auto_efd
    );
    _hw->st25r3916WriteRegister(ST25R3916_REG_MODE, ST25R3916_REG_MODE_targ_targ | ST25R3916_REG_MODE_om0);
    _hw->st25r3916WriteRegister(
        ST25R3916_REG_PASSIVE_TARGET,
        ST25R3916_REG_PASSIVE_TARGET_fdel_2 | ST25R3916_REG_PASSIVE_TARGET_fdel_0 |
            ST25R3916_REG_PASSIVE_TARGET_d_ac_ap2p | ST25R3916_REG_PASSIVE_TARGET_d_212_424_1r
    );
    _hw->st25r3916WriteRegister(ST25R3916_REG_MASK_RX_TIMER, 0x02);
    _hw->st25r3916ExecuteCommand(ST25R3916_CMD_STOP);

    // Habilita as interrupções relevantes ao modo target.
    uint32_t interrupts = ST25R3916_IRQ_MASK_FWL | ST25R3916_IRQ_MASK_TXE | ST25R3916_IRQ_MASK_RXS |
                          ST25R3916_IRQ_MASK_RXE | ST25R3916_IRQ_MASK_PAR | ST25R3916_IRQ_MASK_CRC |
                          ST25R3916_IRQ_MASK_ERR1 | ST25R3916_IRQ_MASK_ERR2 | ST25R3916_IRQ_MASK_NRE |
                          ST25R3916_IRQ_MASK_EON | ST25R3916_IRQ_MASK_EOF | ST25R3916_IRQ_MASK_WU_A_X |
                          ST25R3916_IRQ_MASK_WU_A;
    _hw->st25r3916ClearInterrupts();
    _hw->st25r3916DisableInterrupts(ST25R3916_IRQ_MASK_ALL);
    _hw->st25r3916EnableInterrupts(interrupts);

    // UID de 4 ou 7 bytes.
    _hw->st25r3916ChangeRegisterBits(
        ST25R3916_REG_AUX,
        ST25R3916_REG_AUX_nfc_id_mask,
        (uidLen == 4) ? ST25R3916_REG_AUX_nfc_id_4bytes : ST25R3916_REG_AUX_nfc_id_7bytes
    );

    // PT Memory A (15 bytes): UID[0..len], ATQA em [10..11], SAK em [12..14].
    uint8_t pt[ST25R3916_PTM_A_LEN] = {0};
    memcpy(pt, uidBuf, uidLen);
    pt[10] = atqa[0];
    pt[11] = atqa[1];
    pt[12] = (uidLen == 4) ? (uint8_t)(sak & ~0x04) : 0x04; // cascade level 1
    pt[13] = (uint8_t)(sak & ~0x04);
    pt[14] = (uint8_t)(sak & ~0x04);
    _hw->st25r3916WritePTMem(pt, sizeof(pt));

    // Habilita anti-colisão automática (bit limpo) e entra no estado Sense.
    _hw->st25r3916ClrRegisterBits(ST25R3916_REG_PASSIVE_TARGET, ST25R3916_REG_PASSIVE_TARGET_d_106_ac_a);
    _hw->st25r3916ExecuteCommand(ST25R3916_CMD_GOTO_SENSE);
    return true;
}

void ST25R3916::_listenStop() {
    if (!_hw) return;
    _hw->st25r3916ExecuteCommand(ST25R3916_CMD_STOP);
    _hw->st25r3916DisableInterrupts(ST25R3916_IRQ_MASK_ALL);
    _hw->st25r3916ClearInterrupts();
    // Volta ao modo poller padrão para que rfid read funcione em seguida.
    _hw->rfalSetMode(RFAL_MODE_POLL_NFCA, RFAL_BR_106, RFAL_BR_106);
    _hw->rfalFieldOff();
    _discoveryStarted = false;
}

// Transmite uma resposta (com CRC apêndice automático) via FIFO no modo target.
bool ST25R3916::_listenRespond(const uint8_t *resp, uint16_t len) {
    _hw->st25r3916ExecuteCommand(ST25R3916_CMD_CLEAR_FIFO);
    _hw->st25r3916WriteFifo(resp, len);
    _hw->st25r3916SetNumTxBits((uint16_t)(len * 8U));
    _hw->st25r3916ExecuteCommand(ST25R3916_CMD_TRANSMIT_WITH_CRC);
    uint32_t irqs = _hw->st25r3916WaitForInterruptsTimed(ST25R3916_IRQ_MASK_TXE, 20);
    return (irqs & ST25R3916_IRQ_MASK_TXE) != 0U;
}

// Loop principal: o hardware responde anti-colisão/SELECT sozinho; aqui apenas
// detectamos seleções (WU_A) e respondemos comandos T2T (READ/FAST_READ/GET_VERSION).
int ST25R3916::_handleListenLoop(uint32_t timeoutMs) {
    uint32_t deadline = millis() + timeoutMs;
    int readers = 0;
    uint32_t nRead = 0;
    bool active = false;
    uint8_t fifo[64];
    // Trace diferido: registra a sequência de comandos e imprime só quando o leitor
    // sai do campo, p/ não atrasar o caminho RX->resposta (timing do Switch é estrito).
    uint8_t traceCmd[96];
    uint8_t traceArg[96];
    uint16_t traceN = 0;
    LongPress = true;
    while ((int32_t)(deadline - millis()) > 0) {
        // WaitForInterruptsTimed faz busy-wait sem ceder CPU; este yield dá tempo
        // ao task de input (xHandle) para setar EscPress, senão não dá p/ sair.
        vTaskDelay(pdMS_TO_TICKS(1)); // time to pull EscPress from other tasks
        if (check(EscPress)) {
            ST25R_LOG("emulate: Esc — encerrando");
            break;
        }

        uint32_t irqs = _hw->st25r3916WaitForInterruptsTimed(
            ST25R3916_IRQ_MASK_WU_A | ST25R3916_IRQ_MASK_WU_A_X | ST25R3916_IRQ_MASK_RXE |
                ST25R3916_IRQ_MASK_EOF,
            50
        );
        if (irqs == 0U) continue;

        if ((irqs & (ST25R3916_IRQ_MASK_WU_A | ST25R3916_IRQ_MASK_WU_A_X)) != 0U) {
            if (!active) {
                readers++;
                traceN = 0;
                ST25R_LOG("emulate: reader selecionou a tag (UID enviado) #%d", readers);
            }
            active = true;
            // Passa a tratar dados manualmente (desabilita auto-AC).
            _hw->st25r3916SetRegisterBits(
                ST25R3916_REG_PASSIVE_TARGET, ST25R3916_REG_PASSIVE_TARGET_d_106_ac_a
            );
        }

        if ((irqs & ST25R3916_IRQ_MASK_RXE) != 0U) {
            uint16_t n = _hw->st25r3916GetNumFIFOBytes();
            if (_emuIsMfc || _emuIsT4T) {
                // MIFARE Classic (Crypto1) or Type 4 Tag (ISO-DEP/NDEF).
                if (n > 0U && n <= sizeof(fifo)) {
                    _hw->st25r3916ReadFifo(fifo, n);
                    if (traceN < sizeof(traceCmd)) {
                        traceCmd[traceN] = fifo[0];
                        traceArg[traceN] = (n > 1) ? fifo[1] : 0;
                        traceN++;
                    }
                    if (fifo[0] == 0x30) nRead++;
                    if (_emuIsMfc) _emuMfcHandle(fifo, n);
                    else _emuT4THandle(fifo, n);
                }
                continue;
            }
            if (n > 0U && n <= sizeof(fifo)) {
                _hw->st25r3916ReadFifo(fifo, n);
                uint8_t cmd = fifo[0];
                // Registra no trace (impressão diferida p/ não atrasar a resposta).
                if (cmd == 0x30 || cmd == 0x3A) nRead++;
                if (traceN < sizeof(traceCmd)) {
                    traceCmd[traceN] = cmd;
                    traceArg[traceN] = (n > 1) ? fifo[1] : 0;
                    traceN++;
                }
                if (cmd == 0x30 && n >= 2) { // READ: 16 bytes (4 páginas a partir de addr)
                    uint8_t addr = fifo[1];
                    uint8_t resp[16];
                    for (int i = 0; i < 4; i++) {
                        int p = (_emuPageCount > 0) ? ((addr + i) % _emuPageCount) : (addr + i);
                        memcpy(&resp[i * 4], _emuPages[p & 0xFF], 4);
                    }
                    _listenRespond(resp, sizeof(resp));
                } else if (cmd == 0x3A && n >= 3) { // FAST_READ start..end (até 64 páginas/256 B)
                    uint8_t start = fifo[1], end = fifo[2];
                    if (end >= start && (uint16_t)((end - start + 1) * 4) <= 256) {
                        uint8_t resp[256];
                        uint16_t rl = 0;
                        for (int p = start; p <= end; p++) {
                            int pp = (_emuPageCount > 0) ? (p % _emuPageCount) : p;
                            memcpy(&resp[rl], _emuPages[pp & 0xFF], 4);
                            rl += 4;
                        }
                        _listenRespond(resp, rl);
                    }
                } else if (cmd == 0x60) { // GET_VERSION (necessário p/ NTAG21x e amiibo)
                    uint8_t ver[8];
                    if (ntagHasVersion) {
                        memcpy(ver, ntagVersion, 8);
                    } else {
                        // Default NTAG21x conforme nº de páginas (0F=213, 11=215, 13=216).
                        uint8_t storage = 0x11; // NTAG215 (caso típico amiibo)
                        if (_emuPageCount > 0 && _emuPageCount <= 45) storage = 0x0F;
                        else if (_emuPageCount > 135) storage = 0x13;
                        uint8_t def[8] = {0x00, 0x04, 0x04, 0x02, 0x01, 0x00, storage, 0x03};
                        memcpy(ver, def, 8);
                    }
                    _listenRespond(ver, 8);
                } else if (cmd == 0x3C && n >= 2) { // READ_SIG: assinatura ECC (amiibo/Switch)
                    uint8_t sig[32];
                    if (ntagHasSignature) memcpy(sig, ntagSignature, 32);
                    else memset(sig, 0, 32);
                    _listenRespond(sig, 32);
                } else if (cmd == 0x39 && n >= 2) { // READ_CNT: contador de 24 bits
                    uint8_t idx = fifo[1];
                    uint8_t cnt[3] = {0, 0, 0};
                    if (idx < 3 && ntagHasCounters) {
                        cnt[0] = (uint8_t)(ntagCounters[idx] & 0xFF);
                        cnt[1] = (uint8_t)((ntagCounters[idx] >> 8) & 0xFF);
                        cnt[2] = (uint8_t)((ntagCounters[idx] >> 16) & 0xFF);
                    }
                    _listenRespond(cnt, 3);
                } else if (cmd == 0x1B && n >= 5) { // PWD_AUTH: responde PACK
                    uint8_t pack[2] = {0, 0};
                    if (_emuPageCount > 0) {
                        pack[0] = _emuPages[_emuPageCount - 1][0];
                        pack[1] = _emuPages[_emuPageCount - 1][1];
                    }
                    // Amiibo (NTAG215): a senha deriva do UID e o PACK correto é 80 80.
                    // Na tag genuína a página PACK é lida como 00 00 (protegida), então
                    // sem isto o Switch recebe PACK errado e recusa antes de ler os dados.
                    if (uid.size == 7) {
                        uint8_t apwd[4] = {
                            (uint8_t)(0xAA ^ uid.uidByte[1] ^ uid.uidByte[3]),
                            (uint8_t)(0x55 ^ uid.uidByte[2] ^ uid.uidByte[4]),
                            (uint8_t)(0xAA ^ uid.uidByte[3] ^ uid.uidByte[5]),
                            (uint8_t)(0x55 ^ uid.uidByte[4] ^ uid.uidByte[6]),
                        };
                        if (fifo[1] == apwd[0] && fifo[2] == apwd[1] && fifo[3] == apwd[2] &&
                            fifo[4] == apwd[3]) {
                            pack[0] = 0x80;
                            pack[1] = 0x80;
                        }
                    }
                    _listenRespond(pack, 2);
                } else if (cmd == 0x50) { // HALT
                    _hw->st25r3916ExecuteCommand(ST25R3916_CMD_GOTO_SLEEP);
                } else if (cmd == 0xA2 && n >= 6) { // WRITE: aceita e guarda, ACK
                    uint8_t addr = fifo[1];
                    if (addr < 256) {
                        memcpy(_emuPages[addr], &fifo[2], 4);
                        if (addr + 1 > _emuPageCount) _emuPageCount = addr + 1;
                    }
                    uint8_t ack = 0x0A; // ACK NTAG (4 bits, sem CRC)
                    _hw->st25r3916ExecuteCommand(ST25R3916_CMD_CLEAR_FIFO);
                    _hw->st25r3916WriteFifo(&ack, 1);
                    _hw->st25r3916SetNumTxBits(4);
                    _hw->st25r3916ExecuteCommand(ST25R3916_CMD_TRANSMIT_WITHOUT_CRC);
                }
            }
        }

        if ((irqs & ST25R3916_IRQ_MASK_EOF) != 0U) {
            // Campo do reader desligou: imprime o trace acumulado e rearma.
            if (active) {
                String tr;
                for (uint16_t i = 0; i < traceN; i++) {
                    char b[8];
                    if (traceCmd[i] == 0x30 || traceCmd[i] == 0x3A)
                        sprintf(b, "%02X:%02X ", traceCmd[i], traceArg[i]);
                    else sprintf(b, "%02X ", traceCmd[i]);
                    tr += b;
                }
                ST25R_LOG(
                    "emulate: reader saiu (cmds=%u reads=%lu): %s", traceN, (unsigned long)nRead, tr.c_str()
                );
                if (_emuIsMfc) {
                    ST25R_LOG(
                        "emu mfc: authReq=%u authOk=%u badAr=%u noNr=%u reads=%u lastNrBits=%u",
                        _emuMfcAuthReq,
                        _emuMfcAuthOk,
                        _emuMfcBadAr,
                        _emuMfcNoNr,
                        _emuMfcReads,
                        _emuMfcLastNrBits
                    );
                    ST25R_LOG(
                        "emu mfc dbg: blk=%u nt=%08lX enc=%02X%02X%02X%02X|%02X%02X%02X%02X arCalc=%08lX "
                        "arExp=%08lX",
                        _emuDbgBlock,
                        (unsigned long)_emuDbgNt,
                        _emuDbgEnc[0],
                        _emuDbgEnc[1],
                        _emuDbgEnc[2],
                        _emuDbgEnc[3],
                        _emuDbgEnc[4],
                        _emuDbgEnc[5],
                        _emuDbgEnc[6],
                        _emuDbgEnc[7],
                        (unsigned long)_emuDbgArCalc,
                        (unsigned long)_emuDbgArExp
                    );
                }
            }
            active = false;
            _hw->st25r3916ClrRegisterBits(
                ST25R3916_REG_PASSIVE_TARGET, ST25R3916_REG_PASSIVE_TARGET_d_106_ac_a
            );
            _hw->st25r3916ExecuteCommand(ST25R3916_CMD_GOTO_SENSE);
        }
    }
    LongPress = false;
    return readers;
}

int ST25R3916::emulate() {
    if (!_hw || !_nfc) return FAILURE;

    // Milestone 6 — choose the emulation technology (forced via CLI or detected).
    bool wantFelica = (emuMode == "felica") || printableUID.picc_type.startsWith("FeliCa");
    if (wantFelica) return _emulateFelica();
    _emuIsT4T = (emuMode == "t4t") || printableUID.picc_type == "ISO14443-4" ||
                printableUID.picc_type.indexOf("ISO14443-4") >= 0;

    // Obtém UID: do struct (após read) ou do printableUID (após load).
    uint8_t uidBuf[7] = {0};
    uint8_t uidLen = 0;
    if (uid.size == 4 || uid.size == 7) {
        uidLen = uid.size;
        memcpy(uidBuf, uid.uidByte, uidLen);
    } else if (printableUID.uid.length() > 0) {
        std::vector<uint8_t> ub;
        int hi = -1;
        for (unsigned i = 0; i < printableUID.uid.length(); i++) {
            int v = st25HexNibble(printableUID.uid.charAt(i));
            if (v < 0) continue;
            if (hi < 0) hi = v;
            else {
                ub.push_back((uint8_t)((hi << 4) | v));
                hi = -1;
            }
        }
        if (ub.size() == 4 || ub.size() == 7) {
            uidLen = ub.size();
            memcpy(uidBuf, ub.data(), uidLen);
        }
    }
    if (uidLen != 4 && uidLen != 7) {
        if (_emuIsT4T) {
            // T4T NDEF without a captured tag: synthesize a random 4-byte UID.
            uidLen = 4;
            uidBuf[0] = 0x08; // RID (random UID) marker
            uint32_t r = esp_random();
            uidBuf[1] = (uint8_t)r;
            uidBuf[2] = (uint8_t)(r >> 8);
            uidBuf[3] = (uint8_t)(r >> 16);
        } else {
            ST25R_LOG("emulate: UID inválido/ausente — faça 'rfid read' ou 'rfid loadfile' antes");
            return FAILURE;
        }
    }

    uint8_t atqa[2];
    atqa[0] = uid.atqaByte[0] ? uid.atqaByte[0] : (uidLen == 7 ? 0x44 : 0x04);
    atqa[1] = uid.atqaByte[1];
    uint8_t sak = uid.sak; // 0x00 = NTAG/Ultralight (T2T)

    // MIFARE Classic? (SAK known or stored type string). Build the block/key dump.
    _emuIsMfc =
        !_emuIsT4T && (isMifareClassicSak(sak) || printableUID.picc_type.indexOf("MIFARE Classic") >= 0 ||
                       printableUID.picc_type.indexOf("Mifare Classic") >= 0);
    if (_emuIsT4T) {
        _buildT4TFiles();
        sak = 0x20; // ISO14443-4 (ISO-DEP / Type 4 Tag)
        if (atqa[0] == 0 && atqa[1] == 0) atqa[0] = 0x04;
        _t4tSelected = 0;
        ST25R_LOG("emulate: Type 4 Tag (NDEF) ndefLen=%u", _t4tNdefLen);
    } else if (_emuIsMfc) {
        if (!_buildEmuMfc()) {
            ST25R_LOG("emulate: MFC dump vazio — faça 'rfid read'/'loadfile' antes");
            return FAILURE;
        }
        if (sak == 0x00) sak = (mfcDump.totalBlocks == 256) ? 0x18 : 0x08;
        if (atqa[0] == 0 && atqa[1] == 0) atqa[0] = 0x04;
        _emuMfcAuthed = false;
        _emuMfcAuthReq = _emuMfcAuthOk = _emuMfcBadAr = _emuMfcNoNr = _emuMfcReads = _emuMfcLastNrBits = 0;
    } else {
        _buildEmuPages();
    }

    // Garante que nenhum discovery/poller esteja ativo antes de virar target.
    stopDiscovery();
    _nfc->rfalNfcDeactivate(false);
    _discoveryStarted = false;

    if (!_setupListenMode(uidBuf, uidLen, atqa, sak)) {
        _listenStop();
        return FAILURE;
    }

    ST25R_LOG(
        "emulate: emulando UID=%s ATQA=%02X%02X SAK=%02X pages=%d ver=%d sig=%d (timeout 30s, Esc p/ sair)",
        printableUID.uid.c_str(),
        atqa[1],
        atqa[0],
        sak,
        _emuPageCount,
        ntagHasVersion ? 1 : 0,
        ntagHasSignature ? 1 : 0
    );

    int readers = _handleListenLoop(30000);

    _listenStop();
    ST25R_LOG("emulate: encerrado — readers detectados=%d", readers);
    return SUCCESS;
}

int ST25R3916::load() {
    // GUI path: pick a file, then delegate to the shared parser so the serial
    // command (rfid loadfile) and the GUI behave identically.
    FS *fs;
    if (!getFsStorage(fs)) return FAILURE;

    String filepath = loopSD(*fs, true, "RFID|NFC", "/BruceRFID");
    if (filepath.length() == 0) return FAILURE;

    return loadFromFile(filepath);
}

int ST25R3916::loadFromFile(const String &filepath) {
    FS *fs;
    if (!getFsStorage(fs)) return FAILURE;

    File file = fs->open(filepath, FILE_READ);
    if (!file) return FAILURE;

    String line;
    String strData;
    strAllPages = "";
    totalPages = 0;
    dataPages = 0;
    pageReadSuccess = true;
    pageReadStatus = SUCCESS;
    ntagHasVersion = false;
    ntagHasSignature = false;
    ntagHasCounters = false;

    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();
        strData = line.substring(line.indexOf(":") + 1);
        strData.trim();

        if (line.startsWith("Device type:")) printableUID.picc_type = strData;
        else if (line.startsWith("UID:")) printableUID.uid = strData;
        else if (line.startsWith("SAK:")) printableUID.sak = strData;
        else if (line.startsWith("ATQA:")) printableUID.atqa = strData;
        else if (line.startsWith("Pages total:")) totalPages = strData.toInt();
        else if (line.startsWith("Pages read:")) pageReadSuccess = false;
        // NTAG/Ultralight: restaura versão/assinatura/contadores p/ emulação fiel.
        else if (line.startsWith("Mifare version:")) {
            std::vector<uint8_t> b;
            if (st25ParseHexBytesAfterColon(line, b) && b.size() >= 8) {
                memcpy(ntagVersion, b.data(), 8);
                ntagHasVersion = true;
            }
        } else if (line.startsWith("Signature:")) {
            std::vector<uint8_t> b;
            if (st25ParseHexBytesAfterColon(line, b) && b.size() >= 32) {
                memcpy(ntagSignature, b.data(), 32);
                ntagHasSignature = true;
            }
        } else if (line.startsWith("Counter ")) {
            int idx = line.substring(8, line.indexOf(":")).toInt();
            if (idx >= 0 && idx < 3) {
                ntagCounters[idx] = (uint32_t)strData.toInt();
                ntagHasCounters = true;
            }
        } else if (line.startsWith("Page ")) {
            strAllPages += line + "\n";
            dataPages++;
        }
        // MIFARE Classic dumps (.rfid/.nfc): keep Block and Key lines so the
        // driver can rebuild the dump for clone/emulate (mesma lógica do loadfile serial).
        else if (line.startsWith("Block ")) {
            strAllPages += line + "\n";
            dataPages++;
        } else if (line.startsWith("Key A sector ") || line.startsWith("Key B sector ")) {
            strAllPages += line + "\n";
        }
    }

    file.close();

    if (totalPages == 0) totalPages = dataPages;
    if (printableUID.uid.length() == 0) return FAILURE;

    _parseLoadedData();
    return SUCCESS;
}

String ST25R3916::_getNtagVariant() {
    ntagHasVersion = false;
    memset(ntagVersion, 0, sizeof(ntagVersion));

    uint8_t cmd = 0x60;
    uint8_t rx[10] = {0};
    uint16_t rxLen = 0;
    auto err = _hw->rfalTransceiveBlockingTxRx(
        &cmd, 1, rx, sizeof(rx), &rxLen, RFAL_TXRX_FLAGS_DEFAULT, rfalConvMsTo1fc(20)
    );
    if (err != ST_ERR_NONE || rxLen < 8) {
        ST25R_LOG("_getNtagVariant: GET_VERSION failed err=%d len=%u", (int)err, rxLen);
        return "";
    }
    memcpy(ntagVersion, rx, 8);
    ntagHasVersion = true;
    ST25R_LOG(
        "version=%02X %02X %02X %02X %02X %02X %02X %02X",
        rx[0],
        rx[1],
        rx[2],
        rx[3],
        rx[4],
        rx[5],
        rx[6],
        rx[7]
    );

    uint8_t productType = rx[2]; // 0x03 = Ultralight, 0x04 = NTAG
    uint8_t storage = rx[6];     // storage size code
    if (productType == 0x04) {
        switch (storage) {
            case 0x0F: _ntagPagesHint = 45; return "NTAG213";
            case 0x11: _ntagPagesHint = 135; return "NTAG215";
            case 0x13: _ntagPagesHint = 231; return "NTAG216";
            default: return "NTAG21x";
        }
    }
    if (productType == 0x03) {
        switch (storage) {
            case 0x0B: _ntagPagesHint = 20; return "MF Ultralight EV1 (UL11)";
            case 0x0E: _ntagPagesHint = 41; return "MF Ultralight EV1 (UL21)";
            default: return "MF Ultralight EV1";
        }
    }
    return "";
}

bool ST25R3916::_readNtagSignature() {
    ntagHasSignature = false;
    memset(ntagSignature, 0, sizeof(ntagSignature));

    uint8_t cmd[2] = {0x3C, 0x00};
    uint8_t rx[40] = {0};
    uint16_t rxLen = 0;
    auto err = _hw->rfalTransceiveBlockingTxRx(
        cmd, sizeof(cmd), rx, sizeof(rx), &rxLen, RFAL_TXRX_FLAGS_DEFAULT, rfalConvMsTo1fc(20)
    );
    if (err != ST_ERR_NONE || rxLen < 32) {
        ST25R_LOG("_readNtagSignature: READ_SIG failed err=%d len=%u", (int)err, rxLen);
        return false;
    }
    memcpy(ntagSignature, rx, 32);
    ntagHasSignature = true;
    ST25R_LOG("signature=%02X %02X %02X %02X...%02X %02X", rx[0], rx[1], rx[2], rx[3], rx[30], rx[31]);
    return true;
}

bool ST25R3916::_readNtagCounters() {
    ntagHasCounters = false;
    bool any = false;
    for (uint8_t idx = 0; idx < 3; idx++) {
        ntagCounters[idx] = 0;
        ntagTearing[idx] = 0xBD;

        uint8_t cmd[2] = {0x39, idx};
        uint8_t rx[4] = {0};
        uint16_t rxLen = 0;
        auto err = _hw->rfalTransceiveBlockingTxRx(
            cmd, sizeof(cmd), rx, sizeof(rx), &rxLen, RFAL_TXRX_FLAGS_DEFAULT, rfalConvMsTo1fc(20)
        );
        if (err == ST_ERR_NONE && rxLen >= 3) {
            ntagCounters[idx] = (uint32_t)rx[0] | ((uint32_t)rx[1] << 8) | ((uint32_t)rx[2] << 16);
            any = true;
            ST25R_LOG("counter[%u]=%lu", idx, (unsigned long)ntagCounters[idx]);
        }
    }
    ntagHasCounters = any;
    return any;
}

static String _bytesToHex(const uint8_t *data, size_t len);

int ST25R3916::_readNfcV(rfalNfcDevice *dev) {
    strAllPages = "";
    dataPages = 0;
    totalPages = 0;

    const uint8_t *nfcvUid = dev->nfcid;
    uint8_t flags = RFAL_NFCV_REQ_FLAG_DEFAULT;

    uint8_t blockCount = 0;
    uint8_t blockSize = 4;
    {
        uint8_t sysInfo[32] = {0};
        uint16_t rcvLen = 0;
        auto err =
            _nfc->rfalNfcvPollerGetSystemInformation(flags, nfcvUid, sysInfo, sizeof(sysInfo), &rcvLen);
        if (err == ST_ERR_NONE && rcvLen >= 10) {
            // Layout ISO15693: RES_FLAG(1) InfoFlags(1) UID(8) [DSFID] [AFI] [MemSize(2)] [ICref]
            uint8_t infoFlags = sysInfo[1];
            size_t idx = 10;
            if (infoFlags & 0x01) idx += 1; // DSFID presente
            if (infoFlags & 0x02) idx += 1; // AFI presente
            if ((infoFlags & 0x04) && (idx + 1 < rcvLen)) {
                blockCount = sysInfo[idx] + 1;             // nº de blocos (valor+1)
                blockSize = (sysInfo[idx + 1] & 0x1F) + 1; // tamanho do bloco (valor+1)
            }
            ST25R_LOG("NFC-V SysInfo flags=0x%02X blocks=%u size=%u", infoFlags, blockCount, blockSize);
        } else {
            ST25R_LOG("NFC-V GetSystemInformation err=%d len=%u (lendo até falhar)", (int)err, rcvLen);
        }
    }

    if (blockSize == 0 || blockSize > 8) blockSize = 4;
    uint8_t maxBlocks = (blockCount > 0) ? blockCount : 255;

    for (uint16_t blk = 0; blk < maxBlocks; blk++) {
        uint8_t rxBuf[16] = {0};
        uint16_t rxLen = 0;
        auto err =
            _nfc->rfalNfcvPollerReadSingleBlock(flags, nfcvUid, (uint8_t)blk, rxBuf, sizeof(rxBuf), &rxLen);
        if (err != ST_ERR_NONE || rxLen < (uint16_t)(blockSize + 1)) {
            if (blockCount == 0) break; // tamanho desconhecido: parar no 1º erro
            // tamanho conhecido: bloco ilegível, registra como zeros e segue
            char line[64];
            sprintf(line, "Block %02u: (unreadable err=%d)", blk, (int)err);
            strAllPages += String(line) + "\n";
            continue;
        }
        // rxBuf[0] = RES_FLAG; dados a partir de rxBuf[1]
        String line = "Block ";
        char idxBuf[8];
        sprintf(idxBuf, "%02u: ", blk);
        line += idxBuf;
        for (uint8_t b = 0; b < blockSize; b++) {
            char hb[4];
            sprintf(hb, "%02X", rxBuf[1 + b]);
            line += hb;
            if (b < blockSize - 1) line += " ";
        }
        strAllPages += line + "\n";
        dataPages++;
    }

    totalPages = (blockCount > 0) ? blockCount : dataPages;
    ST25R_LOG("NFC-V read: %d/%d blocos lidos", dataPages, totalPages);

    if (dataPages == 0) {
        pageReadStatus = FAILURE;
        return FAILURE;
    }
    pageReadStatus = SUCCESS;
    pageReadSuccess = true;
    return SUCCESS;
}

void ST25R3916::_parseNfcB(rfalNfcDevice *dev) {
    rfalNfcbListenDevice *nfcb = &dev->dev.nfcb;
    const uint8_t *pupi = nfcb->sensbRes.nfcid0; // 4 bytes (PUPI / pseudo-UID)

    printableUID.picc_type = "ISO14443B";
    printableUID.sak = "--";
    printableUID.atqa = "--";

    printableUID.uid = "";
    for (int i = 0; i < RFAL_NFCB_NFCID0_LEN; i++) {
        char buf[3];
        sprintf(buf, "%02X", pupi[i]);
        printableUID.uid += buf;
        if (i < RFAL_NFCB_NFCID0_LEN - 1) printableUID.uid += " ";
    }

    const uint8_t *appData = (const uint8_t *)&nfcb->sensbRes.appData;
    const uint8_t *protInfo = (const uint8_t *)&nfcb->sensbRes.protInfo;

    strAllPages = "";
    strAllPages += "PUPI: " + printableUID.uid + "\n";
    strAllPages += "Application data: " + _bytesToHex(appData, 4) + "\n";
    strAllPages += "Protocol info: " + _bytesToHex(protInfo, 4) + "\n";

    ST25R_LOG(
        "NFC-B PUPI=%s appData=%02X%02X%02X%02X protInfo=%02X%02X%02X%02X",
        printableUID.uid.c_str(),
        appData[0],
        appData[1],
        appData[2],
        appData[3],
        protInfo[0],
        protInfo[1],
        protInfo[2],
        protInfo[3]
    );
}

int ST25R3916::_readFeliCa(rfalNfcDevice *dev) {
    rfalNfcfListenDevice *nfcf = &dev->dev.nfcf;
    const uint8_t *idm = nfcf->sensfRes.NFCID2;                // 8 bytes (IDm)
    const uint8_t *pmm = (const uint8_t *)nfcf->sensfRes.PAD0; // 8 bytes contíguos (PMm)

    printableUID.picc_type = "FeliCa";
    printableUID.sak = "--";
    printableUID.atqa = "--";

    printableUID.uid = "";
    for (int i = 0; i < RFAL_NFCF_NFCID2_LEN; i++) {
        char buf[3];
        sprintf(buf, "%02X", idm[i]);
        printableUID.uid += buf;
        if (i < RFAL_NFCF_NFCID2_LEN - 1) printableUID.uid += " ";
    }

    strAllPages = "";
    strAllPages += "IDm: " + _bytesToHex(idm, 8) + "\n";
    strAllPages += "PMm: " + _bytesToHex(pmm, 8) + "\n";

    ST25R_LOG("FeliCa IDm=%s PMm=%s", _bytesToHex(idm, 8).c_str(), _bytesToHex(pmm, 8).c_str());

    rfalNfcfServ service = 0x000B;
    rfalNfcfBlockListElem block;
    block.conf = 0x80; // 2-byte block list element, acesso normal
    block.blockNum = 0;
    rfalNfcfServBlockListParam servBlock;
    servBlock.numServ = 1;
    servBlock.servList = &service;
    servBlock.numBlock = 1;
    servBlock.blockList = &block;

    uint8_t rxBuf[32] = {0};
    uint16_t rcvdLen = 0;
    auto err = _nfc->rfalNfcfPollerCheck(idm, &servBlock, rxBuf, sizeof(rxBuf), &rcvdLen);
    if (err == ST_ERR_NONE && rcvdLen >= 16) {
        // resposta T3T Check: status flags + dados do bloco (16 bytes)
        strAllPages += "Block 00: " + _bytesToHex(rxBuf + (rcvdLen - 16), 16) + "\n";
        ST25R_LOG("FeliCa block0 lido (%u bytes)", rcvdLen);
        dataPages = 1;
    } else {
        ST25R_LOG("FeliCa Check err=%d len=%u (serviço pode exigir chave)", (int)err, rcvdLen);
        dataPages = 0;
    }

    totalPages = dataPages;
    pageReadStatus = SUCCESS;
    pageReadSuccess = true;
    return SUCCESS;
}

// ============================================================================
// ISO-DEP / Type 4 Tag (T4T): DESFire, NDEF T4T, EMV
// ============================================================================

bool ST25R3916::_isoDepApdu(const uint8_t *tx, uint16_t txLen, uint8_t *rx, uint16_t rxCap, uint16_t *rxLen) {
    if (rxLen) *rxLen = 0;
    uint8_t *rxData = nullptr;
    uint16_t *rcvLen = nullptr;
    auto err =
        _nfc->rfalNfcDataExchangeStart(const_cast<uint8_t *>(tx), txLen, &rxData, &rcvLen, RFAL_FWT_NONE);
    if (err != ST_ERR_NONE) {
        ST25R_LOG("isoDep APDU start err=%d", (int)err);
        return false;
    }

    uint16_t total = 0;
    uint32_t t0 = millis();
    for (;;) {
        _nfc->rfalNfcWorker();
        err = _nfc->rfalNfcDataExchangeGetStatus();
        if (err == ST_ERR_BUSY) {
            if (millis() - t0 > 2000) {
                ST25R_LOG("isoDep APDU timeout");
                return false;
            }
            continue;
        }
        // Copia o bloco recebido (NONE = último; AGAIN = chaining, virão mais).
        uint16_t n = (rcvLen != nullptr) ? *rcvLen : 0;
        if (rxData != nullptr && rx != nullptr && n > 0 && total < rxCap) {
            uint16_t cp = (total + n > rxCap) ? (uint16_t)(rxCap - total) : n;
            memcpy(rx + total, rxData, cp);
            total += cp;
        }
        if (err == ST_ERR_AGAIN) {
            t0 = millis();
            continue;
        }
        if (err != ST_ERR_NONE) {
            ST25R_LOG("isoDep APDU status err=%d", (int)err);
            return false;
        }
        break;
    }
    if (rxLen) *rxLen = total;
    return true;
}

// Verifica se a resposta termina com Status Word 0x9000.
static bool _swOk(const uint8_t *rx, uint16_t len) {
    return (len >= 2 && rx[len - 2] == 0x90 && rx[len - 1] == 0x00);
}

// NDEF Type 4 Tag: SELECT NDEF App -> SELECT CC -> READ CC -> SELECT NDEF file -> READ.
bool ST25R3916::_readNdefT4T() {
    uint8_t rx[256];
    uint16_t rxLen = 0;

    // 1. SELECT NDEF Tag Application (AID D2 76 00 00 85 01 01)
    static const uint8_t selApp[] = {
        0x00, 0xA4, 0x04, 0x00, 0x07, 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01, 0x00
    };
    if (!_isoDepApdu(selApp, sizeof(selApp), rx, sizeof(rx), &rxLen) || !_swOk(rx, rxLen)) {
        ST25R_LOG("T4T: SELECT NDEF App falhou");
        return false;
    }

    // 2. SELECT Capability Container (file ID E1 03)
    static const uint8_t selCC[] = {0x00, 0xA4, 0x00, 0x0C, 0x02, 0xE1, 0x03};
    if (!_isoDepApdu(selCC, sizeof(selCC), rx, sizeof(rx), &rxLen) || !_swOk(rx, rxLen)) return false;

    // 3. READ BINARY do CC (15 bytes)
    static const uint8_t readCC[] = {0x00, 0xB0, 0x00, 0x00, 0x0F};
    if (!_isoDepApdu(readCC, sizeof(readCC), rx, sizeof(rx), &rxLen) || !_swOk(rx, rxLen) || rxLen < 15 + 2) {
        return false;
    }
    // CC: [7]=NDEF FileCtrl TLV tag(0x04) [8]=len(0x06) [9-10]=NDEF file ID [11-12]=max NDEF size
    uint16_t ndefFileId = ((uint16_t)rx[9] << 8) | rx[10];
    uint16_t maxNdef = ((uint16_t)rx[11] << 8) | rx[12];

    // 4. SELECT NDEF file
    uint8_t selNdef[] = {
        0x00, 0xA4, 0x00, 0x0C, 0x02, (uint8_t)(ndefFileId >> 8), (uint8_t)(ndefFileId & 0xFF)
    };
    if (!_isoDepApdu(selNdef, sizeof(selNdef), rx, sizeof(rx), &rxLen) || !_swOk(rx, rxLen)) return false;

    // 5. READ NLEN (2 bytes no offset 0)
    static const uint8_t readNlen[] = {0x00, 0xB0, 0x00, 0x00, 0x02};
    if (!_isoDepApdu(readNlen, sizeof(readNlen), rx, sizeof(rx), &rxLen) || rxLen < 4) return false;
    uint16_t ndefLen = ((uint16_t)rx[0] << 8) | rx[1];

    printableUID.picc_type = "NDEF T4T";
    strAllPages = "";
    strAllPages += "NDEF T4T\n";
    char fidLine[40];
    sprintf(fidLine, "NDEF file: %04X (max %u)\n", ndefFileId, maxNdef);
    strAllPages += fidLine;
    strAllPages += "NDEF len: " + String(ndefLen) + "\n";

    // 6. READ BINARY da mensagem NDEF (offset 2). Limita a um único READ.
    if (ndefLen > 0) {
        uint16_t toRead = (ndefLen > 240) ? 240 : ndefLen;
        uint8_t readMsg[] = {0x00, 0xB0, 0x00, 0x02, (uint8_t)toRead};
        if (_isoDepApdu(readMsg, sizeof(readMsg), rx, sizeof(rx), &rxLen) && rxLen >= toRead) {
            strAllPages += "NDEF: " + _bytesToHex(rx, toRead) + "\n";
        }
    }
    dataPages = 1;
    pageReadSuccess = true;
    pageReadStatus = SUCCESS;
    return true;
}

// MIFARE DESFire: GetVersion (0x60) e lista de aplicações (0x6A), via APDU wrapped.
bool ST25R3916::_readDESFireInfo() {
    uint8_t rx[64];
    uint16_t rxLen = 0;

    static const uint8_t getVer[] = {0x90, 0x60, 0x00, 0x00, 0x00};
    if (!_isoDepApdu(getVer, sizeof(getVer), rx, sizeof(rx), &rxLen) || rxLen < 2) return false;
    // DESFire responde SW1=0x91 com SW2 0xAF (additional frame) ou 0x00.
    if (rx[rxLen - 2] != 0x91) {
        ST25R_LOG("DESFire GetVersion SW=%02X%02X (não-DESFire)", rx[rxLen - 2], rx[rxLen - 1]);
        return false;
    }

    printableUID.picc_type = "MIFARE DESFire";
    strAllPages = "";
    strAllPages += "MIFARE DESFire\n";
    if (rxLen >= 7 + 2) {
        // HW info: vendor type subtype major minor storage proto
        strAllPages += "HW: " + _bytesToHex(rx, 7) + "\n";
    }

    // Coleta frames adicionais (SW 91 AF) — limite de segurança de 3 frames.
    int guard = 0;
    while (rxLen >= 2 && rx[rxLen - 2] == 0x91 && rx[rxLen - 1] == 0xAF && guard++ < 3) {
        static const uint8_t more[] = {0x90, 0xAF, 0x00, 0x00, 0x00};
        if (!_isoDepApdu(more, sizeof(more), rx, sizeof(rx), &rxLen) || rxLen < 2) break;
        strAllPages += "Frame: " + _bytesToHex(rx, (uint16_t)(rxLen - 2)) + "\n";
    }

    // GetApplicationIDs (0x6A) — lista AIDs de 3 bytes.
    static const uint8_t getApps[] = {0x90, 0x6A, 0x00, 0x00, 0x00};
    if (_isoDepApdu(getApps, sizeof(getApps), rx, sizeof(rx), &rxLen) && rxLen >= 2 &&
        rx[rxLen - 2] == 0x91) {
        uint16_t n = (uint16_t)(rxLen - 2);
        strAllPages += "AIDs: " + (n > 0 ? _bytesToHex(rx, n) : String("none")) + "\n";
    }

    dataPages = 1;
    pageReadSuccess = true;
    pageReadStatus = SUCCESS;
    return true;
}

// Cartão EMV (pagamento): SELECT PPSE e lista os AIDs públicos (tag 4F). Sem dados sensíveis.
bool ST25R3916::_probeEmv() {
    uint8_t rx[256];
    uint16_t rxLen = 0;

    // SELECT PPSE "2PAY.SYS.DDF01"
    static const uint8_t selPpse[] = {0x00, 0xA4, 0x04, 0x00, 0x0E, 0x32, 0x50, 0x41, 0x59, 0x2E,
                                      0x53, 0x59, 0x53, 0x2E, 0x44, 0x44, 0x46, 0x30, 0x31, 0x00};
    if (!_isoDepApdu(selPpse, sizeof(selPpse), rx, sizeof(rx), &rxLen) || !_swOk(rx, rxLen)) {
        ST25R_LOG("EMV: SELECT PPSE falhou");
        return false;
    }

    printableUID.picc_type = "EMV";
    strAllPages = "";
    strAllPages += "EMV (payment card)\n";
    uint16_t fciLen = (uint16_t)(rxLen - 2);
    strAllPages += "FCI: " + _bytesToHex(rx, fciLen) + "\n";

    // Procura tags 4F (Application Identifier) na resposta FCI.
    bool foundAid = false;
    for (uint16_t i = 0; i + 1 < fciLen; i++) {
        if (rx[i] == 0x4F) {
            uint8_t len = rx[i + 1];
            if (len >= 5 && len <= 16 && (uint16_t)(i + 2 + len) <= fciLen) {
                strAllPages += "AID: " + _bytesToHex(rx + i + 2, len) + "\n";
                foundAid = true;
            }
        }
    }
    if (!foundAid) strAllPages += "AID: (não encontrado no PPSE)\n";

    dataPages = 1;
    pageReadSuccess = true;
    pageReadStatus = SUCCESS;
    return true;
}

// Orquestra a leitura ISO-DEP: ATS + NDEF T4T -> DESFire -> EMV.
int ST25R3916::_readIsoDep(rfalNfcDevice *dev) {
    strAllPages = "";
    dataPages = 0;
    totalPages = 0;

    // Extrai ATS (NFC-A) — útil para identificação. RFAL grava o ATS bruto na struct.
    String atsHex;
    if (dev->type == RFAL_NFC_LISTEN_TYPE_NFCA) {
        uint8_t atsLen = dev->proto.isoDep.activation.A.Listener.ATSLen;
        const uint8_t *ats = (const uint8_t *)&dev->proto.isoDep.activation.A.Listener.ATS;
        if (atsLen > 0 && atsLen <= 20) atsHex = _bytesToHex(ats, atsLen);
        ST25R_LOG("ISO-DEP NFC-A ATS(%u)=%s", atsLen, atsHex.c_str());
    } else {
        atsHex = "(NFC-B ISO-DEP)";
    }

    // Tenta NDEF T4T (mais genérico), depois DESFire, depois EMV.
    bool ok = _readNdefT4T();
    if (!ok) ok = _readDESFireInfo();
    if (!ok) ok = _probeEmv();

    if (!ok) {
        // ISO-DEP ativo mas sem aplicação reconhecida — reporta o que se tem.
        printableUID.picc_type = "ISO14443-4";
        strAllPages = "ISO-DEP activated (no known application)\n";
        ST25R_LOG("ISO-DEP: nenhuma aplicação T4T/DESFire/EMV reconhecida");
    }

    // Prefixa a linha de ATS ao dump.
    if (atsHex.length()) strAllPages = "ATS: " + atsHex + "\n" + strAllPages;

    totalPages = dataPages;
    pageReadStatus = SUCCESS;
    pageReadSuccess = true;
    return SUCCESS;
}

static String _bytesToHex(const uint8_t *data, size_t len) {
    String out;
    char buf[4];
    for (size_t i = 0; i < len; i++) {
        sprintf(buf, "%02X", data[i]);
        out += buf;
        if (i < len - 1) out += " ";
    }
    return out;
}

// dump no formato .nfc do Flipper Zero.
int ST25R3916::saveFlipper(String filename) {
    if (mfcLoaded || isMifareClassicSak(uid.sak)) return _saveMifareClassicFlipper(filename);

    FS *fs;
    if (!getFsStorage(fs)) return FAILURE;

    File file = createNewFile(fs, "/BruceRFID", filename + ".nfc");
    if (!file) return FAILURE;

    String devType = ntagVariant.length() ? ntagVariant : printableUID.picc_type;

    file.println("Filetype: Flipper NFC device");
    file.println("Version: 4");
    file.println(
        "# Device type can be ISO14443-3A, ISO14443-3B, ISO14443-4A, ISO14443-4B, "
        "ISO15693-3, FeliCa, NTAG/Ultralight, Mifare Classic, Mifare DESFire, SLIX, "
        "ST25TB, EMV"
    );
    file.println("Device type: " + devType);
    file.println("# UID is common for all formats");
    file.println("UID: " + printableUID.uid);
    // Flipper grava ATQA em little-endian (ex.: NTAG => "44 00")
    char atqaBuf[6];
    sprintf(atqaBuf, "%02X %02X", uid.atqaByte[0], uid.atqaByte[1]);
    file.println("ATQA: " + String(atqaBuf));
    file.println("SAK: " + printableUID.sak);
    file.println("# Mifare Ultralight specific data");
    file.println("Data format version: 2");
    file.println("Signature: " + _bytesToHex(ntagSignature, 32));
    file.println("Mifare version: " + _bytesToHex(ntagVersion, 8));
    for (int i = 0; i < 3; i++) {
        file.println("Counter " + String(i) + ": " + String(ntagCounters[i]));
        char tBuf[3];
        sprintf(tBuf, "%02X", ntagTearing[i]);
        file.println("Tearing " + String(i) + ": " + String(tBuf));
    }
    file.println("Pages total: " + String(totalPages > 0 ? totalPages : dataPages));
    file.println("Pages read: " + String(dataPages));
    file.print(strAllPages);
    file.println("Failed authentication attempts: 0");

    file.close();
    delay(100);
    return SUCCESS;
}

int ST25R3916::save(String filename) {
    FS *fs;
    if (!getFsStorage(fs)) return FAILURE;

    File file = createNewFile(fs, "/BruceRFID", filename + ".rfid");
    if (!file) return FAILURE;

    file.println("Filetype: Bruce RFID File");
    file.println("Version 1");
    file.println("Device type: " + printableUID.picc_type);
    file.println("# UID, ATQA and SAK are common for all formats");
    file.println("UID: " + printableUID.uid);
    file.println("SAK: " + printableUID.sak);
    file.println("ATQA: " + printableUID.atqa);
    // NTAG/Ultralight: persiste versão e assinatura p/ emulação fiel (ex.: amiibo).
    if (ntagHasVersion) file.println("Mifare version: " + _bytesToHex(ntagVersion, 8));
    if (ntagHasSignature) file.println("Signature: " + _bytesToHex(ntagSignature, 32));
    if (ntagHasCounters) {
        for (int i = 0; i < 3; i++) file.println("Counter " + String(i) + ": " + String(ntagCounters[i]));
    }
    file.println("# Memory dump");
    file.println("Pages total: " + String(totalPages > 0 ? totalPages : dataPages));
    if (!pageReadSuccess) file.println("Pages read: " + String(dataPages));
    file.print(strAllPages);

    file.close();
    delay(100);
    return SUCCESS;
}

// ===========================================================================
// MIFARE Classic — leitura, escrita, clone (Crypto1) — Milestone 5
// ===========================================================================

bool ST25R3916::isMifareClassicSak(uint8_t sak) const {
    return (sak == 0x08 || sak == 0x88 || sak == 0x18 || sak == 0x09 || sak == 0x28 || sak == 0x38);
}

uint32_t ST25R3916::_mfcUid32() const {
    // Crypto1 uses the (last) 4 UID bytes. For 7-byte UIDs that is bytes [3..6].
    int off = (uid.size > 4) ? (uid.size - 4) : 0;
    return ((uint32_t)uid.uidByte[off] << 24) | ((uint32_t)uid.uidByte[off + 1] << 16) |
           ((uint32_t)uid.uidByte[off + 2] << 8) | (uint32_t)uid.uidByte[off + 3];
}

uint8_t ST25R3916::_mfcSectorFirstBlock(uint8_t sector) const {
    if (sector < 32) return (uint8_t)(sector * 4);
    return (uint8_t)(128 + (sector - 32) * 16);
}

uint8_t ST25R3916::_mfcSectorBlockCount(uint8_t sector) const { return (sector < 32) ? 4 : 16; }

uint8_t ST25R3916::_mfcBlockToSector(uint16_t block) const {
    if (block < 128) return (uint8_t)(block / 4);
    return (uint8_t)(32 + (block - 128) / 16);
}

// Pack data+parity into an ISO14443-A bitstream (LSB first, 9 bits per byte).
static uint16_t _mfcPackBits(const uint8_t *data, const uint8_t *par, uint8_t nbytes, uint8_t *out) {
    uint16_t bit = 0;
    for (uint8_t i = 0; i < nbytes; i++) {
        for (uint8_t b = 0; b < 8; b++) {
            if (data[i] & (1u << b)) out[bit >> 3] |= (uint8_t)(1u << (bit & 7));
            else out[bit >> 3] &= (uint8_t)~(1u << (bit & 7));
            bit++;
        }
        if (par[i] & 1u) out[bit >> 3] |= (uint8_t)(1u << (bit & 7));
        else out[bit >> 3] &= (uint8_t)~(1u << (bit & 7));
        bit++;
    }
    return bit;
}

// Reception of MIFARE frames with PAR_RX_KEEP ends on an "incomplete byte"
// (the trailing parity bit), which RFAL reports as ST_ERR_INCOMPLETE_BYTE[_0x].
// Those are valid receptions for us — only hard errors (timeout/etc) are failures.
static inline bool _mfcRxOk(::ReturnCode e) {
    return (e == ST_ERR_NONE) || (e >= ST_ERR_INCOMPLETE_BYTE && e <= ST_ERR_INCOMPLETE_BYTE_07);
}

// Unpack an ISO14443-A bitstream (8 data bits + parity bit per byte). Drops parity.
static uint8_t _mfcUnpackBits(const uint8_t *in, uint16_t nbits, uint8_t *outData, uint8_t maxBytes) {
    uint8_t nbytes = 0;
    uint16_t bit = 0;
    while ((bit + 8) <= nbits && nbytes < maxBytes) {
        uint8_t v = 0;
        for (uint8_t b = 0; b < 8; b++) {
            if (in[bit >> 3] & (1u << (bit & 7))) v |= (uint8_t)(1u << b);
            bit++;
        }
        outData[nbytes++] = v;
        if (bit < nbits) bit++; // skip parity bit
    }
    return nbytes;
}

// Encrypted MIFARE frame: software supplies parity (PAR_TX_NONE) and CRC
// (CRC_TX_MANUAL); reception keeps parity + CRC bits and skips HW checks
// (PAR_RX_KEEP disables both parity AND CRC validation on the ST25R3916).
#define MFC_FLAGS_ENC                                                                                        \
    ((uint32_t)RFAL_TXRX_FLAGS_CRC_TX_MANUAL | (uint32_t)RFAL_TXRX_FLAGS_CRC_RX_KEEP |                       \
     (uint32_t)RFAL_TXRX_FLAGS_NFCIP1_OFF | (uint32_t)RFAL_TXRX_FLAGS_AGC_ON |                               \
     (uint32_t)RFAL_TXRX_FLAGS_PAR_RX_KEEP | (uint32_t)RFAL_TXRX_FLAGS_PAR_TX_NONE)

// First-auth command travels in the clear: software supplies the CRC
// (CRC_TX_MANUAL, already in the buffer) but HW adds parity (PAR_TX_AUTO). The
// nonce response carries no CRC, so reception must skip parity/CRC checks.
#define MFC_FLAGS_FIRSTAUTH                                                                                  \
    ((uint32_t)RFAL_TXRX_FLAGS_CRC_TX_MANUAL | (uint32_t)RFAL_TXRX_FLAGS_CRC_RX_KEEP |                       \
     (uint32_t)RFAL_TXRX_FLAGS_NFCIP1_OFF | (uint32_t)RFAL_TXRX_FLAGS_AGC_ON |                               \
     (uint32_t)RFAL_TXRX_FLAGS_PAR_RX_KEEP | (uint32_t)RFAL_TXRX_FLAGS_PAR_TX_AUTO)

::ReturnCode ST25R3916::_mifareTransceiveRaw(
    uint8_t *txBuf, uint16_t txBits, uint8_t *rxBuf, uint16_t rxCapBytes, uint16_t *rxBits, uint32_t fwt,
    uint32_t flags
) {
    rfalTransceiveContext ctx;
    ctx.txBuf = txBuf;
    ctx.txBufLen = txBits;
    ctx.rxBuf = rxBuf;
    ctx.rxBufLen = (uint16_t)rfalConvBytesToBits(rxCapBytes);
    ctx.rxRcvdLen = rxBits;
    ctx.flags = flags;
    ctx.fwt = fwt;

    if (rxBits) *rxBits = 0;
    auto err = _hw->rfalStartTransceive(&ctx);
    if (err != ST_ERR_NONE) return err;

    uint32_t t0 = millis();
    while ((millis() - t0) < 60) {
        _hw->rfalWorker();
        err = _hw->rfalGetTransceiveStatus();
        if (err != ST_ERR_BUSY) break;
    }
    return err;
}

bool ST25R3916::_mifareAuth(uint8_t block, const uint8_t key[6], bool useKeyB) {
    uint64_t k = 0;
    for (int i = 0; i < 6; i++) k = (k << 8) | key[i];

    uint8_t cmd[2] = {(uint8_t)(useKeyB ? 0x61 : 0x60), block};
    uint32_t nt = 0;

    if (!_mfcAuthed) {
        // First authentication: command in the clear with manual CRC + HW parity.
        // The nonce response has no CRC, so RX skips parity/CRC checks.
        uint16_t crc = _hw->rfalCrcCalculateCcitt(0x6363, cmd, 2);
        uint8_t tx[4] = {cmd[0], cmd[1], (uint8_t)(crc & 0xFF), (uint8_t)(crc >> 8)};
        uint8_t rx[8] = {0};
        uint16_t rxBits = 0;
        auto err = _mifareTransceiveRaw(
            tx,
            (uint16_t)rfalConvBytesToBits(4),
            rx,
            sizeof(rx),
            &rxBits,
            rfalConvMsTo1fc(20),
            MFC_FLAGS_FIRSTAUTH
        );
        if (!_mfcRxOk(err) || rxBits < 32) {
            ST25R_LOG("mfc auth1 blk=%u no nonce err=%d bits=%u", block, (int)err, rxBits);
            return false;
        }
        uint8_t ntBuf[4] = {0};
        _mfcUnpackBits(rx, rxBits, ntBuf, 4);
        nt = ((uint32_t)ntBuf[0] << 24) | ((uint32_t)ntBuf[1] << 16) | ((uint32_t)ntBuf[2] << 8) | ntBuf[3];
        crypto1_init(&_mfcCipher, k);
        crypto1_word(&_mfcCipher, nt ^ _mfcUid32(), 0);
    } else {
        // Nested authentication: command + nonce travel over the encrypted channel.
        uint16_t crc = _hw->rfalCrcCalculateCcitt(0x6363, cmd, 2);
        uint8_t plain[4] = {cmd[0], cmd[1], (uint8_t)(crc & 0xFF), (uint8_t)(crc >> 8)};
        uint8_t enc[4], par[4];
        for (int i = 0; i < 4; i++) {
            enc[i] = (uint8_t)(crypto1_byte(&_mfcCipher, 0, 0) ^ plain[i]);
            par[i] = (uint8_t)(crypto1_filter_bit(&_mfcCipher) ^ nfc_oddparity(plain[i]));
        }
        uint8_t tx[8] = {0};
        uint16_t txBits = _mfcPackBits(enc, par, 4, tx);
        uint8_t rx[8] = {0};
        uint16_t rxBits = 0;
        auto err =
            _mifareTransceiveRaw(tx, txBits, rx, sizeof(rx), &rxBits, rfalConvMsTo1fc(20), MFC_FLAGS_ENC);
        if (!_mfcRxOk(err) || rxBits < 32) {
            ST25R_LOG("mfc nested auth blk=%u no nonce err=%d bits=%u", block, (int)err, rxBits);
            return false;
        }
        uint8_t encNt[4] = {0};
        _mfcUnpackBits(rx, rxBits, encNt, 4);
        for (int i = 0; i < 4; i++) {
            uint8_t ksb = crypto1_byte(&_mfcCipher, 0, 0);
            nt = (nt << 8) | (uint8_t)(encNt[i] ^ ksb);
        }
        crypto1_init(&_mfcCipher, k);
        crypto1_word(&_mfcCipher, nt ^ _mfcUid32(), 0);
    }

    // Build encrypted reader answer: nr (fed in) + ar (= suc64(nt)).
    uint8_t nrPlain[4];
    uint32_t nr = esp_random();
    nrPlain[0] = (uint8_t)(nr >> 24);
    nrPlain[1] = (uint8_t)(nr >> 16);
    nrPlain[2] = (uint8_t)(nr >> 8);
    nrPlain[3] = (uint8_t)(nr);

    uint8_t arData[8], arPar[8];
    for (int i = 0; i < 4; i++) {
        arData[i] = (uint8_t)(crypto1_byte(&_mfcCipher, nrPlain[i], 0) ^ nrPlain[i]);
        arPar[i] = (uint8_t)(crypto1_filter_bit(&_mfcCipher) ^ nfc_oddparity(nrPlain[i]));
    }
    uint32_t ar = prng_successor(nt, 64);
    for (int i = 0; i < 4; i++) {
        uint8_t arb = (uint8_t)(ar >> (24 - 8 * i));
        arData[4 + i] = (uint8_t)(crypto1_byte(&_mfcCipher, 0, 0) ^ arb);
        arPar[4 + i] = (uint8_t)(crypto1_filter_bit(&_mfcCipher) ^ nfc_oddparity(arb));
    }

    uint8_t tx[16] = {0};
    uint16_t txBits = _mfcPackBits(arData, arPar, 8, tx);
    uint8_t rx[8] = {0};
    uint16_t rxBits = 0;
    auto err = _mifareTransceiveRaw(tx, txBits, rx, sizeof(rx), &rxBits, rfalConvMsTo1fc(20), MFC_FLAGS_ENC);
    if (!_mfcRxOk(err) || rxBits < 32) {
        ST25R_LOG("mfc auth blk=%u no AT err=%d bits=%u", block, (int)err, rxBits);
        _mfcAuthed = false;
        return false;
    }
    uint8_t atEnc[4] = {0};
    _mfcUnpackBits(rx, rxBits, atEnc, 4);
    uint32_t at = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t ksb = crypto1_byte(&_mfcCipher, 0, 0);
        at = (at << 8) | (uint8_t)(atEnc[i] ^ ksb);
    }
    uint32_t atExpected = prng_successor(nt, 96);
    if (at != atExpected) {
        ST25R_LOG(
            "mfc auth blk=%u AT mismatch (%08lX != %08lX)",
            block,
            (unsigned long)at,
            (unsigned long)atExpected
        );
        _mfcAuthed = false;
        return false;
    }
    _mfcAuthed = true;
    return true;
}

bool ST25R3916::_mifareReadBlock(uint8_t block, uint8_t data[16]) {
    if (!_mfcAuthed) return false;
    uint8_t plain[4] = {0x30, block, 0, 0};
    uint16_t crc = _hw->rfalCrcCalculateCcitt(0x6363, plain, 2);
    plain[2] = (uint8_t)(crc & 0xFF);
    plain[3] = (uint8_t)(crc >> 8);

    uint8_t enc[4], par[4];
    for (int i = 0; i < 4; i++) {
        enc[i] = (uint8_t)(crypto1_byte(&_mfcCipher, 0, 0) ^ plain[i]);
        par[i] = (uint8_t)(crypto1_filter_bit(&_mfcCipher) ^ nfc_oddparity(plain[i]));
    }
    uint8_t tx[8] = {0};
    uint16_t txBits = _mfcPackBits(enc, par, 4, tx);
    uint8_t rx[32] = {0};
    uint16_t rxBits = 0;
    auto err = _mifareTransceiveRaw(tx, txBits, rx, sizeof(rx), &rxBits, rfalConvMsTo1fc(20), MFC_FLAGS_ENC);
    if (!_mfcRxOk(err) || rxBits < (18 * 8)) {
        ST25R_LOG("mfc read blk=%u err=%d bits=%u", block, (int)err, rxBits);
        return false;
    }
    uint8_t recv[18] = {0};
    uint8_t n = _mfcUnpackBits(rx, rxBits, recv, 18);
    if (n < 16) return false;
    // Decrypt all 18 bytes (16 data + 2 CRC) to keep the cipher in sync.
    for (int i = 0; i < 18; i++) recv[i] ^= crypto1_byte(&_mfcCipher, 0, 0);
    memcpy(data, recv, 16);
    return true;
}

bool ST25R3916::_mifareWriteBlock(uint8_t block, const uint8_t data[16]) {
    if (!_mfcAuthed) return false;

    // Phase 1: WRITE command (0xA0 + block).
    uint8_t plain[4] = {0xA0, block, 0, 0};
    uint16_t crc = _hw->rfalCrcCalculateCcitt(0x6363, plain, 2);
    plain[2] = (uint8_t)(crc & 0xFF);
    plain[3] = (uint8_t)(crc >> 8);

    uint8_t enc[4], par[4];
    for (int i = 0; i < 4; i++) {
        enc[i] = (uint8_t)(crypto1_byte(&_mfcCipher, 0, 0) ^ plain[i]);
        par[i] = (uint8_t)(crypto1_filter_bit(&_mfcCipher) ^ nfc_oddparity(plain[i]));
    }
    uint8_t tx[24] = {0};
    uint16_t txBits = _mfcPackBits(enc, par, 4, tx);
    uint8_t rx[8] = {0};
    uint16_t rxBits = 0;
    auto err = _mifareTransceiveRaw(tx, txBits, rx, sizeof(rx), &rxBits, rfalConvMsTo1fc(20), MFC_FLAGS_ENC);
    if (!_mfcRxOk(err) || rxBits < 4) {
        ST25R_LOG("mfc write1 blk=%u err=%d bits=%u", block, (int)err, rxBits);
        return false;
    }
    // ACK nibble (4 bits) encrypted with the next keystream bits; expect 0xA.
    uint8_t ack = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t ks = crypto1_bit(&_mfcCipher, 0, 0);
        uint8_t b = (uint8_t)((rx[0] >> i) & 1);
        ack |= (uint8_t)((b ^ ks) << i);
    }
    if ((ack & 0x0F) != 0x0A) {
        ST25R_LOG("mfc write1 blk=%u NAK 0x%X", block, ack & 0x0F);
        return false;
    }

    // Phase 2: 16 data bytes + CRC.
    uint8_t dplain[18];
    memcpy(dplain, data, 16);
    uint16_t dcrc = _hw->rfalCrcCalculateCcitt(0x6363, dplain, 16);
    dplain[16] = (uint8_t)(dcrc & 0xFF);
    dplain[17] = (uint8_t)(dcrc >> 8);

    uint8_t denc[18], dpar[18];
    for (int i = 0; i < 18; i++) {
        denc[i] = (uint8_t)(crypto1_byte(&_mfcCipher, 0, 0) ^ dplain[i]);
        dpar[i] = (uint8_t)(crypto1_filter_bit(&_mfcCipher) ^ nfc_oddparity(dplain[i]));
    }
    uint8_t tx2[24] = {0};
    uint16_t txBits2 = _mfcPackBits(denc, dpar, 18, tx2);
    uint8_t rx2[8] = {0};
    uint16_t rxBits2 = 0;
    err = _mifareTransceiveRaw(tx2, txBits2, rx2, sizeof(rx2), &rxBits2, rfalConvMsTo1fc(20), MFC_FLAGS_ENC);
    if (!_mfcRxOk(err) || rxBits2 < 4) {
        ST25R_LOG("mfc write2 blk=%u err=%d bits=%u", block, (int)err, rxBits2);
        return false;
    }
    uint8_t ack2 = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t ks = crypto1_bit(&_mfcCipher, 0, 0);
        uint8_t b = (uint8_t)((rx2[0] >> i) & 1);
        ack2 |= (uint8_t)((b ^ ks) << i);
    }
    if ((ack2 & 0x0F) != 0x0A) {
        ST25R_LOG("mfc write2 blk=%u NAK 0x%X", block, ack2 & 0x0F);
        return false;
    }
    return true;
}

void ST25R3916::_mfcHalt() {
    // HLTA (0x50 0x00) + CRC, then drop the cipher so the next auth restarts clean.
    uint8_t halt[4] = {0x50, 0x00, 0, 0};
    uint16_t crc = _hw->rfalCrcCalculateCcitt(0x6363, halt, 2);
    halt[2] = (uint8_t)(crc & 0xFF);
    halt[3] = (uint8_t)(crc >> 8);
    uint8_t rx[4] = {0};
    uint16_t rxLen = 0;
    _hw->rfalTransceiveBlockingTxRx(
        halt, sizeof(halt), rx, sizeof(rx), &rxLen, RFAL_TXRX_FLAGS_CRC_TX_MANUAL, rfalConvMsTo1fc(5)
    );
    _mfcAuthed = false;
}

void ST25R3916::_mfcRebuildStrAllPages() {
    strAllPages = "";
    char line[80];
    for (uint8_t s = 0; s < mfcDump.sectors; s++) {
        if (mfcDump.keyAFound[s]) {
            const uint8_t *k = mfcDump.keyA[s];
            sprintf(
                line, "Key A sector %u: %02X %02X %02X %02X %02X %02X", s, k[0], k[1], k[2], k[3], k[4], k[5]
            );
            strAllPages += String(line) + "\n";
        }
        if (mfcDump.keyBFound[s]) {
            const uint8_t *k = mfcDump.keyB[s];
            sprintf(
                line, "Key B sector %u: %02X %02X %02X %02X %02X %02X", s, k[0], k[1], k[2], k[3], k[4], k[5]
            );
            strAllPages += String(line) + "\n";
        }
    }
    for (uint16_t b = 0; b < mfcDump.totalBlocks; b++) {
        if (mfcDump.blockRead[b]) {
            const uint8_t *d = mfcDump.blocks[b];
            sprintf(
                line,
                "Block %u: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                b,
                d[0],
                d[1],
                d[2],
                d[3],
                d[4],
                d[5],
                d[6],
                d[7],
                d[8],
                d[9],
                d[10],
                d[11],
                d[12],
                d[13],
                d[14],
                d[15]
            );
        } else {
            sprintf(line, "Block %u: [AUTH FAIL]", b);
        }
        strAllPages += String(line) + "\n";
    }
    dataPages = mfcDump.totalBlocks;
    totalPages = mfcDump.totalBlocks;
}

int ST25R3916::_readMifareClassic(rfalNfcDevice *dev) {
    memset(&mfcDump, 0, sizeof(mfcDump));
    _mfcAuthed = false;

    if (uid.sak == 0x18 || uid.sak == 0x38) {
        mfcDump.sectors = 40;
        mfcDump.totalBlocks = 256;
        mfcType = "4K";
    } else if (uid.sak == 0x09) {
        mfcDump.sectors = 5;
        mfcDump.totalBlocks = 20;
        mfcType = "Mini";
    } else {
        mfcDump.sectors = 16;
        mfcDump.totalBlocks = 64;
        mfcType = "1K";
    }
    printableUID.picc_type = "MIFARE Classic " + mfcType;

    const int nKeys = (int)(sizeof(keys) / sizeof(keys[0]));
    int blocksOk = 0;

    for (uint8_t s = 0; s < mfcDump.sectors; s++) {
        uint8_t firstBlock = _mfcSectorFirstBlock(s);
        uint8_t nBlocks = _mfcSectorBlockCount(s);
        uint8_t trailer = (uint8_t)(firstBlock + nBlocks - 1);

        bool authed = false;
        bool usedKeyB = false;
        int usedKeyIdx = -1;

        // Try Key A then Key B from the dictionary against the sector trailer.
        for (int useB = 0; useB <= 1 && !authed; useB++) {
            for (int kidx = 0; kidx < nKeys; kidx++) {
                if (_mifareAuth(trailer, keys[kidx], useB != 0)) {
                    authed = true;
                    usedKeyB = (useB != 0);
                    usedKeyIdx = kidx;
                    break;
                }
                // A failed auth leaves the field/cipher dirty: halt + reselect.
                _mfcHalt();
                _nfc->rfalNfcDeactivate(false);
                delay(5);
                rfalNfcDevice *d2 = nullptr;
                if (!_pollForTag(&d2, 500)) {
                    pageReadStatus = FAILURE;
                    return FAILURE;
                }
                _parseDevice(d2);
                _mfcAuthed = false;
            }
        }

        if (!authed) {
            ST25R_LOG("mfc sector %u: no key found", s);
            continue;
        }

        if (usedKeyB) {
            memcpy(mfcDump.keyB[s], keys[usedKeyIdx], 6);
            mfcDump.keyBFound[s] = true;
        } else {
            memcpy(mfcDump.keyA[s], keys[usedKeyIdx], 6);
            mfcDump.keyAFound[s] = true;
        }

        for (uint8_t bo = 0; bo < nBlocks; bo++) {
            uint8_t blk = (uint8_t)(firstBlock + bo);
            uint8_t data[16] = {0};
            if (_mifareReadBlock(blk, data)) {
                memcpy(mfcDump.blocks[blk], data, 16);
                mfcDump.blockRead[blk] = true;
                blocksOk++;
            } else {
                ST25R_LOG("mfc block %u read fail", blk);
            }
        }

        // Re-select before moving to the next sector (fresh first-auth is more robust).
        _mfcHalt();
        _nfc->rfalNfcDeactivate(false);
        delay(5);
        rfalNfcDevice *d2 = nullptr;
        if (s + 1 < mfcDump.sectors) {
            if (!_pollForTag(&d2, 500)) break;
            _parseDevice(d2);
            _mfcAuthed = false;
        }
    }

    mfcLoaded = (blocksOk > 0);
    _mfcRebuildStrAllPages();
    ST25R_LOG("mfc read done: %d/%u blocks", blocksOk, mfcDump.totalBlocks);

    pageReadStatus = mfcLoaded ? SUCCESS : FAILURE;
    pageReadSuccess = mfcLoaded;
    return mfcLoaded ? SUCCESS : FAILURE;
}

int ST25R3916::_writeMifareClassic(rfalNfcDevice *dev) {
    if (!mfcLoaded) return FAILURE;
    _mfcAuthed = false;

    int written = 0;
    for (uint8_t s = 0; s < mfcDump.sectors; s++) {
        uint8_t firstBlock = _mfcSectorFirstBlock(s);
        uint8_t nBlocks = _mfcSectorBlockCount(s);
        uint8_t trailer = (uint8_t)(firstBlock + nBlocks - 1);

        const uint8_t *key =
            mfcDump.keyAFound[s] ? mfcDump.keyA[s] : (mfcDump.keyBFound[s] ? mfcDump.keyB[s] : nullptr);
        bool useB = !mfcDump.keyAFound[s] && mfcDump.keyBFound[s];
        if (!key) continue;

        if (!_mifareAuth(trailer, key, useB)) {
            _mfcHalt();
            _nfc->rfalNfcDeactivate(false);
            delay(5);
            rfalNfcDevice *d2 = nullptr;
            if (!_pollForTag(&d2, 500)) break;
            _parseDevice(d2);
            _mfcAuthed = false;
            continue;
        }

        for (uint8_t bo = 0; bo < nBlocks; bo++) {
            uint8_t blk = (uint8_t)(firstBlock + bo);
            if (blk == 0) continue;                // block 0 is read-only on genuine cards
            if (!mfcDump.blockRead[blk]) continue; // skip blocks we never recovered
            if (_mifareWriteBlock(blk, mfcDump.blocks[blk])) written++;
        }

        _mfcHalt();
        _nfc->rfalNfcDeactivate(false);
        delay(5);
        rfalNfcDevice *d2 = nullptr;
        if (s + 1 < mfcDump.sectors) {
            if (!_pollForTag(&d2, 500)) break;
            _parseDevice(d2);
            _mfcAuthed = false;
        }
    }

    ST25R_LOG("mfc write done: %d blocks", written);
    _nfc->rfalNfcDeactivate(false);
    return written > 0 ? SUCCESS : FAILURE;
}

int ST25R3916::_writeMifareClassicMagic(rfalNfcDevice *dev) {
    if (!mfcLoaded) return FAILURE;

    // Magic Gen1 backdoor: 0x40 (7 bits) then 0x43; afterwards any block accepts
    // a plain MIFARE WRITE (0xA0) without authentication/crypto.
    auto gen1Cmd = [&](uint8_t cmd, uint8_t bits) -> bool {
        uint8_t rxBuf[4] = {0};
        uint16_t actLen = 0;
        rfalTransceiveContext ctx;
        ctx.txBuf = &cmd;
        ctx.txBufLen = bits;
        ctx.rxBuf = rxBuf;
        ctx.rxBufLen = 32;
        ctx.rxRcvdLen = &actLen;
        ctx.flags = (uint32_t)RFAL_TXRX_FLAGS_CRC_TX_MANUAL | (uint32_t)RFAL_TXRX_FLAGS_PAR_TX_NONE |
                    (uint32_t)RFAL_TXRX_FLAGS_CRC_RX_KEEP | (uint32_t)RFAL_TXRX_FLAGS_PAR_RX_KEEP;
        ctx.fwt = rfalConv64fcTo1fc(5000);
        if (_hw->rfalStartTransceive(&ctx) != ST_ERR_NONE) return false;
        uint32_t t0 = millis();
        while (millis() - t0 < 50) {
            _hw->rfalWorker();
            if (_hw->rfalGetTransceiveState() == RFAL_TXRX_STATE_IDLE) break;
        }
        return (_hw->rfalGetTransceiveStatus() == ST_ERR_NONE && rxBuf[0] == 0x0A);
    };

    if (!gen1Cmd(0x40, 7) || !gen1Cmd(0x43, 8)) {
        ST25R_LOG("mfc magic: not a Gen1 card (backdoor failed)");
        return TAG_NOT_MATCH;
    }

    int written = 0;
    for (uint16_t b = 0; b < mfcDump.totalBlocks; b++) {
        if (!mfcDump.blockRead[b]) continue;
        uint8_t writeCmd[2] = {0xA0, (uint8_t)b};
        uint8_t rxBuf[4] = {0};
        uint16_t actLen = 0;
        auto err = _hw->rfalTransceiveBlockingTxRx(
            writeCmd,
            sizeof(writeCmd),
            rxBuf,
            sizeof(rxBuf),
            &actLen,
            RFAL_TXRX_FLAGS_DEFAULT,
            rfalConvMsTo1fc(20)
        );
        if (err != ST_ERR_NONE || rxBuf[0] != 0x0A) {
            ST25R_LOG("mfc magic: WRITE blk=%u no ACK err=%d", b, (int)err);
            continue;
        }
        uint8_t blockData[16];
        memcpy(blockData, mfcDump.blocks[b], 16);
        memset(rxBuf, 0, sizeof(rxBuf));
        actLen = 0;
        err = _hw->rfalTransceiveBlockingTxRx(
            blockData,
            sizeof(blockData),
            rxBuf,
            sizeof(rxBuf),
            &actLen,
            RFAL_TXRX_FLAGS_DEFAULT,
            rfalConvMsTo1fc(20)
        );
        if (err == ST_ERR_NONE) written++;
    }

    ST25R_LOG("mfc magic clone done: %d blocks", written);
    return written > 0 ? SUCCESS : FAILURE;
}

int ST25R3916::_saveMifareClassicFlipper(const String &filename) {
    FS *fs;
    if (!getFsStorage(fs)) return FAILURE;

    File file = createNewFile(fs, "/BruceRFID", filename + ".nfc");
    if (!file) return FAILURE;

    String type = mfcType.length() ? mfcType : "1K";

    file.println("Filetype: Flipper NFC device");
    file.println("Version: 4");
    file.println("Device type: Mifare Classic");
    file.println("# UID is common for all formats");
    file.println("UID: " + printableUID.uid);
    char atqaBuf[6];
    sprintf(atqaBuf, "%02X %02X", uid.atqaByte[0], uid.atqaByte[1]);
    file.println("ATQA: " + String(atqaBuf));
    file.println("SAK: " + printableUID.sak);
    file.println("# Mifare Classic specific data");
    file.println("Mifare Classic type: " + type);
    file.println("Data format version: 2");

    char line[80];
    for (uint8_t s = 0; s < mfcDump.sectors; s++) {
        const uint8_t *ka =
            mfcDump.keyAFound[s] ? mfcDump.keyA[s] : (const uint8_t *)"\xFF\xFF\xFF\xFF\xFF\xFF";
        const uint8_t *kb =
            mfcDump.keyBFound[s] ? mfcDump.keyB[s] : (const uint8_t *)"\xFF\xFF\xFF\xFF\xFF\xFF";
        sprintf(
            line,
            "Key A sector %u: %02X %02X %02X %02X %02X %02X",
            s,
            ka[0],
            ka[1],
            ka[2],
            ka[3],
            ka[4],
            ka[5]
        );
        file.println(line);
        sprintf(
            line,
            "Key B sector %u: %02X %02X %02X %02X %02X %02X",
            s,
            kb[0],
            kb[1],
            kb[2],
            kb[3],
            kb[4],
            kb[5]
        );
        file.println(line);
    }
    for (uint16_t b = 0; b < mfcDump.totalBlocks; b++) {
        const uint8_t *d = mfcDump.blocks[b];
        if (mfcDump.blockRead[b]) {
            sprintf(
                line,
                "Block %u: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                b,
                d[0],
                d[1],
                d[2],
                d[3],
                d[4],
                d[5],
                d[6],
                d[7],
                d[8],
                d[9],
                d[10],
                d[11],
                d[12],
                d[13],
                d[14],
                d[15]
            );
        } else {
            sprintf(line, "Block %u: ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ??", b);
        }
        file.println(line);
    }

    file.close();
    delay(100);
    return SUCCESS;
}

// ---------------------------------------------------------------------------
// MIFARE Classic emulation (listener-side Crypto1) — Milestone 5
//
// The ST25R3916 handles ATQA/anticollision/SELECT in hardware (PT memory), so a
// reader always sees the right UID/SAK/ATQA/type. On top of that, this code
// answers the Crypto1 authentication and serves encrypted READ/WRITE from the
// loaded dump. Timing is tight at 106 kb/s — validated UID/type always; the
// full crypto path needs on-hardware tuning.
// ---------------------------------------------------------------------------

bool ST25R3916::_buildEmuMfc() {
    memset(&mfcDump, 0, sizeof(mfcDump));
    uint16_t maxBlock = 0;
    int pos = 0;
    while (pos < (int)strAllPages.length()) {
        int nl = strAllPages.indexOf('\n', pos);
        if (nl < 0) nl = strAllPages.length();
        String line = strAllPages.substring(pos, nl);
        pos = nl + 1;
        line.trim();
        std::vector<uint8_t> bytes;
        if (line.startsWith("Block ")) {
            int colon = line.indexOf(':');
            int idx = line.substring(6, colon).toInt();
            if (idx >= 0 && idx < 256 && st25ParseHexBytesAfterColon(line, bytes) && bytes.size() >= 16) {
                memcpy(mfcDump.blocks[idx], bytes.data(), 16);
                mfcDump.blockRead[idx] = true;
                if (idx + 1 > maxBlock) maxBlock = (uint16_t)(idx + 1);
            }
        } else if (line.startsWith("Key A sector ")) {
            int colon = line.indexOf(':');
            int s = line.substring(13, colon).toInt();
            if (s >= 0 && s < 40 && st25ParseHexBytesAfterColon(line, bytes) && bytes.size() >= 6) {
                memcpy(mfcDump.keyA[s], bytes.data(), 6);
                mfcDump.keyAFound[s] = true;
            }
        } else if (line.startsWith("Key B sector ")) {
            int colon = line.indexOf(':');
            int s = line.substring(13, colon).toInt();
            if (s >= 0 && s < 40 && st25ParseHexBytesAfterColon(line, bytes) && bytes.size() >= 6) {
                memcpy(mfcDump.keyB[s], bytes.data(), 6);
                mfcDump.keyBFound[s] = true;
            }
        }
    }
    if (maxBlock == 0) return false;

    mfcDump.totalBlocks = (maxBlock > 64) ? 256 : 64;
    mfcDump.sectors = (mfcDump.totalBlocks == 256) ? 40 : 16;

    // Inject the recovered keys back into each sector trailer (Key A reads as 0
    // on a genuine card, so the dump's trailer must be patched for auth to work).
    for (uint8_t s = 0; s < mfcDump.sectors; s++) {
        uint8_t trailer = (uint8_t)(_mfcSectorFirstBlock(s) + _mfcSectorBlockCount(s) - 1);
        if (mfcDump.keyAFound[s]) memcpy(&mfcDump.blocks[trailer][0], mfcDump.keyA[s], 6);
        if (mfcDump.keyBFound[s]) memcpy(&mfcDump.blocks[trailer][10], mfcDump.keyB[s], 6);
    }
    ST25R_LOG("emu mfc: blocks=%u sectors=%u", mfcDump.totalBlocks, mfcDump.sectors);
    return true;
}

void ST25R3916::_emuTxClear(const uint8_t *data, uint8_t n, bool withCrc) {
    _hw->st25r3916ChangeRegisterBits(
        ST25R3916_REG_ISO14443A_NFC,
        ST25R3916_REG_ISO14443A_NFC_no_tx_par,
        ST25R3916_REG_ISO14443A_NFC_no_tx_par_off
    );
    _hw->st25r3916ExecuteCommand(ST25R3916_CMD_CLEAR_FIFO);
    _hw->st25r3916WriteFifo(data, n);
    _hw->st25r3916SetNumTxBits((uint16_t)(n * 8U));
    _hw->st25r3916ExecuteCommand(
        withCrc ? ST25R3916_CMD_TRANSMIT_WITH_CRC : ST25R3916_CMD_TRANSMIT_WITHOUT_CRC
    );
    _hw->st25r3916WaitForInterruptsTimed(ST25R3916_IRQ_MASK_TXE, 20);
}

void ST25R3916::_emuTxBits(const uint8_t *bitstream, uint16_t nbits) {
    _hw->st25r3916ChangeRegisterBits(
        ST25R3916_REG_ISO14443A_NFC,
        ST25R3916_REG_ISO14443A_NFC_no_tx_par,
        ST25R3916_REG_ISO14443A_NFC_no_tx_par
    );
    _hw->st25r3916ExecuteCommand(ST25R3916_CMD_CLEAR_FIFO);
    _hw->st25r3916WriteFifo(bitstream, (uint16_t)((nbits + 7) / 8));
    _hw->st25r3916SetNumTxBits(nbits);
    _hw->st25r3916ExecuteCommand(ST25R3916_CMD_TRANSMIT_WITHOUT_CRC);
    _hw->st25r3916WaitForInterruptsTimed(ST25R3916_IRQ_MASK_TXE, 20);
}

uint16_t ST25R3916::_emuRxRaw(uint8_t *out, uint8_t maxBytes, uint32_t toMs) {
    _hw->st25r3916ChangeRegisterBits(
        ST25R3916_REG_ISO14443A_NFC,
        ST25R3916_REG_ISO14443A_NFC_no_rx_par,
        ST25R3916_REG_ISO14443A_NFC_no_rx_par
    );
    uint32_t irqs =
        _hw->st25r3916WaitForInterruptsTimed(ST25R3916_IRQ_MASK_RXE | ST25R3916_IRQ_MASK_EOF, toMs);
    if ((irqs & ST25R3916_IRQ_MASK_RXE) == 0U) return 0;
    uint16_t nb = _hw->st25r3916GetNumFIFOBytes();
    uint8_t st2 = 0;
    _hw->st25r3916ReadRegister(ST25R3916_REG_FIFO_STATUS2, &st2);
    uint8_t inc = (uint8_t)((st2 & ST25R3916_REG_FIFO_STATUS2_fifo_lb_mask) >>
                            ST25R3916_REG_FIFO_STATUS2_fifo_lb_shift);
    if (nb == 0U || nb > maxBytes) return 0;
    _hw->st25r3916ReadFifo(out, nb);
    return (uint16_t)(inc ? ((nb - 1) * 8 + inc) : (nb * 8));
}

void ST25R3916::_emuParityOff() {
    _hw->st25r3916ChangeRegisterBits(
        ST25R3916_REG_ISO14443A_NFC,
        ST25R3916_REG_ISO14443A_NFC_no_tx_par | ST25R3916_REG_ISO14443A_NFC_no_rx_par,
        ST25R3916_REG_ISO14443A_NFC_no_tx_par_off | ST25R3916_REG_ISO14443A_NFC_no_rx_par_off
    );
}

bool ST25R3916::_emuMfcHandle(uint8_t *fifo, uint16_t n) {
    uint8_t cmd = fifo[0];
    if ((cmd != 0x60 && cmd != 0x61) || n < 2) return false; // only AUTH bootstraps a session

    uint8_t block = fifo[1];
    uint8_t sector = _mfcBlockToSector(block);
    if (sector >= mfcDump.sectors) return false;
    _emuMfcAuthReq++;

    static const uint8_t ffKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    const uint8_t *key = (cmd == 0x60) ? mfcDump.keyA[sector] : mfcDump.keyB[sector];
    bool haveKey = (cmd == 0x60) ? mfcDump.keyAFound[sector] : mfcDump.keyBFound[sector];
    if (!haveKey) key = ffKey;

    uint64_t k = 0;
    for (int i = 0; i < 6; i++) k = (k << 8) | key[i];

    _emuNt = esp_random();
    uint8_t ntB[4] = {
        (uint8_t)(_emuNt >> 24), (uint8_t)(_emuNt >> 16), (uint8_t)(_emuNt >> 8), (uint8_t)_emuNt
    };
    _emuTxClear(ntB, 4, false); // nonce: parity, no CRC

    crypto1_init(&_emuCipher, k);
    crypto1_word(&_emuCipher, _mfcUid32() ^ _emuNt, 0);

    // Receive reader answer: {nr}{ar} = 8 encrypted bytes + parity (72 bits).
    uint8_t raw[16] = {0};
    uint16_t bits = _emuRxRaw(raw, sizeof(raw), 30);
    _emuMfcLastNrBits = bits;
    if (bits < 64) {
        _emuMfcNoNr++;
        _emuMfcAuthed = false;
        _emuParityOff();
        return false;
    }
    uint8_t enc[8] = {0};
    _mfcUnpackBits(raw, bits, enc, 8);
    for (int i = 0; i < 4; i++) crypto1_byte(&_emuCipher, enc[i], 1); // decrypt+feed nr
    uint32_t ar = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t ks = crypto1_byte(&_emuCipher, 0, 0);
        ar = (ar << 8) | (uint8_t)(ks ^ enc[4 + i]);
    }
    _emuDbgNt = _emuNt;
    _emuDbgBlock = block;
    memcpy(_emuDbgEnc, enc, 8);
    _emuDbgArCalc = ar;
    _emuDbgArExp = prng_successor(_emuNt, 64);
    if (ar != _emuDbgArExp) {
        _emuMfcBadAr++;
        _emuMfcAuthed = false;
        _emuParityOff();
        return false;
    }

    // Answer AT = suc3(nt), encrypted with encrypted parity.
    uint32_t at = prng_successor(_emuNt, 96);
    uint8_t atEnc[4], atPar[4];
    for (int i = 0; i < 4; i++) {
        uint8_t b = (uint8_t)(at >> (24 - 8 * i));
        atEnc[i] = (uint8_t)(crypto1_byte(&_emuCipher, 0, 0) ^ b);
        atPar[i] = (uint8_t)(crypto1_filter_bit(&_emuCipher) ^ nfc_oddparity(b));
    }
    uint8_t bs[8] = {0};
    uint16_t nbits = _mfcPackBits(atEnc, atPar, 4, bs);
    _emuTxBits(bs, nbits);
    _emuMfcAuthed = true;
    _emuMfcAuthOk++;

    // Encrypted session: serve READ/WRITE until HALT, field off or Esc.
    while (true) {
        // Yield 1ms so the input task can set EscPress (RXE IRQ stays latched,
        // so the next reader command is not lost). Lets the user abort mid-session.
        vTaskDelay(pdMS_TO_TICKS(1));
        if (EscPress) break;
        uint8_t rbuf[40] = {0};
        uint16_t rbits = _emuRxRaw(rbuf, sizeof(rbuf), 40);
        if (rbits < 8) break;
        uint8_t dec[34] = {0};
        uint8_t cnt = _mfcUnpackBits(rbuf, rbits, dec, sizeof(dec));
        for (uint8_t i = 0; i < cnt; i++) dec[i] ^= crypto1_byte(&_emuCipher, 0, 0);
        uint8_t c = dec[0];

        if (c == 0x30 && cnt >= 2) { // READ block
            uint8_t blk = dec[1];
            uint8_t plain[18];
            memcpy(plain, (blk < mfcDump.totalBlocks) ? mfcDump.blocks[blk] : ffKey, 16);
            if (blk >= mfcDump.totalBlocks) memset(plain, 0, 16);
            uint16_t crc = _hw->rfalCrcCalculateCcitt(0x6363, plain, 16);
            plain[16] = (uint8_t)(crc & 0xFF);
            plain[17] = (uint8_t)(crc >> 8);
            uint8_t e[18], p[18];
            for (int i = 0; i < 18; i++) {
                e[i] = (uint8_t)(crypto1_byte(&_emuCipher, 0, 0) ^ plain[i]);
                p[i] = (uint8_t)(crypto1_filter_bit(&_emuCipher) ^ nfc_oddparity(plain[i]));
            }
            uint8_t ob[24] = {0};
            uint16_t nb = _mfcPackBits(e, p, 18, ob);
            _emuTxBits(ob, nb);
            _emuMfcReads++;
        } else if (c == 0xA0 && cnt >= 2) { // WRITE block (2 phases)
            uint8_t blk = dec[1];
            // ACK phase 1 (4-bit 0x0A, encrypted)
            uint8_t ackbs = 0;
            for (int i = 0; i < 4; i++)
                ackbs |= (uint8_t)((crypto1_bit(&_emuCipher, 0, 0) ^ ((0x0A >> i) & 1)) << i);
            _emuTxBits(&ackbs, 4);
            // data phase
            uint8_t dbuf[40] = {0};
            uint16_t dbits = _emuRxRaw(dbuf, sizeof(dbuf), 40);
            if (dbits < 8) break;
            uint8_t dd[20] = {0};
            uint8_t dn = _mfcUnpackBits(dbuf, dbits, dd, sizeof(dd));
            for (uint8_t i = 0; i < dn; i++) dd[i] ^= crypto1_byte(&_emuCipher, 0, 0);
            if (blk < mfcDump.totalBlocks && dn >= 16) memcpy(mfcDump.blocks[blk], dd, 16);
            uint8_t ack2 = 0;
            for (int i = 0; i < 4; i++)
                ack2 |= (uint8_t)((crypto1_bit(&_emuCipher, 0, 0) ^ ((0x0A >> i) & 1)) << i);
            _emuTxBits(&ack2, 4);
        } else if (c == 0x50) { // HALT
            break;
        } else {
            break;
        }
    }

    _emuMfcAuthed = false;
    _emuParityOff(); // restore HW parity so the next plain AUTH is received intact
    return true;
}

// ===========================================================================
// Type 4 Tag (NDEF) emulation over the NFC-A target — Milestone 6
// ===========================================================================

void ST25R3916::_buildT4TFiles() {
    // Capability Container: NDEF file E1 04, max 256 bytes, read-only.
    static const uint8_t cc[15] = {
        0x00, 0x0F, 0x20, 0x00, 0x3B, 0x00, 0x34, 0x04, 0x06, 0xE1, 0x04, 0x00, 0xFF, 0x00, 0x00
    };
    memcpy(_t4tCC, cc, sizeof(cc));

    uint8_t rec[400];
    uint16_t rl = 0;
    if (ndefMessage.payloadSize > 0 &&
        (ndefMessage.payloadType == NDEF_URI || ndefMessage.payloadType == NDEF_TEXT)) {
        // Single short NDEF record from the ndefMessage struct (set via 'rfid ndef').
        rec[0] = 0xD1; // MB|ME|SR, TNF=well-known
        rec[1] = 0x01; // type length
        rec[2] = ndefMessage.payloadSize;
        rec[3] = ndefMessage.payloadType;
        memcpy(&rec[4], ndefMessage.payload, ndefMessage.payloadSize);
        rl = (uint16_t)(4 + ndefMessage.payloadSize);
    } else {
        // Default: URI "https://bruce.computer".
        const char *url = "bruce.computer";
        uint8_t ul = (uint8_t)strlen(url);
        rec[0] = 0xD1;
        rec[1] = 0x01;
        rec[2] = (uint8_t)(ul + 1);
        rec[3] = 0x55; // URI record
        rec[4] = 0x04; // prefix "https://"
        memcpy(&rec[5], url, ul);
        rl = (uint16_t)(5 + ul);
    }

    _t4tNdef[0] = (uint8_t)(rl >> 8);
    _t4tNdef[1] = (uint8_t)(rl & 0xFF);
    memcpy(&_t4tNdef[2], rec, rl);
    _t4tNdefLen = (uint16_t)(rl + 2);
}

uint16_t ST25R3916::_t4tProcessApdu(const uint8_t *c, uint16_t clen, uint8_t *r) {
    if (clen < 4) {
        r[0] = 0x6A;
        r[1] = 0x00;
        return 2;
    }
    uint8_t ins = c[1];

    if (ins == 0xA4) {      // SELECT
        if (c[2] == 0x04) { // select by AID (NDEF application)
            _t4tSelected = 0;
            r[0] = 0x90;
            r[1] = 0x00;
            return 2;
        }
        if (c[2] == 0x00 && clen >= 7) { // select EF by file id
            uint8_t f0 = c[5], f1 = c[6];
            if (f0 == 0xE1 && f1 == 0x03) {
                _t4tSelected = 1;
                r[0] = 0x90;
                r[1] = 0x00;
                return 2;
            }
            if (f0 == 0xE1 && f1 == 0x04) {
                _t4tSelected = 2;
                r[0] = 0x90;
                r[1] = 0x00;
                return 2;
            }
        }
        r[0] = 0x6A;
        r[1] = 0x82;
        return 2;
    }

    if (ins == 0xB0) { // READ BINARY
        uint16_t off = (uint16_t)((c[2] << 8) | c[3]);
        uint8_t le = (clen >= 5) ? c[4] : 0;
        const uint8_t *src;
        uint16_t srclen;
        if (_t4tSelected == 1) {
            src = _t4tCC;
            srclen = sizeof(_t4tCC);
        } else if (_t4tSelected == 2) {
            src = _t4tNdef;
            srclen = _t4tNdefLen;
        } else {
            r[0] = 0x6A;
            r[1] = 0x82;
            return 2;
        }
        if (off > srclen) {
            r[0] = 0x6A;
            r[1] = 0x86;
            return 2;
        }
        uint16_t nn = le;
        if (off + nn > srclen) nn = (uint16_t)(srclen - off);
        memcpy(r, &src[off], nn);
        r[nn] = 0x90;
        r[nn + 1] = 0x00;
        return (uint16_t)(nn + 2);
    }

    r[0] = 0x6D; // INS not supported
    r[1] = 0x00;
    return 2;
}

bool ST25R3916::_emuT4THandle(uint8_t *fifo, uint16_t n) {
    uint8_t c0 = fifo[0];

    if (c0 == 0xE0) { // RATS -> ATS
        uint8_t ats[5] = {0x05, 0x78, 0x80, 0x70, 0x02};
        _listenRespond(ats, sizeof(ats));
        return true;
    }
    if ((c0 & 0xF0) == 0xD0) { // PPS -> echo PPS response
        uint8_t resp = 0xD0;
        _listenRespond(&resp, 1);
        return true;
    }
    if (c0 == 0x02 || c0 == 0x03) { // ISO-DEP I-block carrying an APDU
        if (n < 3) return false;
        uint16_t apduLen = (uint16_t)(n - 1 - 2); // strip PCB + CRC(2)
        uint8_t rapdu[300];
        uint16_t rl = _t4tProcessApdu(&fifo[1], apduLen, rapdu);
        uint8_t out[302];
        out[0] = c0; // preserve PCB toggle bit
        memcpy(&out[1], rapdu, rl);
        _listenRespond(out, (uint16_t)(rl + 1));
        return true;
    }
    if (c0 == 0xC2) { // S(DESELECT)
        uint8_t resp = 0xC2;
        _listenRespond(&resp, 1);
        return true;
    }
    if (c0 == 0x50) { // HALT
        _hw->st25r3916ExecuteCommand(ST25R3916_CMD_GOTO_SLEEP);
        return true;
    }
    return false;
}

// ===========================================================================
// FeliCa (NFC-F) emulation — Milestone 6
// ===========================================================================

bool ST25R3916::_setupListenModeF() {
    _deselectSharedSpiDevices();
    _hw->st25r3916OscOn();

    auto mres = _hw->rfalSetMode(RFAL_MODE_LISTEN_NFCF, RFAL_BR_212, RFAL_BR_212);
    if (mres != ST_ERR_NONE) {
        ST25R_LOG("emulate felica: rfalSetMode(LISTEN_NFCF) -> %d", (int)mres);
        return false;
    }

    _hw->st25r3916WriteRegister(
        ST25R3916_REG_OP_CONTROL,
        ST25R3916_REG_OP_CONTROL_en | ST25R3916_REG_OP_CONTROL_rx_en | ST25R3916_REG_OP_CONTROL_en_fd_auto_efd
    );
    _hw->st25r3916WriteRegister(
        ST25R3916_REG_MODE, ST25R3916_REG_MODE_targ_targ | ST25R3916_REG_MODE_om_targ_nfcf
    );

    uint32_t interrupts = ST25R3916_IRQ_MASK_FWL | ST25R3916_IRQ_MASK_TXE | ST25R3916_IRQ_MASK_RXS |
                          ST25R3916_IRQ_MASK_RXE | ST25R3916_IRQ_MASK_EOF | ST25R3916_IRQ_MASK_WU_F;
    _hw->st25r3916ClearInterrupts();
    _hw->st25r3916DisableInterrupts(ST25R3916_IRQ_MASK_ALL);
    _hw->st25r3916EnableInterrupts(interrupts);

    // PT Memory F: IDm(8) + PMm(8) + System Code(2). HW auto-answers SENSF_REQ.
    uint8_t ptF[18] = {0};
    memcpy(&ptF[0], _felicaIDm, 8);
    memcpy(&ptF[8], _felicaPMm, 8);
    ptF[16] = _felicaSys[0];
    ptF[17] = _felicaSys[1];
    _hw->st25r3916WritePTMemF(ptF, sizeof(ptF));

    uint8_t tsn[ST25R3916_PTM_TSN_LEN];
    for (uint8_t i = 0; i < sizeof(tsn); i++) tsn[i] = (uint8_t)esp_random();
    _hw->st25r3916WritePTMemTSN(tsn, sizeof(tsn));

    _hw->st25r3916ExecuteCommand(ST25R3916_CMD_GOTO_SENSE);
    return true;
}

int ST25R3916::_handleFelicaListen(uint32_t timeoutMs) {
    uint32_t deadline = millis() + timeoutMs;
    int polls = 0;
    uint8_t fifo[64];
    LongPress = true;
    while ((int32_t)(deadline - millis()) > 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        if (EscPress) {
            ST25R_LOG("emulate felica: Esc — encerrando");
            break;
        }
        uint32_t irqs = _hw->st25r3916WaitForInterruptsTimed(
            ST25R3916_IRQ_MASK_WU_F | ST25R3916_IRQ_MASK_RXE | ST25R3916_IRQ_MASK_EOF, 50
        );
        if (irqs == 0U) continue;

        if (irqs & ST25R3916_IRQ_MASK_WU_F) {
            polls++;
            ST25R_LOG("emulate felica: reader fez Polling (#%d)", polls);
        }

        if (irqs & ST25R3916_IRQ_MASK_RXE) {
            uint16_t nb = _hw->st25r3916GetNumFIFOBytes();
            if (nb == 0U || nb > sizeof(fifo)) continue;
            _hw->st25r3916ReadFifo(fifo, nb);
            // fifo: LEN, CMD, IDm[8], ... — answer Read Without Encryption from dump.
            if (nb >= 2 && fifo[1] == 0x06) { // Read Without Encryption
                // Minimal RD: respond with status flags 00 00 and zeroed blocks.
                // (Real block data depends on a parsed FeliCa dump — out of scope.)
                uint8_t resp[13 + 16] = {0};
                uint8_t blocks = (nb > 14) ? fifo[14] : 1;
                if (blocks == 0 || blocks > 1) blocks = 1;
                uint8_t len = (uint8_t)(13 + 16 * blocks);
                resp[0] = len;
                resp[1] = 0x07; // response code for 0x06
                memcpy(&resp[2], _felicaIDm, 8);
                resp[10] = 0x00; // status flag 1
                resp[11] = 0x00; // status flag 2
                resp[12] = blocks;
                _hw->st25r3916ExecuteCommand(ST25R3916_CMD_CLEAR_FIFO);
                _hw->st25r3916WriteFifo(resp, len);
                _hw->st25r3916SetNumTxBits((uint16_t)(len * 8U));
                _hw->st25r3916ExecuteCommand(ST25R3916_CMD_TRANSMIT_WITH_CRC);
                _hw->st25r3916WaitForInterruptsTimed(ST25R3916_IRQ_MASK_TXE, 20);
            }
        }

        if (irqs & ST25R3916_IRQ_MASK_EOF) { _hw->st25r3916ExecuteCommand(ST25R3916_CMD_GOTO_SENSE); }
    }
    LongPress = false;
    return polls;
}

int ST25R3916::_emulateFelica() {
    // IDm from the loaded tag (printableUID.uid) or a synthetic one.
    memset(_felicaIDm, 0, 8);
    memset(_felicaPMm, 0xFF, 8);
    _felicaSys[0] = 0x88;
    _felicaSys[1] = 0xB4; // NDEF system code (common); IDm detection is what matters

    std::vector<uint8_t> idm;
    int hi = -1;
    for (unsigned i = 0; i < printableUID.uid.length(); i++) {
        int v = st25HexNibble(printableUID.uid.charAt(i));
        if (v < 0) continue;
        if (hi < 0) hi = v;
        else {
            idm.push_back((uint8_t)((hi << 4) | v));
            hi = -1;
        }
    }
    if (idm.size() >= 8) memcpy(_felicaIDm, idm.data(), 8);
    else {
        _felicaIDm[0] = 0x01;
        _felicaIDm[1] = 0xFE;
        uint32_t r = esp_random();
        memcpy(&_felicaIDm[2], &r, 4);
    }
    // PMm: try to recover from the saved dump ("PMm: ..").
    int p = strAllPages.indexOf("PMm:");
    if (p >= 0) {
        std::vector<uint8_t> pmm;
        if (st25ParseHexBytesAfterColon(strAllPages.substring(p, strAllPages.indexOf('\n', p)), pmm) &&
            pmm.size() >= 8)
            memcpy(_felicaPMm, pmm.data(), 8);
    }

    stopDiscovery();
    _nfc->rfalNfcDeactivate(false);
    _discoveryStarted = false;

    if (!_setupListenModeF()) {
        _listenStop();
        return FAILURE;
    }
    ST25R_LOG(
        "emulate felica: IDm=%s sys=%02X%02X (timeout 30s, Esc p/ sair)",
        _bytesToHex(_felicaIDm, 8).c_str(),
        _felicaSys[0],
        _felicaSys[1]
    );
    int polls = _handleFelicaListen(30000);
    _listenStop();
    ST25R_LOG("emulate felica: encerrado — polls=%d", polls);
    return SUCCESS;
}

#endif // !LITE_VERSION
