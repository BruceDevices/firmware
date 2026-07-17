import os
import re
import glob

print("Patching library conflicts with ESP32 core...")

# List of conflicts: (file_pattern, search_pattern, replace_pattern)
conflicts = [
    (
        ".pio/libdeps/*/ESP8266SAM/src/render.c",
        r'static void yield\(\)',
        'static void sam_yield()'
    ),
    (
        ".pio/libdeps/*/ESP8266SAM/src/render.c",
        r'\byield\(\);',
        'sam_yield();'
    ),
    (
        ".pio/libdeps/*/JPEGDecoder/src/picojpeg.c",
        r'static uint8 init\(void\)',
        'static uint8 picojpeg_init(void)'
    ),
    (
        ".pio/libdeps/*/JPEGDecoder/src/picojpeg.c",
        r'\binit\(\);',
        'picojpeg_init();'
    ),
    (
        ".pio/libdeps/*/ESP Amiibolink/src/amiibolink.h",
        r'#include <NimBLEDevice.h>',
        '#include <Arduino.h>\n#include <NimBLEDevice.h>'
    ),
    (
        ".pio/libdeps/*/ESP Chameleon Ultra/src/chameleonUltra.h",
        r'#include <NimBLEDevice.h>',
        '#include <Arduino.h>\n#include <NimBLEDevice.h>'
    ),
    (
        ".pio/libdeps/*/ESP PN32BLE/src/pn532_ble.h",
        r'#include <NimBLEDevice.h>',
        '#include <Arduino.h>\n#include <NimBLEDevice.h>'
    ),
    # ST25R3916-fork: support IRQ-less operation (I2C units have no interrupt
    # line). When int_pin < 0 we skip attaching the GPIO interrupt and poll the
    # chip's IRQ register over the bus instead of gating on digitalRead(int_pin).
    # The fork is pinned to a fixed commit in platformio.ini so these patterns
    # stay valid.
    (
        ".pio/libdeps/*/ST25R3916-fork/src/rfal_rfst25r3916.cpp",
        r'(?s)pinMode\(int_pin, INPUT\);.*?attachInterrupt\(int_pin, irq_handler, RISING\);',
        'if (int_pin >= 0) {\n'
        '    pinMode(int_pin, INPUT);\n'
        '    Callback<void()>::func = std::bind(&RfalRfST25R3916Class::setISRPending, this);\n'
        '    irq_handler = static_cast<ST25R3916IrqHandler>(Callback<void()>::callback);\n'
        '    attachInterrupt(int_pin, irq_handler, RISING);\n'
        '  }'
    ),
    (
        ".pio/libdeps/*/ST25R3916-fork/src/rfal_rfst25r3916.cpp",
        r'detachInterrupt\(int_pin\);',
        'if (int_pin >= 0) { detachInterrupt(int_pin); }'
    ),
    # Without an IRQ pin, isISRPending() must return true unconditionally: rfalWorker()
    # (rfal_rfst25r3916.cpp) polls it once per tick to decide whether to read the chip's IRQ
    # status registers at all, and that's the *only* thing that drives card-detection polling
    # in I2C mode. Returning the real isr_pending flag instead (which nothing ever sets without
    # a GPIO ISR) makes rfalWorker() never check the chip, so no tag is ever detected.
    (
        ".pio/libdeps/*/ST25R3916-fork/src/rfal_rfst25r3916.cpp",
        r'return \(isr_pending \|\| \(digitalRead\(int_pin\) == HIGH\)\);',
        'return (int_pin < 0) ? true : (isr_pending || (digitalRead(int_pin) == HIGH));'
    ),
    # isISRPending() being unconditionally true in I2C mode (needed above) means every one of
    # the 11 post-transaction "if (isISRPending()) { ...; st25r3916Isr(); }" checks in
    # st25r3916_com.cpp also fires on every single register access, and st25r3916Isr() ->
    # st25r3916CheckForReceivedInterrupts() -> another register read -> checks isISRPending()
    # again -> recurses forever, blowing the stack (stack-canary panic right after
    # "[ST25R] begin: mode=I2C"). Guard st25r3916Isr() against re-entrancy instead of touching
    # isISRPending(), so the legitimate per-tick poll from rfalWorker() keeps working. The guard
    # only applies when int_pin < 0: in real IRQ/SPI mode a nested call means a genuine new
    # interrupt arrived mid-transaction (isr_pending was already cleared by the com.cpp caller
    # right before it), and skipping that call would drop/delay a real event, so SPI keeps the
    # library's original (unbounded but legitimate) recursive behavior untouched.
    (
        ".pio/libdeps/*/ST25R3916-fork/src/rfal_rfst25r3916.h",
        r'volatile bool bus_busy;',
        'volatile bool bus_busy;\n    volatile bool isr_active;'
    ),
    (
        ".pio/libdeps/*/ST25R3916-fork/src/rfal_rfst25r3916.cpp",
        r'bus_busy = false;',
        'bus_busy = false;\n  isr_active = false;'
    ),
    (
        ".pio/libdeps/*/ST25R3916-fork/src/st25r3916_interrupt.cpp",
        r'void RfalRfST25R3916Class::st25r3916Isr\(void\)\n'
        r'\{\n'
        r'  if \(!isBusBusy\(\)\) \{\n'
        r'    st25r3916CheckForReceivedInterrupts\(\);\n'
        r'\n'
        r'    // Check if callback is set and run it\n'
        r'    if \(NULL != st25r3916interrupt\.callback\) \{\n'
        r'      st25r3916interrupt\.callback\(\);\n'
        r'    \}\n'
        r'  \} else \{\n'
        r'    setISRPending\(\);\n'
        r'  \}\n'
        r'\}\n',
        'void RfalRfST25R3916Class::st25r3916Isr(void)\n'
        '{\n'
        '  if (isr_active && int_pin < 0) { return; }\n'
        '  isr_active = true;\n'
        '  if (!isBusBusy()) {\n'
        '    st25r3916CheckForReceivedInterrupts();\n'
        '\n'
        '    // Check if callback is set and run it\n'
        '    if (NULL != st25r3916interrupt.callback) {\n'
        '      st25r3916interrupt.callback();\n'
        '    }\n'
        '  } else {\n'
        '    setISRPending();\n'
        '  }\n'
        '  isr_active = false;\n'
        '}\n'
    ),
    # In polling mode read the IRQ registers exactly once per call (the read
    # auto-clears them on the chip); in IRQ mode keep draining while the pin is
    # high. Converts the edge-drain while-loop into a do/while.
    (
        ".pio/libdeps/*/ST25R3916-fork/src/st25r3916_interrupt.cpp",
        r'(?s)while \(digitalRead\(int_pin\) == HIGH\) \{.*?iregs\[3\] << 24;\s*\}',
        'do {\n'
        '    st25r3916ReadMultipleRegisters(ST25R3916_REG_IRQ_MAIN, iregs, ST25R3916_INT_REGS_LEN);\n'
        '\n'
        '    irqStatus |= (uint32_t)iregs[0];\n'
        '    irqStatus |= (uint32_t)iregs[1] << 8;\n'
        '    irqStatus |= (uint32_t)iregs[2] << 16;\n'
        '    irqStatus |= (uint32_t)iregs[3] << 24;\n'
        '  } while (int_pin >= 0 && digitalRead(int_pin) == HIGH);'
    ),
    # The I2C constructor's member-init list never sets cs_pin, so it's left
    # as whatever garbage was on the heap. rfalInitialize() then unconditionally
    # does pinMode(cs_pin, OUTPUT)/digitalWrite(cs_pin, HIGH), which panics with
    # "Invalid IO <garbage>" on I2C-only boards that have no CS line wired.
    # Skip the CS pin setup entirely when running in I2C mode.
    (
        ".pio/libdeps/*/ST25R3916-fork/src/rfal_rfst25r3916.cpp",
        r'pinMode\(cs_pin, OUTPUT\);\s*\n\s*digitalWrite\(cs_pin, HIGH\);',
        'if (!i2c_enabled) {\n'
        '    pinMode(cs_pin, OUTPUT);\n'
        '    digitalWrite(cs_pin, HIGH);\n'
        '  }'
    ),
]

for file_pattern, search, replace in conflicts:
    files = glob.glob(file_pattern)
    for file_path in files:
        try:
            with open(file_path, 'r') as f:
                content = f.read()

            if replace in content:
                print(f"Already patched: {file_path}")
                continue

            new_content = re.sub(search, replace, content)
            if new_content != content:
                with open(file_path, 'w') as f:
                    f.write(new_content)
                print(f"Patched: {file_path}")
        except Exception as e:
            print(f"Failed to patch {file_path}: {e}")
