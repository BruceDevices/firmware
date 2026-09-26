# Bruce + Cardputer-Adv — Suporte ao Cap CC1101 (contexto e patches)

Documento de contexto para retomar o trabalho pelo terminal, dentro do repositório
do Bruce (`BruceDevices/firmware`), branch de trabalho `cap-cc1101`.

---

## 1. Problema original

Cap CC1101 (M5-U219, chip TI CC1101 + NFC ST25R3916) no Cardputer-Adv não
reconhecia nada no Bruce. Duas causas confirmadas:

1. **Pinagem errada.** O padrão do Bruce para o Cardputer-Adv usava a pinagem de
   shield de terceiros (CS=13, GDO0=5). O Cap CC1101 usa **CS=G5, GDO0=G15**, e o
   **G13 é a chave de banda RF_SW0** (não é CS). Resultado: o chip nunca era
   selecionado → "CC1101 not found".
2. **Chaveamento de banda ausente.** O Cap tem chaves de RF (SP3T) para
   multiplexar 315/433/868/915 MHz. Controladas por:
   - `RF_SW0` = **G13** do Cardputer-Adv
   - `RF_SW1` = **GDO2** do CC1101 (via registrador IOCFG2, por SPI)
   - Tabela: 315 → SW0=0,SW1=0 | 433 → SW0=0,SW1=1 | 868/915 → SW0=1,SW1=1
   O Bruce não controla isso, então mesmo detectando o chip a antena ficava no
   caminho errado.

