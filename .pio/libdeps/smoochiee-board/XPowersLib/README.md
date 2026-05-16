

```
 __   __     _____                           _      _ _
 \ \ / /    |  __ \                         | |    (_) |
  \ V /_____| |__) |____      _____ _ __ ___| |     _| |__
   > <______|  ___/ _ \ \ /\ / / _ \ '__/ __| |    | | '_ \
  / . \     | |  | (_) \ V  V /  __/ |  \__ \ |____| | |_) |
 /_/ \_\    |_|   \___/ \_/\_/ \___|_|  |___/______|_|_.__/

```
[![Build esp-idf](https://github.com/lewisxhe/XPowersLib/actions/workflows/esp-idf.yml/badge.svg)](https://github.com/lewisxhe/XPowersLib/actions/workflows/esp-idf.yml)
[![Build PlatformIO](https://github.com/lewisxhe/XPowersLib/actions/workflows/pio.yml/badge.svg)](https://github.com/lewisxhe/XPowersLib/actions/workflows/pio.yml)
[![Build Arduino](https://github.com/lewisxhe/XPowersLib/actions/workflows/arduino_ci.yml/badge.svg)](https://github.com/lewisxhe/XPowersLib/actions/workflows/arduino_ci.yml)
[![arduino-library-badge](https://www.ardu-badge.com/badge/XPowersLib.svg?)](https://www.ardu-badge.com/XPowersLib)

[![LICENSE](https://img.shields.io/github/license/lewisxhe/XPowersLib)](https://github.com/lewisxhe/XPowersLib/blob/master/LICENSE)
[![ISSUES](https://img.shields.io/github/issues/lewisxhe/XPowersLib)](https://github.com/lewisxhe/XPowersLib/issues)
[![FROK](https://img.shields.io/github/forks/lewisxhe/XPowersLib)](https://github.com/lewisxhe/XPowersLib/graphs/contributors)
[![STAR](https://img.shields.io/github/stars/lewisxhe/XPowersLib)](https://github.com/lewisxhe/XPowersLib/stargazers)
[![releases](https://img.shields.io/github/release/lewisxhe/XPowersLib)](https://github.com/lewisxhe/XPowersLib/releases)

```
🎉 LilyGo invests time and resources to provide this open source code, please support LilyGo 
and open source hardware by purchasing products from LilyGo!
Written by Lewis He for LilyGo. MIT license, all text above must be included in 
any redistribution
```

# ❗️❗️❗️  WARN:

```
⚠️ Please do not run the example without knowing the external load voltage of the PMU,
it may burn your external load,please check the voltage setting before running the example,
if there is any loss,please bear it by yourself
```


✨ Library specially written for XPowers, supporting **CircuitPython**, **Micropython**, **Arduino**, **ESP-IDF**

✨ Through esp32,esp32s3 verification,other platforms have not been tested. Due to the many functions of the chip,it cannot be verified one by one.


# Chip Resource List

| CHIP       | AXP192            | AXP202            | AXP2101                                                     | Remarks          |
| ---------- | ----------------- | ----------------- | ----------------------------------------------------------- | ---------------- |
| DC1        | 0.7V-3.5V  /1.2A  | X                 | 1.5-3.4V                        /2A                         |                  |
| DC2        | 0.7-2.275V /1.6A  | 0.7-2.275V /1.6A  | 0.5-1.2V,1.22-1.54V             /2A                         |                  |
| DC3        | 0.7-3.5V   /0.7A  | 0.7-3.5V   /1.2A  | 0.5-1.2V,1.22-1.54V,1.6-3.4V    /2A                         |                  |
| DC4        | x                 | x                 | 0.5-1.2V,1.22-1.84V            /1.5A                        |                  |
| DC5        | x                 | x                 | 1.2V,1.4-3.7V                   /1A                         |                  |
| LDO1(VRTC) | 3.3V       /30mA  | 3.3V       /30mA  | 1.8V                            /30mA                       |                  |
| LDO2       | 1.8V-3.3V  /200mA | 1.8V-3.3V  /200mA | x                                                           |                  |
| LDO3       | 1.8-3.3V   /200mA | 0.7-3.5V   /200mA | x                                                           |                  |
| LDO4       | X                 | 1.8V-3.3V  /200mA | x                                                           |                  |
| LDO5/IO0   | 1.8-3.3V   /50mA  | 1.8-3.3V   /50mA  | x                                                           |                  |
| ALDO1      | x                 | x                 | 0.5-3.5V                        /300mA                      |                  |
| ALDO2      | x                 | x                 | 0.5-3.5V                        /300mA                      |                  |
| ALDO3      | x                 | x                 | 0.5-3.5V                        /300mA                      |                  |
| ALDO4      | x                 | x                 | 0.5-3.5V                        /300mA                      |                  |
| BLDO1      | x                 | x                 | 0.5-3.5V                        /300mA                      |                  |
| BLDO2      | x                 | x                 | 0.5-3.5V                        /300mA                      |                  |
| DLDO1      | x                 | x                 | (LDO Mode)0.5-3.3V/ 0.5-1.4V/300mA,(SW Mode)Switch Function | Dependent on DC1 |
| DLDO2      | x                 | x                 | (LDO Mode)0.5-3.3V/ 0.5-1.4V/300mA,(SW Mode)Switch Function | Dependent on DC4 |
| CPUSLDO    | x                 | x                 | 0.5-1.4V                        /30mA                       | Dependent on DC4 |


# AXP2101 Notes:

* Whether DLDO1/DLDO2/RTCLDO2/GPIO1 can be used depends on the chip. It is not available by default. RTCLDO1 has a default voltage, which is generally 1.8V by default. The voltage value cannot be changed or turned off through the register.
