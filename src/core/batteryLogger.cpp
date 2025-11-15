#include "batteryLogger.h"

#include "core/scrollableTextArea.h"
#include "core/sd_functions.h"
#include "display.h"
#include <globals.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace BatteryLogger {
namespace {
constexpr const char *kLogFilePath = "/battery_log.csv";
constexpr size_t kMaxGraphEntries = 240;

uint32_t intervalMs = 0;
uint32_t lastLogMs = 0;
bool pendingImmediateSample = false;
bool clearedOnFullCharge = false;

struct LogEntry {
    String timestamp;
    int percent = 0;
    float voltage = 0.0f;
    bool voltageValid = false;
};

String makeTimestamp() {
    struct tm timeinfo = rtc.getTimeStruct();
    char buffer[24];
    if (clock_set && timeinfo.tm_year >= 70) {
        snprintf(
            buffer,
            sizeof(buffer),
            "%04d-%02d-%02d %02d:%02d:%02d",
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec
        );
        return String(buffer);
    }

    snprintf(buffer, sizeof(buffer), "T+%lu", millis() / 1000UL);
    return String(buffer);
}

bool ensureWritableFs(FS *&fs) { return getFsStorage(fs); }

bool selectFsForReading(FS *&fs) {
    if ((sdcardMounted || setupSdCard()) && SD.exists(kLogFilePath)) {
        fs = &SD;
        return true;
    }

    if (LittleFS.exists(kLogFilePath)) {
        fs = &LittleFS;
        return true;
    }

    return getFsStorage(fs);
}

void appendSample() {
    int percent = getBattery();

    FS *fs = nullptr;
    if (!ensureWritableFs(fs)) return;

    if (percent >= 100) {
        if (!clearedOnFullCharge && fs != nullptr) {
            fs->remove(kLogFilePath);
            clearedOnFullCharge = true;
        }
    } else if (percent <= 98) {
        clearedOnFullCharge = false;
    }

    File file = fs->open(kLogFilePath, FILE_APPEND);
    if (!file) {
        log_e("BatteryLogger: failed to open %s", kLogFilePath);
        return;
    }

    if (file.size() == 0) file.println("timestamp,percent,voltage");

    String timestamp = makeTimestamp();
    float voltage = getBatteryVoltage();
    String voltageToken = voltage > 0.0f ? String(voltage, 3) : String("NA");

    file.printf("%s,%d,%s\n", timestamp.c_str(), percent, voltageToken.c_str());
    file.close();
}

bool parseLine(const String &line, LogEntry &entry) {
    int firstComma = line.indexOf(',');
    if (firstComma < 0) return false;
    int secondComma = line.indexOf(',', firstComma + 1);
    if (secondComma < 0) return false;

    entry.timestamp = line.substring(0, firstComma);
    entry.percent = line.substring(firstComma + 1, secondComma).toInt();

    String voltageToken = line.substring(secondComma + 1);
    voltageToken.trim();
    entry.voltageValid = !voltageToken.equalsIgnoreCase("NA") && !voltageToken.isEmpty();
    entry.voltage = entry.voltageValid ? voltageToken.toFloat() : 0.0f;
    return true;
}

bool readEntries(std::vector<LogEntry> &entries) {
    FS *fs = nullptr;
    if (!selectFsForReading(fs)) return false;
    if (!fs->exists(kLogFilePath)) return false;

    File file = fs->open(kLogFilePath, FILE_READ);
    if (!file) return false;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.isEmpty() || line.startsWith("timestamp")) continue;

        LogEntry entry;
        if (parseLine(line, entry)) entries.emplace_back(std::move(entry));
    }

    file.close();

    if (entries.size() > kMaxGraphEntries) {
        entries.erase(entries.begin(), entries.end() - kMaxGraphEntries);
    }
    return !entries.empty();
}

