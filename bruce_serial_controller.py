#!/usr/bin/env python3
"""
Bruce Serial Controller - Simple Edition
Just pick a number, get clean output.

Requirements: pip install pyserial
"""

import serial
import serial.tools.list_ports
import sys
import time
import threading
import os

# ============================================================================
# Controller
# ============================================================================

class BruceController:
    def __init__(self, port: str):
        self.port = port
        self.serial = None
        self.running = False
        self.capturing = False
        
    def connect(self) -> bool:
        try:
            self.serial = serial.Serial(self.port, 115200, timeout=0.5)
            time.sleep(0.3)
            self.running = True
            # Start reader thread
            self.reader = threading.Thread(target=self._read_loop, daemon=True)
            self.reader.start()
            return True
        except Exception as e:
            print(f"  Error: {e}")
            return False
    
    def disconnect(self):
        self.running = False
        if self.serial:
            self.serial.close()
    
    def _read_loop(self):
        """Background reader - only prints when capturing"""
        while self.running:
            try:
                if self.serial and self.serial.in_waiting:
                    line = self.serial.readline().decode('utf-8', errors='ignore').strip()
                    if line and self.capturing:
                        # Filter and print important lines
                        if any(x in line.lower() for x in ['captured', 'found', 'connected', 'started', 'stopped', 'error', 'ssid', 'mac', 'ip:', '===']):
                            print(f"  {line}")
                        elif line.startswith('[') or line.startswith('─'):
                            print(f"  {line}")
            except:
                pass
            time.sleep(0.01)
    
    def send(self, cmd: str, capture_time: float = 2.0):
        """Send command and capture output for a bit"""
        if not self.serial:
            print("  Not connected!")
            return
        
        self.serial.write((cmd + '\n').encode())
        self.serial.flush()
        
        # Capture output briefly
        self.capturing = True
        time.sleep(capture_time)
        self.capturing = False

# ============================================================================
# Menus
# ============================================================================

def clear():
    os.system('cls' if os.name == 'nt' else 'clear')

def print_header():
    print()
    print("  ╔═══════════════════════════════════════╗")
    print("  ║     🦈 BRUCE SERIAL CONTROLLER        ║")
    print("  ╚═══════════════════════════════════════╝")
    print()

def select_port() -> str:
    """Select serial port"""
    ports = list(serial.tools.list_ports.comports())
    
    if not ports:
        print("  No serial ports found!")
        return None
    
    print("  Available Ports:")
    print("  ─────────────────")
    for i, p in enumerate(ports, 1):
        print(f"  {i}. {p.device} - {p.description[:40]}")
    print()
    
    while True:
        try:
            choice = input("  Select port [1-{}]: ".format(len(ports))).strip()
            idx = int(choice) - 1
            if 0 <= idx < len(ports):
                return ports[idx].device
        except (ValueError, KeyboardInterrupt):
            return None
        print("  Invalid choice.")

def main_menu(ctrl: BruceController):
    """Main menu"""
    while True:
        clear()
        print_header()
        print(f"  Connected: {ctrl.port}")
        print()
        print("  ┌─────────────────────────────────────┐")
        print("  │  MAIN MENU                          │")
        print("  ├─────────────────────────────────────┤")
        print("  │  1. WiFi Control                    │")
        print("  │  2. WiFi Attacks                    │")
        print("  │  3. Bluetooth                       │")
        print("  │  4. System                          │")
        print("  │  0. Exit                            │")
        print("  └─────────────────────────────────────┘")
        print()
        
        choice = input("  Select [0-4]: ").strip()
        
        if choice == "1":
            wifi_menu(ctrl)
        elif choice == "2":
            attacks_menu(ctrl)
        elif choice == "3":
            bluetooth_menu(ctrl)
        elif choice == "4":
            system_menu(ctrl)
        elif choice == "0":
            break

