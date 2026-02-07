#include "rf_jammer.h"
#include "core/display.h"
#include "rf_utils.h"

static const uint32_t MAX_JAM_TIME_MS = 20000;
static const uint32_t MAX_SEQUENCE = 50;
static const uint32_t DURATION_CYCLES = 3;

RFJammer::RFJammer(bool full) : fullJammer(full) { 
    setup(); 
}

RFJammer::~RFJammer() { 
    deinitRfModule(); 
}

void RFJammer::setup() {
    nTransmitterPin = bruceConfigPins.rfTx;
    if (!initRfModule("tx")) return;

    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
        nTransmitterPin = bruceConfigPins.CC1101_bus.io0;
    }

    sendRF = true;
    display_banner();

    if (fullJammer) run_full_jammer();
    else run_itmt_jammer();
}

void RFJammer::display_banner() {
    drawMainBorderWithTitle("RF Jammer");
    printSubtitle(String(fullJammer ? "Full Jammer" : "Intermittent Jammer"));
    padprintln("Sending...");
    padprintln("");
    padprintln("");
    
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    padprintln("Press [ESC] for options.");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
}

void RFJammer::run_full_jammer() {
    digitalWrite(nTransmitterPin, HIGH);
    uint32_t startTime = millis();
    uint32_t lastCheckTime = startTime;
    
    while (sendRF) {
        static uint8_t microPulse = 0;
        
        if (micros() % 100 < 2) {
            digitalWrite(nTransmitterPin, LOW);
            delayMicroseconds(1);
            digitalWrite(nTransmitterPin, HIGH);
        }
        
        uint32_t currentTime = millis();
        if (currentTime - lastCheckTime > 100) {
            lastCheckTime = currentTime;
            
            if (check(EscPress) || (currentTime - startTime > MAX_JAM_TIME_MS)) {
                sendRF = false;
                returnToMenu = true;
                break;
            }
        }
        
        if (currentTime % 500 < 10) {
            digitalWrite(nTransmitterPin, LOW);
            delayMicroseconds(5);
            digitalWrite(nTransmitterPin, HIGH);
        }
    }
    digitalWrite(nTransmitterPin, LOW);
}

void RFJammer::run_itmt_jammer() {
    uint32_t startTime = millis();
    uint32_t lastCheckTime = startTime;
    
    uint32_t sequenceValues[MAX_SEQUENCE];
    for (int i = 0; i < MAX_SEQUENCE; i++) {
        sequenceValues[i] = 10 * (i + 1);
    }
    
    while (sendRF) {
        for (int sequence = 0; sequence < MAX_SEQUENCE && sendRF; sequence++) {
            uint32_t pulseWidth = sequenceValues[sequence];
            
            for (int duration = 0; duration < DURATION_CYCLES && sendRF; duration++) {
                send_optimized_pulse(pulseWidth);
                
                uint32_t currentTime = millis();
                if (currentTime - lastCheckTime > 50) {
                    lastCheckTime = currentTime;
                    
                    if (check(EscPress) || (currentTime - startTime > MAX_JAM_TIME_MS)) {
                        sendRF = false;
                        returnToMenu = true;
                        break;
                    }
                }
            }
        }
        
        if (sendRF) {
            send_random_pattern(100);
        }
    }
    digitalWrite(nTransmitterPin, LOW);
}

void RFJammer::send_optimized_pulse(int width) {
    digitalWrite(nTransmitterPin, HIGH);
    
    for (uint32_t i = 0; i < width; i += 10) {
        digitalWrite(nTransmitterPin, HIGH);
        delayMicroseconds(5);
        
        if (i % 20 == 0) {
            digitalWrite(nTransmitterPin, LOW);
            delayMicroseconds(2);
            digitalWrite(nTransmitterPin, HIGH);
        }
        
        delayMicroseconds(5);
    }
    
    digitalWrite(nTransmitterPin, LOW);
    
    uint32_t lowPeriod = width + (width % 23);
    for (uint32_t i = 0; i < lowPeriod; i += 10) {
        digitalWrite(nTransmitterPin, LOW);
        delayMicroseconds(10);
    }
}

void RFJammer::send_random_pattern(int numPulses) {
    uint32_t startTime = millis();
    
    for (int i = 0; i < numPulses && sendRF; i++) {
        uint32_t pulseWidth = 5 + (millis() % 46);
        
        digitalWrite(nTransmitterPin, HIGH);
        delayMicroseconds(pulseWidth);
        
        digitalWrite(nTransmitterPin, LOW);
        
        uint32_t spaceWidth = 5 + (micros() % 96);
        delayMicroseconds(spaceWidth);
        
        if (millis() - startTime > 100) {
            break;
        }
    }
}
