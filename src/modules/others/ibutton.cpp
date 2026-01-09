#ifndef LITE_VERSION
#include "ibutton.h"
#include "core/display.h"
#include "core/mykeyboard.h"

#define ONE_WIRE_BUS 0

OneWire *oneWire;
byte buffer[8];

void setup_ibutton() {
Reset:
    oneWire = new OneWire(bruceConfigPins.iButton);
    tft.fillScreen(TFT_BLACK);
    setiButtonPinMenu();
    drawMainBorderWithTitle("iButton");
    tft.setCursor(10, 50);
    padprintln("Waiting for signal");
    padprintln("press [Next] to setup");
    delay(100);

    for (;;) {
        if (check(EscPress)) {
            returnToMenu = true;
            delete oneWire;
            break;
        }
        if (check(NextPress)) {
            setiButtonPinMenu();
            delete oneWire;
            goto Reset;
        }
        // iButton is plugged
        if (oneWire->reset() != 0) {
            // Main Button is pressed
            if (check(SelPress)) {
                write_ibutton();
            } else {
                read_ibutton();
            }
            delay(500);
        }
    }
}

void write_byte_rw1990(byte data) {
    int data_bit;
    uint8_t pin = bruceConfigPins.iButton;
    for (data_bit = 0; data_bit < 8; data_bit++) {
        if (data & 1) {
            digitalWrite(pin, LOW);
            pinMode(pin, OUTPUT);
            delayMicroseconds(60);
            pinMode(pin, INPUT);
            digitalWrite(pin, HIGH);
        } else {
            digitalWrite(pin, LOW);
            pinMode(pin, OUTPUT);
            pinMode(pin, INPUT);
            digitalWrite(pin, HIGH);
        }
        delay(10);
        data = data >> 1;
    }
}

// Not working (((
// void write_byte_rw1990(byte data) {
//     for (int data_bit = 0; data_bit < 8; data_bit++) {
//         delay(25);
//         // oneWire->write_bit(~data);
//         oneWire->write_bit(data & 0x01);
//         data >>= 1;
//     }
// }

void write_ibutton() {

    // Dislay ID
    tft.fillScreen(TFT_BLACK);
    drawMainBorderWithTitle("iButton Write");
    tft.setCursor(11, 50);
    tft.print("Current buffer:");
    tft.setCursor(40, 57);
    for (byte i = 0; i < 8; i++) {
        tft.print(buffer[i], HEX);
        tft.print(":");
    }
    delay(1000);
    tft.setCursor(52, 102);
    tft.print("Wait...");
    tft.setCursor(110, 102);

    tft.print('-');
    oneWire->skip();
    oneWire->reset();
    oneWire->write(0x33); // Read ROM

    oneWire->skip();
    oneWire->reset();
    oneWire->write(0x3C); // Set write mode for some models
    tft.print('-');
    delay(50);

    oneWire->skip();
    oneWire->reset();
    oneWire->write(0xD1); // Write command
    tft.print('-');
    delay(50);

    // Write don't work without this code
    uint8_t pin = bruceConfigPins.iButton;
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
    delayMicroseconds(60);
    pinMode(pin, INPUT);
    digitalWrite(pin, HIGH);
    delay(10);

    oneWire->skip();
    oneWire->reset();
    oneWire->write(0xD5); // Enter write mode
    tft.print('-');
    delay(50);
    tft.print('>');
    for (byte i = 0; i < 8; i++) {
        write_byte_rw1990(buffer[i]); // Write each byte
        tft.print('*');
        delayMicroseconds(25);
    }
    oneWire->reset(); // Reset bus
    oneWire->skip();

    // Step 3 : Finalise
    oneWire->write(0xD1); // End of write command
    delayMicroseconds(16);
    oneWire->reset(); // Reset bus

    // Display end of copy
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(90, 50);
    tft.setTextSize(FM);
    displayTextLine("COPIED");
    tft.setCursor(40, 80);
    tft.print("Release button");

    delay(3000);

    tft.fillScreen(TFT_BLACK);
    drawMainBorderWithTitle("iButton");
    tft.setCursor(10, 60);
    displayTextLine("Waiting iButton...");
}

void read_ibutton() {
    oneWire->write(0x33);           // Read ID command
    oneWire->read_bytes(buffer, 8); // Read ID

    // Display iButton
    tft.fillScreen(TFT_BLACK);
    drawMainBorderWithTitle("iButton ID");

    // Dislay ID
    tft.setTextSize(1.75);
    tft.setCursor(12, 57);
    for (byte i = 0; i < 8; i++) {
        tft.print(buffer[i], HEX);
        tft.print(":");
    }

    if (OneWire::crc8(buffer, 7) != buffer[7]) {
        tft.setCursor(55, 85);
        tft.setTextSize(FM);
        tft.setTextColor(TFT_RED);
        tft.println("CRC ERROR!");
    } else {
        // Display copy infos
        tft.setCursor(55, 85);
        tft.setTextSize(1.5);
        tft.println("Hold OK to copy");
    }
}

/*********************************************************************
**  Function: setiButtonPin
**  Main Menu to manually iButton Pin
**********************************************************************/
void setiButtonPinMenu() {
    options = {};
    gpio_num_t sel = GPIO_NUM_NC;
    for (int8_t i = -1; i <= GPIO_NUM_MAX; i++) {
        String tmp = "GPIO " + String(i);
        options.push_back({tmp.c_str(), [i, &sel]() { sel = (gpio_num_t)i; }});
    }
    loopOptions(options, bruceConfigPins.iButton + 1);
    options.clear();
    bruceConfigPins.setiButtonPin(sel);
}
#endif