Bug adicional relevante (issue #2864): no Cardputer-Adv o `_post_setup_gpio()`
sobrescreve os pinos salvos a cada boot, então config feita pela tela/arquivo
voltava ao padrão após reboot/deep sleep.

### Pinagem de referência do Cap CC1101 (doc M5Stack)
- CC1101 SPI: CS=G5, MOSI=G14, MISO=G39, SCK=G40, GDO0=G15 (interrupção)
- RF_SW0 = G13 | RF_SW1 = GDO2 do CC1101
- NFC (ST25R3916): mesmo SPI, IRQ=G4 — **não suportado pelo Bruce** (ver §6)

---

## 2. Ambiente de build (macOS)

Dois obstáculos resolvidos antes de compilar:

1. **PlatformIO Core desatualizado.** A plataforma pioarduino exige Core >= 6.1.19.
   Atualizado via:
   ```bash
   pio upgrade --dev      # ou ~/.platformio/penv/bin/pio upgrade --dev
   pio --version          # ficou 6.2.0
   ```
2. **Conflito da macro `FP` com a FastLED.** O `platformio.ini` trazia
   `fastled/FastLED @^3.10.3`, que baixava a 3.10.5 (usa `using FP = ...`,
   colide com a macro `-D FP` do Bruce). **Correção: travar a versão.**
   ```ini
   ; platformio.ini, ~linha 205
   fastled/FastLED @3.10.3
   ```
   Depois:
   ```bash
   rm -rf .pio/libdeps/m5stack-cardputer
   pio run -e m5stack-cardputer
   ```

Comandos de build usados:
```bash
pio run -e m5stack-cardputer            # compilar
pio run -e m5stack-cardputer -t upload  # gravar (device em modo download)
pio device monitor -b 115200            # monitor serial
```
Modo download do Cardputer-Adv: chave de energia em OFF, segurar G0, ligar.

---

## 3. Patches aplicados (estado ATUAL, funcionando)

### Patch 1 — pinos padrão do Cap CC1101
Arquivo: `boards/m5stack-cardputer/interface.cpp`, dentro de `_post_setup_gpio()`.

```cpp
// GPS fora do G13/G15 (usados pelo Cap CC1101)
bruceConfigPins.gps_bus.rx = (gpio_num_t)1;
bruceConfigPins.gps_bus.tx = (gpio_num_t)2;
bruceConfigPins.gpsBaudrate = 115200;

// Cap CC1101: CS=G5, GDO0=G15
bruceConfigPins.CC1101_bus.sck  = (gpio_num_t)40;
bruceConfigPins.CC1101_bus.miso = (gpio_num_t)39;
bruceConfigPins.CC1101_bus.mosi = (gpio_num_t)14;
bruceConfigPins.CC1101_bus.cs   = (gpio_num_t)5;   // era 13
bruceConfigPins.CC1101_bus.io0  = (gpio_num_t)15;  // era 5
```
Como esse padrão é aplicado no boot, ele contorna o bug de persistência (#2864):
o valor certo é reimposto todo boot.

### Patch 2 — chaveamento de banda
Arquivo: `src/modules/rf/rf_utils.cpp`.

Definir **acima** de `void setMHZ(float frequency)`:
```cpp
// Cap CC1101 (Cardputer-Adv): RF_SW0 = G13, RF_SW1 = GDO2 do CC1101
// 315: SW0=0 SW1=0 | 433: SW0=0 SW1=1 | 868/915: SW0=1 SW1=1
static bool isCapCC1101() {
    return bruceConfigPins.CC1101_bus.cs == 5 && bruceConfigPins.CC1101_bus.io0 == 15;
}

static void capCC1101SetBand(float frequency) {
    const uint8_t RF_SW0 = 13;
    bool sw0, sw1;
    if (frequency <= 350) {        sw0 = false; sw1 = false; }  // 315
    else if (frequency <= 468) {   sw0 = false; sw1 = true;  }  // 433
    else {                         sw0 = true;  sw1 = true;  }  // 868/915
    pinMode(RF_SW0, OUTPUT);
    digitalWrite(RF_SW0, sw0 ? HIGH : LOW);
    // IOCFG2 = 0x2F: GDO2 fixo LOW ; 0x6F: fixo HIGH (bit de inversão)
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_IOCFG2, sw1 ? 0x6F : 0x2F);
}
```

Chamar dentro de `setMHZ`, logo após `ELECHOUSE_cc1101.setMHZ(frequency);`
(era ~linha 425, antes do `SetTx()/SetRx()`):
```cpp
ELECHOUSE_cc1101.setMHZ(frequency);
if (isCapCC1101()) capCC1101SetBand(frequency);
```
Verificado que nada reescreve o IOCFG2 depois disso no fluxo; o Bruce usa só o
GDO0 para TX/RX, então o GDO2 fica livre para a chave.

### Status
- CC1101 detectado ("cc1101 Connection OK"), pinos persistem após reboot e
  chaveamento de banda funcionando. **Validado no aparelho.**

---

## 4. Nota sobre configs por SD (bruce.conf / brucePins.conf)

Usar os `.conf` no SD **não substitui os patches**:
- O `brucePins.conf` que eu tinha vinha com CC1101 `cs:13, io0:5` (pinagem antiga)
  e GPS em `rx:15, tx:13` (conflito). Para funcionar via arquivo, teria de ser
  CC1101 `cs:5, io0:15` e GPS `rx:1, tx:2`.
- Mesmo corrigido, o arquivo **não faz o chaveamento de banda** (isso é código,
  Patch 2) e pode ser sobrescrito pelo `_post_setup_gpio` (bug #2864).

---

## 5. PRÓXIMO PASSO — perfis de hardware selecionáveis (CC1101 x LoRa x Stock)

Objetivo: trocar de Cap sem recompilar, escolhendo o perfil no menu. Guarda a
escolha em NVS (Preferences), e o `_post_setup_gpio` vira a única fonte da verdade
sobre os pinos (mata o bug #2864 de vez).

**Cuidado pendente:** os pinos do Cap CC1101 e do shield stock estão validados.
Os do **Cap LoRa (SX1262)** vêm da doc M5Stack (NSS=5, RST=3, BUSY=6, IRQ=4, SPI
14/39/40) mas **falta confirmar como o Bruce nomeia RST/BUSY/IRQ** e se ele já
habilita o expansor **PI4IOE5V6408** do Cap LoRa (P0 em HIGH liga o switch da
antena — exigência da doc). O perfil LoRa fica como esqueleto até validar isso.

O patch atual **não serve** para o Cap LoRa: além do chip diferente (SX1262, sem
IOCFG2), o GPS que fixamos em 1/2 precisa voltar para 15/13 no perfil LoRa.

### Esqueleto já desenhado
- `boards/m5stack-cardputer/hw_profiles.h` — enum `HWProfile { HW_CAP_CC1101,
  HW_CAP_LORA, HW_STOCK }` + `hwProfileName()`.
- Em `interface.cpp`: `loadHWProfile()` / `saveHWProfile()` via `Preferences`
  (namespace `bruce_hw`, chave `profile`, default `HW_CAP_CC1101`).
- `applyHWProfile(prof)` seta os pinos por perfil; chamada em `_post_setup_gpio`
  no lugar do bloco fixo.
- `isCapCC1101()` continua valendo (CS=5 && io0=15 só ocorre no perfil CC1101).

### Greps que faltam rodar para montar o item de menu
```bash
grep -rn -E "addOption|options.push_back|loopOptions|MenuOptions" src/core/menu_items/ | head -20
grep -rln -i "advanced" src/core/menu_items/
```
Com a saída disso, adicionar em **Config > Advanced** uma entrada
"Hardware Profile" que lista os 3 perfis, chama `saveHWProfile()` +
`applyHWProfile()` na hora (aplicar sem reboot) e mostra o nome atual.

---

## 6. NFC do Cap — não funciona no Bruce

O chip NFC do Cap é o **ST25R3916**. O Bruce só fala com **PN532** (e
PN532Killer/Chameleon). Não há driver ST25R3916 no Bruce, então o NFC do Cap é
inacessível por qualquer configuração de pino. Caminhos:
1. Usar o NFC do Cap **fora do Bruce**, com o exemplo Arduino da M5Stack (RadioLib
   + lib própria) — firmware separado.
2. **Portar** driver ST25R3916 (stack RFAL da ST) para o Bruce — projeto grande.
3. Para ler/emular logo dentro do Bruce: **PN532 separado** (NFC-A/NTAG/Ultralight
   ok; MIFARE Classic só com PN532Killer/Chameleon). Atenção a pinos: PN532 por
   I2C usaria o Grove (G1/G2), hoje ocupados pelo GPS no perfil CC1101.

---

## 7. Git

```bash
git add -A
git commit -m "Cardputer-Adv: Cap CC1101 support (pins + band switching)"
# trabalhar sempre na branch cap-cc1101; rebase quando sair versão nova do Bruce
```
Vale abrir/atualizar comentário na issue #2864 (persistência de pinos no
Cardputer-Adv) — várias pessoas presas no mesmo problema; a correção "certa" é
dar prioridade à config do usuário sobre os defaults do `_post_setup_gpio`.
