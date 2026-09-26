# Customizações Bruce — M5Stack Cardputer-Adv

Ramo: `cap-cc1101`
Hardware alvo: **M5Stack Cardputer-Adv** com teclado TCA8418, codec ES8311, amplificador NS4168.

---

## 1. Perfis de Hardware (HW Profiles)

### Problema
O Cardputer-Adv suporta diferentes módulos de RF encaixáveis ("Cap"). Cada um usa pinos GPIO diferentes. A selecção de pinos no `ini` não sobrevive a troca de perfil sem reboot, e havia um bug de persistência (#2864) no mecanismo original.

### Solução
Ficheiros novos:
- `boards/m5stack-cardputer/hw_profiles.h`
- Implementação em `boards/m5stack-cardputer/interface.cpp`

**Enum de perfis:**
```cpp
enum HWProfile : uint8_t {
    HW_CAP_CC1101 = 0,  // M5-U219: CC1101 + NFC (NFC não suportado pelo Bruce)
    HW_CAP_LORA   = 1,  // Cap LoRa SX1262 (beta)
    HW_STOCK      = 2,  // Cardputer-Adv sem módulo Cap RF
};
```

**Persistência:** NVS via `Preferences`, namespace `bruce_hw`, chave `profile`. Sobrevive a reboot e a flash de firmware (partição NVS separada).

**`applyHWProfile(uint8_t prof)`** — função central que define os pinos:

| Perfil | CS | GDO0/IO0 | GPS RX/TX | Nota |
|---|---|---|---|---|
| Cap CC1101 | G5 | G15 | G1/G2 | G13 = RF_SW0 (band switch) |
| Cap LoRa | G5 | — | G15/G13 | SX1262; pinos RST/BUSY a validar |
| Stock | G13 | G5 | G15/G13 | Shield de terceiros |

SPI compartilhado: SCK=G40, MISO=G39, MOSI=G14.

**Chamada no boot:**
```cpp
// _post_setup_gpio() em interface.cpp
applyHWProfile(loadHWProfile());
```
Aplica imediatamente sem reboot. Contorna o bug #2864.

### Menu
Entrada de topo no menu principal: **`Profile`**, com ícone de chip (chip quadrado com pinos nos lados).

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
    // GDO2 fixo LOW (0x2F) ou fixo HIGH (0x6F, bit de inversão)
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_IOCFG2, sw1 ? 0x6F : 0x2F);
}
```

Chamado automaticamente dentro de `setMHZ()`:
```cpp
ELECHOUSE_cc1101.setMHZ(frequency);
if (isCapCC1101()) capCC1101SetBand(frequency);
```

O utilizador não precisa de fazer nada — a banda correcta é seleccionada automaticamente ao mudar a frequência no menu RF.

---

## 3. Leitura de Password WiFi do SD Card

### Ficheiro: `/wifi.conf` na raiz do SD

**Formato:**
```
ssid=MinhaRede
password=minhasenha

ssid=OutraRede
password=outrasenha
```

Regras:
- Um bloco por rede, separados por linha em branco
- Comentários com `#` não suportados (linha é ignorada se não contiver `=`)
- A comparação do SSID é **case-insensitive**

### Implementação
`src/core/wifi/wifi_common.cpp` — função `wifiPasswordFromSD()`:

```cpp
static String wifiPasswordFromSD(const String &ssid) {
    if (!sdcardMounted) return "";
    File f = SD.open("/wifi.conf", FILE_READ);
    if (!f) return "";
    // ... parsing key=value ...
    // compara SSID com equalsIgnoreCase()
}
```

Integrada em `_wifiConnect()`: tenta o SD antes de pedir password pelo teclado. Se encontrar, conecta directo; se não encontrar, abre o teclado normalmente.

---

## 4. Defaults do SD Card

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

Aplicado uma vez no boot, em `src/main.cpp`, após carregar o ficheiro de config principal. Só sobrescreve valores que estejam explicitamente no ficheiro; valores ausentes ficam com o default compilado.

Após aplicar, chama `saveFile()` para persistir no LittleFS — o ficheiro do SD não é mais necessário em boots seguintes.

---

## 5. Defaults Compilados

Valores padrão alterados em relação ao Bruce original:

| Propriedade | Original | Custom | Ficheiro |
|---|---|---|---|
| `soundVolume` | 100 | **50** | `src/core/config.h` |
| `bright` | 100 | **50** | `src/core/config.h` |
| `ledColor` | 0x960064 (roxo) | **0xFF6600** (laranja) | `src/core/config.h` |
| `priColor` (UI) | 0xA80F | **0xFD20** (laranja RGB565) | `src/core/theme.h` |

---

## 6. Som de Tecla (UI Click)

### Problema
O ES8311 + NS4168 precisam de ~500ms de warmup após inicializar o I2S. Beeps curtos terminavam antes do hardware estar pronto, resultando em silêncio. A abordagem com `AudioGeneratorWAV` recriava o canal I2S em cada beep, causando warmup a cada toque.

