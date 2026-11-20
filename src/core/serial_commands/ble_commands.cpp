#include "ble_commands.h"
#include "modules/ble/gatt_server.h"
#include "modules/ble/gatt_explorer.h"
#include <globals.h>

#if !defined(LITE_VERSION)

static uint32_t bleCallback(cmd *c) {
    Command cmd(c);
    Argument actionArg = cmd.getArgument("action");
    String action = actionArg.getValue();
    action.trim();
    action.toLowerCase();

    Argument param1Arg = cmd.getArgument("param1");
    String param1 = param1Arg.getValue();
    param1.trim();

    Argument param2Arg = cmd.getArgument("param2");
    String param2 = param2Arg.getValue();
    param2.trim();

    if (action == "server") {
        if (param1 == "stop") {
            stopGattServerService();
            serialDevice->println("GATT Server stopped.");
            return true;
        } else if (param1 == "start" || param1 == "on" || param1 == "") {
            int profile = param2.toInt();
            if (profile < 0 || profile > 3) profile = 0;
            if (startGattServerService(profile)) {
                serialDevice->println("GATT Server started in profile " + String(profile));
                return true;
            } else {
                serialDevice->println("Failed to start GATT Server.");
                return false;
            }
        } else if (param1 == "status") {
            serialDevice->println(String("GATT Server status: ") + (isGattServerActive() ? "RUNNING" : "STOPPED"));
            return true;
        }
    } else if (action == "connect") {
        if (param1 == "") {
            serialDevice->println("Usage: ble connect <MAC> [pub|rnd]");
            return false;
        }
        uint8_t addrType = 0; // BLE_ADDR_PUBLIC
        if (param2.equalsIgnoreCase("rnd") || param2.equalsIgnoreCase("random") || param2 == "1") {
            addrType = 1; // BLE_ADDR_RANDOM
        }
        bool ok = gattConnectCli(param1, addrType);
        return ok;
    } else if (action == "scan") {
        int timeoutSec = param1.toInt();
        if (timeoutSec <= 0) timeoutSec = 5;
        gattScanCli(timeoutSec);
        return true;
    }

    serialDevice->println(
        "Invalid ble command.\n"
        "Usage:\n"
        "  ble server start [0-3]   (0=All-in-One, 1=DIS+Bat, 2=Echo, 3=NUS)\n"
        "  ble server stop\n"
        "  ble server status\n"
        "  ble scan [seconds]\n"
        "  ble connect <MAC> [pub|rnd]"
    );
    return false;
}

void createBleCommands(SimpleCLI *cli) {
    Command bleCmd = cli->addCommand("ble", bleCallback);
    bleCmd.addPosArg("action", "");
    bleCmd.addPosArg("param1", "");
    bleCmd.addPosArg("param2", "");
}

#else

void createBleCommands(SimpleCLI *cli) {}

#endif // !LITE_VERSION
