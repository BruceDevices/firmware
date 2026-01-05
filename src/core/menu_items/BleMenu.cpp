void BleMenu::optionsMenu() {
    options.clear();

    if (BLEConnected) {
        options.push_back({"Disconnect", [=]() {
#if defined(CONFIG_IDF_TARGET_ESP32C5)
                               esp_bt_controller_deinit();
#else
                               BLEDevice::deinit();
#endif
                               BLEConnected = false;
                               delete hid_ble;
                               hid_ble = nullptr;
                               if (_Ask_for_restart == 1)
                                   _Ask_for_restart = 2;
                           }});
    }

    options.push_back({"Media Cmds", [=]() { MediaCommands(hid_ble, true); }});
#if !defined(LITE_VERSION)
    options.push_back({"BLE Scan", ble_scan});
    options.push_back({"Bad BLE", [=]() { ducky_setup(hid_ble, true); }});
#endif
    options.push_back({"BLE Keyboard", [=]() { ducky_keyboard(hid_ble, true); }});
    options.push_back({"Apple iOS", lambdaHelper(aj_adv, 0)});
    options.push_back({"SwiftPair", lambdaHelper(aj_adv, 1)});
    options.push_back({"Samsung", lambdaHelper(aj_adv, 2)});
    options.push_back({"FastPair", lambdaHelper(aj_adv, 3)});
    options.push_back({"Spam All", lambdaHelper(aj_adv, 4)});
    options.push_back({"Custom Name", lambdaHelper(aj_adv, 5)});
    options.push_back({"Name Flood", lambdaHelper(aj_adv, 6)});
#if !defined(LITE_VERSION)
    options.push_back({"Ninebot", [=]() { BLENinebot(); }});
#endif
    addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "Bluetooth");
}
