# Customizações Bruce — M5Stack Cardputer-Adv

Branches: `cap-cc1101` (principal), `adv3in1` (perfil ADV 3in1)
Hardware alvo: **M5Stack Cardputer-Adv** com teclado TCA8418, codec ES8311, amplificador NS4168.

---

## 1. Perfis de Hardware (HW Profiles)

### Problema
O Cardputer-Adv suporta diferentes módulos de RF encaixáveis ("Cap"). Cada um usa pinos GPIO diferentes. A selecção de pinos no `ini` não sobrevive a troca de perfil sem reboot, e havia um bug de persistência (#2864) no mecanismo original.

### Solução
Ficheiros:
- `boards/m5stack-cardputer/hw_profiles.h`
- Implementação em `boards/m5stack-cardputer/interface.cpp`

**Enum de perfis:**
```cpp
enum HWProfile : uint8_t {
    HW_CAP_CC1101 = 0,  // M5-U219: CC1101 + NFC ST25R3916
    HW_CAP_LORA   = 1,  // Cap LoRa SX1262 (beta)
    HW_STOCK      = 2,  // Cardputer-Adv sem módulo Cap RF
    HW_GROVE_GPS  = 3,  // GPS Unit v1.1 MAX2659 no Grove G1/G2
    HW_ADV_3IN1   = 4,  // JosephCGS: CC1101+LoRa+NRF24 (G8/G9 mux)
};
```

**Persistência:** NVS via `Preferences`, namespace `bruce_hw`, chave `profile`. Sobrevive a reboot e a flash de firmware (partição NVS separada).

**`applyHWProfile(uint8_t prof)`** — função central que define os pinos:

| Perfil | CS | GDO0/IO0 | GPS RX/TX | Extra |
|---|---|---|---|---|
| Cap CC1101 | G5 | G15 | G1/G2 | NFC CS=G6, IRQ=G4; G13=RF_SW0 |
| Cap LoRa | G5 | — | G15/G13 | SX1262; pinos RST/BUSY a validar |
| Stock | G13 | G5 | G15/G13 | Shield de terceiros |
| Grove GPS v1.1 | — | — | G1/G2 | MAX2659, 9600 baud |
| ADV 3in1 | G15 | G13 | G1/G2 | LoRa CS=G5/RST=G3/DIO0=G4; NRF24 SS=G9/CE=G8 |

SPI compartilhado: SCK=G40, MISO=G39, MOSI=G14.

**Nota:** Mudar para/de `HW_ADV_3IN1` exige reboot automático (G8/G9 multiplexing configurado ao boot).

### Menu
Entrada de topo no menu principal: **`Profile`**, com ícone de chip.
Caminho: `/Profile` → mostra o perfil activo (ex.: `Profile: Cap CC1101`).
Compilado apenas quando `TCA8418_I2C_ADDR` está definido (específico do Cardputer-Adv).

---

## 2. RF Band Switching — Cap CC1101

### Problema
O módulo M5-U219 (Cap CC1101) tem um switch de RF analógico de 3 bandas controlado por dois sinais digitais:
- **RF_SW0** → pino GPIO G13
- **RF_SW1** → controlado via registo `IOCFG2` do CC1101 (pino GDO2)

Sem este switching, o módulo só funciona numa banda.

### Solução
Adicionado em `src/modules/rf/rf_utils.cpp`:

```cpp
static bool isCapCC1101() {
    return bruceConfigPins.CC1101_bus.cs == 5 &&
           bruceConfigPins.CC1101_bus.io0 == 15;
}

static void capCC1101SetBand(float frequency) {
    const uint8_t RF_SW0 = 13;
    bool sw0, sw1;
    if      (frequency <= 350) { sw0 = false; sw1 = false; } // 315 MHz
    else if (frequency <= 468) { sw0 = false; sw1 = true;  } // 433 MHz
    else                       { sw0 = true;  sw1 = true;  } // 868/915 MHz

    pinMode(RF_SW0, OUTPUT);
    digitalWrite(RF_SW0, sw0 ? HIGH : LOW);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_IOCFG2, sw1 ? 0x6F : 0x2F);
}
```

Chamado automaticamente dentro de `setMHZ()`.

---

## 3. NFC ST25R3916 — Cap CC1101

### Estado
O driver ST25R3916 já existe no Bruce (3342 linhas, `src/modules/rfid/ST25R3916.cpp`) com suporte a NFC-A/B/V/F, MIFARE Classic, emulação NDEF, ISO-DEP.

### Activação no perfil Cap CC1101
Em `applyHWProfile(HW_CAP_CC1101)` em `interface.cpp`:
```cpp
bruceConfigPins.ST25R_bus.sck  = (gpio_num_t)40;
bruceConfigPins.ST25R_bus.miso = (gpio_num_t)39;
bruceConfigPins.ST25R_bus.mosi = (gpio_num_t)14;
bruceConfigPins.ST25R_bus.cs   = (gpio_num_t)6;  // NFC_CS
bruceConfigPins.ST25R_bus.io0  = (gpio_num_t)4;  // NFC_IRQ
bruceConfigPins.rfidModule     = ST25R3916_SPI_MODULE;
```
O menu RFID reconhece automaticamente e mostra `(ST25R-SPI)`.

---

## 4. Perfil ADV 3in1 — G8/G9 Multiplexing

### Problema
No board JosephCGS ADV, G8/G9 são partilhados entre:
- TCA8418 I2C (SDA=G8, SCL=G9) — teclado
- ES8311 áudio codec (também usa Wire1 em G8/G9)
- NRF24 (CE=G8, SS=G9)

### Solução — Design Correto
G8/G9 ficam em **OUTPUT por defeito** após boot. Cada acesso I2C usa uma janela breve com reference counting:

```cpp
// Apenas abre Wire1 se não houver janela aberta (evita nesting)
static int i2c_window_depth = 0;
static void setI2cPinsToI2c() {
    if (i2c_window_depth == 0) { Wire1.begin(8, 9); delay(5); }
    i2c_window_depth++;
}
static void setI2cPinsToOutput() {
    if (i2c_window_depth > 0) i2c_window_depth--;
    if (i2c_window_depth == 0) {
        Wire1.end();
        pinMode(8, OUTPUT); digitalWrite(8, HIGH);
        pinMode(9, OUTPUT); digitalWrite(9, HIGH);
    }
}
```

Aplica-se a **TCA8418 E ES8311** (ambos usam Wire1). O reference count evita que chamadas aninhadas (ex: `uiBeep→codec` dentro do `InputHandler`) terminem Wire1 prematuramente.

### API pública (declarada em `include/interface.h`)
```cpp
#ifdef TCA8418_I2C_ADDR
void adv_flush_keyboard_events(); // drena FIFO TCA8418 + limpa EscPress
void adv_release_keyboard();      // no-op (mantido para flush no restore)
void adv_keyboard_restore();      // flush FIFO após NRF24
#endif
```

### NRF24Menu
```cpp
auto nrfRun = [](void (*fn)()) {
    adv_release_keyboard();
    fn();
    adv_keyboard_restore(); // flush FIFO ao sair
};
```

### Porque falharam as tentativas anteriores
1. Tentativas 1 e 2: `Wire1.end()` sem guard no ES8311 → crash ao abrir WiFi (audio init)
2. Tentativa 3: ES8311 nunca foi guardado → mesmo crash
3. Tentativa 4: flag `adv_keyboard_released` bloqueava `InputHandler` → Esc não funcionava no spectrum scan
4. **Solução final**: reference counting + sem flag de bloqueio (single-threaded, sem concorrência real)

---

## 5. Leitura de Password WiFi do SD Card

### Ficheiro: `/wifi.conf` na raiz do SD

**Formato:**
```
ssid=MinhaRede
password=minhasenha

ssid=OutraRede
password=outrasenha
```

### Implementação
`src/core/wifi/wifi_common.cpp` — `wifiPasswordFromSD()`:
Integrada em `_wifiConnect()`: tenta o SD antes de pedir password pelo teclado.

---

## 6. Defaults do SD Card

### Ficheiro: `/defaults.conf` na raiz do SD

**Formato:**
```
soundVolume=50
bright=60
dimmerSet=30
priColor=FD20
bgColor=0000
ledColor=FF6600
```

Aplicado uma vez no boot, após carregar o config principal. Persiste no LittleFS.

---

## 7. Defaults Compilados

| Propriedade | Original | Custom | Ficheiro |
|---|---|---|---|
| `soundVolume` | 100 | **50** | `src/core/config.h` |
| `bright` | 100 | **50** | `src/core/config.h` |
| `ledColor` | 0x960064 (roxo) | **0xFF6600** (laranja) | `src/core/config.h` |
| `priColor` (UI) | 0xA80F | **0xFD20** (laranja RGB565) | `src/core/theme.h` |
| LoRa chat name | BruceTest | **JomarOLider** | `src/modules/lora/LoRaRF.cpp` |

---

## 8. Som de Tecla (UI Click)

Task FreeRTOS dedicada (`uiBeepTask`) em `src/modules/others/audio.cpp`:
- `AudioOutputI2S` criado uma vez e nunca destruído entre beeps
- Primeiro beep após boot: 420ms de warmup
- Waveform: `v(t) = [sin(2π·f·t) + 0.25·sin(2π·f·2.756·t)] · e^(-9t)` (sino real)
- Dispara no handler de keypress do TCA8418

---

## 9. Fixes

### GPS sai imediatamente (phantom Esc)
TCA8418 FIFO tem eventos residuais → decoded como Esc.
Fix: `vTaskDelay(300ms) + EscPress = false` após `GPSserial.begin()` em `gps_tracker.cpp` e `wardriving.cpp`.

### SD Card falha após M5Launcher
Fix em `src/core/sd_functions.cpp`: `SD.end() + sdcardSPI.end()` antes de montar + reset GPIO matrix.

### NRF24 conflito SPI com CC1101
Fix em `nrf_common.cpp`: deasserta CC1101 CS HIGH no início de `nrf_start()`.

### Codec ES8311 re-inicialização desnecessária
Cache com `static bool codecInitialized` em `_setup_codec_speaker()`.

### Reset ao abrir ficheiros com SD de 8GB
TWDT disparava durante `readFs()`. Fix: yield temporal a cada 100ms em vez de por iterações.

---

## Mapa de Ficheiros Modificados

| Ficheiro | Branch | O que muda |
|---|---|---|
| `boards/m5stack-cardputer/hw_profiles.h` | ambas | Enum HWProfile (5 perfis) |
| `boards/m5stack-cardputer/interface.cpp` | ambas | `applyHWProfile()`, G8/G9 mux, codec cache, uiBeep |
| `boards/m5stack-cardputer/m5stack-cardputer.ini` | cap-cc1101 | Build flags, pin defines |
| `include/interface.h` | adv3in1 | adv_flush/release/restore |
| `src/core/menu_items/HWProfileMenu.cpp` | ambas | Menu perfis (5 entradas + reboot ADV) |
| `src/core/menu_items/NRF24.cpp` | adv3in1 | nrfRun() wrapper |
| `src/modules/NRF24/nrf_common.cpp` | ambas | Deasserta CC1101 CS antes de init |
| `src/modules/rf/rf_utils.cpp` | cap-cc1101 | `isCapCC1101()` + `capCC1101SetBand()` |
| `src/core/wifi/wifi_common.cpp` | cap-cc1101 | `wifiPasswordFromSD()` |
| `src/core/config.h` | cap-cc1101 | Defaults: volume=50, bright=50, ledColor laranja |
| `src/core/theme.h` | cap-cc1101 | `DEFAULT_PRICOLOR` laranja (0xFD20) |
| `src/modules/others/audio.cpp` | cap-cc1101 | `uiBeepTask`, bell waveform |
| `src/core/sd_functions.cpp` | cap-cc1101 | Yield temporal, strcasecmp no sort, fix M5Launcher |
| `src/modules/gps/gps_tracker.cpp` | cap-cc1101 | Fix phantom Esc |
| `src/modules/gps/wardriving.cpp` | cap-cc1101 | Fix phantom Esc |
| `src/modules/lora/LoRaRF.cpp` | cap-cc1101 | Username chat = JomarOLider |
