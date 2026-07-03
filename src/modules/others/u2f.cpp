#include "u2f.h"
#include "core/display.h"
#include "core/sd_functions.h"
#include <cstdio>
#include <cstring>

#ifdef USB_as_HID

#include "globals.h"
#include <Preferences.h>
#include <USB.h>
#include <USBHID.h>
#include <esp_system.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/ecp.h>
#include <mbedtls/entropy.h>
#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/private_access.h>
#include <mbedtls/sha256.h>
#include <mbedtls/x509_crt.h>
#include <vector>

namespace {

static constexpr uint8_t kU2fReportId = 0x00;
static constexpr uint32_t kBroadcastCid = 0xFFFFFFFFu;
static constexpr uint16_t kInitPayloadSize = 57; // 64 - 7
static constexpr uint16_t kContPayloadSize = 59; // 64 - 5
static constexpr uint16_t kMaxMessageSize = 1024;

static constexpr uint8_t CTAPHID_PING = 0x81;
static constexpr uint8_t CTAPHID_MSG = 0x83;
static constexpr uint8_t CTAPHID_LOCK = 0x84;
static constexpr uint8_t CTAPHID_INIT = 0x86;
static constexpr uint8_t CTAPHID_WINK = 0x88;
static constexpr uint8_t CTAPHID_CBOR = 0x90;
static constexpr uint8_t CTAPHID_CANCEL = 0x91;
static constexpr uint8_t CTAPHID_KEEPALIVE = 0xBB;
static constexpr uint8_t CTAPHID_ERROR = 0xBF;
static constexpr uint8_t CAPFLAG_WINK = 0x01;
static constexpr uint8_t CAPFLAG_CBOR = 0x04;

static constexpr uint8_t U2F_INS_REGISTER = 0x01;
static constexpr uint8_t U2F_INS_AUTHENTICATE = 0x02;
static constexpr uint8_t U2F_INS_VERSION = 0x03;

static constexpr uint8_t U2F_AUTH_CHECK_ONLY = 0x07;
static constexpr uint8_t U2F_AUTH_ENFORCE = 0x03;
static constexpr uint8_t U2F_AUTH_DONT_ENFORCE = 0x08;

static constexpr uint8_t CTAP1_ERR_INVALID_CMD = 0x01;
static constexpr uint8_t CTAP1_ERR_INVALID_LEN = 0x03;
static constexpr uint8_t CTAP1_ERR_INVALID_SEQ = 0x04;
static constexpr uint8_t CTAP1_ERR_INVALID_CID = 0x0B;
static constexpr uint8_t CTAP1_ERR_OTHER = 0x7F;

static constexpr uint8_t CTAP2_OK = 0x00;
static constexpr uint8_t CTAP2_ERR_INVALID_COMMAND = 0x01;
static constexpr uint8_t CTAP2_ERR_INVALID_PARAMETER = 0x02;
static constexpr uint8_t CTAP2_ERR_INVALID_CBOR = 0x12;
static constexpr uint8_t CTAP2_ERR_OPERATION_DENIED = 0x27;
static constexpr uint8_t CTAP2_ERR_NO_CREDENTIALS = 0x2E;
static constexpr uint8_t CTAP2_ERR_PIN_NOT_SET = 0x35;

static constexpr uint16_t SW_NO_ERROR = 0x9000;
static constexpr uint16_t SW_WRONG_DATA = 0x6A80;
static constexpr uint16_t SW_WRONG_LENGTH = 0x6700;
static constexpr uint16_t SW_INS_NOT_SUPPORTED = 0x6D00;
static constexpr uint16_t SW_CONDITIONS_NOT_SATISFIED = 0x6985;

static constexpr uint8_t kKeyHandleVersion = 0x01;
static constexpr size_t kKeyHandleNonceLen = 16;
static constexpr size_t kKeyHandleMacLen = 32;
static constexpr size_t kKeyHandleLen = 1 + kKeyHandleNonceLen + kKeyHandleMacLen;

static const uint8_t U2F_REPORT_DESCRIPTOR[] = {
    0x06, 0xD0, 0xF1, // USAGE_PAGE (FIDO Alliance)
    0x09, 0x01,       // USAGE (U2F Authenticator Device)
    0xA1, 0x01,       // COLLECTION (Application)
    0x09, 0x20,       //   USAGE (Input Report Data)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00, //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x95, 0x40,       //   REPORT_COUNT (64)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x09, 0x21,       //   USAGE (Output Report Data)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00, //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x95, 0x40,       //   REPORT_COUNT (64)
    0x91, 0x02,       //   OUTPUT (Data,Var,Abs)
    0xC0              // END_COLLECTION
};

struct RxMessageState {
    bool active = false;
    uint32_t cid = 0;
    uint8_t cmd = 0;
    uint16_t expectedLen = 0;
    uint16_t receivedLen = 0;
    uint8_t nextSeq = 0;
    uint8_t data[kMaxMessageSize] = {0};
};

struct PendingCommandState {
    volatile bool ready = false;
    uint32_t cid = 0;
    uint8_t cmd = 0;
    uint16_t len = 0;
    uint8_t data[kMaxMessageSize] = {0};
};

struct CredentialRecord {
    uint8_t credId[64] = {0};
    uint8_t credIdLen = 0;
    uint8_t rpIdHash[32] = {0};
    uint32_t signCount = 0;
    String path = "";
};

struct CborCursor {
    const uint8_t *p = nullptr;
    const uint8_t *end = nullptr;
};

class U2fHidDevice : public USBHIDDevice {
public:
    U2fHidDevice() : _hid(HID_ITF_PROTOCOL_NONE) { USBHID::addDevice(this, sizeof(U2F_REPORT_DESCRIPTOR)); }

    void begin() {
        if (_started) return;
        USB.begin();
        _hid.begin();
        uint32_t t0 = millis();
        while (!_hid.ready() && (millis() - t0) < 4000) delay(10);
        loadState();
        _started = true;
    }

    void end() {
        if (!_started) return;
        _hid.end();
        if (_prefsReady) {
            _prefs.putUInt("ctr", _counter);
            _prefs.end();
            _prefsReady = false;
        }
        _started = false;
    }

    bool waitingForPresence() const { return _waitingForPresence; }

    void poll() {
        if (!_pending.ready) return;
        handleCommand(_pending.cid, _pending.cmd, _pending.data, _pending.len);
        _pending.ready = false;
    }

    uint16_t _onGetDescriptor(uint8_t *buffer) override {
        memcpy(buffer, U2F_REPORT_DESCRIPTOR, sizeof(U2F_REPORT_DESCRIPTOR));
        return sizeof(U2F_REPORT_DESCRIPTOR);
    }

    uint16_t _onGetFeature(uint8_t, uint8_t *buffer, uint16_t len) override {
        // Some hosts probe HID feature reports on control endpoint.
        // Return zeroed data instead of STALL to maximize interoperability.
        if (buffer == nullptr || len == 0) return 0;
        memset(buffer, 0, len);
        return len;
    }

    void _onOutput(uint8_t report_id, const uint8_t *buffer, uint16_t len) override {
        if (buffer == nullptr || len < 5) return;
        if (kU2fReportId == 0) {
            if (report_id != 0) return;
            // Some hosts prepend a dummy report-id byte (0x00) even for report-id-less descriptors.
            if (len == 65 && buffer[0] == 0x00) {
                buffer++;
                len--;
            }
        } else {
            if (report_id != 0 && report_id != kU2fReportId) return;
            if (report_id == 0 && len >= 6 && buffer[0] == kU2fReportId) {
                // Legacy path where host prepends report-id in payload.
                buffer++;
                len--;
                if (len < 5) return;
            }
        }

        // CTAPHID uses fixed-size 64-byte reports for this interface.
        if (len != 64) return;

        const uint32_t cid = (uint32_t(buffer[0]) << 24) | (uint32_t(buffer[1]) << 16) |
                             (uint32_t(buffer[2]) << 8) | uint32_t(buffer[3]);

        if (buffer[4] & 0x80) {
            if (len < 7) {
                sendError(cid, CTAP1_ERR_INVALID_LEN);
                return;
            }
            handleInitFrame(cid, buffer, len);
            return;
        }
        handleContFrame(cid, buffer, len);
    }

private:
    static bool cborReadHead(CborCursor &c, uint8_t &major, uint8_t &add) {
        if (c.p >= c.end) return false;
        uint8_t b = *c.p++;
        major = b >> 5;
        add = b & 0x1F;
        return true;
    }

