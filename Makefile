# Bruce Firmware Makefile
# Auto-detects PlatformIO & serial port (default: /dev/cu.usbmodem1101) to build and flash firmware.

SHELL := /bin/sh

# Default target board & serial port (override via CLI, e.g. make install ENV=lilygo-t-deck-pro PORT=/dev/cu.usbmodem1101)
ENV          ?= lilygo-t-deck
PORT         ?= /dev/cu.usbmodem1101
BAUD         ?= 921600
MONITOR_BAUD ?= 115200
CHIP         ?= esp32s3
BIN          ?= Bruce-$(ENV).bin

# Prefer PlatformIO's managed venv (avoids Homebrew Python 3.15 incompatibility with ESP32 platform)
PIO ?= $(shell \
	if [ -x "$(HOME)/.platformio/penv/bin/pio" ]; then \
		echo "$(HOME)/.platformio/penv/bin/pio"; \
	elif command -v pio >/dev/null 2>&1; then \
		command -v pio; \
	elif command -v platformio >/dev/null 2>&1; then \
		command -v platformio; \
	fi)

ESPTOOL ?= $(shell \
	if command -v esptool >/dev/null 2>&1; then \
		command -v esptool; \
	elif command -v esptool.py >/dev/null 2>&1; then \
		command -v esptool.py; \
	fi)

# Helper shell snippet: resolves ACTIVE_PORT starting from $(PORT) (default /dev/cu.usbmodem1101)
define RESOLVE_PORT
	ACTIVE_PORT=""; \
	if [ -e "$(PORT)" ]; then \
		ACTIVE_PORT="$(PORT)"; \
		echo "✅ Detected device on preferred port: $$ACTIVE_PORT"; \
	else \
		echo "⚠️  Preferred port $(PORT) not found. Scanning for USB serial devices..."; \
		FOUND_PORTS="$$(ls /dev/cu.usbmodem* /dev/cu.usbserial* /dev/cu.SLAB_USBtoUART* /dev/cu.wchusbserial* /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || true)"; \
		COUNT="$$(printf '%s\n' "$$FOUND_PORTS" | sed '/^$$/d' | wc -l | tr -d ' ')"; \
		if [ "$$COUNT" -eq 1 ]; then \
			ACTIVE_PORT="$$(printf '%s\n' "$$FOUND_PORTS" | sed '/^$$/d')"; \
			echo "✅ Auto-selected available port: $$ACTIVE_PORT"; \
		elif [ "$$COUNT" -gt 1 ]; then \
			echo "❌ Multiple serial ports detected:"; \
			printf '%s\n' "$$FOUND_PORTS" | sed 's/^/   - /'; \
			echo "Please specify one explicitly, e.g.: make install PORT=/dev/cu.usbmodemXXXX"; \
			exit 1; \
		else \
			echo "❌ No USB serial device found (expected $(PORT))."; \
			echo ""; \
			echo "👉 To put LilyGO T-Deck into Download (Bootloader) Mode:"; \
			echo "   1. Connect the T-Deck via a USB-C data cable."; \
			echo "   2. Slide the power switch OFF."; \
			echo "   3. Press and HOLD the Trackball button (GPIO0 / BOOT)."; \
			echo "   4. While holding the Trackball, slide the power switch ON (or press side RST)."; \
			echo "   5. Release the Trackball (the screen will stay black) and re-run 'make install'."; \
			exit 1; \
		fi; \
	fi
endef

.PHONY: help check-pio check-port ports envs build install upload flash-bin monitor clean tdeck tdeck-plus cardputer

.DEFAULT_GOAL := help

