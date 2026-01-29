
#include <Arduino.h>
#include <MD5Builder.h>

#include "mykeyboard.h"
#include "passwords.h"
#include "sd_functions.h"
#include "type_convertion.h"
#include <globals.h>

String xorEncryptDecryptMD5(const String &input, const String &password, const int MD5_PASSES) {

    MD5Builder md5;
    String hash = password;

    for (int i = 0; i < MD5_PASSES; i++) {
        md5.begin();
        md5.add(hash);
        md5.calculate();
    }

    uint8_t md5Hash[16];
    md5.getBytes(md5Hash); // Store MD5 hash in the output array

    String output = input; // Copy input to output for modification
    for (size_t i = 0; i < input.length(); i++) {
        output[i] = input[i] ^ md5Hash[i % 16]; // XOR each byte with the MD5 hash
    }

    return output;
}

bool isValidAscii(const String &text) {
    for (int i = 0; i < text.length(); i++) {
        char c = text[i];
        // Check if the character is within the printable ASCII range or is a newline/carriage return
        if (!(c >= 32 && c <= 126) && c != 10 && c != 13) {
            return false; // Invalid character found
        }
    }
    return true; // All characters are valid
}

/* OLD:
String readDecryptedFileOLD(FS &fs, String filepath) {
  String cyphertext = readSmallFile(fs, filepath);
  if(cyphertext.length() == 0) return "";

  if(cachedPassword.length()==0) {
    cachedPassword = keyboard("", 32, "Password:", true);
    if(cachedPassword.length()==0) return "";  // cancelled
  }

  //Serial.println(cyphertext);
  //Serial.println(cachedPassword);

  // else try to decrypt
  String plaintext = decryptString(cyphertext, cachedPassword);

  // check if really plaintext
  if(!isValidAscii(plaintext)) {
    // invalidate cached password -> will ask again on the next try
    cachedPassword = "";
    Serial.println("invalid password");
    //Serial.println(plaintext);
    return "";
  }

  // else
  return plaintext;
}
*/

String readDecryptedFile(FS &fs, String filepath) {

    if (cachedPassword.length() == 0) {
        cachedPassword = keyboard("", 32, "Password:", true);
        if (cachedPassword.length() == 0) return ""; // cancelled
    }

    File cyphertextFile = fs.open(filepath, FILE_READ);
    if (!cyphertextFile) return "";

    String line;
    String cypertextData = "";
    String plaintext = "";
    bool unsupported_params = false;

    while (cyphertextFile.available()) {
        line = cyphertextFile.readStringUntil('\n');
        if (line.startsWith("Filetype:") && !line.endsWith("Bruce Encrypted File")) unsupported_params = true;
        if (line.startsWith("Algo:") && !line.endsWith("XOR")) unsupported_params = true;
        if (line.startsWith("KeyDerivationAlgo:") && !line.endsWith("MD5")) unsupported_params = true;
        if (line.startsWith("KeyDerivationPasses:") && !line.endsWith("10"))
            unsupported_params = true; // TODO: parse
        if (line.startsWith("Data:")) cypertextData = line.substring(strlen("Data:"));
    }

    cyphertextFile.close();

    if (unsupported_params || cypertextData.length() == 0) {
        Serial.println("err: invalid Encrypted file (altered?)");
        return "";
    }

    // else try decrypting
    cypertextData.trim();
    String cypertextDataDec = "";
    cypertextDataDec.reserve(cypertextData.length());

    uint8_t decimal = 0;
    char temp[3]; // Temporary storage for each hex pair

    for (int i = 0; i < cypertextData.length(); i += 3) {
        // Converts two characters hex to a single byte

        uint8_t highNibble = hexCharToDecimal(cypertextData[i]);
        uint8_t lowNibble = hexCharToDecimal(cypertextData[i + 1]);
        decimal = (highNibble << 4) | lowNibble;

        // Serial.println((char) decimal);

        // cypertextDataDec += decimal;
        // cypertextDataDec_index += 1;

        // temp[0] = cypertextData[i];        // First hex nibble
        // temp[1] = cypertextData[i + 1];    // Second hex nibble
        // temp[2] = '\0';                // Null-terminate the string

        // Convert the hex pair to a byte (char)
        // char decimal = (char) strtol(temp, NULL, 16);
        /*
        cypertextDataDec[i/3] = decimal;
        */
        cypertextDataDec += (char)decimal;
        // Serial.println(decimal);
    }

    // Serial.println(cachedPassword);
    // Serial.println(cypertextData);
    // Serial.println(cypertextDataDec);

    plaintext = xorEncryptDecryptMD5(cypertextDataDec, cachedPassword, 10);

    if (!isValidAscii(plaintext)) {
        // invalidate cached password -> will ask again on the next try
        cachedPassword = "";
        displayError("decryption failed (invalid password?)");
        // Serial.println(plaintext);
        return "";
    }
    // else
    return (plaintext);
}

// void writeEncryptedFile(FS &fs, String filepath, String& plaintext)

String encryptString(String &plaintext, const String &password_str) {
    String dataStr = xorEncryptDecryptMD5(plaintext, password_str, 10);
    String dataStrHex = "";

    for (size_t i = 0; i < dataStr.length(); i++) dataStrHex += String(dataStr[i], HEX) + " ";
    dataStrHex.toUpperCase();
    dataStrHex.trim();

    String out = "Filetype: Bruce Encrypted File\nVersion: 1\n";
    out += "Algo: XOR\n"; // TODO: add AES
    out += "KeyDerivationAlgo: MD5\n";
    out += "KeyDerivationPasses: 10\n";
    out += "Data: " + dataStrHex + "\n";

    return out;
}

/* OLD:
String decryptString(String& cypertext, const String& password_str)

  return xorEncryptDecryptMD5(cypertextData, password_str);
}
*/