    static bool cborReadLen(CborCursor &c, uint8_t add, uint32_t &len) {
        if (add <= 23) {
            len = add;
            return true;
        }
        if (add == 24) {
            if (c.p + 1 > c.end) return false;
            len = *c.p++;
            return true;
        }
        if (add == 25) {
            if (c.p + 2 > c.end) return false;
            len = (uint32_t(c.p[0]) << 8) | uint32_t(c.p[1]);
            c.p += 2;
            return true;
        }
        return false;
    }

    static bool cborSkip(CborCursor &c) {
        uint8_t major = 0, add = 0;
        if (!cborReadHead(c, major, add)) return false;
        uint32_t n = 0;
        if (major <= 1) return cborReadLen(c, add, n);
        if (major == 2 || major == 3) {
            if (!cborReadLen(c, add, n)) return false;
            if (c.p + n > c.end) return false;
            c.p += n;
            return true;
        }
        if (major == 4) {
            if (!cborReadLen(c, add, n)) return false;
            for (uint32_t i = 0; i < n; i++) {
                if (!cborSkip(c)) return false;
            }
            return true;
        }
        if (major == 5) {
            if (!cborReadLen(c, add, n)) return false;
            for (uint32_t i = 0; i < n; i++) {
                if (!cborSkip(c) || !cborSkip(c)) return false;
            }
            return true;
        }
        if (major == 7 && add <= 23) return true;
        return false;
    }

    static bool cborReadUint(CborCursor &c, uint32_t &v) {
        uint8_t major = 0, add = 0;
        if (!cborReadHead(c, major, add) || major != 0) return false;
        return cborReadLen(c, add, v);
    }

    static bool cborReadInt(CborCursor &c, int32_t &v) {
        uint8_t major = 0, add = 0;
        if (!cborReadHead(c, major, add)) return false;
        uint32_t n = 0;
        if (!cborReadLen(c, add, n)) return false;
        if (major == 0) {
            v = int32_t(n);
            return true;
        }
        if (major == 1) {
            v = -1 - int32_t(n);
            return true;
        }
        return false;
    }

    static bool cborReadBytes(CborCursor &c, const uint8_t *&ptr, uint32_t &len) {
        uint8_t major = 0, add = 0;
        if (!cborReadHead(c, major, add) || major != 2) return false;
        if (!cborReadLen(c, add, len)) return false;
        if (c.p + len > c.end) return false;
        ptr = c.p;
        c.p += len;
        return true;
    }

    static bool cborReadText(CborCursor &c, String &out) {
        uint8_t major = 0, add = 0;
        if (!cborReadHead(c, major, add) || major != 3) return false;
        uint32_t len = 0;
        if (!cborReadLen(c, add, len)) return false;
        if (c.p + len > c.end) return false;
        out = "";
        for (uint32_t i = 0; i < len; i++) out += char(c.p[i]);
        c.p += len;
        return true;
    }

    static void cborAppendTypeLen(std::vector<uint8_t> &o, uint8_t major, uint32_t len) {
        if (len <= 23) {
            o.push_back(uint8_t((major << 5) | len));
        } else if (len <= 0xFF) {
            o.push_back(uint8_t((major << 5) | 24));
            o.push_back(uint8_t(len));
        } else {
            o.push_back(uint8_t((major << 5) | 25));
            o.push_back(uint8_t((len >> 8) & 0xFF));
            o.push_back(uint8_t(len & 0xFF));
        }
    }

    static void cborAppendMap(std::vector<uint8_t> &o, uint32_t len) { cborAppendTypeLen(o, 5, len); }
    static void cborAppendArray(std::vector<uint8_t> &o, uint32_t len) { cborAppendTypeLen(o, 4, len); }
    static void cborAppendUint(std::vector<uint8_t> &o, uint32_t v) { cborAppendTypeLen(o, 0, v); }
    static void cborAppendInt(std::vector<uint8_t> &o, int32_t v) {
        if (v >= 0) cborAppendUint(o, uint32_t(v));
        else cborAppendTypeLen(o, 1, uint32_t(-1 - v));
    }
    static void cborAppendText(std::vector<uint8_t> &o, const String &s) {
        cborAppendTypeLen(o, 3, s.length());
        for (size_t i = 0; i < s.length(); i++) o.push_back(uint8_t(s[i]));
    }
    static void cborAppendBytes(std::vector<uint8_t> &o, const uint8_t *p, size_t len) {
        cborAppendTypeLen(o, 2, uint32_t(len));
        for (size_t i = 0; i < len; i++) o.push_back(p[i]);
    }
    static void cborAppendBool(std::vector<uint8_t> &o, bool b) { o.push_back(b ? 0xF5 : 0xF4); }

    static bool loadP256Generator(mbedtls_ecp_point *g) {
        // secp256r1 generator coordinates (uncompressed base point)
        static const char *kGx = "6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296";
        static const char *kGy = "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5";
        return mbedtls_ecp_point_read_string(g, 16, kGx, kGy) == 0;
    }

    USBHID _hid;
    Preferences _prefs;
    bool _prefsReady = false;
    bool _started = false;
    bool _masterLoaded = false;
    uint8_t _masterSecret[32] = {0};
    uint32_t _counter = 0;
    uint32_t _nextCid = 1;
    bool _waitingForPresence = false;
    RxMessageState _rx;
    PendingCommandState _pending;
    std::vector<uint8_t> _attestationCertDer;

    static int rngCallback(void *, unsigned char *output, size_t len) {
        esp_fill_random(output, len);
        return 0;
    }

    static bool constantTimeEqual(const uint8_t *a, const uint8_t *b, size_t len) {
        uint8_t diff = 0;
        for (size_t i = 0; i < len; i++) diff |= (a[i] ^ b[i]);
        return diff == 0;
    }

    static bool sha256(const uint8_t *input, size_t len, uint8_t out[32]) {
        return mbedtls_sha256(input, len, out, 0) == 0;
    }

    static bool
    hmacSha256(const uint8_t *key, size_t keyLen, const uint8_t *msg, size_t msgLen, uint8_t out[32]) {
        const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
        if (md == nullptr) return false;
        mbedtls_md_context_t ctx;
        mbedtls_md_init(&ctx);
        if (mbedtls_md_setup(&ctx, md, 1) != 0) {
            mbedtls_md_free(&ctx);
            return false;
        }
        bool ok = (mbedtls_md_hmac_starts(&ctx, key, keyLen) == 0) &&
                  (mbedtls_md_hmac_update(&ctx, msg, msgLen) == 0) &&
                  (mbedtls_md_hmac_finish(&ctx, out) == 0);
        mbedtls_md_free(&ctx);
        return ok;
    }

    void loadState() {
        if (!_prefsReady) _prefsReady = _prefs.begin("u2f", false);
        if (!_prefsReady) return;

        size_t got = _prefs.getBytes("master", _masterSecret, sizeof(_masterSecret));
        if (got != sizeof(_masterSecret)) {
            esp_fill_random(_masterSecret, sizeof(_masterSecret));
            _prefs.putBytes("master", _masterSecret, sizeof(_masterSecret));
        }
        _masterLoaded = true;
        _counter = _prefs.getUInt("ctr", 0);
    }

    bool getCredentialFs(FS *&fs) {
        fs = nullptr;
        if (setupSdCard()) {
            fs = &SD;
            return true;
        }
        if (!LittleFS.begin()) return false;
        fs = &LittleFS;
        return true;
    }

    bool ensureCredentialDir(FS &fs) {
        const String dir = "/fido_creds";
        if (fs.exists(dir)) return true;
        return fs.mkdir(dir);
    }