def wifi_menu(ctrl: BruceController):
    """WiFi control menu"""
    while True:
        clear()
        print_header()
        print("  ┌─────────────────────────────────────┐")
        print("  │  WIFI CONTROL                       │")
        print("  ├─────────────────────────────────────┤")
        print("  │  1. Start WebUI                     │")
        print("  │  2. Scan Networks                   │")
        print("  │  3. WiFi Info                       │")
        print("  │  4. Connect WiFi                    │")
        print("  │  5. Disconnect                      │")
        print("  │  6. Start AP Mode                   │")
        print("  │  0. Back                            │")
        print("  └─────────────────────────────────────┘")
        print()
        
        choice = input("  Select [0-6]: ").strip()
        
        if choice == "1":
            print("\n  Starting WebUI...")
            print("  URL: http://192.168.4.1")
            print("  Login: admin / bruce")
            ctrl.send("webui", 3)
            input("\n  Press Enter to continue...")
        elif choice == "2":
            print("\n  Scanning networks...")
            ctrl.send("wifi scan", 5)
            input("\n  Press Enter to continue...")
        elif choice == "3":
            print("\n  Getting WiFi info...")
            ctrl.send("wifi info", 2)
            input("\n  Press Enter to continue...")
        elif choice == "4":
            print("\n  Connecting to known network...")
            ctrl.send("wifi on", 5)
            input("\n  Press Enter to continue...")
        elif choice == "5":
            print("\n  Disconnecting...")
            ctrl.send("wifi off", 1)
            print("  Done.")
            input("\n  Press Enter to continue...")
        elif choice == "6":
            print("\n  Starting AP mode...")
            ctrl.send("wifi ap", 2)
            input("\n  Press Enter to continue...")
        elif choice == "0":
            break

def attacks_menu(ctrl: BruceController):
    """WiFi attacks menu"""
    while True:
        clear()
        print_header()
        print("  ┌─────────────────────────────────────┐")
        print("  │  WIFI ATTACKS                       │")
        print("  ├─────────────────────────────────────┤")
        print("  │  1. Evil Portal (Start)             │")
        print("  │  2. Evil Portal (Stop)              │")
        print("  │  3. Deauth Flood                    │")
        print("  │  4. Deauth Scan                     │")
        print("  │  5. Beacon Spam                     │")
        print("  │  6. Packet Sniffer                  │")
        print("  │  7. ARP Scan                        │")
        print("  │  0. Back                            │")
        print("  └─────────────────────────────────────┘")
        print()
        
        choice = input("  Select [0-7]: ").strip()
        
        if choice == "1":
            ssid = input("\n  Enter SSID [Free WiFi]: ").strip()
            if not ssid:
                ssid = "Free WiFi"
            print(f"\n  Starting Evil Portal with SSID: {ssid}")
            ctrl.send(f'evilportal "{ssid}"', 3)
            print("\n  Portal is running!")
            print("  Victims connecting will appear below.")
            print("  Press Enter to return (portal keeps running)")
            
            # Keep capturing
            ctrl.capturing = True
            input()
            ctrl.capturing = False
            
        elif choice == "2":
            print("\n  Stopping Evil Portal...")
            ctrl.send("portalstop", 2)
            input("\n  Press Enter to continue...")
        elif choice == "3":
            print("\n  Starting Deauth Flood...")
            print("  This will deauth all clients from all networks.")
            ctrl.send("deauth flood", 2)
            input("\n  Press Enter to continue...")
        elif choice == "4":
            print("\n  Scanning for deauth targets...")
            ctrl.send("deauth scan", 5)
            input("\n  Press Enter to continue...")
        elif choice == "5":
            print("\n  Starting Beacon Spam...")
            ctrl.send("beacon", 2)
            input("\n  Press Enter to continue...")
        elif choice == "6":
            print("\n  Starting Packet Sniffer...")
            ctrl.send("sniffer", 2)
            input("\n  Press Enter to continue...")
        elif choice == "7":
            print("\n  Scanning network for hosts...")
            ctrl.send("arp", 5)
            input("\n  Press Enter to continue...")
        elif choice == "0":
            break

