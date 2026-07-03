#include "type_convertion.h"

String hexStrToBinStr(const String &hexStr) {
    String binStr = "";
    String hexByte = "";

    // Variables for decimal value
    int value;

    for (int i = 0; i < hexStr.length(); i++) {
        char c = hexStr.charAt(i);

        // Check if the character is a hexadecimal digit
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
            hexByte += c;
            if (hexByte.length() == 2) {
                // Convert the hexadecimal pair to a decimal value
                value = strtol(hexByte.c_str(), NULL, 16);

                // Convert the decimal value to binary and add to the binary string
                for (int j = 7; j >= 0; j--) { binStr += (value & (1 << j)) ? '1' : '0'; }
                // binStr += ' ';

                // Clear the hexByte string for the next byte
                hexByte = "";
            }
        }
    }

    // Remove the extra trailing space, if any
    if (binStr.length() > 0 && binStr.charAt(binStr.length() - 1) == ' ') {
        binStr.remove(binStr.length() - 1);
    }

    return binStr;
}

void decimalToHexString(uint64_t decimal, char *output) {
    char hexDigits[] = "0123456789ABCDEF";
    char temp[65];
    int index = 15;

    // Initialize tem string with zeros
    for (int i = 0; i < 64; i++) { temp[i] = '0'; }
    temp[65] = '\0';

    // Convert decimal to hexadecimal
    while (decimal > 0) {
        temp[index--] = hexDigits[decimal % 16];
        decimal /= 16;
    }

    // Format string with spaces
    int outputIndex = 0;
    for (int i = 0; i < 16; i++) {
        output[outputIndex++] = temp[i];
        if ((i % 2) == 1 && i != 15) { output[outputIndex++] = ' '; }
    }
    output[outputIndex] = '\0';
}

// Converts a Hex char to decimal
uint8_t hexCharToDecimal(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return 0;
}

// converts a Hex string like "11 22 AE FF"  to decimal
uint32_t hexStringToDecimal(const char *hexString) {
    uint32_t decimal = 0;
    int length = strlen(hexString);

    for (int i = 0; i < length; i += 3) {
        decimal <<= 8; // Shift left to accommodate next byte

        // Converts two characters hex to a single byte
        uint8_t highNibble = hexCharToDecimal(hexString[i]);
        uint8_t lowNibble = hexCharToDecimal(hexString[i + 1]);
        decimal |= (highNibble << 4) | lowNibble;
    }

    return decimal;
}

char *dec2binWzerofill(uint64_t Dec, unsigned int bitLength) {
    // Allocate memory dynamically for safety
    char *bin = (char *)malloc(bitLength + 1);
    if (!bin) return NULL; // Handle allocation failure

    bin[bitLength] = '\0'; // Null-terminate string

    for (int i = bitLength - 1; i >= 0; i--) {
        bin[i] = (Dec & 1) ? '1' : '0';
        Dec >>= 1;
    }

    return bin;
}

String hexToStr(uint8_t *data, uint8_t len, char separator) {
    String str = "";

    for (size_t i = 0; i < len; i++) {
        str += separator;
        if (data[i] < 0x10) str += '0';
        str += String(data[i], HEX);
    }

    str.trim();
    str.toUpperCase();
    return str;
}
