#include "rf_commands.h"
#include "cJSON.h"
#include "core/sd_functions.h"
#include "core/type_convertion.h" // decimalToHexString
#include "helpers.h"
#include "modules/rf/protocols/rf_config.h"   // RF_DEBUG
#include "modules/rf/protocols/rf_encoder.h"  // rf_tx_protocol, rf_encoder_selftest
#include "modules/rf/protocols/rf_keeloq.h"   // rf_keeloq_selftest
#include "modules/rf/protocols/rf_registry.h" // rf_find_protocol
#include "modules/rf/rf_scan.h"
#include "modules/rf/rf_send.h"
#include "modules/rf/rf_utils.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <globals.h>

uint32_t rfRxCallback(cmd *c) {
    Command cmd(c);

    Argument rawArg = cmd.getArgument("raw");
    Argument freqArg = cmd.getArgument("frequency");
    bool raw = rawArg.isSet();
    String strFreq = freqArg.getValue();

    float frequency = strFreq.toFloat();
    frequency /= 1000000; // passed as a long int (e.g. 433920000)

    // serialDevice->print("frequency: ");
    // serialDevice->println(frequency);

    String r = "";
    if (raw) {
        r = rfReceiveSignal(frequency, 10, true, true); // raw mode, headless (Serial only)
    } else {
        r = rfReceiveSignal(frequency, 10, false, true); // decoded mode, headless (Serial only)
    }

    if (r.length() == 0) return false;

    serialDevice->println(r);
    return true;
}

uint32_t rfTxCallback(cmd *c) {
    // flipperzero-like cmd  https://docs.flipper.net/development/cli/#wLVht
    // e.g. subghz tx 0000000000200001 868250000 403 10  //
    // https://forum.flipper.net/t/friedland-libra-48249sl-wireless-doorbell-request/4528/20
    //                {hex_key}     {frequency} {te} {count}
    // subghz tx 445533 433920000 174 10

    Command cmd(c);

    Argument keyArg = cmd.getArgument("key");
    Argument freqArg = cmd.getArgument("frequency");
    Argument teArg = cmd.getArgument("te");
    Argument cntArg = cmd.getArgument("count");
    String strKey = keyArg.getValue();
    String strFrequency = freqArg.getValue();
    String strTe = teArg.getValue();
    String strCount = cntArg.getValue();

    uint64_t key = std::stoull(strKey.c_str(), nullptr, 16);
    unsigned long frequency = std::stoul(strFrequency.c_str());
    unsigned int te = std::stoul(strTe.c_str());
    unsigned int count = std::stoul(strCount.c_str());

    unsigned int bits = 24; // TODO: compute from key

    // check valid frequency and init the rf module
    if (!initRfModule("tx", float(frequency / 1000000.0))) return false;

    rfTransmitCode(key, bits, te, 1, count);
    deinitRfModule();
    return true;
}

uint32_t rfTxByNameCallback(cmd *c) {
    // Transmit by protocol NAME (registry identity), e.g.
    //   subghz txp CAME 433920000 12 0xA5A 0 10
    //   subghz txp Linear 433920000 10 0x2A9
    // Single-line counterpart to a `.sub` replay: resolves `rf_find_protocol`
    // and drives the RMT encoder directly, so it round-trips with the decoder
    // (`Protocol: <name>`). Accepts a full 64-bit hex key.
    Command cmd(c);

    String name = cmd.getArgument("protocol").getValue();
    String strFrequency = cmd.getArgument("frequency").getValue();
    String strBits = cmd.getArgument("bits").getValue();
    String strKey = cmd.getArgument("key").getValue();
    String strTe = cmd.getArgument("te").getValue();
    String strRepeat = cmd.getArgument("repeat").getValue();

    const RfProtocolDef *def = rf_find_protocol(name);
    if (def == nullptr) {
        serialDevice->println("unknown protocol: " + name);
        return false;
    }

    uint64_t key = std::stoull(strKey.c_str(), nullptr, 16);
    unsigned long frequency = std::stoul(strFrequency.c_str());
    unsigned int bits = std::stoul(strBits.c_str());
    int te = strTe.length() ? (int)std::stoul(strTe.c_str()) : 0;
    int repeat = strRepeat.length() ? (int)std::stoul(strRepeat.c_str()) : 10;

    if (!initRfModule("tx", float(frequency / 1000000.0))) return false;
    rf_tx_protocol(key, bits, te, def, repeat);
    deinitRfModule();
    return true;
}

