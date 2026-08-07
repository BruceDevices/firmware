#include "crash_diag.h"

#include "core/display.h"
#include <Arduino.h>
#include <esp_core_dump.h>
#include <esp_system.h>
#include <string.h>

// RISC-V ESP32 targets (ESP32-C3/C5/C6/H2/P4/...) expose a different coredump
// summary layout than Xtensa (ESP32/S2/S3): mcause/mtval and a raw stackdump
// instead of exc_cause/exc_vaddr and a decoded backtrace (bt[]/depth).
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C5) ||   \
    defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32C61) || \
    defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32P4)
#define BRUCE_CRASH_DIAG_RISCV 1
#endif

// ---------------------------------------------------------------------------
// Crash diagnostics via the ESP-IDF core dump.
//
// The Arduino/IDF panic handler writes an ELF core dump into the "coredump"
// flash partition (custom_16Mb.csv) on every panic/abort/watchdog reset.
// On the next boot we read back the panic reason, the crashing task name, the
// faulting PC and the backtrace with the coredump API (no panic hook needed -
// the Arduino core's set_arduino_panic_handler / --wrap hook is dead under
// -flto). The record is erased after being shown.
// ---------------------------------------------------------------------------

namespace {

const char *resetReasonText(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_UNKNOWN: return "UNKNOWN";
        case ESP_RST_POWERON: return "POWER-ON";
        case ESP_RST_EXT: return "EXTERNAL";
        case ESP_RST_SW: return "SOFTWARE";
        case ESP_RST_PANIC: return "PANIC (exception/abort)";
        case ESP_RST_INT_WDT: return "INTERRUPT WATCHDOG";
        case ESP_RST_TASK_WDT: return "TASK WATCHDOG (task hung)";
        case ESP_RST_WDT: return "WATCHDOG";
        case ESP_RST_DEEPSLEEP: return "DEEP-SLEEP WAKE";
        case ESP_RST_BROWNOUT: return "BROWNOUT (power)";
        case ESP_RST_SDIO: return "SDIO";
        default: return "?";
    }
}

bool abnormalReset(esp_reset_reason_t r) {
    return r == ESP_RST_PANIC || r == ESP_RST_INT_WDT || r == ESP_RST_TASK_WDT || r == ESP_RST_WDT ||
           r == ESP_RST_BROWNOUT;
}

void drawWrap(const char *text, uint8_t font, uint16_t color, uint16_t bg, int &y) {
    int maxCh = tftWidth / (font == FM ? 12 : 6);
    if (maxCh < 8) maxCh = 8;
    int len = strlen(text);
    int start = 0;
    while (start < len && y < tftHeight - 8) {
        int n = len - start;
        if (n > maxCh) n = maxCh;
        char line[260];
        memcpy(line, text + start, n);
        line[n] = '\0';
        tft.setTextColor(color, bg);
        tft.drawString(line, 4, y, font);
        y += (font == FM ? 14 : 8);
        start += n;
    }
}

} // namespace

