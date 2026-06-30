#pragma once
#if !defined(LITE_VERSION)

#include "RFIDInterface.h"
#include "crypto1.h"
#include <SPI.h>
#include <Wire.h>
#include <rfal_nfc.h>
#include <rfal_rfst25r3916.h>
#include <vector>

class ST25R3916 : public RFIDInterface {
public:
    enum CONNECTION_TYPE { SPI_MODE = 0, I2C_MODE = 1 };

    static constexpr uint8_t I2C_ADDRESS = 0x50;

    ST25R3916(CONNECTION_TYPE connection_type = SPI_MODE);
    ~ST25R3916();

    bool begin() override;
    int read(int cardBaudRate = 0) override;
    int clone() override;
    int erase() override;
    int write(int cardBaudRate = 0) override;
    int write_ndef() override;
    int emulate() override;
    int load() override;
    int loadFromFile(const String &filepath) override;
    int save(String filename) override;
    int saveFlipper(String filename) override;

    void stopDiscovery();

    // Milestone 1 — advanced NFC-A info (NTAG / Ultralight)
    String ntagVariant;        // "NTAG213", "NTAG215", "NTAG216", "MF Ultralight", ...
    uint8_t ntagVersion[8];    // raw GET_VERSION response
    uint8_t ntagSignature[32]; // raw READ_SIG response (ECC-P256)
    uint32_t ntagCounters[3];  // monotonic counters 0/1/2
    uint8_t ntagTearing[3];    // tearing flags per counter
    bool ntagHasVersion = false;
    bool ntagHasSignature = false;
    bool ntagHasCounters = false;
    int _ntagPagesHint = 0; // page count derived from GET_VERSION (0 = unknown)

    // Milestone 5 — MIFARE Classic dump (1K / 4K / Mini)
    struct MifareClassicDump {
        uint8_t blocks[256][16];      // up to 256 blocks (4K)
        uint8_t keyA[40][6];          // recovered Key A per sector
        uint8_t keyB[40][6];          // recovered Key B per sector
        bool blockRead[256];          // which blocks were read successfully
        bool keyAFound[40];           // which Key A were found
        bool keyBFound[40];           // which Key B were found
        uint8_t sectors;              // 5 (Mini), 16 (1K), 40 (4K)
        uint16_t totalBlocks;         // 20, 64 or 256
    };
    MifareClassicDump mfcDump;
    bool mfcLoaded = false;           // true when mfcDump holds a valid MIFARE Classic dump
    String mfcType;                   // "Mini", "1K", "4K"

    bool isMifareClassicSak(uint8_t sak) const;

private:
    CONNECTION_TYPE _connection_type;
    RfalRfST25R3916Class *_hw = nullptr;
    RfalNfcClass *_nfc = nullptr;
    SPIClass *_spi = nullptr;
    bool _discoveryStarted = false;

    bool _initSPI();
    bool _initI2C();
    bool _startDiscovery();
    bool _pollForTag(rfalNfcDevice **dev, uint32_t timeoutMs = 5000);
    void _logOpControl(const char *where);
    void _probeField(const char *where);
    void _parseDevice(rfalNfcDevice *dev);
    String _getNfcaTypeName(uint8_t sak);
    void _parseLoadedData();
    int _readDataBlocks(rfalNfcDevice *dev);
    int _writeUltralight(rfalNfcDevice *dev);
    int _eraseUltralight(rfalNfcDevice *dev);
    int _writeNdefBlocks(rfalNfcDevice *dev);
    bool _buildLoadedNdefMessage(std::vector<uint8_t> &ndefOut);
    bool _writeT2TPage(uint8_t page, const uint8_t data[4], bool verify = true);
    bool _isUltralightUserPage(int page) const;
    bool _writeMagicGen1UID(rfalNfcDevice *dev);
    bool _writeMagicGen2UID(rfalNfcDevice *dev);

