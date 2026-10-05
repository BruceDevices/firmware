"""Source-level regression checks for the dedicated Cardputer ADV 3IN1 build.

These checks do not emulate the ESP32 or prove electrical radio operation.
"""

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def source(relative_path: str) -> str:
    return (ROOT / relative_path).read_text(encoding="utf-8")


class CardputerAdv3In1ProfileTests(unittest.TestCase):
    def test_profile_pinout_and_persistence(self):
        ini = source("boards/m5stack-cardputer/m5stack-cardputer-adv-3in1.ini")
        pins = source("src/core/configPins.h")
        config = source("src/core/configPins.cpp")
        self.assertIn("board = m5stack-cardputer-adv", ini)
        self.assertIn("-DCARDPUTER_ADV_3IN1=1", ini)
        for radio, pinout in (
            ("CC1101_bus", (40, 39, 14, 15, 13)),
            ("NRF24_bus", (40, 39, 14, 9, 8)),
            ("LoRa_bus", (40, 39, 14, 5, 3, 4)),
        ):
            values = ", ".join(f"GPIO_NUM_{pin}" for pin in pinout)
            self.assertIn(f"SPIPins {radio} = {{{values}}};", pins)
            self.assertIn(values, config)
        self.assertRegex(config, r"if \(applyBoardProfile\(\)\) count\+\+;\s*#endif\s*validateConfig\(\);")
        self.assertRegex(config, r"if \(count > 0\) saveFile\(\);")

    def test_keyboard_nrf_keyboard_sequence(self):
        board = source("boards/m5stack-cardputer/interface.cpp")
        common = source("src/modules/NRF24/nrf_common.cpp")
        main = source("src/main.cpp")
        enter = board.split("bool cardputerAdvEnterNrf()", 1)[1].split("bool cardputerAdvLeaveNrf()", 1)[0]
        leave = board.split("bool cardputerAdvLeaveNrf()", 1)[1].split("#endif", 1)[0]
        for step in ("cardputerAdvLockInput()", "detachInterrupt", "Wire1.end()", "digitalWrite(8, LOW)", "digitalWrite(9, HIGH)", "pinMode(8, OUTPUT)", "pinMode(9, OUTPUT)"):
            self.assertIn(step, enter)
        self.assertLess(enter.index("Wire1.end()"), enter.index("pinMode(8, OUTPUT)"))
        for step in ("digitalWrite(8, LOW)", "digitalWrite(9, HIGH)", "pinMode(8, INPUT)", "pinMode(9, INPUT)", "Wire1.begin", "tca.begin", "tca.enableInterrupts()"):
            self.assertIn(step, leave)
        self.assertLess(leave.index("pinMode(9, INPUT)"), leave.index("Wire1.begin"))
        self.assertIn("if (!cardputerAdvEnterNrf()) return false;", common)
        self.assertIn("cardputerAdvLeaveNrf();", common)
        self.assertIn("if (cardputerAdvNrfActive()) EscPress = true;", board)
        self.assertIn("if (!cardputerAdvNrfActive()) checkAndRecoverSysI2CBus();", main)
        self.assertIn("cardputerAdvKeyboardRecoveryPending() && millis() - lastKeyboardRetry >= 500", main)
        self.assertIn("cardputerAdvLeaveNrf();", main)

    def test_radio_cs_and_sx1262_configuration(self):
        hal = source("src/core/bus_HAL.cpp")
        rf = source("src/modules/rf/rf_utils.cpp")
        nrf = source("src/modules/NRF24/nrf_common.cpp")
        lora = source("src/modules/lora/LoRaRF.cpp")
        ini = source("boards/m5stack-cardputer/m5stack-cardputer-adv-3in1.ini")
        self.assertIn("selected != RadioSPISelection::CC1101", hal)
        self.assertIn("selected != RadioSPISelection::LoRa", hal)
        self.assertIn("cardputerAdvNrfActive()", hal)
        self.assertIn("prepareRadioSPI(RadioSPISelection::CC1101)", rf)
        self.assertIn("prepareRadioSPI(RadioSPISelection::NRF24)", nrf)
        self.assertIn("prepareRadioSPI(RadioSPISelection::LoRa)", lora)
        self.assertIn("-DLORA_BUSY=6", ini)
        self.assertIn("-DLORA_IRQ=4", ini)
        self.assertIn("loraRadioVariant = LoRaRadioVariant::SX1262", lora)
        self.assertIn("new Module(getLoraCsPin(), irqPin, getLoraResetPin(), busyPin, *loraSpi)", lora)

    def test_radio_menus_remain_registered(self):
        for name, item in (("RFMenu.cpp", "CC1101"), ("NRF24.cpp", "NRF24"), ("LoRaMenu.cpp", "LoRa")):
            text = source(f"src/core/menu_items/{name}")
            self.assertIn(item, text)
            self.assertIn("loopOptions", text)


if __name__ == "__main__":
    unittest.main()
