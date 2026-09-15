#include "crypto_commands.h"
#include "core/passwords.h"
#include "core/sd_functions.h"
#include "helpers.h"
#include "modules/badusb_ble/ducky_typer.h"
#include <globals.h>
#ifndef LITE_VERSION
uint32_t decryptFileCallback(cmd *c) {
    // crypto decrypt_from_file passwords/github.com.txt.enc 1234

    Command cmd(c);

    Argument arg = cmd.getArgument("filepath");
    Argument pwdArg = cmd.getArgument("password");
    String filepath = arg.getValue();
    String password = pwdArg.getValue();
    filepath.trim();
    password.trim();

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    cachedPassword = password;

    FS *fs;
    if (!getFsStorage(fs)) return false;

    if (!(*fs).exists(filepath)) {
        serialDevice->println("File does not exist");
        return false;
    }

    String plaintext = readDecryptedFile(*fs, filepath);
    if (plaintext == "") return false;

    serialDevice->println(plaintext);
    return true;
}

uint32_t encryptFileCallback(cmd *c) {
    // crypto encrypt_to_file passwords/github.com.txt.enc 1234

    Command cmd(c);

    Argument arg = cmd.getArgument("filepath");
    Argument pwdArg = cmd.getArgument("password");
    String filepath = arg.getValue();
    String password = pwdArg.getValue();
    filepath.trim();
    password.trim();

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    cachedPassword = password;

    char *txt = _readFileFromSerial();
    if (strlen(txt) == 0) return false;
    String txtString = String(txt);

    FS *fs;
    if (!getFsStorage(fs)) return false;

    File f = fs->open(filepath, FILE_WRITE);
    if (!f) return false;

    String cyphertxt = encryptString(txtString, cachedPassword);
    if (cyphertxt == "") return false;

    f.write((const uint8_t *)cyphertxt.c_str(), cyphertxt.length());
    f.close();
    serialDevice->println("File written: " + filepath);
    return true;
}

uint32_t typeFileCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("filepath");
    Argument pwdArg = cmd.getArgument("password");
    String filepath = arg.getValue();
    String password = pwdArg.getValue();
    filepath.trim();
    password.trim();

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    cachedPassword = password;

    FS *fs;
    if (!getFsStorage(fs)) return false;

    if (!(*fs).exists(filepath)) {
        serialDevice->println("File does not exist");
        return false;
    }

    String plaintext = readDecryptedFile(*fs, filepath);
    if (plaintext == "") return false;

    serialDevice->println(plaintext);

    key_input_from_string(plaintext);
    return true;
}
#endif
void createCryptoCommands(SimpleCLI *cli) {
#ifndef LITE_VERSION
    Command cryptoCmd = cli->addCompositeCmd("crypto");

    Command decryptCmd = cli->addCommand("decrypt", decryptFileCallback);
    decryptCmd.addPosArg("filepath");
    decryptCmd.addPosArg("password");

    Command decryptFileCmd = cryptoCmd.addCommand("decrypt_from_file", decryptFileCallback);
    decryptFileCmd.addPosArg("filepath");
    decryptFileCmd.addPosArg("password");

    Command encryptCmd = cli->addCommand("encrypt", encryptFileCallback);
    encryptCmd.addPosArg("filepath");
    encryptCmd.addPosArg("password");

    Command encryptFileCmd = cryptoCmd.addCommand("encrypt_to_file", encryptFileCallback);
    encryptFileCmd.addPosArg("filepath");
    encryptFileCmd.addPosArg("password");
#ifdef USB_as_HID
    Command typeFileCmd = cryptoCmd.addCommand("type_from_file", typeFileCallback);
    typeFileCmd.addPosArg("filepath");
    typeFileCmd.addPosArg("password");
#endif
#endif
}
