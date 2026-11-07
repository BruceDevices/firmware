#if !defined(LITE_VERSION)
#include "wire_commands.h"
#include "modules/wire/wire_tools.h"
#include <globals.h>

/*********************************************************************
**  wire uart ...
**********************************************************************/
uint32_t wireUartBeginCallback(cmd *c) {
    Command command(c);

    Argument baudArg = command.getArgument("baud");
    Argument rxArg = command.getArgument("rx");
    Argument txArg = command.getArgument("tx");
    uint32_t baud = (uint32_t)strtoul(baudArg.getValue().c_str(), nullptr, 10);
    int rx = (int)strtol(rxArg.getValue().c_str(), nullptr, 10);
    int tx = (int)strtol(txArg.getValue().c_str(), nullptr, 10);

    if (baud < 300 || baud > 5000000) {
        serialDevice->println("ERROR: baud must be 300..5000000");
        return false;
    }
    if (!wire_uart_begin(baud, (int8_t)rx, (int8_t)tx)) {
        serialDevice->println("ERROR: open failed (check rx/tx pins and USB-serial bridge)");
        return false;
    }
    serialDevice->println(wire_uart_status());
    return true;
}

uint32_t wireUartSendCallback(cmd *c) {
    Command command(c);
    Argument textArg = command.getArgument("text");
    String text = textArg.getValue();
    if (text.isEmpty()) {
        serialDevice->println("ERROR: empty text");
        return false;
    }
    int sent = wire_uart_send(text + "\n");
    if (sent < 0) {
        serialDevice->println("ERROR: not open (wire uart begin)");
        return false;
    }
    serialDevice->println("SENT " + String(sent) + " bytes");
    return true;
}

uint32_t wireUartReadCallback(cmd *c) {
    Command command(c);
    Argument timeoutArg = command.getArgument("timeout");
    uint32_t timeoutMs = (uint32_t)strtoul(timeoutArg.getValue().c_str(), nullptr, 10);
    if (timeoutMs == 0 || timeoutMs > 60000) timeoutMs = 1000;

    String out;
    int received = wire_uart_read(out, timeoutMs);
    if (received < 0) {
        serialDevice->println("ERROR: not open (wire uart begin)");
        return false;
    }
    serialDevice->println("RECV " + String(received) + " bytes");
    if (received) serialDevice->println(out);
    return true;
}

uint32_t wireUartEndCallback(cmd *c) {
    wire_uart_end();
    serialDevice->println("UART closed");
    return true;
}

uint32_t wireUartStatusCallback(cmd *c) {
    serialDevice->println(wire_uart_status());
    return true;
}

/*********************************************************************
**  wire i2c ...
**********************************************************************/
uint32_t wireI2cScanCallback(cmd *c) {
    Command command(c);
    int sda = (int)strtol(command.getArgument("sda").getValue().c_str(), nullptr, 10);
    int scl = (int)strtol(command.getArgument("scl").getValue().c_str(), nullptr, 10);

    std::vector<WireI2cDevice> found;
    int count = wire_i2c_scan(found, (int8_t)sda, (int8_t)scl);
    if (count < 0) {
        serialDevice->println("ERROR: set sda/scl pins or configure i2c_bus first");
        return false;
    }
    serialDevice->println("FOUND " + String(count) + " device(s):");
    for (const WireI2cDevice &dev : found) {
        serialDevice->println("0x" + String(dev.addr, HEX) + " " + i2cBusLabel(dev.bus));
    }
    return true;
}

// Target bus: explicit sda/scl pins win; otherwise i2c_bus, or sys_i2c only with -sys (system
// peripherals live there, so it is never picked implicitly).
static bool wireI2cParseAddrReg(
    Command &command, uint8_t &addr, uint8_t &reg, I2CBusId &bus, int8_t &sda, int8_t &scl
) {
    addr = (uint8_t)strtoul(command.getArgument("addr").getValue().c_str(), nullptr, 16);
    reg = (uint8_t)strtoul(command.getArgument("reg").getValue().c_str(), nullptr, 16);
    sda = (int8_t)strtol(command.getArgument("sda").getValue().c_str(), nullptr, 10);
    scl = (int8_t)strtol(command.getArgument("scl").getValue().c_str(), nullptr, 10);
    bus = command.getArgument("sys").isSet() ? I2CBusId::Sys : I2CBusId::User;
    if (addr < 0x08 || addr > 0x77) {
        serialDevice->println("ERROR: addr must be 0x08..0x77");
        return false;
    }
    return true;
}

uint32_t wireI2cReadCallback(cmd *c) {
    Command command(c);
    uint8_t addr, reg;
    I2CBusId bus;
    int8_t sda, scl;
    if (!wireI2cParseAddrReg(command, addr, reg, bus, sda, scl)) return false;
    serialDevice->println(
        "0x" + String(addr, HEX) + ":reg 0x" + String(reg, HEX) + " -> " +
        wire_i2c_read_reg(addr, reg, bus, sda, scl)
    );
    return true;
}