#if RF_DEBUG
uint32_t rfSelftestCallback(cmd *c) {
    // Golden encoder self-test (spec-fidelity of the registry defs). Diagnostic
    // only; compiled under RF_DEBUG.
    return rf_encoder_selftest() ? true : false;
}

uint32_t rfKeeloqTestCallback(cmd *c) {
    // KeeLoq route self-test (per-manufacturer hop framing + cipher round-trip).
    // Diagnostic only; compiled under RF_DEBUG.
    return rf_keeloq_selftest() ? true : false;
}

uint32_t rfKeeloqFileTestCallback(cmd *c) {
    // KeeLoq round-trip driven by the real /mfcodes keystore (every manufacturer
    // with its actual key + learning type). Diagnostic only; under RF_DEBUG.
    return rf_keeloq_filetest() ? true : false;
}
#endif

uint32_t rfKeeloqTxCallback(cmd *c) {
    // Emit a KeeLoq rolling-code frame for a manufacturer in the keystore:
    //   subghz keeloqtx <manufacturer> <freq> <button> <serial_hex> <counter> [repeat]
    // Builds the 64-bit code via keeloq_step (reads /mfcodes) and transmits with
    // the dedicated KeeLoq encoder.
    Command cmd(c);
    String mf = cmd.getArgument("manufacturer").getValue();
    String strFreq = cmd.getArgument("frequency").getValue();
    String strBtn = cmd.getArgument("button").getValue();
    String strSerial = cmd.getArgument("serial").getValue();
    String strCnt = cmd.getArgument("counter").getValue();
    String strRepeat = cmd.getArgument("repeat").getValue();

    RfCodes data{};
    data.mf_name = mf;
    data.btn = (uint8_t)std::stoul(strBtn.c_str());
    data.serial = (uint32_t)std::stoul(strSerial.c_str(), nullptr, 16);
    data.cnt = (uint16_t)std::stoul(strCnt.c_str());
    unsigned long frequency = std::stoul(strFreq.c_str());
    int repeat = strRepeat.length() ? (int)std::stoul(strRepeat.c_str()) : 10;

    data.fix = ((uint32_t)data.btn << 28) | data.serial;
    data.Bit = 64;
    data.keeloq_step(0); // assembles data.key from the manufacturer keystore

    char keyHex[32] = {0};
    decimalToHexString(data.key, keyHex);
    serialDevice->println("keeloqtx mf=" + mf + " key=" + String(keyHex));

    if (!initRfModule("tx", float(frequency / 1000000.0))) return false;
    rf_tx_keeloq(data.key, repeat);
    deinitRfModule();
    return true;
}

// --- /mfcodes management on LittleFS (single-line, robust) ------------------
// Writes the KeeLoq manufacturer keystore to LittleFS so it survives without an
// SD card. Each entry is one line: `mf_name;key_hex;type` (matches the parser).
uint32_t rfMfcodesAddCallback(cmd *c) {
    Command cmd(c);
    String entry = cmd.getArgument("entry").getValue();
    entry.trim();
    if (entry.length() == 0) {
        serialDevice->println("empty entry");
        return false;
    }
    int sc = 0;
    for (unsigned int i = 0; i < entry.length(); i++)
        if (entry[i] == ';') sc++;
    if (sc != 2) {
        serialDevice->println("invalid (need name;keyhex;type): " + entry);
        return false;
    }
    File f = LittleFS.open("/mfcodes", FILE_APPEND, true);
    if (!f) {
        serialDevice->println("LittleFS open failed");
        return false;
    }
    f.print(entry);
    f.print("\n");
    f.close();
    serialDevice->println("added: " + entry);
    return true;
}

uint32_t rfMfcodesClearCallback(cmd *c) {
    if (LittleFS.exists("/mfcodes")) LittleFS.remove("/mfcodes");
    File f = LittleFS.open("/mfcodes", FILE_WRITE, true);
    if (f) f.close();
    serialDevice->println("/mfcodes cleared (LittleFS)");
    return true;
}

