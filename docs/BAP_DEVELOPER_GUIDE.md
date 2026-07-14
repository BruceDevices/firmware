# Bruce Application Development Guide
## HÆ°á»›ng Dáº«n PhÃ¡t Triá»ƒn á»¨ng Dá»¥ng BAP (Bruce App Package)

> TÃ i liá»‡u nÃ y dÃ nh cho láº­p trÃ¬nh viÃªn muá»‘n viáº¿t á»©ng dá»¥ng cháº¡y trÃªn firmware Bruce.
> Há»‡ thá»‘ng BAP cho phÃ©p báº¡n viáº¿t app báº±ng C/C++, biÃªn dá»‹ch ngoÃ i firmware, vÃ  náº¡p Ä‘á»™ng (dynamic loading) vÃ o ESP32 thÃ´ng qua tháº» SD hoáº·c LittleFS â€” tÆ°Æ¡ng tá»± FAP trÃªn Flipper Zero.

---

## Má»¥c Lá»¥c

1. [Tá»•ng Quan Kiáº¿n TrÃºc](#1-tá»•ng-quan-kiáº¿n-trÃºc)
2. [CÃ i Äáº·t MÃ´i TrÆ°á»ng](#2-cÃ i-Ä‘áº·t-mÃ´i-trÆ°á»ng)
3. [Táº¡o Dá»± Ãn Má»›i](#3-táº¡o-dá»±-Ã¡n-má»›i)
4. [Cáº¥u TrÃºc ThÆ° Má»¥c](#4-cáº¥u-trÃºc-thÆ°-má»¥c)
5. [File Manifest (application.bam)](#5-file-manifest-applicationbam)
6. [Viáº¿t Code á»¨ng Dá»¥ng](#6-viáº¿t-code-á»©ng-dá»¥ng)
7. [Bruce SDK API Reference](#7-bruce-sdk-api-reference)
8. [Sá»­ Dá»¥ng Assets (HÃ¬nh áº¢nh)](#8-sá»­-dá»¥ng-assets-hÃ¬nh-áº£nh)
9. [Sá»­ Dá»¥ng ThÆ° Viá»‡n BÃªn NgoÃ i](#9-sá»­-dá»¥ng-thÆ°-viá»‡n-bÃªn-ngoÃ i)
10. [Build vÃ  ÄÃ³ng GÃ³i](#10-build-vÃ -Ä‘Ã³ng-gÃ³i)
11. [Deploy lÃªn Thiáº¿t Bá»‹](#11-deploy-lÃªn-thiáº¿t-bá»‹)
12. [Debug vÃ  Xá»­ LÃ½ Lá»—i](#12-debug-vÃ -xá»­-lÃ½-lá»—i)
13. [Quy Táº¯c vÃ  Giá»›i Háº¡n](#13-quy-táº¯c-vÃ -giá»›i-háº¡n)
14. [VÃ­ Dá»¥ Máº«u](#14-vÃ­-dá»¥-máº«u)
15. [FAQ â€” CÃ¢u Há»i ThÆ°á»ng Gáº·p](#15-faq--cÃ¢u-há»i-thÆ°á»ng-gáº·p)

---

## 1. Tá»•ng Quan Kiáº¿n TrÃºc

### Há»‡ thá»‘ng hoáº¡t Ä‘á»™ng nhÆ° tháº¿ nÃ o?

```
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚                  BRUCE FIRMWARE                     â”‚
â”‚                                                     â”‚
â”‚  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”    â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”    â”‚
â”‚  â”‚  Apps Menu   â”‚â”€â”€â”€â–¶â”‚   BAP Loader (ELF)       â”‚    â”‚
â”‚  â”‚  (AppsMenu)  â”‚    â”‚   - Read BapHeader       â”‚    â”‚
â”‚  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜    â”‚   - Load ELF into RAM     â”‚    â”‚
â”‚                      â”‚   - Relocate addresses    â”‚    â”‚
â”‚                      â”‚   - Call app_main(api)    â”‚    â”‚
â”‚                      â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜    â”‚
â”‚                                 â”‚                    â”‚
â”‚  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”    â”‚
â”‚  â”‚         Jump Table (BruceAPI struct)          â”‚    â”‚
â”‚  â”‚  draw_string, draw_image, bruce_malloc, ...   â”‚    â”‚
â”‚  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜    â”‚
â”‚                                                     â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
         â–²
         â”‚ Giao tiáº¿p qua con trá» hÃ m
         â”‚
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”´â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚              YOUR APP (.bruce file)                   â”‚
â”‚                                                     â”‚
â”‚  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”     â”‚
â”‚  â”‚ app.c   â”‚  â”‚ assets/  â”‚  â”‚ third_party/   â”‚     â”‚
â”‚  â”‚         â”‚  â”‚ logo.png â”‚  â”‚ custom_lib.c   â”‚     â”‚
â”‚  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜     â”‚
â”‚                                                     â”‚
â”‚  application.bam (manifest)                         â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
```

**NguyÃªn táº¯c cá»‘t lÃµi:**
- App cá»§a báº¡n **khÃ´ng gá»i trá»±c tiáº¿p** cÃ¡c hÃ m firmware. Thay vÃ o Ä‘Ã³, firmware truyá»n má»™t con trá» `BruceAPI*` chá»©a táº¥t cáº£ cÃ¡c hÃ m Ä‘Æ°á»£c phÃ©p sá»­ dá»¥ng.
- App Ä‘Æ°á»£c biÃªn dá»‹ch thÃ nh file ELF (Position Independent Code), Ä‘Ã³ng gÃ³i vÃ o `.bruce` vá»›i header metadata.
- Firmware Ä‘á»c header, náº¡p ELF vÃ o RAM, thá»±c hiá»‡n relocation, rá»“i gá»i hÃ m `app_main()`.

---

## 2. CÃ i Äáº·t MÃ´i TrÆ°á»ng

### YÃªu cáº§u há»‡ thá»‘ng

| ThÃ nh pháº§n | PhiÃªn báº£n | Má»¥c Ä‘Ã­ch |
|-----------|---------|---------|
| Python | 3.10 â€“ 3.13 | Cháº¡y `bbt.py` (build tool) |
| xtensa-esp32-elf-gcc | ESP-IDF v5.x | Cross-compiler cho ESP32 |
| Pillow (Python) | Báº¥t ká»³ | BiÃªn dá»‹ch assets (hÃ¬nh áº£nh) |

### BÆ°á»›c 1: CÃ i toolchain Xtensa

CÃ¡ch nhanh nháº¥t lÃ  cÃ i **ESP-IDF** (https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/).

Sau khi cÃ i, cháº¡y:
```bash
# TrÃªn Windows (PowerShell)
C:\Espressif\frameworks\esp-idf\export.ps1

# TrÃªn Linux/macOS
source $HOME/esp/esp-idf/export.sh
```

Kiá»ƒm tra:
```bash
xtensa-esp32-elf-gcc --version
# Pháº£i in ra phiÃªn báº£n GCC (vÃ­ dá»¥: 13.2.0)
```

### BÆ°á»›c 2: CÃ i Pillow (tÃ¹y chá»n â€” chá»‰ cáº§n khi dÃ¹ng assets)

```bash
pip install Pillow
```

### BÆ°á»›c 3: Clone firmware repository

```bash
git clone https://github.com/your-org/bruce-firmware.git
cd bruce-firmware
```

File `bbt.py` náº±m á»Ÿ thÆ° má»¥c gá»‘c cá»§a firmware.

---

## 3. Táº¡o Dá»± Ãn Má»›i

### CÃ¡ch nhanh: DÃ¹ng scaffold

```bash
cd bruce-firmware
python bbt.py create my_cool_app
```

Lá»‡nh nÃ y táº¡o ra thÆ° má»¥c `my_cool_app/` vá»›i Ä‘áº§y Ä‘á»§ cáº¥u trÃºc:
```
my_cool_app/
â”œâ”€â”€ application.bam       # Manifest
â”œâ”€â”€ app.c                 # Source code máº«u
â””â”€â”€ assets/               # ThÆ° má»¥c chá»©a hÃ¬nh áº£nh (trá»‘ng)
```

### CÃ¡ch thá»§ cÃ´ng

Báº¡n cÅ©ng cÃ³ thá»ƒ tá»± táº¡o thÆ° má»¥c vÃ  cÃ¡c file báº±ng tay. Xem pháº§n tiáº¿p theo Ä‘á»ƒ biáº¿t cáº¥u trÃºc chi tiáº¿t.

---

## 4. Cáº¥u TrÃºc ThÆ° Má»¥c

```
my_app/
â”œâ”€â”€ application.bam           # [Báº®T BUá»˜C] Manifest (JSON)
â”œâ”€â”€ app.c                     # [Báº®T BUá»˜C] Code chÃ­nh (chá»©a app_main)
â”œâ”€â”€ app.ld                    # [TÃ™Y CHá»ŒN] Linker script (náº¿u khÃ´ng cÃ³, dÃ¹ng máº·c Ä‘á»‹nh)
â”‚
â”œâ”€â”€ helpers.c                 # [TÃ™Y CHá»ŒN] File source phá»¥
â”œâ”€â”€ helpers.h                 #
â”‚
â”œâ”€â”€ libs/                     # [TÃ™Y CHá»ŒN] ThÆ° viá»‡n bÃªn ngoÃ i
â”‚   â”œâ”€â”€ tiny_json.c           #   BBT sáº½ tá»± quÃ©t vÃ  biÃªn dá»‹ch
â”‚   â””â”€â”€ tiny_json.h           #
â”‚
â”œâ”€â”€ assets/                   # [TÃ™Y CHá»ŒN] HÃ¬nh áº£nh/tÃ i nguyÃªn
â”‚   â”œâ”€â”€ logo.png              #   ÄÆ°á»£c biÃªn dá»‹ch thÃ nh máº£ng C
â”‚   â”œâ”€â”€ icon_wifi.png         #
â”‚   â””â”€â”€ splash.bmp            #
â”‚
â”œâ”€â”€ .bbt_build/               # [Tá»° Äá»˜NG] ThÆ° má»¥c build (táº¡m)
â”‚   â”œâ”€â”€ bruce_assets.h        #   Generated header
â”‚   â”œâ”€â”€ bruce_assets.c        #   Generated source
â”‚   â””â”€â”€ *.o                   #   Object files
â”‚
â””â”€â”€ dist/                     # [Tá»° Äá»˜NG] Káº¿t quáº£ build
    â””â”€â”€ my_app.bruce            #   File app cuá»‘i cÃ¹ng
```

**LÆ°u Ã½ quan trá»ng:**
- `bbt.py` sáº½ **tá»± Ä‘á»™ng quÃ©t toÃ n bá»™** file `.c`, `.cpp`, `.cc`, `.cxx` trong thÆ° má»¥c app (bao gá»“m cáº£ thÆ° má»¥c con), ngoáº¡i trá»« `.bbt_build/` vÃ  `dist/`.
- Báº¡n **khÃ´ng cáº§n** viáº¿t Makefile, CMakeLists.txt, hay báº¥t ká»³ file cáº¥u hÃ¬nh build nÃ o.

---

## 5. File Manifest (application.bam)

File `application.bam` lÃ  file JSON mÃ´ táº£ thÃ´ng tin á»©ng dá»¥ng. ÄÃ¢y lÃ  **file báº¯t buá»™c**.

### VÃ­ dá»¥ Ä‘áº§y Ä‘á»§

```json
{
    "appid": "wifi_scanner",
    "name": "WiFi Scanner Pro",
    "version": "2.1",
    "author": "Phat K.",
    "entry_point": "app_main",
    "assets_dir": "assets"
}
```

### Giáº£i thÃ­ch tá»«ng trÆ°á»ng

| TrÆ°á»ng | Kiá»ƒu | Báº¯t buá»™c | MÃ´ táº£ |
|--------|------|----------|-------|
| `appid` | string | âœ… | ID duy nháº¥t, dÃ¹ng lÃ m tÃªn file `.bruce`. Chá»‰ dÃ¹ng chá»¯ thÆ°á»ng, sá»‘, vÃ  gáº¡ch dÆ°á»›i. VÃ­ dá»¥: `wifi_scanner`, `ir_remote` |
| `name` | string | âœ… | TÃªn hiá»ƒn thá»‹ trong menu Apps trÃªn thiáº¿t bá»‹ (tá»‘i Ä‘a 31 kÃ½ tá»±). VÃ­ dá»¥: `"WiFi Scanner Pro"` |
| `version` | string | âœ… | PhiÃªn báº£n app (tá»‘i Ä‘a 7 kÃ½ tá»±). VÃ­ dá»¥: `"2.1"`, `"1.0.3"` |
| `author` | string | âœ… | TÃªn tÃ¡c giáº£ |
| `entry_point` | string | âœ… | TÃªn hÃ m C sáº½ Ä‘Æ°á»£c firmware gá»i khi app khá»Ÿi Ä‘á»™ng. **Pháº£i luÃ´n lÃ  `"app_main"`** |
| `assets_dir` | string | âŒ | TÃªn thÆ° má»¥c chá»©a assets. Máº·c Ä‘á»‹nh: `"assets"` |

---

## 6. Viáº¿t Code á»¨ng Dá»¥ng

### HÃ m entry point

Má»i app **báº¯t buá»™c** pháº£i cÃ³ hÃ m `app_main` vá»›i signature sau:

```c
#include "bruce_api.h"

void app_main(BruceAPI* api) {
    // Code app cá»§a báº¡n á»Ÿ Ä‘Ã¢y
}
```

Tham sá»‘ `api` lÃ  con trá» Ä‘áº¿n báº£ng Jump Table â€” chá»©a **táº¥t cáº£** cÃ¡c hÃ m mÃ  firmware cho phÃ©p báº¡n sá»­ dá»¥ng.

### Quy táº¯c viáº¿t code

#### âœ… NÃŠN lÃ m:

```c
void app_main(BruceAPI* api) {
    // 1. LuÃ´n kiá»ƒm tra api trÆ°á»›c khi dÃ¹ng
    if (!api) return;

    // 2. DÃ¹ng api->bruce_malloc thay cho malloc
    void* buffer = api->bruce_malloc(256);

    // 3. DÃ¹ng api->bruce_log thay cho printf/Serial.print
    api->bruce_log("Buffer allocated at %p\n", buffer);

    // 4. Giáº£i phÃ³ng bá»™ nhá»› khi khÃ´ng dÃ¹ng ná»¯a
    api->bruce_free(buffer);

    // 5. ThoÃ¡t sáº¡ch â€” chá»‰ cáº§n return
}
```

#### âŒ KHÃ”NG Ä‘Æ°á»£c lÃ m:

```c
void app_main(BruceAPI* api) {
    // âŒ KhÃ´ng gá»i malloc/free trá»±c tiáº¿p â€” firmware khÃ´ng theo dÃµi Ä‘Æ°á»£c
    void* p = malloc(100);

    // âŒ KhÃ´ng gá»i hÃ m firmware trá»±c tiáº¿p (vÃ­ dá»¥: tft.drawString)
    // VÃ¬ app khÃ´ng link vá»›i firmware, sáº½ crash ngay láº­p tá»©c
    tft.drawString("Hello", 0, 0);

    // âŒ KhÃ´ng dÃ¹ng Serial.println â€” dÃ¹ng api->bruce_log
    Serial.println("test");

    // âŒ KhÃ´ng táº¡o FreeRTOS task â€” cÃ³ thá»ƒ gÃ¢y race condition
    xTaskCreate(...);
}
```

### VÃ²ng láº·p chÃ­nh (Main Loop)

Háº§u háº¿t app sáº½ cÃ³ má»™t vÃ²ng láº·p chÃ­nh Ä‘á»ƒ xá»­ lÃ½ input:

```c
void app_main(BruceAPI* api) {
    if (!api) return;

    api->clear_screen(0x0000);
    api->draw_main_border_with_title("My App");

    while (1) {
        // ThoÃ¡t khi nháº¥n ESC
        if (api->check_escape_press()) break;

        // Xá»­ lÃ½ phÃ­m NEXT
        if (api->check_next_press()) {
            api->bruce_log("Next pressed!\n");
        }

        // TrÃ¡nh busy-loop â€” luÃ´n delay Ã­t nháº¥t 20-50ms
        api->bruce_delay(50);
    }
    // Khi return khá»i app_main, firmware tá»± Ä‘á»™ng dá»n dáº¹p
}
```

---

## 7. Bruce SDK API Reference

ÄÃ¢y lÃ  danh sÃ¡ch Ä‘áº§y Ä‘á»§ cÃ¡c hÃ m trong `BruceAPI` (API version 2):

### 7.1. Há»‡ Thá»‘ng & Bá»™ Nhá»›

| HÃ m | MÃ´ táº£ |
|-----|-------|
| `void* bruce_malloc(size_t size)` | Cáº¥p phÃ¡t bá»™ nhá»›. **Quota: 64KB má»—i app.** Tráº£ vá» `NULL` náº¿u vÆ°á»£t quota hoáº·c háº¿t RAM. |
| `void bruce_free(void* ptr)` | Giáº£i phÃ³ng bá»™ nhá»› Ä‘Ã£ cáº¥p phÃ¡t bá»Ÿi `bruce_malloc`. |
| `uint32_t bruce_millis()` | Tráº£ vá» sá»‘ millisecond ká»ƒ tá»« khi thiáº¿t bá»‹ báº­t. |
| `void bruce_delay(uint32_t ms)` | Táº¡m dá»«ng `ms` millisecond. |
| `void bruce_log(const char* format, ...)` | In log ra Serial (há»— trá»£ format giá»‘ng `printf`). Buffer 256 bytes, an toÃ n. |

**LÆ°u Ã½ vá» bá»™ nhá»›:**
- Má»—i app Ä‘Æ°á»£c phÃ©p cáº¥p phÃ¡t tá»‘i Ä‘a **64KB** thÃ´ng qua `bruce_malloc`.
- Má»—i app Ä‘Æ°á»£c theo dÃµi tá»‘i Ä‘a **64 láº§n cáº¥p phÃ¡t** (64 con trá»).
- Khi app thoÃ¡t, firmware sáº½ tá»± Ä‘á»™ng giáº£i phÃ³ng báº¥t ká»³ bá»™ nhá»› nÃ o báº¡n quÃªn `bruce_free` (Garbage Collector).

### 7.2. Input (PhÃ­m Báº¥m)

| HÃ m | MÃ´ táº£ |
|-----|-------|
| `bool check_next_press()` | Kiá»ƒm tra phÃ­m NEXT (xuá»‘ng/pháº£i) |
| `bool check_prev_press()` | Kiá»ƒm tra phÃ­m PREV (lÃªn/trÃ¡i) |
| `bool check_select_press()` | Kiá»ƒm tra phÃ­m SELECT (OK/Enter) |
| `bool check_escape_press()` | Kiá»ƒm tra phÃ­m ESC (quay láº¡i) |
| `bool check_any_key_press()` | Kiá»ƒm tra báº¥t ká»³ phÃ­m nÃ o |

Táº¥t cáº£ cÃ¡c hÃ m tráº£ vá» `true` náº¿u phÃ­m vá»«a Ä‘Æ°á»£c nháº¥n, `false` náº¿u khÃ´ng.

### 7.3. Hiá»ƒn Thá»‹ & Giao Diá»‡n

| HÃ m | MÃ´ táº£ |
|-----|-------|
| `void draw_string(const char* text, int x, int y, uint16_t color)` | Váº½ chuá»—i text táº¡i vá»‹ trÃ­ (x, y) |
| `void fill_rect(int x, int y, int w, int h, uint16_t color)` | Váº½ hÃ¬nh chá»¯ nháº­t Ä‘áº·c |
| `void draw_rect(int x, int y, int w, int h, uint16_t color)` | Váº½ viá»n hÃ¬nh chá»¯ nháº­t |
| `void clear_screen(uint16_t color)` | XÃ³a toÃ n bá»™ mÃ n hÃ¬nh |
| `void draw_main_border_with_title(const char* title)` | Váº½ khung viá»n vÃ  tiÃªu Ä‘á» chuáº©n firmware |
| `void draw_image(int x, int y, const uint16_t* data, int w, int h)` | **[Má»šI]** Váº½ áº£nh RGB565 lÃªn mÃ n hÃ¬nh |
| `void draw_pixel(int x, int y, uint16_t color)` | **[Má»šI]** Váº½ 1 pixel |
| `int get_tft_width()` | Chiá»u rá»™ng mÃ n hÃ¬nh (pixel) |
| `int get_tft_height()` | Chiá»u cao mÃ n hÃ¬nh (pixel) |

### 7.4. Tiá»‡n Ãch

| HÃ m | MÃ´ táº£ |
|-----|-------|
| `uint32_t bruce_random()` | Tráº£ vá» sá»‘ ngáº«u nhiÃªn 32-bit (hardware RNG) |
| `uint32_t bruce_get_time()` | Tráº£ vá» Unix timestamp hiá»‡n táº¡i (giÃ¢y) |

### 7.5. Há»‡ MÃ u RGB565

MÃ n hÃ¬nh ESP32 dÃ¹ng há»‡ mÃ u **RGB565** (16-bit). DÆ°á»›i Ä‘Ã¢y lÃ  cÃ¡c mÃ u phá»• biáº¿n:

```c
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_ORANGE  0xFD20
```

CÃ´ng thá»©c chuyá»ƒn Ä‘á»•i tá»« RGB888 sang RGB565:
```c
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
```

---

## 8. Sá»­ Dá»¥ng Assets (HÃ¬nh áº¢nh)

### CÃ¡ch hoáº¡t Ä‘á»™ng

1. Báº¡n Ä‘áº·t file áº£nh `.png` hoáº·c `.bmp` vÃ o thÆ° má»¥c `assets/`.
2. Khi cháº¡y `bbt.py build`, tool sáº½ tá»± Ä‘á»™ng:
   - Äá»c tá»«ng file áº£nh báº±ng thÆ° viá»‡n Pillow.
   - Convert sang há»‡ mÃ u **RGB565** (16-bit má»—i pixel).
   - Sinh ra file `bruce_assets.h` vÃ  `bruce_assets.c` chá»©a máº£ng byte.
3. Báº¡n `#include "bruce_assets.h"` trong code C vÃ  gá»i `api->draw_image()`.

### VÃ­ dá»¥

**BÆ°á»›c 1:** Äáº·t file `logo.png` (kÃ­ch thÆ°á»›c 32x32 pixel) vÃ o `assets/`

**BÆ°á»›c 2:** Trong code:

```c
#include "bruce_api.h"
#include "bruce_assets.h"  // Auto-generated bá»Ÿi bbt.py

void app_main(BruceAPI* api) {
    if (!api) return;

    api->clear_screen(COLOR_BLACK);

    // Váº½ logo á»Ÿ vá»‹ trÃ­ (10, 20)
    api->draw_image(10, 20, logo_data, logo_width, logo_height);

    // Chá» ngÆ°á»i dÃ¹ng nháº¥n ESC
    while (!api->check_escape_press()) {
        api->bruce_delay(50);
    }
}
```

**BÆ°á»›c 3:** Build: `python bbt.py build`

### Quy táº¯c Ä‘áº·t tÃªn asset

| TÃªn file áº£nh | Biáº¿n C Ä‘Æ°á»£c sinh ra |
|-------------|-------------------|
| `logo.png` | `logo_data[]`, `logo_width`, `logo_height` |
| `icon_wifi.png` | `icon_wifi_data[]`, `icon_wifi_width`, `icon_wifi_height` |
| `my-sprite.bmp` | `my_sprite_data[]`, `my_sprite_width`, `my_sprite_height` |

- Dáº¥u `-` vÃ  dáº¥u cÃ¡ch sáº½ Ä‘Æ°á»£c chuyá»ƒn thÃ nh `_`.
- Náº¿u tÃªn báº¯t Ä‘áº§u báº±ng sá»‘, sáº½ thÃªm `_` á»Ÿ Ä‘áº§u.

### Khuyáº¿n nghá»‹ vá» kÃ­ch thÆ°á»›c áº£nh

| Thiáº¿t bá»‹ | MÃ n hÃ¬nh | áº¢nh tá»‘i Ä‘a khuyáº¿n nghá»‹ |
|---------|---------|---------------------|
| M5Stack Cardputer | 240x135 | 240x135 (full screen) |
| M5Stack Core | 320x240 | 160x160 |
| ESP32 Generic | TÃ¹y thuá»™c | Giá»›i háº¡n bá»Ÿi RAM |

> âš ï¸ **Cáº£nh bÃ¡o**: Má»—i pixel chiáº¿m 2 bytes. áº¢nh 240x135 = 64,800 bytes â‰ˆ 63KB. ÄÃ¢y gáº§n báº±ng quota 64KB! NÃªn dÃ¹ng áº£nh nhá» (icon 32x32 = 2KB, hoÃ n háº£o).

---

## 9. Sá»­ Dá»¥ng ThÆ° Viá»‡n BÃªn NgoÃ i

### NguyÃªn táº¯c

Báº¡n cÃ³ thá»ƒ dÃ¹ng **báº¥t ká»³ thÆ° viá»‡n C/C++ nÃ o** miá»…n lÃ :
1. ThÆ° viá»‡n lÃ  **pure C/C++** (khÃ´ng phá»¥ thuá»™c vÃ o há»‡ Ä‘iá»u hÃ nh, POSIX, Linux...).
2. ThÆ° viá»‡n **khÃ´ng gá»i trá»±c tiáº¿p** cÃ¡c hÃ m firmware (tft, Serial, WiFi...).
3. Náº¿u thÆ° viá»‡n cáº§n cáº¥p phÃ¡t bá»™ nhá»›, báº¡n cáº§n wrapper láº¡i Ä‘á»ƒ dÃ¹ng `api->bruce_malloc`.

### CÃ¡ch thÃªm thÆ° viá»‡n

ÄÆ¡n giáº£n copy file `.c` vÃ  `.h` cá»§a thÆ° viá»‡n vÃ o thÆ° má»¥c app:

```
my_app/
â”œâ”€â”€ application.bam
â”œâ”€â”€ app.c
â””â”€â”€ libs/
    â”œâ”€â”€ cJSON.c         # ThÆ° viá»‡n JSON parser
    â”œâ”€â”€ cJSON.h
    â”œâ”€â”€ tiny_regex.c    # ThÆ° viá»‡n regex
    â””â”€â”€ tiny_regex.h
```

`bbt.py` sáº½ **tá»± Ä‘á»™ng phÃ¡t hiá»‡n** vÃ  biÃªn dá»‹ch táº¥t cáº£ file `.c/.cpp` trong thÆ° má»¥c `libs/`. Báº¡n khÃ´ng cáº§n cáº¥u hÃ¬nh gÃ¬ thÃªm.

### VÃ­ dá»¥: DÃ¹ng cJSON trong app

```c
#include "bruce_api.h"
#include "libs/cJSON.h"

void app_main(BruceAPI* api) {
    if (!api) return;

    // Táº¡o JSON object
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device", "Bruce");
    cJSON_AddNumberToObject(root, "uptime", api->bruce_millis());

    char* json_str = cJSON_Print(root);
    if (json_str) {
        api->bruce_log("JSON: %s\n", json_str);
        // LÆ°u Ã½: cJSON_Print dÃ¹ng malloc() ná»™i bá»™.
        // NÃªn dÃ¹ng thÆ° viá»‡n Ä‘Ã£ patch hoáº·c trÃ¡nh leak.
        free(json_str);
    }
    cJSON_Delete(root);
}
```

> **LÆ°u Ã½ nÃ¢ng cao**: Má»™t sá»‘ thÆ° viá»‡n (nhÆ° cJSON) gá»i `malloc()` ná»™i bá»™. Bá»™ nhá»› nÃ y sáº½ **khÃ´ng Ä‘Æ°á»£c firmware theo dÃµi**. Náº¿u báº¡n cáº§n kiá»ƒm soÃ¡t cháº·t, hÃ£y dÃ¹ng phiÃªn báº£n thÆ° viá»‡n há»— trá»£ custom allocator (cJSON há»— trá»£ `cJSON_InitHooks`).

---

## 10. Build vÃ  ÄÃ³ng GÃ³i

### Lá»‡nh build cÆ¡ báº£n

```bash
# Di chuyá»ƒn vÃ o thÆ° má»¥c app
cd my_app

# Build cho ESP32 (máº·c Ä‘á»‹nh)
python ../bbt.py build

# Build cho ESP32-S3
python ../bbt.py build --arch esp32s3

# Chá»‰ Ä‘á»‹nh Ä‘Æ°á»ng dáº«n SDK thá»§ cÃ´ng
python ../bbt.py build --sdk /path/to/bruce-firmware/include
```

### QuÃ¡ trÃ¬nh build (5 bÆ°á»›c tá»± Ä‘á»™ng)

```
[bbt] === Step 1: Reading manifest ===
[bbt]   App: WiFi Scanner Pro v2.1 by Phat K.
[bbt] === Step 2: Compiling assets ===
[bbt]   Asset: logo.png â†’ logo (RGB565)
[bbt]   Asset: icon_wifi.png â†’ icon_wifi (RGB565)
[bbt]   Assets compiled: 2 image(s)
[bbt] === Step 3: Discovering sources ===
[bbt]   Found 3 app source(s) + 1 asset source(s)
[bbt] === Step 4: Compiling & Linking ===
[bbt]   CC  app.c
[bbt]   CC  helpers.c
[bbt]   CC  tiny_json.c
[bbt]   CC  bruce_assets.c
[bbt]   LD  app.elf
[bbt]   ELF size: 4832 bytes
[bbt] === Step 5: Packing .bruce ===
[bbt] Package created: wifi_scanner.bruce (4874 bytes)
[bbt] ==================================================
[bbt] BUILD SUCCESS: dist/wifi_scanner.bruce
[bbt] Copy wifi_scanner.bruce to your ESP32 SD card / LittleFS.
```

### Dá»n dáº¹p

```bash
python ../bbt.py clean
```

XÃ³a thÆ° má»¥c `.bbt_build/` vÃ  `dist/`.

---

## 11. Deploy lÃªn Thiáº¿t Bá»‹

### CÃ¡ch 1: Qua tháº» SD

1. Káº¿t ná»‘i tháº» SD vá»›i mÃ¡y tÃ­nh.
2. Táº¡o thÆ° má»¥c `/apps/` trÃªn tháº» SD (náº¿u chÆ°a cÃ³).
3. Copy file `.bruce` vÃ o `/apps/`.
4. Cáº¯m tháº» SD vÃ o thiáº¿t bá»‹.
5. Khá»Ÿi Ä‘á»™ng firmware â†’ vÃ o menu **Apps** â†’ chá»n app.

### CÃ¡ch 2: Qua LittleFS (Serial upload)

Náº¿u thiáº¿t bá»‹ khÃ´ng cÃ³ khe SD, báº¡n cÃ³ thá»ƒ upload file `.bruce` vÃ o phÃ¢n vÃ¹ng LittleFS:

```bash
# DÃ¹ng PlatformIO
pio run --target uploadfs

# Hoáº·c dÃ¹ng esptool
python -m esptool write_flash 0x290000 littlefs.bin
```

### Cáº¥u trÃºc thÆ° má»¥c trÃªn thiáº¿t bá»‹

```
/apps/                  â† Firmware quÃ©t thÆ° má»¥c nÃ y
â”œâ”€â”€ hello_bruce.bruce
â”œâ”€â”€ wifi_scanner.bruce
â””â”€â”€ ir_remote.bruce
```

---

## 12. Debug vÃ  Xá»­ LÃ½ Lá»—i

### Äá»c log Serial

Káº¿t ná»‘i thiáº¿t bá»‹ qua USB vÃ  má»Ÿ Serial Monitor (115200 baud):

```
[BAP] Loading 'WiFi Scanner Pro' (heap: 128456 bytes free)
[ELF] Loaded 4832 bytes at 0x3FFD1000, entry 0x3FFD1044
[BAP] 'WiFi Scanner Pro' exited. Heap: 128200 bytes free (-256)
```

### Lá»—i thÆ°á»ng gáº·p

| Log Message | NguyÃªn nhÃ¢n | CÃ¡ch sá»­a |
|------------|------------|---------|
| `[BAP] Bad magic` | File khÃ´ng pháº£i `.bruce` hoáº·c bá»‹ há»ng | Build láº¡i báº±ng `bbt.py build` |
| `[BAP] Arch mismatch` | App build cho ESP32 nhÆ°ng cháº¡y trÃªn ESP32-S3 | Build láº¡i vá»›i `--arch esp32s3` |
| `[ELF] OOM: need X bytes IRAM` | KhÃ´ng Ä‘á»§ RAM Ä‘á»ƒ náº¡p app | Giáº£m kÃ­ch thÆ°á»›c app, táº¯t bá»›t module firmware |
| `[BAP] ERROR: App requested X bytes, exceeds quota` | App cáº¥p phÃ¡t hÆ¡n 64KB | Tá»‘i Æ°u bá»™ nhá»›, dÃ¹ng Ã­t `bruce_malloc` hÆ¡n |
| `[BAP GC] Leaked alloc at slot X` | App quÃªn gá»i `bruce_free` | ThÃªm `api->bruce_free(ptr)` trÆ°á»›c khi thoÃ¡t |
| `[ELF] Unsupported relocation type X` | Compiler sinh ra relocation khÃ´ng há»— trá»£ | Äáº£m báº£o dÃ¹ng `-fPIC -mlongcalls` khi build |

### Lá»—i khi build (bbt.py)

| Lá»—i | NguyÃªn nhÃ¢n | CÃ¡ch sá»­a |
|-----|------------|---------|
| `Manifest not found` | KhÃ´ng tÃ¬m tháº¥y `application.bam` | Kiá»ƒm tra báº¡n Ä‘ang á»Ÿ Ä‘Ãºng thÆ° má»¥c app |
| `Compilation failed for X.c` | Lá»—i cÃº phÃ¡p C | Äá»c thÃ´ng bÃ¡o lá»—i tá»« GCC, sá»­a code |
| `Linking failed` | Symbol khÃ´ng tÃ¬m tháº¥y | Äáº£m báº£o táº¥t cáº£ hÃ m Ä‘á»u gá»i qua `api->` |
| `Cannot find Bruce SDK` | KhÃ´ng tÃ¬m tháº¥y `bruce_api.h` | DÃ¹ng `--sdk /path/to/include` |
| `Pillow is required` | ChÆ°a cÃ i Pillow | `pip install Pillow` |

---

## 13. Quy Táº¯c vÃ  Giá»›i Háº¡n

### Giá»›i háº¡n ká»¹ thuáº­t

| ThÃ´ng sá»‘ | GiÃ¡ trá»‹ |
|---------|---------|
| RAM quota má»—i app | 64 KB |
| Sá»‘ láº§n cáº¥p phÃ¡t tá»‘i Ä‘a | 64 |
| KÃ­ch thÆ°á»›c tÃªn app (manifest) | 31 kÃ½ tá»± |
| KÃ­ch thÆ°á»›c version | 7 kÃ½ tá»± |
| Kiáº¿n trÃºc há»— trá»£ | ESP32, ESP32-S3 |
| Há»‡ mÃ u display | RGB565 (16-bit) |
| Há»‡ Ä‘iá»u hÃ nh firmware | FreeRTOS (single-task mode cho app) |

### Quy táº¯c an toÃ n

1. **KhÃ´ng táº¡o FreeRTOS task** â€” App cháº¡y Ä‘á»“ng bá»™ trong task cá»§a firmware.
2. **KhÃ´ng gá»i hÃ m hardware trá»±c tiáº¿p** â€” Chá»‰ dÃ¹ng hÃ m qua `api->`.
3. **KhÃ´ng dÃ¹ng `malloc`/`free` há»‡ thá»‘ng** â€” DÃ¹ng `api->bruce_malloc`/`api->bruce_free`.
4. **KhÃ´ng truy cáº­p WiFi/Bluetooth/GPIO trá»±c tiáº¿p** â€” CÃ¡c API nÃ y sáº½ Ä‘Æ°á»£c thÃªm trong phiÃªn báº£n sau.
5. **LuÃ´n kiá»ƒm tra `api != NULL`** á»Ÿ Ä‘áº§u `app_main`.

---

## 14. VÃ­ Dá»¥ Máº«u

### VÃ­ dá»¥ 1: Hello World

```c
#include "bruce_api.h"

void app_main(BruceAPI* api) {
    if (!api) return;

    api->clear_screen(0x0000);
    api->draw_main_border_with_title("Hello World");
    api->draw_string("Welcome to Bruce!", 20, 60, 0xFFFF);
    api->draw_string("Press ESC to exit", 20, 90, 0xF800);

    while (!api->check_escape_press()) {
        api->bruce_delay(50);
    }
}
```

### VÃ­ dá»¥ 2: Bá»™ Äáº¿m vá»›i áº¢nh

```c
#include "bruce_api.h"
#include "bruce_assets.h"

void app_main(BruceAPI* api) {
    if (!api) return;
    int w = api->get_tft_width();
    int count = 0;

    api->clear_screen(0x0000);
    api->draw_main_border_with_title("Counter App");

    // Váº½ logo (náº¿u cÃ³) á»Ÿ gÃ³c trÃªn pháº£i
    api->draw_image(w - logo_width - 5, 5, logo_data, logo_width, logo_height);

    while (1) {
        if (api->check_escape_press()) break;
        if (api->check_next_press()) count++;
        if (api->check_prev_press()) count--;

        // XÃ³a vÃ¹ng hiá»ƒn thá»‹ counter
        api->fill_rect(20, 60, 100, 30, 0x0000);

        // Hiá»ƒn thá»‹
        api->draw_string("Count:", 20, 60, 0x07E0);
        api->bruce_log("Counter = %d\n", count);

        api->bruce_delay(100);
    }
}
```

### VÃ­ dá»¥ 3: Pixel Art Drawing

```c
#include "bruce_api.h"

void app_main(BruceAPI* api) {
    if (!api) return;

    int w = api->get_tft_width();
    int h = api->get_tft_height();

    api->clear_screen(0x0000);

    // Váº½ gradient mÃ u cáº§u vá»“ng
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint16_t r = (x * 31) / w;
            uint16_t g = (y * 63) / h;
            uint16_t b = 31 - r;
            uint16_t color = (r << 11) | (g << 5) | b;
            api->draw_pixel(x, y, color);
        }
    }

    api->draw_string("Gradient Demo", 10, 10, 0xFFFF);

    while (!api->check_escape_press()) {
        api->bruce_delay(50);
    }
}
```

---

## 15. FAQ â€” CÃ¢u Há»i ThÆ°á»ng Gáº·p

### Q: TÃ´i cÃ³ thá»ƒ dÃ¹ng C++ khÃ´ng?
**A:** CÃ³! `bbt.py` há»— trá»£ cáº£ file `.cpp`, `.cc`, `.cxx`. Tuy nhiÃªn, hÃ m `app_main` pháº£i Ä‘Æ°á»£c khai bÃ¡o trong extern "C" block:
```cpp
extern "C" {
    void app_main(BruceAPI* api);
}
```

### Q: App cá»§a tÃ´i cÃ³ thá»ƒ truy cáº­p WiFi/Bluetooth khÃ´ng?
**A:** ChÆ°a â€” trong phiÃªn báº£n API v2 hiá»‡n táº¡i, chá»‰ cÃ³ display, input, vÃ  memory. WiFi/BLE/GPIO sáº½ Ä‘Æ°á»£c thÃªm trong cÃ¡c phiÃªn báº£n tÆ°Æ¡ng lai. HÃ£y theo dÃµi `BRUCE_API_VERSION`.

### Q: Táº¡i sao app tÃ´i bá»‹ crash khi gá»i `malloc()`?
**A:** VÃ¬ `malloc()` cáº¥p phÃ¡t bá»™ nhá»› ngoÃ i sá»± kiá»ƒm soÃ¡t cá»§a firmware. HÃ£y dÃ¹ng `api->bruce_malloc()`. Náº¿u dÃ¹ng thÆ° viá»‡n ngoÃ i cÃ³ gá»i `malloc`, firmware váº«n cháº¡y Ä‘Æ°á»£c, nhÆ°ng bá»™ nhá»› Ä‘Ã³ khÃ´ng Ä‘Æ°á»£c theo dÃµi (cÃ³ thá»ƒ leak).

### Q: KÃ­ch thÆ°á»›c tá»‘i Ä‘a cá»§a file .bruce lÃ  bao nhiÃªu?
**A:** KhÃ´ng cÃ³ giá»›i háº¡n cá»©ng cho file `.bruce`. Tuy nhiÃªn, pháº§n ELF pháº£i vá»«a trong IRAM (thÆ°á»ng 128-256KB trÃªn ESP32). Thá»±c táº¿ nÃªn giá»¯ dÆ°á»›i 100KB.

### Q: TÃ´i cÃ³ thá»ƒ debug báº±ng GDB khÃ´ng?
**A:** KhÃ´ng trá»±c tiáº¿p â€” vÃ¬ app Ä‘Æ°á»£c load Ä‘á»™ng vÃ o RAM. HÃ£y dÃ¹ng `api->bruce_log()` Ä‘á»ƒ in debug info ra Serial.

### Q: LÃ m sao Ä‘á»ƒ build cho cáº£ ESP32 vÃ  ESP32-S3?
**A:** Build 2 láº§n:
```bash
python ../bbt.py build --arch esp32
cp dist/my_app.bruce dist/my_app_esp32.bruce

python ../bbt.py build --arch esp32s3
cp dist/my_app.bruce dist/my_app_s3.bruce
```

### Q: CÃ³ cáº§n cÃ i PlatformIO khÃ´ng?
**A:** KhÃ´ng! `bbt.py` gá»i trá»±c tiáº¿p `xtensa-esp32-elf-gcc`. Báº¡n chá»‰ cáº§n cÃ i toolchain Xtensa (Ä‘i kÃ¨m ESP-IDF).

---

## Changelog

| Version | Thay Ä‘á»•i |
|---------|---------|
| API v1 | Initial release: display, input, memory, logging, utilities |
| API v2 | Added `draw_image`, `draw_pixel`. Bumped `BRUCE_API_VERSION` to 2 |

---

*TÃ i liá»‡u nÃ y Ä‘Æ°á»£c táº¡o cho Bruce Firmware â€” Dynamic App Loader System.*
*Cáº­p nháº­t láº§n cuá»‘i: 2026-07-13*