    // MIFARE Classic (Crypto1) — Milestone 5
    Crypto1State _mfcCipher;
    bool _mfcAuthed = false;
    int _readMifareClassic(rfalNfcDevice *dev);
    uint32_t _mfcUid32() const;
    uint8_t _mfcSectorFirstBlock(uint8_t sector) const;
    uint8_t _mfcSectorBlockCount(uint8_t sector) const;
    uint8_t _mfcBlockToSector(uint16_t block) const;
    ::ReturnCode _mifareTransceiveRaw(
        uint8_t *txBuf, uint16_t txBits, uint8_t *rxBuf, uint16_t rxCapBytes, uint16_t *rxBits, uint32_t fwt,
        uint32_t flags
    );
    bool _mifareAuth(uint8_t block, const uint8_t key[6], bool useKeyB);
    bool _mifareReadBlock(uint8_t block, uint8_t data[16]);
    bool _mifareWriteBlock(uint8_t block, const uint8_t data[16]);
    void _mfcHalt();
    int _writeMifareClassic(rfalNfcDevice *dev);       // authenticated write of loaded dump
    int _writeMifareClassicMagic(rfalNfcDevice *dev);  // clone to Magic Gen1
    void _mfcRebuildStrAllPages();
    int _saveMifareClassicFlipper(const String &filename);

    // helpers
    String _getNtagVariant();
    bool _readNtagSignature();
    bool _readNtagCounters();

    // ISO15693 (NFC-V), NFC-B e NFC-F (FeliCa)
    int _readNfcV(rfalNfcDevice *dev);
    void _parseNfcB(rfalNfcDevice *dev);
    int _readFeliCa(rfalNfcDevice *dev);

    // ISO-DEP / Type 4 Tag (T4T): DESFire, NDEF T4T, EMV
    int _readIsoDep(rfalNfcDevice *dev);
    bool _isoDepApdu(const uint8_t *tx, uint16_t txLen, uint8_t *rx, uint16_t rxCap, uint16_t *rxLen);
    bool _readNdefT4T();
    bool _readDESFireInfo();
    bool _probeEmv();

    // Emulation NFC-A
    bool _setupListenMode(const uint8_t *uidBuf, uint8_t uidLen, const uint8_t *atqa, uint8_t sak);
    void _listenStop();
    int _handleListenLoop(uint32_t timeoutMs);
    bool _listenRespond(const uint8_t *resp, uint16_t len);
    int _buildEmuPages();

    uint8_t _emuPages[256][4];
    int _emuPageCount = 0;

    // MIFARE Classic emulation (listener-side Crypto1) — Milestone 5
    bool _emuIsMfc = false;
    Crypto1State _emuCipher;
    bool _emuMfcAuthed = false;
    uint32_t _emuNt = 0;
    void _emuParityOff(); // restore HW parity (TX+RX) after an encrypted session
    // Deferred diagnostics (printed on field-off so logging doesn't break timing).
    uint16_t _emuMfcAuthReq = 0;
    uint16_t _emuMfcAuthOk = 0;
    uint16_t _emuMfcBadAr = 0;
    uint16_t _emuMfcNoNr = 0;
    uint16_t _emuMfcReads = 0;
    uint16_t _emuMfcLastNrBits = 0;
    uint32_t _emuDbgNt = 0;
    uint8_t _emuDbgEnc[8] = {0};
    uint32_t _emuDbgArCalc = 0;
    uint32_t _emuDbgArExp = 0;
    uint8_t _emuDbgBlock = 0;
    bool _buildEmuMfc();                 // parse strAllPages -> mfcDump (keys injected into trailers)
    bool _emuMfcHandle(uint8_t *fifo, uint16_t n);          // handle one MFC command in listen loop
    void _emuTxClear(const uint8_t *data, uint8_t n, bool withCrc); // plain TX (HW parity)
    void _emuTxBits(const uint8_t *bitstream, uint16_t nbits);      // raw TX, no HW parity
    uint16_t _emuRxRaw(uint8_t *out, uint8_t maxBytes, uint32_t toMs); // RX keeping parity, returns bits

    // Type 4 Tag (NDEF) emulation over the NFC-A target — Milestone 6
    bool _emuIsT4T = false;
    uint8_t _t4tCC[15];
    uint8_t _t4tNdef[512];   // NLEN(2) + NDEF message
    uint16_t _t4tNdefLen = 0;
    uint8_t _t4tSelected = 0; // 0 = none, 1 = CC, 2 = NDEF
    void _buildT4TFiles();
    bool _emuT4THandle(uint8_t *fifo, uint16_t n);
    uint16_t _t4tProcessApdu(const uint8_t *c, uint16_t clen, uint8_t *r);

    // FeliCa (NFC-F) emulation — Milestone 6
    uint8_t _felicaIDm[8];
    uint8_t _felicaPMm[8];
    uint8_t _felicaSys[2];
    bool _setupListenModeF();
    int _handleFelicaListen(uint32_t timeoutMs);
    int _emulateFelica();
};

#endif // !LITE_VERSION