def bluetooth_menu(ctrl: BruceController):
    """Bluetooth menu"""
    while True:
        clear()
        print_header()
        print("  ┌─────────────────────────────────────┐")
        print("  │  BLUETOOTH                          │")
        print("  ├─────────────────────────────────────┤")
        print("  │  1. BLE Scan                        │")
        print("  │  2. Apple Spam                      │")
        print("  │  3. Android Spam                    │")
        print("  │  4. Samsung Spam                    │")
        print("  │  5. Windows Spam                    │")
        print("  │  6. All Random Spam                 │")
        print("  │  7. iBeacon                         │")
        print("  │  8. BLE Info                        │")
        print("  │  0. Back                            │")
        print("  └─────────────────────────────────────┘")
        print()
        
        choice = input("  Select [0-8]: ").strip()
        
        if choice == "1":
            print("\n  Scanning for BLE devices...")
            ctrl.send("blescan", 5)
            input("\n  Press Enter to continue...")
        elif choice == "2":
            print("\n  Starting Apple popup spam...")
            ctrl.send("blespam apple", 2)
            input("\n  Press Enter to continue...")
        elif choice == "3":
            print("\n  Starting Android popup spam...")
            ctrl.send("blespam android", 2)
            input("\n  Press Enter to continue...")
        elif choice == "4":
            print("\n  Starting Samsung popup spam...")
            ctrl.send("blespam samsung", 2)
            input("\n  Press Enter to continue...")
        elif choice == "5":
            print("\n  Starting Windows popup spam...")
            ctrl.send("blespam windows", 2)
            input("\n  Press Enter to continue...")
        elif choice == "6":
            print("\n  Starting random BLE spam (all types)...")
            ctrl.send("blespam all", 2)
            input("\n  Press Enter to continue...")
        elif choice == "7":
            name = input("\n  iBeacon name [Bruce iBeacon]: ").strip()
            if not name:
                name = "Bruce iBeacon"
            print(f"\n  Broadcasting iBeacon: {name}")
            ctrl.send(f'ibeacon "{name}"', 2)
            input("\n  Press Enter to continue...")
        elif choice == "8":
            print("\n  Getting BLE info...")
            ctrl.send("bleinfo", 2)
            input("\n  Press Enter to continue...")
        elif choice == "0":
            break

def system_menu(ctrl: BruceController):
    """System menu"""
    while True:
        clear()
        print_header()
        print("  ┌─────────────────────────────────────┐")
        print("  │  SYSTEM                             │")
        print("  ├─────────────────────────────────────┤")
        print("  │  1. Device Info                     │")
        print("  │  2. Help                            │")
        print("  │  3. Reboot                          │")
        print("  │  4. Raw Command                     │")
        print("  │  0. Back                            │")
        print("  └─────────────────────────────────────┘")
        print()
        
        choice = input("  Select [0-4]: ").strip()
        
        if choice == "1":
            print("\n  Getting device info...")
            ctrl.send("info", 2)
            input("\n  Press Enter to continue...")
        elif choice == "2":
            print("\n  Available commands:")
            ctrl.send("wifihelp", 2)
            input("\n  Press Enter to continue...")
        elif choice == "3":
            confirm = input("\n  Reboot device? [y/N]: ").strip().lower()
            if confirm == 'y':
                print("  Rebooting...")
                ctrl.send("reboot", 1)
        elif choice == "4":
            cmd = input("\n  Enter command: ").strip()
            if cmd:
                print(f"  Sending: {cmd}")
                ctrl.send(cmd, 3)
            input("\n  Press Enter to continue...")
        elif choice == "0":
            break

# ============================================================================
# Main
# ============================================================================

def main():
    clear()
    print_header()
    
    # Select port
    port = select_port()
    if not port:
        print("\n  No port selected. Exiting.")
        return
    
    print(f"\n  Connecting to {port}...")
    
    ctrl = BruceController(port)
    if not ctrl.connect():
        print("  Failed to connect!")
        return
    
    print("  Connected!")
    time.sleep(1)
    
    try:
        main_menu(ctrl)
    except KeyboardInterrupt:
        pass
    finally:
        ctrl.disconnect()
    
    clear()
    print("\n  Goodbye! 👋\n")

if __name__ == "__main__":
    main()