uint32_t wireI2cWriteCallback(cmd *c) {
    Command command(c);
    uint8_t addr, reg;
    I2CBusId bus;
    int8_t sda, scl;
    if (!wireI2cParseAddrReg(command, addr, reg, bus, sda, scl)) return false;
    uint8_t value = (uint8_t)strtoul(command.getArgument("value").getValue().c_str(), nullptr, 16);
    String result = wire_i2c_write_reg(addr, reg, value, bus, sda, scl);
    serialDevice->println(result.startsWith("OK") ? result : "ERROR: " + result.substring(5));
    return true;
}

/*********************************************************************
**  wire spi ...
**********************************************************************/
uint32_t wireSpiBeginCallback(cmd *c) {
    Command command(c);
    int sck = (int)strtol(command.getArgument("sck").getValue().c_str(), nullptr, 10);
    int miso = (int)strtol(command.getArgument("miso").getValue().c_str(), nullptr, 10);
    int mosi = (int)strtol(command.getArgument("mosi").getValue().c_str(), nullptr, 10);
    int cs = (int)strtol(command.getArgument("cs").getValue().c_str(), nullptr, 10);
    uint32_t freq = (uint32_t)strtoul(command.getArgument("freq").getValue().c_str(), nullptr, 10);
    if (freq == 0) freq = 1000000;

    if (!wire_spi_begin((int8_t)sck, (int8_t)miso, (int8_t)mosi, (int8_t)cs, freq)) {
        serialDevice->println("ERROR: open failed (need sck/miso/mosi/cs; bus may be taken)");
        return false;
    }
    serialDevice->println("OK SPI " + String(freq) + "Hz CS:" + String(cs));
    return true;
}

uint32_t wireSpiXferCallback(cmd *c) {
    Command command(c);
    String hex = command.getArgument("bytes").getValue();
    if (hex.isEmpty()) {
        serialDevice->println("ERROR: empty hex");
        return false;
    }
    serialDevice->println(wire_spi_transfer(hex));
    return true;
}

uint32_t wireSpiFlashIdCallback(cmd *c) {
    serialDevice->println(wire_spi_flash_detect());
    return true;
}

uint32_t wireSpiEndCallback(cmd *c) {
    wire_spi_end();
    serialDevice->println("SPI released");
    return true;
}

/*********************************************************************
**  wire jtag ...
**********************************************************************/
uint32_t wireJtagIdcodeCallback(cmd *c) {
    Command command(c);
    int tck = (int)strtol(command.getArgument("tck").getValue().c_str(), nullptr, 10);
    int tms = (int)strtol(command.getArgument("tms").getValue().c_str(), nullptr, 10);
    int tdi = (int)strtol(command.getArgument("tdi").getValue().c_str(), nullptr, 10);
    int tdo = (int)strtol(command.getArgument("tdo").getValue().c_str(), nullptr, 10);
    serialDevice->println(wire_jtag_read_idcode((int8_t)tck, (int8_t)tms, (int8_t)tdi, (int8_t)tdo));
    return true;
}

/*********************************************************************
**  wire la ...
**********************************************************************/
uint32_t wireLaCaptureCallback(cmd *c) {
    Command command(c);
    uint32_t res = strtoul(command.getArgument("res").getValue().c_str(), nullptr, 10);
    uint32_t depth = strtoul(command.getArgument("depth").getValue().c_str(), nullptr, 10);
    int pins[4];
    for (int i = 0; i < 4; i++) pins[i] = (int)strtol(command.getArgument("pin" + String(i + 1)).getValue().c_str(), nullptr, 10);
    serialDevice->println(
        wire_la_capture(res, depth, (int8_t)pins[0], (int8_t)pins[1], (int8_t)pins[2], (int8_t)pins[3])
    );
    return true;
}

uint32_t wireLaDumpCallback(cmd *c) {
    wire_la_dump();
    return true;
}

uint32_t wireLaStatusCallback(cmd *c) {
    serialDevice->println(wire_la_status());
    return true;
}

/*********************************************************************
**  wire cont ...
**********************************************************************/
uint32_t wireContTestCallback(cmd *c) {
    Command command(c);
    int pinA = (int)strtol(command.getArgument("pin_a").getValue().c_str(), nullptr, 10);
    int pinB = (int)strtol(command.getArgument("pin_b").getValue().c_str(), nullptr, 10);
    serialDevice->println(wire_cont_test((int8_t)pinA, (int8_t)pinB));
    return true;
}