    bool writeCredentialFile(FS &fs, const CredentialRecord &r, const String &path) {
        File f = fs.open(path, FILE_WRITE);
        if (!f) return false;
        const uint8_t magic[4] = {'F', '2', 'C', '1'};
        uint8_t version = 2;
        f.write(magic, sizeof(magic));
        f.write(&version, 1);
        uint8_t idLen = r.credIdLen;
        if (idLen == 0 || idLen > sizeof(r.credId)) {
            f.close();
            return false;
        }
        f.write(&idLen, 1);
        f.write(r.credId, idLen);
        f.write(r.rpIdHash, sizeof(r.rpIdHash));
        uint8_t c[4] = {
            uint8_t((r.signCount >> 24) & 0xFF),
            uint8_t((r.signCount >> 16) & 0xFF),
            uint8_t((r.signCount >> 8) & 0xFF),
            uint8_t(r.signCount & 0xFF),
        };
        f.write(c, sizeof(c));
        f.close();
        return true;
    }

    bool readCredentialFile(FS &fs, const String &path, CredentialRecord &r) {
        File f = fs.open(path, FILE_READ);
        if (!f) return false;
        uint8_t magic[4] = {0};
        uint8_t version = 0;
        if (f.read(magic, 4) != 4 || memcmp(magic, "F2C1", 4) != 0) {
            f.close();
            return false;
        }
        if (f.read(&version, 1) != 1 || (version != 1 && version != 2)) {
            f.close();
            return false;
        }
        memset(r.credId, 0, sizeof(r.credId));
        r.credIdLen = 0;
        if (version == 1) {
            r.credIdLen = 16;
            if (f.read(r.credId, 16) != 16 ||
                f.read(r.rpIdHash, sizeof(r.rpIdHash)) != int(sizeof(r.rpIdHash))) {
                f.close();
                return false;
            }
        } else {
            uint8_t idLen = 0;
            if (f.read(&idLen, 1) != 1 || idLen == 0 || idLen > sizeof(r.credId)) {
                f.close();
                return false;
            }
            r.credIdLen = idLen;
            if (f.read(r.credId, idLen) != idLen ||
                f.read(r.rpIdHash, sizeof(r.rpIdHash)) != int(sizeof(r.rpIdHash))) {
                f.close();
                return false;
            }
        }
        uint8_t c[4] = {0};
        if (f.read(c, 4) != 4) {
            f.close();
            return false;
        }
        r.signCount =
            (uint32_t(c[0]) << 24) | (uint32_t(c[1]) << 16) | (uint32_t(c[2]) << 8) | uint32_t(c[3]);
        r.path = path;
        f.close();
        return true;
    }

    bool saveNewCredential(CredentialRecord &r) {
        FS *fs = nullptr;
        if (!getCredentialFs(fs) || fs == nullptr) return false;
        if (!ensureCredentialDir(*fs)) return false;

        for (int i = 0; i < 32; i++) {
            String path = "/fido_creds/cred_" + String((uint32_t)esp_random(), HEX) + "_" +
                          String(millis(), HEX) + "_" + String(i) + ".bin";
            if (fs->exists(path)) continue;
            if (writeCredentialFile(*fs, r, path)) {
                r.path = path;
                return true;
            }
        }
        return false;
    }

    bool updateCredentialSignCount(const CredentialRecord &r) {
        if (r.path.length() == 0) return false;
        FS *fs = nullptr;
        if (!getCredentialFs(fs) || fs == nullptr) return false;
        if (!fs->exists(r.path)) return false;
        return writeCredentialFile(*fs, r, r.path);
    }

    bool findCredentialById(const uint8_t *credId, size_t credIdLen, CredentialRecord &out) {
        if (credId == nullptr || credIdLen == 0 || credIdLen > 64) return false;
        FS *fs = nullptr;
        if (!getCredentialFs(fs) || fs == nullptr) return false;
        if (!ensureCredentialDir(*fs)) return false;

        File dir = fs->open("/fido_creds", FILE_READ);
        if (!dir) return false;

        File entry = dir.openNextFile();
        while (entry) {
            String path = entry.path();
            entry.close();
            CredentialRecord c;
            if (readCredentialFile(*fs, path, c) && c.credIdLen == credIdLen &&
                memcmp(c.credId, credId, credIdLen) == 0) {
                out = c;
                dir.close();
                return true;
            }
            entry = dir.openNextFile();
        }
        dir.close();
        return false;
    }

    bool findFirstCredentialByRpHash(const uint8_t rpHash[32], CredentialRecord &out) {
        FS *fs = nullptr;
        if (!getCredentialFs(fs) || fs == nullptr) return false;
        if (!ensureCredentialDir(*fs)) return false;

        File dir = fs->open("/fido_creds", FILE_READ);
        if (!dir) return false;

        File entry = dir.openNextFile();
        while (entry) {
            String path = entry.path();
            entry.close();
            CredentialRecord c;
            if (readCredentialFile(*fs, path, c) && memcmp(c.rpIdHash, rpHash, 32) == 0) {
                out = c;
                dir.close();
                return true;
            }
            entry = dir.openNextFile();
        }
        dir.close();
        return false;
    }

    bool
    waitForUserPresence(uint32_t timeoutMs = 20000, bool sendKeepalive = false, uint32_t keepaliveCid = 0) {
        _waitingForPresence = true;

        auto isSelectPressedRaw = []() -> bool {
            bool pressed = false;
#ifdef SEL_BTN
            if (SEL_BTN >= 0 && digitalRead(SEL_BTN) == BTN_ACT) pressed = true;
#endif
#ifdef ENCODER_KEY
            if (ENCODER_KEY >= 0 && digitalRead(ENCODER_KEY) == BTN_ACT) pressed = true;
#endif
            return pressed;
        };

        auto isEscPressedRaw = []() -> bool {
            bool pressed = false;
#ifdef BK_BTN
            if (BK_BTN >= 0 && digitalRead(BK_BTN) == BTN_ACT) pressed = true;
#endif
            return pressed;
        };

        auto sendKeepaliveNonBlocking = [&](uint8_t status) {
            // Single-frame CTAPHID keepalive, sent with timeout=0 to avoid blocking
            if (keepaliveCid == 0) return;
            uint8_t report[64] = {0};
            report[0] = (keepaliveCid >> 24) & 0xFF;
            report[1] = (keepaliveCid >> 16) & 0xFF;
            report[2] = (keepaliveCid >> 8) & 0xFF;
            report[3] = keepaliveCid & 0xFF;
            report[4] = CTAPHID_KEEPALIVE;
            report[5] = 0x00;
            report[6] = 0x01;
            report[7] = status;
            _hid.SendReport(kU2fReportId, report, 64, 0);
        };

        // Debounce raw selection pin directly. This avoids timing/latch issues

        uint8_t pressedTicks = isSelectPressedRaw() ? 1 : 0;
        uint32_t lastKeepalive = 0;
        const uint32_t start = millis();
        while ((millis() - start) < timeoutMs && !returnToMenu) {
            wakeUpScreen();

            if (sendKeepalive && keepaliveCid != 0 && (millis() - lastKeepalive) >= 100) {
                sendKeepaliveNonBlocking(0x02); // STATUS_UPNEEDED
                lastKeepalive = millis();
            }

            bool selNow = isSelectPressedRaw();
            if (EscPress || isEscPressedRaw()) {
                EscPress = false;
                _waitingForPresence = false;
                return false;
            }

            if (selNow) {
                pressedTicks = (pressedTicks < 50) ? uint8_t(pressedTicks + 1) : pressedTicks;
            } else {
                pressedTicks = 0;
            }
            // Require ~30ms stable press (3 * 10ms) to debounce.
            if (pressedTicks >= 3) {
                _waitingForPresence = false;
                return true;
            }
            delay(10);
        }

        _waitingForPresence = false;
        return false;
    }

    bool sendReportRaw(const uint8_t report[64]) { return _hid.SendReport(kU2fReportId, report, 64); }

