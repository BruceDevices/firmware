#include "ir_commands.h"
#include "core/sd_functions.h"
#include "helpers.h"
#include "modules/ir/custom_ir.h"
#include "modules/ir/ir_read.h"
#include <ArduinoJson.h>
#include <globals.h>

uint32_t irCallback(cmd *c) {
    serialDevice->println("Turning off IR LED");
    digitalWrite(bruceConfigPins.irTx, LED_OFF);
    return true;
}

uint32_t irRxCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("raw");
    bool raw = arg.isSet();

    IrRead *i = NULL; // avoid calling the constructor here

    if (raw) {
        i = new IrRead(true, true); // true -> headless mode, true = raw mode
    } else {
        i = new IrRead(true); // true -> headless mode
    }

    serialDevice->println("Waiting for signal...");
    String r = i->loop_headless(10); // 10s timeout
    if (r.length() == 0) return false;

    serialDevice->println(r);
    delete i;
    return true;
}

uint32_t irTxCallback(cmd *c) {
    // usage: ir tx <protocol> <address without spaces> <command without spaces>
    // e.g. ir tx NEC 04000000 08000000

    Command cmd(c);

    Argument prtcArg = cmd.getArgument("protocol");
    Argument addrArg = cmd.getArgument("address");
    Argument cmdArg = cmd.getArgument("command");
    String protocol = prtcArg.getValue();
    String address = addrArg.getValue();
    String command = cmdArg.getValue();
    protocol.trim();
    address.trim();
    command.trim();

    if (address.length() != 8 || command.length() != 8) {
        serialDevice->println("Address and command must be 8 characters long");
        return false;
    }

    IRCode code;
    code.type = "parsed";
    code.protocol = protocol;
    code.address = address;
    code.command = command;

    sendIRCommand(&code);
    return true;
}

uint32_t irTxRawCallback(cmd *c) {
    // usage: ir tx_raw <frequency> <samples>

    Command cmd(c);

    Argument freqArg = cmd.getArgument("frequency");
    Argument samplesArg = cmd.getArgument("samples");
    String freqStr = freqArg.getValue();
    String samples = samplesArg.getValue();
    freqStr.trim();
    samples.trim();

    uint32_t frequency = freqStr.toInt();

    if (frequency == 0) {
        serialDevice->println("Invalid frequency: " + String(frequency));
        return false;
    }

    if (samples.length() == 0) {
        serialDevice->println("Missing data samples");
        return false;
    }

    IRCode code;
    code.type = "raw";
    code.frequency = frequency;
    code.data = samples;

    sendIRCommand(&code);
    return true;
}

uint32_t irTxFileCallback(cmd *c) {
    // example: ir tx_from_file LG_AKB72915206_power.ir false

    Command cmd(c);

    Argument filepathArg = cmd.getArgument("filepath");
    Argument hideDefaultUIArg = cmd.getArgument("hideDefaultUI");
    String filepath = filepathArg.getValue();
    String hideDefaultUI = hideDefaultUIArg.getValue();
    filepath.trim();

    if (filepath.indexOf(".ir") == -1) {
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

    return txIrFile(fs, filepath, hideDefaultUI);
}

uint32_t irTxBufferCallback(cmd *c) {
#ifndef LITE_VERSION
    if (!(_setupPsramFs())) return false;

    char *txt = _readFileFromSerial();
    String tmpfilepath = "/tmpramfile"; // TODO: Change to use char *txt directly
    File f = PSRamFS.open(tmpfilepath, FILE_WRITE);
    if (!f) return false;

    f.write((const uint8_t *)txt, strlen(txt));
    f.close();
    free(txt);

    bool r = txIrFile(&PSRamFS, tmpfilepath);
    PSRamFS.remove(tmpfilepath);

    return r;
#else
    return false;
#endif
}

uint32_t irSendCallback(cmd *c) {
    // tasmota json command  https://tasmota.github.io/docs/Tasmota-IR/#sending-ir-commands
    // e.g. IRSend {\"Protocol\":\"NEC\",\"Bits\":32,\"Data\":\"0x20DF10EF\"}
    // TODO: decode "data" into "address+command" and use existing "send*Command" funcs

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

    uint16_t bits = 32;         // defaults to 32 bits
    String protocolStr = "nec"; // defaults to NEC protocol
    String dataStr = "";

    if (args_json["Data"].isNull()) {
        serialDevice->println("json missing data field");
        return false;
    } else {
        dataStr = args_json["Data"].as<String>();
    }

    if (!args_json["Protocol"].isNull()) protocolStr = args_json["Protocol"].as<String>();

    if (!args_json["Bits"].isNull()) bits = args_json["Bits"].as<int>();

    return sendDecodedCommand(protocolStr, dataStr, bits);
}

void createIrRxCommand(Command *irCmd) {
    Command cmd = irCmd->addCommand("rx", irRxCallback);
    cmd.addFlagArg("raw");
}

void createIrTxCommand(Command *irCmd) {
    Command cmd = irCmd->addCommand("tx", irTxCallback);
    cmd.addPositionalArgument("protocol");
    cmd.addPositionalArgument("address");
    cmd.addPositionalArgument("command");
}

void createIrTxRawCommand(Command *irCmd) {
    Command cmd = irCmd->addCommand("tx_raw", irTxRawCallback);
    cmd.addPositionalArgument("frequency");
    cmd.addPositionalArgument("samples");
}

void createIrTxFileCommand(Command *irCmd) {
    Command cmd = irCmd->addCommand("tx_from_file", irTxFileCallback);
    cmd.addPositionalArgument("filepath");
    cmd.addPositionalArgument("hideDefaultUI", "false");
}

void createIrTxBufferCommand(Command *irCmd) {
    Command cmd = irCmd->addCommand("tx_from_buffer", irTxBufferCallback);
}

void createIrCommands(SimpleCLI *cli) {
    Command cmd = cli->addCompositeCmd("ir", irCallback);

    createIrRxCommand(&cmd);
    createIrTxCommand(&cmd);
    createIrTxRawCommand(&cmd);
    createIrTxFileCommand(&cmd);
    createIrTxBufferCommand(&cmd);

    cli->addSingleArgCmd("IRSend", irSendCallback);
}