help: ## Show this help message
	@echo ""
	@echo "🦈 Bruce Firmware Build & Flash Helper"
	@echo "======================================"
	@echo "  ENV          = $(ENV)"
	@echo "  PORT         = $(PORT)"
	@echo "  BAUD         = $(BAUD)"
	@echo "  PIO          = $(if $(PIO),$(PIO),<not found>)"
	@echo ""
	@echo "Targets:"
	@grep -E '^[a-zA-Z0-9_-]+:.*?## ' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-14s\033[0m %s\n", $$1, $$2}'
	@echo ""
	@echo "Examples:"
	@echo "  make install                                # Check /dev/cu.usbmodem1101, build & flash lilygo-t-deck"
	@echo "  make tdeck                                  # Install on standard LilyGO T-Deck"
	@echo "  make tdeck-plus                             # Install on LilyGO T-Deck Plus"
	@echo "  make install ENV=lilygo-t-deck PORT=/dev/cu.usbmodem1101"
	@echo "  make upload                                 # Re-flash without rebuilding"
	@echo "  make monitor                                # Open serial monitor (115200 baud)"
	@echo ""

check-pio: ## Verify PlatformIO CLI is available
	@if [ -z "$(PIO)" ] || [ ! -x "$(PIO)" ]; then \
		echo "❌ PlatformIO CLI not found."; \
		echo "Install PlatformIO or ensure ~/.platformio/penv/bin/pio exists."; \
		exit 1; \
	fi
	@echo "✅ Using PlatformIO: $(PIO)"

check-port: ## Check if /dev/cu.usbmodem1101 (or another USB serial port) is connected
	@$(RESOLVE_PORT)

ports: ## List all detected USB serial ports
	@echo "🔍 Scanning /dev/cu.* and /dev/tty* ..."
	@FOUND="$$(ls /dev/cu.usbmodem* /dev/cu.usbserial* /dev/cu.SLAB_USBtoUART* /dev/cu.wchusbserial* /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || true)"; \
	if [ -n "$$FOUND" ]; then \
		printf '%s\n' "$$FOUND" | sed 's/^/  - /'; \
	else \
		echo "  (no USB serial devices found)"; \
	fi

envs: ## List all supported board environments in platformio.ini
	@echo "📋 Available environments in platformio.ini:"
	@awk '/^default_envs =/{flag=1; next} /^$$/{flag=0} flag{gsub(/^[ \t]*;?[ \t]*/, ""); sub(/[ \t]*;.*/, ""); if (length($$0)) print "  - " $$0}' platformio.ini

build: check-pio ## Build firmware for ENV (generates Bruce-<ENV>.bin)
	@echo "🔨 Building Bruce firmware for $(ENV)..."
	"$(PIO)" run -e "$(ENV)"
	@if [ -f "$(BIN)" ]; then \
		echo "✅ Merged binary ready: $(BIN)"; \
	fi

install: check-pio ## Check port (/dev/cu.usbmodem1101), build, and flash firmware
	@set -e; \
	$(RESOLVE_PORT); \
	echo "🚀 Building and flashing $(ENV) to $$ACTIVE_PORT..."; \
	"$(PIO)" run -e "$(ENV)" -t upload --upload-port "$$ACTIVE_PORT"; \
	echo ""; \
	echo "🎉 Flash complete! Press the Reset button (or power-cycle) on your device to boot Bruce."

upload: check-pio ## Flash already-built firmware without rebuilding (nobuild)
	@set -e; \
	$(RESOLVE_PORT); \
	echo "⚡ Uploading prebuilt $(ENV) firmware to $$ACTIVE_PORT..."; \
	"$(PIO)" run -e "$(ENV)" -t upload -t nobuild --upload-port "$$ACTIVE_PORT"; \
	echo ""; \
	echo "🎉 Upload complete! Press the Reset button (or power-cycle) on your device to boot Bruce."

flash-bin: ## Flash merged Bruce-<ENV>.bin directly via esptool at 0x00000
	@set -e; \
	if [ ! -f "$(BIN)" ]; then \
		echo "ℹ️  $(BIN) not found. Building first..."; \
		$(MAKE) build ENV="$(ENV)"; \
	fi; \
	$(RESOLVE_PORT); \
	echo "⚡ Flashing $(BIN) to $$ACTIVE_PORT (chip=$(CHIP), baud=$(BAUD))..."; \
	if [ -n "$(ESPTOOL)" ]; then \
		"$(ESPTOOL)" --chip "$(CHIP)" --port "$$ACTIVE_PORT" --baud "$(BAUD)" write_flash 0x00000 "$(BIN)"; \
	elif [ -n "$(PIO)" ]; then \
		"$(PIO)" pkg exec -p tool-esptoolpy -- esptool.py --chip "$(CHIP)" --port "$$ACTIVE_PORT" --baud "$(BAUD)" write_flash 0x00000 "$(BIN)"; \
	else \
		echo "❌ Neither esptool nor PlatformIO was found."; \
		exit 1; \
	fi; \
	echo ""; \
	echo "🎉 Flash complete! Press the Reset button (or power-cycle) on your device to boot Bruce."

monitor: check-pio ## Open serial monitor on the connected port
	@set -e; \
	$(RESOLVE_PORT); \
	echo "📟 Opening serial monitor on $$ACTIVE_PORT ($(MONITOR_BAUD) baud)... (Ctrl+C to exit)"; \
	"$(PIO)" device monitor --port "$$ACTIVE_PORT" --baud "$(MONITOR_BAUD)"

clean: check-pio ## Clean PlatformIO build artifacts for ENV
	"$(PIO)" run -e "$(ENV)" -t clean

# Convenience device shortcuts
tdeck: ## Auto-install Bruce on standard LilyGO T-Deck
	@$(MAKE) install ENV=lilygo-t-deck PORT="$(PORT)"

tdeck-plus: ## Auto-install Bruce on LilyGO T-Deck Plus
	@$(MAKE) install ENV=lilygo-t-deck-pro PORT="$(PORT)"

cardputer: ## Auto-install Bruce on M5Stack Cardputer
	@$(MAKE) install ENV=m5stack-cardputer PORT="$(PORT)"