    bool sendMessage(uint32_t cid, uint8_t cmd, const uint8_t *payload, uint16_t len) {
        uint8_t report[64] = {0};

        report[0] = (cid >> 24) & 0xFF;
        report[1] = (cid >> 16) & 0xFF;
        report[2] = (cid >> 8) & 0xFF;
        report[3] = cid & 0xFF;
        report[4] = cmd;
        report[5] = (len >> 8) & 0xFF;
        report[6] = len & 0xFF;

        uint16_t offset = 0;
        uint16_t first = (len < kInitPayloadSize) ? len : kInitPayloadSize;
        if (first > 0 && payload != nullptr) memcpy(report + 7, payload, first);
        if (!sendReportRaw(report)) return false;
        offset += first;

        uint8_t seq = 0;
        while (offset < len) {
            memset(report, 0, sizeof(report));
            report[0] = (cid >> 24) & 0xFF;
            report[1] = (cid >> 16) & 0xFF;
            report[2] = (cid >> 8) & 0xFF;
            report[3] = cid & 0xFF;
            report[4] = seq++;
            uint16_t chunk = ((len - offset) < kContPayloadSize) ? (len - offset) : kContPayloadSize;
            memcpy(report + 5, payload + offset, chunk);
            if (!sendReportRaw(report)) return false;
            offset += chunk;
        }
        return true;
    }

    void sendError(uint32_t cid, uint8_t code) {
        uint8_t p[1] = {code};
        sendMessage(cid, CTAPHID_ERROR, p, sizeof(p));
    }

    void sendApduStatus(uint32_t cid, uint16_t sw) {
        uint8_t p[2] = {uint8_t((sw >> 8) & 0xFF), uint8_t(sw & 0xFF)};
        sendMessage(cid, CTAPHID_MSG, p, sizeof(p));
    }

    void sendApduDataWithStatus(uint32_t cid, const uint8_t *data, size_t dataLen, uint16_t sw) {
        if (dataLen + 2 > kMaxMessageSize) {
            sendError(cid, CTAP1_ERR_INVALID_LEN);
            return;
        }
        std::vector<uint8_t> out;
        out.reserve(dataLen + 2);
        for (size_t i = 0; i < dataLen; i++) out.push_back(data[i]);
        out.push_back(uint8_t((sw >> 8) & 0xFF));
        out.push_back(uint8_t(sw & 0xFF));
        sendMessage(cid, CTAPHID_MSG, out.data(), static_cast<uint16_t>(out.size()));
    }

    void resetRx() { _rx.active = false; }

    bool queueCommand(uint32_t cid, uint8_t cmd, const uint8_t *payload, uint16_t len) {
        if (_pending.ready) return false;
        _pending.cid = cid;
        _pending.cmd = cmd;
        _pending.len = len;
        if (len > 0 && payload != nullptr) memcpy(_pending.data, payload, len);
        _pending.ready = true;
        return true;
    }

    void handleInitFrame(uint32_t cid, const uint8_t *buffer, uint16_t len) {
        const uint8_t cmd = buffer[4];
        const uint16_t bc = (uint16_t(buffer[5]) << 8) | uint16_t(buffer[6]);
        if (bc > kMaxMessageSize) {
            sendError(cid, CTAP1_ERR_INVALID_LEN);
            resetRx();
            return;
        }

        _rx.active = true;
        _rx.cid = cid;
        _rx.cmd = cmd;
        _rx.expectedLen = bc;
        _rx.receivedLen = 0;
        _rx.nextSeq = 0;

        uint16_t chunk = (len > 7) ? (len - 7) : 0;
        if (chunk > bc) chunk = bc;
        if (chunk > 0) {
            memcpy(_rx.data, buffer + 7, chunk);
            _rx.receivedLen = chunk;
        }

        if (_rx.receivedLen >= _rx.expectedLen) {
            queueCommand(_rx.cid, _rx.cmd, _rx.data, _rx.expectedLen);
            resetRx();
        }
    }

    void handleContFrame(uint32_t cid, const uint8_t *buffer, uint16_t len) {
        if (!_rx.active || cid != _rx.cid) {
            sendError(cid, CTAP1_ERR_INVALID_CID);
            return;
        }
        if (len < 6) {
            sendError(cid, CTAP1_ERR_INVALID_LEN);
            resetRx();
            return;
        }
        uint8_t seq = buffer[4];
        if (seq != _rx.nextSeq) {
            sendError(cid, CTAP1_ERR_INVALID_SEQ);
            resetRx();
            return;
        }
        uint16_t remaining = _rx.expectedLen - _rx.receivedLen;
        uint16_t chunk = (len > 5) ? (len - 5) : 0;
        if (chunk > remaining) chunk = remaining;
        if (chunk > 0) {
            memcpy(_rx.data + _rx.receivedLen, buffer + 5, chunk);
            _rx.receivedLen += chunk;
        }
        _rx.nextSeq++;

        if (_rx.receivedLen >= _rx.expectedLen) {
            queueCommand(_rx.cid, _rx.cmd, _rx.data, _rx.expectedLen);
            resetRx();
        }
    }

    bool derivePrivateScalar(
        const char *label, const uint8_t appParam[32], const uint8_t nonce[kKeyHandleNonceLen],
        uint8_t outPriv[32]
    ) {
        if (!_masterLoaded) return false;

        uint8_t msg[8 + 32 + kKeyHandleNonceLen + 1] = {0};
        size_t labelLen = strlen(label);
        if (labelLen > 8) labelLen = 8;
        memcpy(msg, label, labelLen);
        memcpy(msg + 8, appParam, 32);
        memcpy(msg + 8 + 32, nonce, kKeyHandleNonceLen);

        mbedtls_ecp_group grp;
        mbedtls_mpi d;
        mbedtls_ecp_group_init(&grp);
        mbedtls_mpi_init(&d);

        if (mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1) != 0) {
            mbedtls_mpi_free(&d);
            mbedtls_ecp_group_free(&grp);
            return false;
        }

        for (uint8_t ctr = 0; ctr < 16; ctr++) {
            msg[sizeof(msg) - 1] = ctr;
            uint8_t h[32] = {0};
            if (!hmacSha256(_masterSecret, sizeof(_masterSecret), msg, sizeof(msg), h)) break;
            if (mbedtls_mpi_read_binary(&d, h, sizeof(h)) != 0) break;
            if (mbedtls_ecp_check_privkey(&grp, &d) == 0) {
                bool ok = (mbedtls_mpi_write_binary(&d, outPriv, 32) == 0);
                mbedtls_mpi_free(&d);
                mbedtls_ecp_group_free(&grp);
                return ok;
            }
        }

        mbedtls_mpi_free(&d);
        mbedtls_ecp_group_free(&grp);
        return false;
    }

    bool computePublicKey(const uint8_t priv[32], uint8_t outPub[65]) {
        mbedtls_ecp_group grp;
        mbedtls_mpi d;
        mbedtls_ecp_point q;
        mbedtls_ecp_point g;
        mbedtls_ecp_group_init(&grp);
        mbedtls_mpi_init(&d);
        mbedtls_ecp_point_init(&q);
        mbedtls_ecp_point_init(&g);

        bool ok = (mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1) == 0) &&
                  (mbedtls_mpi_read_binary(&d, priv, 32) == 0) && loadP256Generator(&g) &&
                  (mbedtls_ecp_mul(&grp, &q, &d, &g, rngCallback, nullptr) == 0);
        if (ok) {
            size_t olen = 0;
            ok = (mbedtls_ecp_point_write_binary(&grp, &q, MBEDTLS_ECP_PF_UNCOMPRESSED, &olen, outPub, 65) ==
                  0) &&
                 (olen == 65);
        }