uint32_t rfMfcodesListCallback(cmd *c) {
    File f = LittleFS.open("/mfcodes", FILE_READ);
    if (!f) {
        serialDevice->println("no /mfcodes on LittleFS");
        return false;
    }
    int n = 0;
    while (f.available()) {
        String l = f.readStringUntil('\n');
        l.trim();
        if (l.length()) {
            serialDevice->println(l);
            n++;
        }
    }
    f.close();
    serialDevice->println("total: " + String(n));
    return true;
}

uint32_t rfScanCallback(cmd *c) {
    // subghz scan 433 434

    Command cmd(c);

    Argument startArg = cmd.getArgument("start_frequency");
    Argument stopArg = cmd.getArgument("stop_frequency");
    String startFreqStr = startArg.getValue();
    String stopFreqStr = stopArg.getValue();

    float startFreq = startFreqStr.toFloat();
    float stopFreq = stopFreqStr.toFloat();

    if (startFreq == 0 || stopFreq == 0) {
        serialDevice->println("Invalid frequency range: " + String(startFreq) + " - " + String(stopFreq));
        return false;
    }

    // passed as a long int (e.g. 433920000)
    startFreq /= 1000000;
    stopFreq /= 1000000;

    rf_scan(startFreq, stopFreq, 10 * 1000); // 10s timeout
    return true;
}

uint32_t rfTxFileCallback(cmd *c) {
    // example: subghz tx_from_file plug1_on.sub false

    Command cmd(c);

    Argument filepathArg = cmd.getArgument("filepath");
    Argument hideDefaultUIArg = cmd.getArgument("hideDefaultUI");
    String filepath = filepathArg.getValue();
    String hideDefaultUIStr = hideDefaultUIArg.getValue();
    hideDefaultUIStr.trim();
    // CLI: keep the display clean by default; pass "false" to mirror on screen.
    bool hideDefaultUI = !hideDefaultUIStr.equalsIgnoreCase("false");
    filepath.trim();

    if (filepath.indexOf(".sub") == -1) {
        serialDevice->println("Invalid file");
        return false;
    }

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    FS *fs;
    if (!getFsStorage(fs)) return false;

    if (!(*fs).exists(filepath)) {
        serialDevice->println("File does not exist");
        return false;
    }

    RfCodes data{};

    return readSubFile(fs, filepath, data) && txSubFile(data, hideDefaultUI);
}

uint32_t rfTxBufferCallback(cmd *c) {
#ifndef LITE_VERSION
    if (!(_setupPsramFs())) return false;

    char *txt = _readFileFromSerial();
    String tmpfilepath = "/tmpramfile"; // TODO: Change to use char *txt directly
    File f = PSRamFS.open(tmpfilepath, FILE_WRITE);
    if (!f) return false;

    f.write((const uint8_t *)txt, strlen(txt));
    f.close();
    free(txt);

    RfCodes data{};

    bool r = readSubFile(&PSRamFS, tmpfilepath, data);

    r = txSubFile(data, true); // CLI: don't pollute the display
    PSRamFS.remove(tmpfilepath);

    return r;
#else
    return false;
#endif
}

uint32_t rfSendCallback(cmd *c) {
    // tasmota json command  https://tasmota.github.io/docs/Tasmota-IR/#sending-ir-commands
    // e.g. RfSend {\"Data\":\"0x447503\",\"Bits\":24,\"Protocol\":1,\"Pulse\":174,\"Repeat\":10}  // on
    // e.g. RfSend {\"Data\":\"0x44750C\",\"Bits\":24,\"Protocol\":1,\"Pulse\":174,\"Repeat\":10}  // off

    Command cmd(c);

    Argument args = cmd.getArgument(0);
    String args_str = args.getValue();
    args_str.trim();
    // serialDevice->println(command);

    JsonDocument jsonDoc;
    if (deserializeJson(jsonDoc, args_str)) {
        serialDevice->println("Failed to parse json");
        serialDevice->println(args_str);
        return false;
    }

    JsonObject args_json = jsonDoc.as<JsonObject>(); // root

    unsigned int bits = 32; // defaults to 32 bits
    String dataStr = "";
    int protocol = 1; // defaults to 1
    int pulse = 0;    // 0 leave the library use the default value depending on protocol
    int repeat = 10;

    if (args_json["Data"].isNull()) {
        serialDevice->println("json missing data field");
        return false;
    } else {
        dataStr = args_json["Data"].as<String>();
    }

    uint64_t data_int = strtoul(dataStr.c_str(), nullptr, 16);
    if (data_int == 0) {
        serialDevice->println("rfSendCallback: invalid data value: 0");
        serialDevice->println(dataStr);
        return false;
    }

    if (!args_json["Bits"].isNull()) bits = args_json["Bits"].as<unsigned int>();

    if (!args_json["Pulse"].isNull()) pulse = args_json["Pulse"].as<int>();

    if (!args_json["Protocol"].isNull()) protocol = args_json["Protocol"].as<int>();

    if (!args_json["Repeat"].isNull()) repeat = args_json["Repeat"].as<int>();

    if (!initRfModule("tx")) return false;

    rfTransmitCode(data_int, bits, pulse, protocol, repeat);

    return true;
}

