---
name: Module not working
about: Template to use when your module isn't working
title: ''
labels: ''
assignees: ''

---

**IMPORTANT: Before submitting an issue, please check the following:**
1. **Read [CONTRIBUTING.md](https://github.com/jgromes/RadioLib/blob/master/CONTRIBUTING.md)!** Issues that do not follow this document will be closed/locked/deleted/ignored.
2. RadioLib has a [Troubleshooting Guide](https://github.com/jgromes/RadioLib/wiki/Troubleshooting-Guide) Wiki page and an extensive [API documentation](https://jgromes.github.io/RadioLib/). If you are seeing an error code, we have [online status code decoder](https://radiolib-org.github.io/status_decoder/decode.html).
3. Make sure you're using the latest release of the library! Releases can be found [here](https://github.com/jgromes/RadioLib/releases).
4. Use [Arduino forums](https://forum.arduino.cc/) to ask generic questions about wireless modules, wiring, usage, etc. Only create issues for problems specific to RadioLib!
5. Error codes, their meaning and how to fix them can be found on [this page](https://jgromes.github.io/RadioLib/group__status__codes.html).

<details><summary>Sketch that is causing the module fail</summary>
<p>

```c++
paste the sketch here, even if it is an unmodified example code
```

</p>
</details> 

**Hardware setup**
Wiring diagram, schematic, pictures etc.

**Debug mode output**
Enable the appropriate [debug levels](https://github.com/jgromes/RadioLib/wiki/Debug-mode) and paste the Serial monitor output here. For debugging protocols, enable `RADIOLIB_DEBUG_PROTOCOL`. For debugging issues with the radio module itself, enable `RADIOLIB_DEBUG_SPI`.

<details><summary>Debug mode output</summary>
<p>

```
paste the debug output here
```

</p>
</details> 

**Additional info (please complete):**
 - MCU: [e.g. Arduino Uno, ESP8266 etc.]
 - Link to Arduino core: [e.g. https://github.com/stm32duino/Arduino_Core_STM32 when using official STM32 core. See readme for links to all supported cores]
 - Wireless module type [e.g. CC1101, SX1268, etc.]
 - Arduino IDE version  [e.g. 1.8.5]
 - Library version [e.g. 3.0.0 or git hash]