/*********************************************************************
**  Registration
**********************************************************************/
void createWireUartCommand(Command *wireCmd) {
    Command wireUartCmd = wireCmd->addCompositeCmd("uart");

    Command beginCmd = wireUartCmd.addCommand("begin", wireUartBeginCallback);
    beginCmd.addPosArg("baud", "115200");
    beginCmd.addPosArg("rx", "-1"); // -1 = bruceConfigPins.uart_bus
    beginCmd.addPosArg("tx", "-1");

    Command sendCmd = wireUartCmd.addCommand("send", wireUartSendCallback);
    sendCmd.addBoundlessCommand("text");

    Command readCmd = wireUartCmd.addCommand("read", wireUartReadCallback);
    readCmd.addPosArg("timeout", "1000");

    wireUartCmd.addCommand("end", wireUartEndCallback);
    wireUartCmd.addCommand("status", wireUartStatusCallback);
}

void createWireI2cCommand(Command *wireCmd) {
    Command wireI2cCmd = wireCmd->addCompositeCmd("i2c");

    Command scanCmd = wireI2cCmd.addCommand("scan", wireI2cScanCallback);
    scanCmd.addPosArg("sda", "-1");
    scanCmd.addPosArg("scl", "-1");

    Command readCmd = wireI2cCmd.addCommand("read", wireI2cReadCallback);
    readCmd.addPosArg("addr");
    readCmd.addPosArg("reg");
    readCmd.addPosArg("sda", "-1");
    readCmd.addPosArg("scl", "-1");
    readCmd.addFlagArg("sys"); // target sys_i2c instead of i2c_bus

    Command writeCmd = wireI2cCmd.addCommand("write", wireI2cWriteCallback);
    writeCmd.addPosArg("addr");
    writeCmd.addPosArg("reg");
    writeCmd.addPosArg("value");
    writeCmd.addPosArg("sda", "-1");
    writeCmd.addPosArg("scl", "-1");
    writeCmd.addFlagArg("sys"); // target sys_i2c instead of i2c_bus
}

void createWireSpiCommand(Command *wireCmd) {
    Command wireSpiCmd = wireCmd->addCompositeCmd("spi");

    Command beginCmd = wireSpiCmd.addCommand("begin", wireSpiBeginCallback);
    beginCmd.addPosArg("sck", "-1");
    beginCmd.addPosArg("miso", "-1");
    beginCmd.addPosArg("mosi", "-1");
    beginCmd.addPosArg("cs", "-1");
    beginCmd.addPosArg("freq", "1000000");

    Command xferCmd = wireSpiCmd.addCommand("xfer", wireSpiXferCallback);
    xferCmd.addPosArg("bytes");

    wireSpiCmd.addCommand("flashid", wireSpiFlashIdCallback);
    wireSpiCmd.addCommand("end", wireSpiEndCallback);
}

void createWireJtagCommand(Command *wireCmd) {
    Command wireJtagCmd = wireCmd->addCompositeCmd("jtag");

    Command idcodeCmd = wireJtagCmd.addCommand("idcode", wireJtagIdcodeCallback);
    idcodeCmd.addPosArg("tck", "-1");
    idcodeCmd.addPosArg("tms", "-1");
    idcodeCmd.addPosArg("tdi", "-1");
    idcodeCmd.addPosArg("tdo", "-1");
}

void createWireLaCommand(Command *wireCmd) {
    Command wireLaCmd = wireCmd->addCompositeCmd("la");

    Command captureCmd = wireLaCmd.addCommand("capture", wireLaCaptureCallback);
    captureCmd.addPosArg("res", "1000000");
    captureCmd.addPosArg("depth", "2048");
    captureCmd.addPosArg("pin1", "-1"); // -1 pins reuse the Logic Analyzer menu's session pins
    captureCmd.addPosArg("pin2", "-1");
    captureCmd.addPosArg("pin3", "-1");
    captureCmd.addPosArg("pin4", "-1");

    wireLaCmd.addCommand("dump", wireLaDumpCallback);
    wireLaCmd.addCommand("status", wireLaStatusCallback);
}

void createWireContCommand(Command *wireCmd) {
    Command wireContCmd = wireCmd->addCompositeCmd("cont");

    Command testCmd = wireContCmd.addCommand("test", wireContTestCallback);
    testCmd.addPosArg("pin_a");
    testCmd.addPosArg("pin_b", "-1"); // pin_b < 0 tests pin_a against GND
}

void createWireCommands(SimpleCLI *cli) {
    Command wireCmd = cli->addCompositeCmd("wire");
    createWireUartCommand(&wireCmd);
    createWireI2cCommand(&wireCmd);
    createWireSpiCommand(&wireCmd);
    createWireJtagCommand(&wireCmd);
    createWireContCommand(&wireCmd);
    // TODO(logic-analyzer): held back from this release; re-enable together with the
    // Logic Analyzer menu entry in WireToolsMenu.cpp:
    // createWireLaCommand(&wireCmd);
}
#endif
