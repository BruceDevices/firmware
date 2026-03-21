#include "nrf_middleman.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <globals.h>

#define MM_PKT_SIZE 32
#define MM_CHANNELS 80
#define MM_BUFFER 16

struct MitmPkt {
    uint8_t data[MM_PKT_SIZE];
    uint8_t len;
    uint8_t ch;
    unsigned long ts;
};

static MitmPkt pktBuf[MM_BUFFER];
static int pktTotal = 0;
static int pktWIdx = 0;

static bool isEmptyPayload(uint8_t *payload, uint8_t len) {
    for (int i = 0; i < len; i++) {
        if (payload[i] != 0x00 && payload[i] != 0xFF) return false;
    }
    return true;
}

void nrf_middleman() {
    if (!nrf_start(NRF_MODE_SPI)) {
        displayError("NRF24 not found");
        delay(500);
        return;
    }

    NRFradio.setAutoAck(false);
    NRFradio.disableCRC();
    NRFradio.setAddressWidth(2);
    const uint8_t promAddr[][2] = {
        {0x55, 0x55}, {0xAA, 0xAA}, {0xA0, 0xAA},
        {0xAB, 0xAA}, {0xAC, 0xAA}, {0xAD, 0xAA}
    };
    for (uint8_t i = 0; i < 6; ++i) NRFradio.openReadingPipe(i, promAddr[i]);
    NRFradio.setDataRate(RF24_1MBPS);
    NRFradio.setPayloadSize(MM_PKT_SIZE);

    uint8_t activity[MM_CHANNELS] = {0};

    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawCentreString("NRF Middleman", tftWidth / 2, 10, 1);
    tft.setTextSize(FP);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setCursor(10, 40);
    tft.println("Scanning channels...");

    for (int scan = 0; scan < 8; scan++) {
        for (int ch = 0; ch < MM_CHANNELS; ch++) {
            NRFradio.setChannel(ch);
            NRFradio.startListening();
            delayMicroseconds(256);
            NRFradio.stopListening();
            if (NRFradio.testRPD()) activity[ch]++;
        }
        tft.fillRect(10, 58, tftWidth - 20, FP * LH, bruceConfig.bgColor);
        tft.setCursor(10, 58);
        tft.printf("Progress: %d%%", ((scan + 1) * 100) / 8);

        if (check(EscPress)) {
            NRFradio.stopListening();
            return;
        }
    }

    int bestCh = 0;
    int bestVal = 0;
    for (int i = 0; i < MM_CHANNELS; i++) {
        if (activity[i] > bestVal) {
            bestVal = activity[i];
            bestCh = i;
        }
    }

    int curCh = bestCh;
    pktTotal = 0;
    pktWIdx = 0;
    bool redraw = true;
    bool relayMode = false;
    int selPkt = 0;

    NRFradio.setChannel(curCh);
    NRFradio.startListening();

    while (!check(EscPress)) {
        if (NRFradio.available()) {
            uint8_t payload[MM_PKT_SIZE];
            NRFradio.read(payload, MM_PKT_SIZE);

            if (!isEmptyPayload(payload, MM_PKT_SIZE)) {
                int idx = pktWIdx % MM_BUFFER;
                memcpy(pktBuf[idx].data, payload, MM_PKT_SIZE);
                pktBuf[idx].len = MM_PKT_SIZE;
                pktBuf[idx].ch = curCh;
                pktBuf[idx].ts = millis();
                pktWIdx++;
                if (pktTotal < MM_BUFFER) pktTotal++;
                redraw = true;
            }
        }

        if (redraw) {
            tft.fillScreen(bruceConfig.bgColor);
            tft.setTextSize(FM);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.drawCentreString("NRF Middleman", tftWidth / 2, 5, 1);

            tft.setTextSize(FP);
            tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
            tft.setCursor(5, 26);
            tft.printf("CH:%d (%dMHz) PKT:%d", curCh, 2400 + curCh, pktTotal);

            tft.setCursor(5, 38);
            tft.printf("Mode: %s", relayMode ? "RELAY" : "SNIFF");

            int yPos = 54;
            int maxLines = (tftHeight - yPos - 5) / (FP * LH);
            int startI = (pktTotal > maxLines) ? pktTotal - maxLines : 0;

            for (int i = startI; i < pktTotal && yPos + FP * LH < tftHeight; i++) {
                int bIdx = i % MM_BUFFER;
                tft.setCursor(3, yPos);

                if (relayMode && i == selPkt)
                    tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
                else
                    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);

                char line[48];
                int showBytes = min((int)pktBuf[bIdx].len, 6);
                int pos = snprintf(line, sizeof(line), "%02d|", i);
                for (int b = 0; b < showBytes; b++)
                    pos += snprintf(line + pos, sizeof(line) - pos, "%02X", pktBuf[bIdx].data[b]);

                tft.print(line);
                yPos += FP * LH;
            }

            tft.drawRoundRect(2, 2, tftWidth - 4, tftHeight - 4, 5, bruceConfig.priColor);
            redraw = false;
        }

        if (check(NextPress)) {
            if (relayMode) {
                selPkt++;
                if (selPkt >= pktTotal) selPkt = 0;
            } else {
                NRFradio.stopListening();
                curCh++;
                if (curCh > 125) curCh = 0;
                NRFradio.setChannel(curCh);
                NRFradio.startListening();
                pktTotal = 0;
                pktWIdx = 0;
            }
            redraw = true;
        }

        if (check(PrevPress)) {
            if (relayMode) {
                selPkt--;
                if (selPkt < 0) selPkt = pktTotal - 1;
            } else {
                NRFradio.stopListening();
                curCh--;
                if (curCh < 0) curCh = 125;
                NRFradio.setChannel(curCh);
                NRFradio.startListening();
                pktTotal = 0;
                pktWIdx = 0;
            }
            redraw = true;
        }

        if (check(SelPress)) {
            if (relayMode && pktTotal > 0) {
                int bIdx = selPkt % MM_BUFFER;
                NRFradio.stopListening();
                NRFradio.openWritingPipe(promAddr[0]);
                NRFradio.write(pktBuf[bIdx].data, pktBuf[bIdx].len);
                NRFradio.startListening();
                tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
                tft.drawCentreString("TX!", tftWidth / 2, tftHeight / 2, 1);
                delay(300);
            } else {
                relayMode = !relayMode;
                if (relayMode && pktTotal > 0) selPkt = pktTotal - 1;
            }
            redraw = true;
        }

        delay(1);
    }

    NRFradio.stopListening();
}