void crashDiagShow() {
    esp_reset_reason_t reason = esp_reset_reason();
    if (!abnormalReset(reason)) return;

    char reasonStr[256] = {0};
    esp_core_dump_summary_t summary;
    memset(&summary, 0, sizeof(summary));
    bool haveSummary = false;
    bool haveCoredump = false;

    if (esp_core_dump_image_check() == ESP_OK) {
        haveCoredump = true;
        haveSummary = (esp_core_dump_get_summary(&summary) == ESP_OK);
        esp_core_dump_get_panic_reason(reasonStr, sizeof(reasonStr));
        esp_core_dump_image_erase(); // consume the record
    }

    // Always echo the details to serial: the backtrace addresses can be
    // resolved offline with xtensa-esp32s3-elf-addr2line (Xtensa) or
    // riscv32-esp-elf-gdb (RISC-V) against firmware.elf.
    Serial.printf("[CRASH DIAG] Reset: %s  coredump=%u summary=%u\n", resetReasonText(reason),
                  haveCoredump ? 1U : 0U, haveSummary ? 1U : 0U);
    if (haveSummary) {
        Serial.printf("[CRASH DIAG] Task: %s\n", summary.exc_task[0] ? summary.exc_task : "?");
#ifdef BRUCE_CRASH_DIAG_RISCV
        Serial.printf("[CRASH DIAG] PC:   0x%08X  Cause: 0x%02X  vaddr: 0x%08X\n", summary.exc_pc,
                      summary.ex_info.mcause, summary.ex_info.mtval);
        Serial.printf(
            "[CRASH DIAG] BT:   stackdump=%u bytes (decode offline with riscv32-esp-elf-gdb)\n",
            summary.exc_bt_info.dump_size
        );
#else
        Serial.printf("[CRASH DIAG] PC:   0x%08X  Cause: 0x%02X  vaddr: 0x%08X\n", summary.exc_pc,
                      summary.ex_info.exc_cause, summary.ex_info.exc_vaddr);
        Serial.printf("[CRASH DIAG] BT:   depth=%u corrupt=%u\n", summary.exc_bt_info.depth,
                      summary.exc_bt_info.corrupted ? 1U : 0U);
        for (uint32_t i = 0; i < summary.exc_bt_info.depth && i < 16; i++)
            Serial.printf("[CRASH DIAG]        %u: 0x%08X\n", i, summary.exc_bt_info.bt[i] & ~1u);
#endif
    }
    if (reasonStr[0]) Serial.printf("[CRASH DIAG] Reason: %s\n", reasonStr);

#ifdef HAS_SCREEN
    uint16_t bg = bruceConfig.bgColor;
    uint16_t hi = bruceConfig.secColor;
    uint16_t lo = bruceConfig.priColor;
    tft.fillScreen(bg);

    int y = 2;
    tft.setTextSize(FM);
    tft.setTextColor(hi, bg);
    tft.drawString("CRASH DIAG", 4, y, 1);
    y += 15;

    tft.setTextSize(FP);
    tft.setTextColor(lo, bg);
    tft.drawString("Reset: " + String(resetReasonText(reason)), 4, y, 1);
    y += 9;

    if (haveSummary) {
        if (summary.exc_task[0]) {
            tft.setTextColor(lo, bg);
            tft.drawString("Task: " + String(summary.exc_task), 4, y, 1);
            y += 9;
        }
        drawWrap("Reason: ", FP, hi, bg, y);
        if (reasonStr[0]) {
            drawWrap(reasonStr, FP, lo, bg, y);
        } else {
            char causeLine[48];
#ifdef BRUCE_CRASH_DIAG_RISCV
            snprintf(causeLine, sizeof(causeLine), "Cause: 0x%02X vaddr 0x%08X", summary.ex_info.mcause,
                     summary.ex_info.mtval);
#else
            snprintf(causeLine, sizeof(causeLine), "Cause: 0x%02X vaddr 0x%08X", summary.ex_info.exc_cause,
                     summary.ex_info.exc_vaddr);
#endif
            drawWrap(causeLine, FP, lo, bg, y);
        }
        char pcLine[32];
        snprintf(pcLine, sizeof(pcLine), "PC: 0x%08X", summary.exc_pc);
        tft.setTextColor(lo, bg);
        tft.drawString(pcLine, 4, y, 1);
        y += 9;
#ifdef BRUCE_CRASH_DIAG_RISCV
        char btLine[48];
        snprintf(btLine, sizeof(btLine), "BT: stackdump %u bytes", summary.exc_bt_info.dump_size);
        tft.setTextColor(lo, bg);
        tft.drawString(btLine, 4, y, 1);
        y += 9;
#else
        for (uint32_t i = 0; i < summary.exc_bt_info.depth && i < 16 && y < tftHeight - 8; i++) {
            char btLine[32];
            snprintf(btLine, sizeof(btLine), "  %u: 0x%08X", i, summary.exc_bt_info.bt[i] & ~1u);
            tft.setTextColor(lo, bg);
            tft.drawString(btLine, 4, y, 1);
            y += 9;
        }
#endif
    } else if (haveCoredump) {
        tft.setTextColor(hi, bg);
        tft.drawString("Core dump present but", 4, y, 1);
        y += 9;
        tft.drawString("summary parse failed.", 4, y, 1);
    } else {
        tft.setTextColor(hi, bg);
        tft.drawString("No core dump record.", 4, y, 1);
        y += 9;
        tft.drawString("See serial for details.", 4, y, 1);
    }

    tft.setTextColor(hi, bg);
    tft.drawString("Reboot in 8s...", 4, tftHeight - 8, 1);
    for (uint32_t i = 0; i < 8000; i++) delay(1);
    tft.fillScreen(bg);
#else
    if (reasonStr[0]) Serial.println(reasonStr);
#endif
}