### Solução
Task FreeRTOS dedicada (`uiBeepTask`) em `src/modules/others/audio.cpp`:

1. `AudioOutputI2S` criado uma vez e **nunca destruído** entre beeps (mantém I2S e amp aquecidos).
2. Primeiro beep após boot: 420ms de silêncio → waveform do sino.
3. Beeps seguintes: instantâneos (pipeline já quente).
4. Após cada sino: flush de 5760 frames de silêncio para zerar o ring buffer DMA (evita som contínuo).
5. Samples gerados directamente via `ConsumeSample()`, sem passar por `AudioGeneratorWAV`.

**Waveform do sino:**
```
v(t) = [sin(2π·f·t) + 0.25·sin(2π·f·2.756·t)] · e^(-9t)
```
Fundamental + 2.º parcial inarmónico (×2.756) com decaimento exponencial — característica de sinos reais.

**Onde dispara:** handler de keypress do TCA8418 em `interface.cpp`, uma vez por tecla física premida. Não dispara na navegação up/down do menu.

**API pública** (declarada em `src/core/utils.h`):
```cpp
void uiBeep(unsigned int freq = 1000, unsigned long ms = 30);
```
Non-blocking: posta para a queue e retorna imediatamente.

---

## 7. Fix — Reset ao Abrir Files com SD de 8GB

### Causa
O ESP32 TWDT (task watchdog, ~5s) disparava durante `readFs()` ao listar um cartão SD FAT32 grande. Cada chamada `getNextFileName()` pode demorar 10-100ms em FAT32. Com a guarda por contagem de iterações (50 iter), era possível acumular 5000ms = exactamente o limite.

### Fix
`src/core/sd_functions.cpp` — guarda **temporal** em vez de por iterações:

```cpp
unsigned long lastYield = millis();
while (true) {
    if (millis() - lastYield >= 100) {
        vTaskDelay(pdMS_TO_TICKS(1));
        lastYield = millis();
    }
    // getNextFileName() ...
}
```

Garante yield ao idle task a cada 100ms no máximo, independente da velocidade do cartão.

Também: `sortList()` substituiu alocações temporárias de `String` por `strcasecmp()`, eliminando pressão de heap O(N log N) durante a ordenação.

---

## 8. Codec ES8311 — Fix de Re-inicialização

### Problema
`_setup_codec_speaker(true)` era chamado em cada tom/beep, re-inicializando o codec via I2C. Além de desnecessário (o codec fica alimentado), causava cliques audíveis.

### Fix
Cache com variável `static`:
```cpp
void _setup_codec_speaker(bool enable) {
    static bool codecInitialized = false;
    if (enable && codecInitialized) return;  // skip se já inicializado
    if (enable) codecInitialized = true;
    // ...
}
```
O path `enable=false` envia `disabled_bulk_data = {0}` que é apenas o byte terminador — no-op no hardware. O codec fica sempre ligado após a primeira inicialização.

---

## Mapa de Ficheiros Modificados

| Ficheiro | Tipo | O que muda |
|---|---|---|
| `boards/m5stack-cardputer/hw_profiles.h` | **NOVO** | Enum + declarações de perfis |
| `boards/m5stack-cardputer/interface.cpp` | modificado | `applyHWProfile()`, codec cache, `uiBeep()` no keypress |
| `src/core/menu_items/HWProfileMenu.h` | **NOVO** | Classe menu item Profile |
| `src/core/menu_items/HWProfileMenu.cpp` | **NOVO** | `optionsMenu()` + `drawIcon()` (chip) |
| `src/core/main_menu.h` | modificado | Inclui e instancia `HWProfileMenu` |
| `src/core/main_menu.cpp` | modificado | Adiciona `&hwProfileMenu` à lista |
| `src/core/menu_items/ConfigMenu.h` | modificado | Remove `hwProfileMenu()` |
| `src/core/menu_items/ConfigMenu.cpp` | modificado | Remove entrada do Advanced e implementação |
| `src/modules/rf/rf_utils.cpp` | modificado | `isCapCC1101()` + `capCC1101SetBand()` |
| `src/core/wifi/wifi_common.cpp` | modificado | `wifiPasswordFromSD()` + integração |
| `src/core/config.h` | modificado | Defaults: volume=50, bright=50, ledColor laranja |
| `src/core/config.cpp` | modificado | `loadDefaultsFromSD()` |
| `src/core/theme.h` | modificado | `DEFAULT_PRICOLOR` laranja (0xFD20) |
| `src/core/utils.h` | modificado | Declaração `uiBeep()` |
| `src/modules/others/audio.cpp` | modificado | `uiBeepTask`, `uiBeep()`, bell waveform |
| `src/core/display.cpp` | modificado | Remove `uiBeep` do menu loop |
| `src/core/sd_functions.cpp` | modificado | Yield temporal no TWDT, `strcasecmp` no sort |
| `src/main.cpp` | modificado | Chama `loadDefaultsFromSD()` no boot |
