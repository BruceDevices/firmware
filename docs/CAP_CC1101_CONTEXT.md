# Bruce + Cardputer-Adv — Contexto e Patches

Branches: `cap-cc1101` (principal), `adv3in1` (WIP — perfil ADV 3in1)
Remote: `jomar` → `https://github.com/JoaoSantosBH/capCC1101-m5stack.git`
Build: `.pio/build/m5stack-cardputer/firmware.factory.bin`

---

## 1. Problema original — Cap CC1101

Cap CC1101 (M5-U219, chip TI CC1101 + NFC ST25R3916) no Cardputer-Adv não reconhecia nada no Bruce. Duas causas:

1. **Pinagem errada.** O padrão do Bruce usava CS=13, GDO0=5 (shield de terceiros). O Cap CC1101 usa **CS=G5, GDO0=G15**, e o **G13 é a chave de banda RF_SW0**.
2. **Chaveamento de banda ausente.** O Cap tem chaves de RF (SP3T) para multiplexar 315/433/868/915 MHz via RF_SW0=G13 e RF_SW1=GDO2 do CC1101.

Bug adicional: #2864 — `_post_setup_gpio()` sobrescreve pinos salvos a cada boot.

---

## 2. Ambiente de build (macOS)

```bash
pio upgrade --dev           # PlatformIO Core >= 6.1.19
pio run -e m5stack-cardputer
pio run -e m5stack-cardputer -t upload
pio device monitor -b 115200
```

Modo download: chave de energia em OFF, segurar G0, ligar.

Conflito FastLED: travar versão no `platformio.ini`:
```ini
fastled/FastLED @3.10.3
```

---

## 3. Perfis de Hardware — Estado Actual

| Valor | Nome | Validado |
|-------|------|----------|
| 0 | Cap CC1101 | ✅ CC1101 + NFC ST25R3916 |
| 1 | Cap LoRa | ⚠️ beta — pinos RST/BUSY a validar |
| 2 | Stock/Shield | ✅ shield terceiros |
| 3 | Grove GPS v1.1 | ✅ MAX2659 em G1/G2, 9600 baud |
| 4 | ADV 3in1 | ✅ JosephCGS: CC1101+LoRa+NRF24 |

Mudar para/de ADV 3in1 → **reboot automático** (G8/G9 multiplexing).

---

## 4. Pinagem de referência

### Cap CC1101 (M5-U219)
| Função | Pin |
|--------|-----|
| CC1101 CS | G5 |
| CC1101 GDO0 | G15 |
| CC1101 RF_SW0 | G13 (band switch — não é CS) |
| NFC CS (ST25R3916) | G6 |
| NFC IRQ | G4 |
| GPS RX/TX | G1/G2 |
| SPI compartilhado | SCK=G40, MOSI=G14, MISO=G39 |

### ADV 3in1 (JosephCGS)
| Função | Pin |
|--------|-----|
| CC1101 CS | G15 |
| CC1101 GDO0 | G13 |
| LoRa CS | G5 |
| LoRa RST | G3 |
| LoRa DIO0 | G4 |
| NRF24 SS | G9 ← partilhado TCA8418 SCL |
| NRF24 CE | G8 ← partilhado TCA8418 SDA |
| GPS RX/TX | G1/G2 |

---

## 5. NFC ST25R3916

Driver completo já no Bruce (`src/modules/rfid/ST25R3916.cpp`, 3342 linhas). Activado com 6 linhas em `applyHWProfile(HW_CAP_CC1101)`. Validado no hardware.

---

## 6. ADV 3in1 — G8/G9 Multiplexing

G8/G9 partilhados entre TCA8418 (keyboard I2C), ES8311 (audio codec I2C), e NRF24 (CE/SS).

**Design**: G8/G9 ficam OUTPUT por defeito. Cada acesso Wire1 usa janela breve com reference counting (`i2c_window_depth`). O reference count evita que chamadas aninhadas (ex: `uiBeep→codec` dentro do `InputHandler`) terminem Wire1 prematuramente.

Historial de tentativas falhadas:
- Tentativas 1-3: `Wire1.end()` sem guard no ES8311 → crash ao abrir WiFi
- Tentativa 4: flag `adv_keyboard_released` bloqueava `InputHandler` → Esc não funcionava
- **Solução final**: reference counting, sem flag de bloqueio

---

## 7. Git

```bash
git push jomar cap-cc1101
git push jomar adv3in1
```

Para criar PR: `https://github.com/JoaoSantosBH/capCC1101-m5stack/pull/new/adv3in1`