void waitForExit() {
    while (check(SelPress)) { vTaskDelay(pdMS_TO_TICKS(50)); }
    while (check(EscPress)) { vTaskDelay(pdMS_TO_TICKS(50)); }

    while (true) {
        if (check(SelPress) || check(EscPress)) break;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void renderGraph(const std::vector<LogEntry> &entries) {
    drawMainBorderWithTitle("Battery log", true);
    int16_t margin = BORDER_PAD_X + 8;
    int16_t top = BORDER_PAD_Y + 42;
    int16_t bottomPad = BORDER_PAD_Y + 40;
    int16_t graphWidth = tftWidth - 2 * margin;
    if (graphWidth < 10) graphWidth = tftWidth - 2 * BORDER_PAD_X;
    int16_t graphHeight = tftHeight - top - bottomPad;
    if (graphHeight < 30) graphHeight = (tftHeight > 40) ? tftHeight - top - BORDER_PAD_Y : 30;

    tft.fillRect(margin, top, graphWidth, graphHeight, bruceConfig.bgColor);
    tft.drawRect(margin, top, graphWidth, graphHeight, bruceConfig.priColor);

    if (entries.size() < 2) {
        tft.drawCentreString("Need more samples", tftWidth / 2, top + graphHeight / 2, SMOOTH_FONT);
        printCenterFootnote("Sel/Esc to exit");
        waitForExit();
        return;
    }

    bool hasVoltage = false;
    float minVolt = std::numeric_limits<float>::max();
    float maxVolt = std::numeric_limits<float>::lowest();
    for (const auto &entry : entries) {
        if (!entry.voltageValid) continue;
        hasVoltage = true;
        minVolt = std::min(minVolt, entry.voltage);
        maxVolt = std::max(maxVolt, entry.voltage);
    }
    if (hasVoltage && fabsf(maxVolt - minVolt) < 0.01f) maxVolt = minVolt + 0.05f;

    auto xForIndex = [&](size_t idx) -> int16_t {
        if (entries.size() <= 1) return margin;
        float ratio = static_cast<float>(idx) / static_cast<float>(entries.size() - 1);
        return margin + static_cast<int16_t>(ratio * (graphWidth - 1));
    };

    auto yForPercent = [&](int percentValue) -> int16_t {
        if (percentValue < 0) percentValue = 0;
        else if (percentValue > 100) percentValue = 100;
        float ratio = percentValue / 100.0f;
        return top + graphHeight - 1 - static_cast<int16_t>(ratio * (graphHeight - 1));
    };

    auto yForVoltage = [&](float voltage) -> int16_t {
        if (!hasVoltage) return top + graphHeight - 1;
        float ratio = (voltage - minVolt) / (maxVolt - minVolt);
        if (ratio < 0.0f) ratio = 0.0f;
        else if (ratio > 1.0f) ratio = 1.0f;
        return top + graphHeight - 1 - static_cast<int16_t>(ratio * (graphHeight - 1));
    };

    uint16_t gridColor = bruceConfig.secColor ? bruceConfig.secColor : TFT_DARKGREY;
    for (int i = 1; i < 5; ++i) {
        int16_t y = top + (graphHeight * i) / 5;
        tft.drawLine(margin + 1, y, margin + graphWidth - 1, y, gridColor);
    }

    for (size_t i = 1; i < entries.size(); ++i) {
        tft.drawLine(
            xForIndex(i - 1),
            yForPercent(entries[i - 1].percent),
            xForIndex(i),
            yForPercent(entries[i].percent),
            TFT_GREEN
        );
    }

    if (hasVoltage) {
        bool havePrev = false;
        int16_t lastX = 0;
        int16_t lastY = 0;
        for (size_t i = 0; i < entries.size(); ++i) {
            if (!entries[i].voltageValid) continue;
            int16_t x = xForIndex(i);
            int16_t y = yForVoltage(entries[i].voltage);
            if (havePrev) tft.drawLine(lastX, lastY, x, y, TFT_CYAN);
            havePrev = true;
            lastX = x;
            lastY = y;
        }
    } else {
        tft.drawCentreString("No voltage data", tftWidth / 2, top + graphHeight / 2, SMOOTH_FONT);
    }

    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString("100%", margin - 38, top - FP / 2);
    tft.drawString("0%", margin - 26, top + graphHeight - FP * (LH - 1));

    if (hasVoltage) {
        int16_t rightLabelX = margin + graphWidth + 4;
        if (rightLabelX > tftWidth - BORDER_PAD_X) rightLabelX = tftWidth - BORDER_PAD_X;
        tft.drawRightString(String(maxVolt, 2) + "V", rightLabelX, top - FP / 2, 1);
        tft.drawRightString(String(minVolt, 2) + "V", rightLabelX, top + graphHeight - FP * (LH - 1), 1);
    }

    int16_t legendY = BORDER_PAD_Y + 10;
    int16_t legendX = margin;
    tft.drawLine(legendX, legendY + 6, legendX + 24, legendY + 6, TFT_GREEN);
    tft.drawString("Charge %", legendX + 30, legendY, 1);
    legendX += 140;
    if (legendX + 80 > margin + graphWidth) legendX = margin + graphWidth - 120;
    if (legendX < margin) legendX = margin;
    tft.drawLine(legendX, legendY + 6, legendX + 24, legendY + 6, TFT_CYAN);
    tft.drawString("Voltage", legendX + 30, legendY, 1);

    if (!entries.empty()) {
        tft.drawCentreString(
            entries.front().timestamp + " -> " + entries.back().timestamp,
            tftWidth / 2,
            top + graphHeight + 8,
            SMOOTH_FONT
        );
    }

    printCenterFootnote("Sel/Esc to exit");
    waitForExit();
}
} // namespace

void begin() {
    updateIntervalFromConfig(false);
    lastLogMs = millis();
}

void update() {
    if (!intervalMs) return;
    uint32_t now = millis();
    if (!pendingImmediateSample && now - lastLogMs < intervalMs) return;

    pendingImmediateSample = false;
    lastLogMs = now;
    appendSample();
}

void updateIntervalFromConfig(bool immediateSample) {
    if (bruceConfig.batteryLogInterval <= 0) {
        intervalMs = 0;
        pendingImmediateSample = false;
        return;
    }

    intervalMs = static_cast<uint32_t>(bruceConfig.batteryLogInterval) * 1000UL;
    pendingImmediateSample = immediateSample;
    lastLogMs = millis();
}

bool deleteLogFile() {
    bool removed = false;
    if (LittleFS.exists(kLogFilePath)) { removed |= LittleFS.remove(kLogFilePath); }

    if (sdcardMounted || setupSdCard()) {
        if (SD.exists(kLogFilePath)) { removed |= SD.remove(kLogFilePath); }
    }

    return removed;
}

bool logFileExists() {
    if (LittleFS.exists(kLogFilePath)) return true;
    if (sdcardMounted || setupSdCard()) return SD.exists(kLogFilePath);
    return false;
}

void showLogAsText() {
    FS *fs = nullptr;
    if (!selectFsForReading(fs) || !fs->exists(kLogFilePath)) {
        displayWarning("Log file not found", true);
        return;
    }

    File file = fs->open(kLogFilePath, FILE_READ);
    if (!file) {
        displayWarning("Can't open log file", true);
        return;
    }

    ScrollableTextArea area("BATTERY LOG");
    area.addLine("TIME | LEVEL | VOLTAGE");
    bool hasData = false;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.isEmpty() || line.startsWith("timestamp")) continue;

        LogEntry entry;
        if (!parseLine(line, entry)) continue;
        hasData = true;
        String row = entry.timestamp + " | " + String(entry.percent) + "% | ";
        row += entry.voltageValid ? String(entry.voltage, 3) + "V" : "N/A";
        area.addLine(row);
    }

    file.close();
    if (!hasData) area.addLine("No samples yet");
    area.show();
}

void showLogAsGraph() {
    std::vector<LogEntry> entries;
    entries.reserve(kMaxGraphEntries);
    if (!readEntries(entries)) {
        displayWarning("Not enough samples", true);
        return;
    }

    renderGraph(entries);
}

const char *logFilePath() { return kLogFilePath; }
} // namespace BatteryLogger