        mbedtls_ecp_point_free(&g);
        mbedtls_ecp_point_free(&q);
        mbedtls_mpi_free(&d);
        mbedtls_ecp_group_free(&grp);
        return ok;
    }

    bool signWithPrivate(
        const uint8_t priv[32], const uint8_t hash[32], uint8_t *outSig, size_t outCap, size_t &outLen
    ) {
        mbedtls_ecdsa_context ecdsa;
        mbedtls_ecdsa_init(&ecdsa);
        bool ok = (mbedtls_ecp_group_load(&ecdsa.MBEDTLS_PRIVATE(grp), MBEDTLS_ECP_DP_SECP256R1) == 0) &&
                  (mbedtls_mpi_read_binary(&ecdsa.MBEDTLS_PRIVATE(d), priv, 32) == 0) &&
                  (mbedtls_ecdsa_write_signature(
                       &ecdsa, MBEDTLS_MD_SHA256, hash, 32, outSig, outCap, &outLen, rngCallback, nullptr
                   ) == 0);
        mbedtls_ecdsa_free(&ecdsa);
        return ok;
    }

    void buildKeyHandle(
        const uint8_t appParam[32], const uint8_t nonce[kKeyHandleNonceLen], uint8_t outHandle[kKeyHandleLen]
    ) {
        outHandle[0] = kKeyHandleVersion;
        memcpy(outHandle + 1, nonce, kKeyHandleNonceLen);

        uint8_t msg[1 + 32 + kKeyHandleNonceLen] = {0};
        msg[0] = kKeyHandleVersion;
        memcpy(msg + 1, appParam, 32);
        memcpy(msg + 1 + 32, nonce, kKeyHandleNonceLen);

        uint8_t mac[32] = {0};
        hmacSha256(_masterSecret, sizeof(_masterSecret), msg, sizeof(msg), mac);
        memcpy(outHandle + 1 + kKeyHandleNonceLen, mac, kKeyHandleMacLen);
    }

    bool parseAndValidateKeyHandle(
        const uint8_t appParam[32], const uint8_t *keyHandle, size_t keyHandleLen,
        uint8_t outNonce[kKeyHandleNonceLen]
    ) {
        if (keyHandleLen != kKeyHandleLen) return false;
        if (keyHandle[0] != kKeyHandleVersion) return false;

        uint8_t expected[kKeyHandleLen] = {0};
        memcpy(outNonce, keyHandle + 1, kKeyHandleNonceLen);
        buildKeyHandle(appParam, outNonce, expected);
        return constantTimeEqual(keyHandle, expected, kKeyHandleLen);
    }

    bool ensureAttestationCert(const uint8_t attPriv[32]) {
        if (!_attestationCertDer.empty()) return true;

        mbedtls_pk_context pk;
        mbedtls_x509write_cert crt;
        mbedtls_entropy_context entropy;
        mbedtls_ctr_drbg_context ctr;
        uint8_t serialRaw[16] = {0};
        uint8_t der[1024] = {0};
        int len = 0;
        int rc = 0;

        mbedtls_pk_init(&pk);
        mbedtls_x509write_crt_init(&crt);
        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&ctr);

        bool ok = false;
        const char *pers = "u2f-att";
        do {
            rc = mbedtls_ctr_drbg_seed(
                &ctr, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers)
            );
            if (rc != 0) break;

            rc = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
            if (rc != 0) break;

            {
                mbedtls_ecp_keypair *ec = mbedtls_pk_ec(pk);
                mbedtls_ecp_point g;
                mbedtls_ecp_point_init(&g);
                rc = mbedtls_ecp_group_load(&ec->MBEDTLS_PRIVATE(grp), MBEDTLS_ECP_DP_SECP256R1);
                if (rc == 0) rc = mbedtls_mpi_read_binary(&ec->MBEDTLS_PRIVATE(d), attPriv, 32);
                if (rc == 0 && !loadP256Generator(&g)) rc = -1;
                if (rc == 0) {
                    rc = mbedtls_ecp_mul(
                        &ec->MBEDTLS_PRIVATE(grp),
                        &ec->MBEDTLS_PRIVATE(Q),
                        &ec->MBEDTLS_PRIVATE(d),
                        &g,
                        rngCallback,
                        nullptr
                    );
                }
                mbedtls_ecp_point_free(&g);
                if (rc != 0) break;
            }

            esp_fill_random(serialRaw, sizeof(serialRaw));
            serialRaw[0] |= 0x01;

            mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
            mbedtls_x509write_crt_set_subject_key(&crt, &pk);
            mbedtls_x509write_crt_set_issuer_key(&crt, &pk);
            mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
            rc = mbedtls_x509write_crt_set_serial_raw(&crt, serialRaw, sizeof(serialRaw));
            if (rc != 0) break;
            rc = mbedtls_x509write_crt_set_subject_name(&crt, "CN=Bruce U2F Attestation,O=BruceDevices,C=US");
            if (rc != 0) break;
            rc = mbedtls_x509write_crt_set_issuer_name(&crt, "CN=Bruce U2F Attestation,O=BruceDevices,C=US");
            if (rc != 0) break;
            rc = mbedtls_x509write_crt_set_validity(&crt, "20260101000000", "20360101000000");
            if (rc != 0) break;
            rc = mbedtls_x509write_crt_set_basic_constraints(&crt, 0, -1);
            if (rc != 0) break;
            rc = mbedtls_x509write_crt_set_key_usage(&crt, MBEDTLS_X509_KU_DIGITAL_SIGNATURE);
            if (rc != 0) break;

            len = mbedtls_x509write_crt_der(&crt, der, sizeof(der), mbedtls_ctr_drbg_random, &ctr);
            if (len <= 0) break;

            _attestationCertDer.assign(der + sizeof(der) - len, der + sizeof(der));
            ok = true;
        } while (false);

        mbedtls_ctr_drbg_free(&ctr);
        mbedtls_entropy_free(&entropy);
        mbedtls_x509write_crt_free(&crt);
        mbedtls_pk_free(&pk);
        return ok;
    }

    uint32_t nextCounter() {
        _counter++;
        if (_prefsReady) _prefs.putUInt("ctr", _counter);
        return _counter;
    }

    void handleRegister(uint32_t cid, const uint8_t *data, uint32_t lc) {
        if (lc != 64) {
            sendApduStatus(cid, SW_WRONG_DATA);
            return;
        }
        if (!waitForUserPresence()) {
            sendApduStatus(cid, SW_CONDITIONS_NOT_SATISFIED);
            return;
        }

        const uint8_t *challengeParam = data;
        const uint8_t *appParam = data + 32;

        uint8_t nonce[kKeyHandleNonceLen] = {0};
        esp_fill_random(nonce, sizeof(nonce));

        uint8_t userPriv[32] = {0};
        uint8_t userPub[65] = {0};
        uint8_t keyHandle[kKeyHandleLen] = {0};
        if (!derivePrivateScalar("U2FKEY", appParam, nonce, userPriv) ||
            !computePublicKey(userPriv, userPub)) {
            sendError(cid, CTAP1_ERR_OTHER);
            return;
        }
        buildKeyHandle(appParam, nonce, keyHandle);

        uint8_t attApp[32] = {0};
        uint8_t attNonce[kKeyHandleNonceLen] = {0};
        uint8_t attPriv[32] = {0};
        if (!derivePrivateScalar("ATTEST", attApp, attNonce, attPriv) || !ensureAttestationCert(attPriv)) {
            sendError(cid, CTAP1_ERR_OTHER);
            return;
        }

        uint8_t signMsg[1 + 32 + 32 + kKeyHandleLen + 65] = {0};
        size_t pos = 0;
        signMsg[pos++] = 0x00;
        memcpy(signMsg + pos, appParam, 32);
        pos += 32;
        memcpy(signMsg + pos, challengeParam, 32);
        pos += 32;
        memcpy(signMsg + pos, keyHandle, kKeyHandleLen);
        pos += kKeyHandleLen;
        memcpy(signMsg + pos, userPub, 65);
        pos += 65;

        uint8_t hash[32] = {0};
        if (!sha256(signMsg, pos, hash)) {
            sendError(cid, CTAP1_ERR_OTHER);
            return;
        }

        uint8_t sig[80] = {0};
        size_t sigLen = 0;
        if (!signWithPrivate(attPriv, hash, sig, sizeof(sig), sigLen)) {
            sendError(cid, CTAP1_ERR_OTHER);
            return;
        }

        std::vector<uint8_t> resp;
        resp.reserve(1 + 65 + 1 + kKeyHandleLen + _attestationCertDer.size() + sigLen + 2);
        resp.push_back(0x05);
        for (size_t i = 0; i < sizeof(userPub); i++) resp.push_back(userPub[i]);
        resp.push_back(static_cast<uint8_t>(kKeyHandleLen));
        for (size_t i = 0; i < sizeof(keyHandle); i++) resp.push_back(keyHandle[i]);
        resp.insert(resp.end(), _attestationCertDer.begin(), _attestationCertDer.end());
        for (size_t i = 0; i < sigLen; i++) resp.push_back(sig[i]);

        sendApduDataWithStatus(cid, resp.data(), static_cast<uint16_t>(resp.size()), SW_NO_ERROR);
    }

    void handleAuthenticate(uint32_t cid, uint8_t p1, const uint8_t *data, uint32_t lc) {
        if (lc < 65) {
            sendApduStatus(cid, SW_WRONG_DATA);
            return;
        }

        const uint8_t *challengeParam = data;
        const uint8_t *appParam = data + 32;
        uint8_t keyHandleLen = data[64];
        if (lc != (65u + keyHandleLen)) {
            sendApduStatus(cid, SW_WRONG_LENGTH);
            return;
        }
        const uint8_t *keyHandle = data + 65;

        uint8_t nonce[kKeyHandleNonceLen] = {0};
        bool keyOk = parseAndValidateKeyHandle(appParam, keyHandle, keyHandleLen, nonce);

        if (p1 == U2F_AUTH_CHECK_ONLY) {
            sendApduStatus(cid, keyOk ? SW_CONDITIONS_NOT_SATISFIED : SW_WRONG_DATA);
            return;
        }
        if (p1 != U2F_AUTH_ENFORCE && p1 != U2F_AUTH_DONT_ENFORCE) {
            sendApduStatus(cid, SW_WRONG_DATA);
            return;
        }
        if (!keyOk) {
            sendApduStatus(cid, SW_WRONG_DATA);
            return;
        }
        if (!waitForUserPresence()) {
            sendApduStatus(cid, SW_CONDITIONS_NOT_SATISFIED);
            return;
        }

        uint8_t userPriv[32] = {0};
        if (!derivePrivateScalar("U2FKEY", appParam, nonce, userPriv)) {
            sendError(cid, CTAP1_ERR_OTHER);
            return;
        }

        uint8_t userPresence = 0x01;
        uint32_t counter = nextCounter();
        uint8_t counterBe[4] = {
            uint8_t((counter >> 24) & 0xFF),
            uint8_t((counter >> 16) & 0xFF),
            uint8_t((counter >> 8) & 0xFF),
            uint8_t(counter & 0xFF),
        };

        uint8_t signMsg[32 + 1 + 4 + 32] = {0};
        size_t pos = 0;
        memcpy(signMsg + pos, appParam, 32);
        pos += 32;
        signMsg[pos++] = userPresence;
        memcpy(signMsg + pos, counterBe, sizeof(counterBe));
        pos += sizeof(counterBe);
        memcpy(signMsg + pos, challengeParam, 32);
        pos += 32;

        uint8_t hash[32] = {0};
        if (!sha256(signMsg, pos, hash)) {
            sendError(cid, CTAP1_ERR_OTHER);
            return;
        }

        uint8_t sig[80] = {0};
        size_t sigLen = 0;
        if (!signWithPrivate(userPriv, hash, sig, sizeof(sig), sigLen)) {
            sendError(cid, CTAP1_ERR_OTHER);
            return;
        }

        std::vector<uint8_t> resp;
        resp.reserve(1 + 4 + sigLen);
        resp.push_back(userPresence);
        resp.push_back(counterBe[0]);
        resp.push_back(counterBe[1]);
        resp.push_back(counterBe[2]);
        resp.push_back(counterBe[3]);
        for (size_t i = 0; i < sigLen; i++) resp.push_back(sig[i]);

        sendApduDataWithStatus(cid, resp.data(), static_cast<uint16_t>(resp.size()), SW_NO_ERROR);
    }

    bool parseMakeCredentialRequest(
        const uint8_t *payload, uint16_t len, uint8_t clientDataHash[32], uint8_t rpIdHash[32]
    ) {
        CborCursor c{payload, payload + len};
        uint8_t major = 0, add = 0;
        uint32_t pairs = 0;
        if (!cborReadHead(c, major, add) || major != 5 || !cborReadLen(c, add, pairs)) return false;

        String rpId = "";
        bool gotClientHash = false;
        bool gotRpId = false;

        for (uint32_t i = 0; i < pairs; i++) {
            uint32_t key = 0;
            if (!cborReadUint(c, key)) return false;

            if (key == 1) {
                const uint8_t *b = nullptr;
                uint32_t bl = 0;
                if (!cborReadBytes(c, b, bl) || bl != 32) return false;
                memcpy(clientDataHash, b, 32);
                gotClientHash = true;
                continue;
            }

            if (key == 2) {
                uint8_t m = 0, a = 0;
                uint32_t rpPairs = 0;
                if (!cborReadHead(c, m, a) || m != 5 || !cborReadLen(c, a, rpPairs)) return false;
                for (uint32_t r = 0; r < rpPairs; r++) {
                    String k = "";
                    if (!cborReadText(c, k)) return false;
                    if (k == "id") {
                        if (!cborReadText(c, rpId)) return false;
                        gotRpId = true;
                    } else {
                        if (!cborSkip(c)) return false;
                    }
                }
                continue;
            }

            if (!cborSkip(c)) return false;
        }

        if (!gotClientHash || !gotRpId || rpId.length() == 0) return false;
        return sha256((const uint8_t *)rpId.c_str(), rpId.length(), rpIdHash);
    }

    bool parseGetAssertionRequest(
        const uint8_t *payload, uint16_t len, uint8_t clientDataHash[32], uint8_t rpIdHash[32],
        uint8_t outAllowId[64], size_t &outAllowIdLen, bool &hasAllowId
    ) {
        CborCursor c{payload, payload + len};
        uint8_t major = 0, add = 0;
        uint32_t pairs = 0;
        if (!cborReadHead(c, major, add) || major != 5 || !cborReadLen(c, add, pairs)) return false;

        String rpId = "";
        bool gotClientHash = false;
        bool gotRpId = false;
        hasAllowId = false;
        outAllowIdLen = 0;
        bool preferredAllowIdFound = false;

        for (uint32_t i = 0; i < pairs; i++) {
            uint32_t key = 0;
            if (!cborReadUint(c, key)) return false;

            if (key == 1) {
                if (!cborReadText(c, rpId)) return false;
                gotRpId = true;
                continue;
            }
            if (key == 2) {
                const uint8_t *b = nullptr;
                uint32_t bl = 0;
                if (!cborReadBytes(c, b, bl) || bl != 32) return false;
                memcpy(clientDataHash, b, 32);
                gotClientHash = true;
                continue;
            }
            if (key == 3) {
                uint8_t am = 0, aa = 0;
                uint32_t n = 0;
                if (!cborReadHead(c, am, aa) || am != 4 || !cborReadLen(c, aa, n)) return false;
                if (n > 0) {
                    for (uint32_t item = 0; item < n; item++) {
                        uint8_t dm = 0, da = 0;
                        uint32_t dPairs = 0;
                        if (!cborReadHead(c, dm, da) || dm != 5 || !cborReadLen(c, da, dPairs)) return false;

                        const uint8_t *candidateId = nullptr;
                        uint32_t candidateLen = 0;

                        for (uint32_t d = 0; d < dPairs; d++) {
                            String dk = "";
                            if (!cborReadText(c, dk)) return false;
                            if (dk == "id") {
                                if (!cborReadBytes(c, candidateId, candidateLen)) return false;
                            } else {
                                if (!cborSkip(c)) return false;
                            }
                        }

                        if (candidateId == nullptr || candidateLen == 0 || candidateLen > 64) continue;

                        // Fallback: keep first valid ID.
                        if (!hasAllowId) {
                            memcpy(outAllowId, candidateId, candidateLen);
                            outAllowIdLen = candidateLen;
                            hasAllowId = true;
                        }

                        // Prefer our stateless key-handle sized credential ID.
                        if (!preferredAllowIdFound && candidateLen == kKeyHandleLen &&
                            candidateId[0] == kKeyHandleVersion) {
                            memcpy(outAllowId, candidateId, candidateLen);
                            outAllowIdLen = candidateLen;
                            hasAllowId = true;
                            preferredAllowIdFound = true;
                        }
                    }
                }
                continue;
            }
            if (!cborSkip(c)) return false;
        }

        if (!gotClientHash || !gotRpId || rpId.length() == 0) return false;
        return sha256((const uint8_t *)rpId.c_str(), rpId.length(), rpIdHash);
    }

    void buildCoseEc2PublicKey(const uint8_t pubkey65[65], std::vector<uint8_t> &out) {
        // COSE_Key: {1:2,3:-7,-1:1,-2:x,-3:y}
        cborAppendMap(out, 5);
        cborAppendUint(out, 1);
        cborAppendUint(out, 2);
        cborAppendUint(out, 3);
        cborAppendInt(out, -7);
        cborAppendInt(out, -1);
        cborAppendUint(out, 1);
        cborAppendInt(out, -2);
        cborAppendBytes(out, pubkey65 + 1, 32);
        cborAppendInt(out, -3);
        cborAppendBytes(out, pubkey65 + 33, 32);
    }

    void handleCtap2MakeCredential(uint32_t cid, const uint8_t *payload, uint16_t len) {
        uint8_t clientDataHash[32] = {0};
        uint8_t rpIdHash[32] = {0};
        if (!parseMakeCredentialRequest(payload, len, clientDataHash, rpIdHash)) {
            uint8_t err[1] = {CTAP2_ERR_INVALID_CBOR};
            sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
            return;
        }

        if (!waitForUserPresence(20000, true, cid)) {
            uint8_t err[1] = {CTAP2_ERR_OPERATION_DENIED};
            sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
            return;
        }

        uint8_t nonce[kKeyHandleNonceLen] = {0};
        uint8_t credId[kKeyHandleLen] = {0};
        esp_fill_random(nonce, sizeof(nonce));
        buildKeyHandle(rpIdHash, nonce, credId);
        CredentialRecord rec;
        memcpy(rec.rpIdHash, rpIdHash, 32);
        memcpy(rec.credId, credId, kKeyHandleLen);
        rec.credIdLen = kKeyHandleLen;
        rec.signCount = 0;

        uint8_t userPriv[32] = {0};
        uint8_t userPub[65] = {0};
        if (!derivePrivateScalar("F2KEY", rpIdHash, nonce, userPriv) ||
            !computePublicKey(userPriv, userPub)) {
            uint8_t err[1] = {CTAP1_ERR_OTHER};
            sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
            return;
        }
        if (!saveNewCredential(rec)) {
            uint8_t err[1] = {CTAP1_ERR_OTHER};
            sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
            return;
        }

        std::vector<uint8_t> authData;
        authData.reserve(32 + 1 + 4 + 16 + 2 + kKeyHandleLen + 80);
        for (int i = 0; i < 32; i++) authData.push_back(rpIdHash[i]);
        authData.push_back(0x41); // UP + AT
        authData.push_back(0x00);
        authData.push_back(0x00);
        authData.push_back(0x00);
        authData.push_back(0x00);                              // signCount
        for (int i = 0; i < 16; i++) authData.push_back(0x00); // AAGUID
        authData.push_back(uint8_t((kKeyHandleLen >> 8) & 0xFF));
        authData.push_back(uint8_t(kKeyHandleLen & 0xFF));
        for (int i = 0; i < int(kKeyHandleLen); i++) authData.push_back(credId[i]);
        buildCoseEc2PublicKey(userPub, authData);

        std::vector<uint8_t> respCbor;
        cborAppendMap(respCbor, 3);
        cborAppendUint(respCbor, 1);
        cborAppendText(respCbor, "none");
        cborAppendUint(respCbor, 2);
        cborAppendBytes(respCbor, authData.data(), authData.size());
        cborAppendUint(respCbor, 3);
        cborAppendMap(respCbor, 0); // attStmt

        std::vector<uint8_t> out;
        out.reserve(1 + respCbor.size());
        out.push_back(CTAP2_OK);
        out.insert(out.end(), respCbor.begin(), respCbor.end());
        sendMessage(cid, CTAPHID_CBOR, out.data(), uint16_t(out.size()));
    }

    void handleCtap2GetAssertion(uint32_t cid, const uint8_t *payload, uint16_t len) {
        uint8_t clientDataHash[32] = {0};
        uint8_t rpIdHash[32] = {0};
        uint8_t allowId[64] = {0};
        size_t allowIdLen = 0;
        bool hasAllowId = false;
        if (!parseGetAssertionRequest(
                payload, len, clientDataHash, rpIdHash, allowId, allowIdLen, hasAllowId
            )) {
            uint8_t err[1] = {CTAP2_ERR_INVALID_CBOR};
            sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
            return;
        }

        uint8_t selectedId[64] = {0};
        size_t selectedIdLen = 0;
        uint8_t nonce[kKeyHandleNonceLen] = {0};

        if (hasAllowId && allowIdLen > 0) {
            memcpy(selectedId, allowId, allowIdLen);
            selectedIdLen = allowIdLen;
            if (!parseAndValidateKeyHandle(rpIdHash, selectedId, selectedIdLen, nonce)) {
                uint8_t err[1] = {CTAP2_ERR_NO_CREDENTIALS};
                sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
                return;
            }
        } else {
            // Fallback for flows without allowList: use first persisted credential for RP.
            CredentialRecord rec;
            if (!findFirstCredentialByRpHash(rpIdHash, rec) || rec.credIdLen == 0) {
                uint8_t err[1] = {CTAP2_ERR_NO_CREDENTIALS};
                sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
                return;
            }
            memcpy(selectedId, rec.credId, rec.credIdLen);
            selectedIdLen = rec.credIdLen;
            if (!parseAndValidateKeyHandle(rpIdHash, selectedId, selectedIdLen, nonce)) {
                uint8_t err[1] = {CTAP2_ERR_NO_CREDENTIALS};
                sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
                return;
            }
        }

        if (!waitForUserPresence(20000, true, cid)) {
            uint8_t err[1] = {CTAP2_ERR_OPERATION_DENIED};
            sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
            return;
        }

        uint8_t priv[32] = {0};
        if (!derivePrivateScalar("F2KEY", rpIdHash, nonce, priv)) {
            uint8_t err[1] = {CTAP1_ERR_OTHER};
            sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
            return;
        }

        uint32_t signCount = nextCounter();

        uint8_t authData[37] = {0};
        memcpy(authData, rpIdHash, 32);
        authData[32] = 0x01; // UP
        authData[33] = uint8_t((signCount >> 24) & 0xFF);
        authData[34] = uint8_t((signCount >> 16) & 0xFF);
        authData[35] = uint8_t((signCount >> 8) & 0xFF);
        authData[36] = uint8_t(signCount & 0xFF);

        uint8_t toSign[69] = {0};
        memcpy(toSign, authData, 37);
        memcpy(toSign + 37, clientDataHash, 32);
        uint8_t hash[32] = {0};
        if (!sha256(toSign, sizeof(toSign), hash)) {
            uint8_t err[1] = {CTAP1_ERR_OTHER};
            sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
            return;
        }

        uint8_t sig[80] = {0};
        size_t sigLen = 0;
        if (!signWithPrivate(priv, hash, sig, sizeof(sig), sigLen)) {
            uint8_t err[1] = {CTAP1_ERR_OTHER};
            sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
            return;
        }

        std::vector<uint8_t> respCbor;
        cborAppendMap(respCbor, 4);
        cborAppendUint(respCbor, 1);
        cborAppendMap(respCbor, 2);
        cborAppendText(respCbor, "type");
        cborAppendText(respCbor, "public-key");
        cborAppendText(respCbor, "id");
        cborAppendBytes(respCbor, selectedId, selectedIdLen);
        cborAppendUint(respCbor, 2);
        cborAppendBytes(respCbor, authData, sizeof(authData));
        cborAppendUint(respCbor, 3);
        cborAppendBytes(respCbor, sig, sigLen);
        cborAppendUint(respCbor, 5);
        cborAppendUint(respCbor, 1);

        std::vector<uint8_t> out;
        out.reserve(1 + respCbor.size());
        out.push_back(CTAP2_OK);
        out.insert(out.end(), respCbor.begin(), respCbor.end());
        sendMessage(cid, CTAPHID_CBOR, out.data(), uint16_t(out.size()));
    }

    void handleU2fApdu(uint32_t cid, const uint8_t *payload, uint16_t len) {
        if (len < 7) {
            sendApduStatus(cid, SW_WRONG_LENGTH);
            return;
        }

        uint8_t cla = payload[0];
        uint8_t ins = payload[1];
        uint8_t p1 = payload[2];
        uint32_t lc = (uint32_t(payload[4]) << 16) | (uint32_t(payload[5]) << 8) | uint32_t(payload[6]);

        if (cla != 0x00) {
            sendApduStatus(cid, SW_INS_NOT_SUPPORTED);
            return;
        }
        if (len < (7u + lc)) {
            sendApduStatus(cid, SW_WRONG_LENGTH);
            return;
        }
        const uint8_t *data = payload + 7;

        switch (ins) {
            case U2F_INS_VERSION: {
                static const uint8_t version[] = {'U', '2', 'F', '_', 'V', '2'};
                sendApduDataWithStatus(cid, version, sizeof(version), SW_NO_ERROR);
                return;
            }
            case U2F_INS_REGISTER: {
                handleRegister(cid, data, lc);
                return;
            }
            case U2F_INS_AUTHENTICATE: {
                handleAuthenticate(cid, p1, data, lc);
                return;
            }
            default: {
                sendApduStatus(cid, SW_INS_NOT_SUPPORTED);
                return;
            }
        }
    }

    void handleCbor(uint32_t cid, const uint8_t *payload, uint16_t len) {
        if (len < 1) {
            sendError(cid, CTAP1_ERR_INVALID_LEN);
            return;
        }
        const uint8_t cborCmd = payload[0];
        if (cborCmd == 0x04) {
            // CTAP2 authenticatorGetInfo: strict CTAP2.0-compatible profile.
            std::vector<uint8_t> info;
            cborAppendMap(info, 7);

            cborAppendUint(info, 0x01);
            cborAppendArray(info, 3);
            cborAppendText(info, "FIDO_2_1");
            cborAppendText(info, "FIDO_2_0");
            cborAppendText(info, "U2F_V2");

            cborAppendUint(info, 0x03);
            // Derive stable per-device AAGUID from master secret.
            uint8_t aaguid[16] = {0};
            if (_masterLoaded) {
                uint8_t h[32] = {0};
                if (sha256(_masterSecret, sizeof(_masterSecret), h)) memcpy(aaguid, h, sizeof(aaguid));
            }
            cborAppendBytes(info, aaguid, sizeof(aaguid));

            cborAppendUint(info, 0x04);
            cborAppendMap(info, 5);
            cborAppendText(info, "rk");
            cborAppendBool(info, true);
            cborAppendText(info, "up");
            cborAppendBool(info, true);
            cborAppendText(info, "uv");
            cborAppendBool(info, false);
            cborAppendText(info, "plat");
            cborAppendBool(info, false);
            cborAppendText(info, "makeCredUvNotRqd");
            cborAppendBool(info, true);

            cborAppendUint(info, 0x05);
            cborAppendUint(info, kMaxMessageSize);

            cborAppendUint(info, 0x08);
            cborAppendUint(info, 64); // max credential ID length

            cborAppendUint(info, 0x09);
            cborAppendArray(info, 1);
            cborAppendText(info, "usb");

            cborAppendUint(info, 0x0A);
            cborAppendArray(info, 1);
            cborAppendMap(info, 2);
            cborAppendText(info, "type");
            cborAppendText(info, "public-key");
            cborAppendText(info, "alg");
            cborAppendInt(info, -7); // ES256

            std::vector<uint8_t> out;
            out.reserve(1 + info.size());
            out.push_back(CTAP2_OK);
            out.insert(out.end(), info.begin(), info.end());
            sendMessage(cid, CTAPHID_CBOR, out.data(), uint16_t(out.size()));
            return;
        }

        // makeCredential
        if (cborCmd == 0x01) {
            handleCtap2MakeCredential(cid, payload + 1, len - 1);
            return;
        }

        // getAssertion
        if (cborCmd == 0x02) {
            handleCtap2GetAssertion(cid, payload + 1, len - 1);
            return;
        }

        // authenticatorSelection (CTAP2.1)
        if (cborCmd == 0x0B) {
            if (!waitForUserPresence(20000, true, cid)) {
                uint8_t err[1] = {CTAP2_ERR_OPERATION_DENIED};
                sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
                return;
            }
            uint8_t ok[1] = {CTAP2_OK};
            sendMessage(cid, CTAPHID_CBOR, ok, sizeof(ok));
            return;
        }

        // clientPIN: we do not support PIN; report explicit status.
        if (cborCmd == 0x06) {
            uint8_t err[1] = {CTAP2_ERR_INVALID_COMMAND};
            sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
            return;
        }

        // clientPIN / reset / selection / default
        uint8_t err[1] = {CTAP2_ERR_INVALID_COMMAND};
        sendMessage(cid, CTAPHID_CBOR, err, sizeof(err));
    }

    void handleCommand(uint32_t cid, uint8_t cmd, const uint8_t *payload, uint16_t len) {
        switch (cmd) {
            case CTAPHID_INIT: {
                if (len != 8) {
                    sendError(cid, CTAP1_ERR_INVALID_LEN);
                    return;
                }

                // Per CTAPHID, only broadcast CID allocates a new channel.
                // INIT on an existing CID is a sync command and must keep that CID.
                uint32_t assignedCid = cid;
                if (cid == kBroadcastCid) {
                    assignedCid = _nextCid++;
                    while (assignedCid == 0 || assignedCid == kBroadcastCid) assignedCid = _nextCid++;
                } else if (cid == 0) {
                    sendError(cid, CTAP1_ERR_INVALID_CID);
                    return;
                }

                uint8_t response[17] = {0};
                memcpy(response, payload, 8); // nonce echo
                response[8] = (assignedCid >> 24) & 0xFF;
                response[9] = (assignedCid >> 16) & 0xFF;
                response[10] = (assignedCid >> 8) & 0xFF;
                response[11] = assignedCid & 0xFF;
                response[12] = 2; // protocol version
                response[13] = 1; // fw major
                response[14] = 0; // fw minor
                response[15] = 0; // fw build
                response[16] = (CAPFLAG_WINK | CAPFLAG_CBOR);

                sendMessage(cid, CTAPHID_INIT, response, sizeof(response));
                return;
            }
            case CTAPHID_PING: {
                sendMessage(cid, CTAPHID_PING, payload, len);
                return;
            }
            case CTAPHID_WINK: {
                sendMessage(cid, CTAPHID_WINK, nullptr, 0);
                return;
            }
            case CTAPHID_LOCK: {
                // Optional command; accept and ignore lock for single-client usage.
                sendMessage(cid, CTAPHID_LOCK, nullptr, 0);
                return;
            }
            case CTAPHID_MSG: {
                handleU2fApdu(cid, payload, len);
                return;
            }
            case CTAPHID_CBOR: {
                handleCbor(cid, payload, len);
                return;
            }
            case CTAPHID_CANCEL: {
                // Host cancellation signal; acknowledge by clearing pending receive state.
                resetRx();
                return;
            }
            default: {
                if (cid == 0 || cid == kBroadcastCid) sendError(cid, CTAP1_ERR_INVALID_CID);
                else sendError(cid, CTAP1_ERR_INVALID_CMD);
                return;
            }
        }
    }
};