void createRfRxCommand(Command *rfCmd) {
    Command cmd = rfCmd->addCommand("rx", rfRxCallback);
    cmd.addPosArg("frequency", String(bruceConfigPins.rfFreq).c_str());
    cmd.addFlagArg("raw");
}

void createRfTxCommand(Command *rfCmd) {
    Command cmd = rfCmd->addCommand("tx", rfTxCallback);
    cmd.addPosArg("key", "0");
    cmd.addPosArg("frequency", "433920000");
    cmd.addPosArg("te", "0");
    cmd.addPosArg("count", "10");
}

void createRfTxByNameCommand(Command *rfCmd) {
    Command cmd = rfCmd->addCommand("txp", rfTxByNameCallback);
    cmd.addPosArg("protocol");
    cmd.addPosArg("frequency", "433920000");
    cmd.addPosArg("bits", "24");
    cmd.addPosArg("key", "0");
    cmd.addPosArg("te", "0");
    cmd.addPosArg("repeat", "10");
}

#if RF_DEBUG
void createRfSelftestCommand(Command *rfCmd) {
    rfCmd->addCommand("selftest", rfSelftestCallback);
}

void createRfKeeloqTestCommand(Command *rfCmd) {
    rfCmd->addCommand("keeloqtest", rfKeeloqTestCallback);
}

void createRfKeeloqFileTestCommand(Command *rfCmd) {
    rfCmd->addCommand("keeloqfiletest", rfKeeloqFileTestCallback);
}
#endif

void createRfMfcodesCommand(Command *rfCmd) {
    Command cmd = rfCmd->addCompositeCmd("mfcodes");
    Command addCmd = cmd.addCommand("add", rfMfcodesAddCallback);
    addCmd.addPosArg("entry");
    cmd.addCommand("list", rfMfcodesListCallback);
    cmd.addCommand("clear", rfMfcodesClearCallback);
}

void createRfKeeloqTxCommand(Command *rfCmd) {
    Command cmd = rfCmd->addCommand("keeloqtx", rfKeeloqTxCallback);
    cmd.addPosArg("manufacturer");
    cmd.addPosArg("frequency", "433920000");
    cmd.addPosArg("button", "1");
    cmd.addPosArg("serial", "0");
    cmd.addPosArg("counter", "1");
    cmd.addPosArg("repeat", "10");
}

void createRfScanCommand(Command *rfCmd) {
    Command cmd = rfCmd->addCommand("scan", rfScanCallback);
    cmd.addPosArg("start_frequency");
    cmd.addPosArg("stop_frequency");
}

void createRfTxFileCommand(Command *rfCmd) {
    Command cmd = rfCmd->addCommand("tx_from_file", rfTxFileCallback);
    cmd.addPosArg("filepath");
    cmd.addPosArg("hideDefaultUI", "false");
}

void createRfTxBufferCommand(Command *rfCmd) {
    Command cmd = rfCmd->addCommand("tx_from_buffer", rfTxBufferCallback);
}

void createRfCommands(SimpleCLI *cli) {
    Command cmd = cli->addCompositeCmd("rf,subghz");

    createRfRxCommand(&cmd);
    createRfTxCommand(&cmd);
    createRfTxByNameCommand(&cmd);
    createRfScanCommand(&cmd);
    createRfTxFileCommand(&cmd);
    createRfTxBufferCommand(&cmd);
    createRfMfcodesCommand(&cmd);
    createRfKeeloqTxCommand(&cmd);
#if RF_DEBUG
    createRfSelftestCommand(&cmd);
    createRfKeeloqTestCommand(&cmd);
    createRfKeeloqFileTestCommand(&cmd);
#endif

    cli->addSingleArgCmd("RfSend", rfSendCallback);
}