U2fHidDevice g_u2f;

void drawU2fStatusScreen() {
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(2);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(6, 8);
    tft.print("USB U2F");

    tft.setTextSize(1);
    tft.setCursor(6, 36);
    tft.print("Ready for registration/login");
    tft.setCursor(6, 50);
    tft.print("Press center when prompted");
    tft.setCursor(6, 64);
    tft.print("ESC: Back");
}

void updateU2fRuntimeInfo(const U2fHidDevice &device) {
    tft.fillRect(0, 84, tftWidth, tftHeight - 84, bruceConfig.bgColor);
    tft.setTextSize(2);
    tft.setCursor(6, 94);
    tft.print(device.waitingForPresence() ? "Confirm now" : "Waiting...");
    tft.setTextSize(1);
}

} // namespace

void u2f_setup() {
    g_u2f.begin();
    drawU2fStatusScreen();

    uint32_t lastDrawMs = 0;
    while (!check(EscPress) && !returnToMenu) {
        InputHandler();
        wakeUpScreen();
        g_u2f.poll();

        if (millis() - lastDrawMs > 250) {
            updateU2fRuntimeInfo(g_u2f);
            lastDrawMs = millis();
        }
        delay(10);
    }

    g_u2f.end();
}

#else

void u2f_setup() { displayError("USB HID disabled"); }

#endif
