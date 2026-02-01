#!/usr/bin/env python3
"""
Bruce Serial Controller
A Python CLI tool to control Bruce firmware via serial commands.

Usage:
    python bruce_serial_controller.py [port]
    
Examples:
    python bruce_serial_controller.py COM6
    python bruce_serial_controller.py /dev/ttyUSB0

Requirements:
    pip install pyserial
"""

import serial
import serial.tools.list_ports
import sys
import time
import threading
import os
from typing import Optional

# ANSI color codes
class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    DIM = '\033[2m'

def color(text: str, c: str) -> str:
    """Apply color to text."""
    if sys.platform == 'win32':
        os.system('')  # Enable ANSI on Windows
    return f"{c}{text}{Colors.ENDC}"


class BruceController:
    """Serial controller for Bruce firmware."""
    
    def __init__(self, port: str, baudrate: int = 115200):
        self.port = port
        self.baudrate = baudrate
        self.serial: Optional[serial.Serial] = None
        self.running = False
        self.reader_thread: Optional[threading.Thread] = None
        
    def connect(self) -> bool:
        """Connect to the Bruce device."""
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=0.1,
                write_timeout=2
            )
            time.sleep(0.5)  # Wait for connection to stabilize
            self.running = True
            self.reader_thread = threading.Thread(target=self._read_loop, daemon=True)
            self.reader_thread.start()
            return True
        except serial.SerialException as e:
            print(color(f"Failed to connect: {e}", Colors.RED))
            return False
    
    def disconnect(self):
        """Disconnect from the device."""
        self.running = False
        if self.serial and self.serial.is_open:
            self.serial.close()
        print(color("Disconnected.", Colors.YELLOW))
    
    def _read_loop(self):
        """Background thread to read serial output."""
        while self.running and self.serial and self.serial.is_open:
            try:
                if self.serial.in_waiting:
                    data = self.serial.readline()
                    if data:
                        try:
                            text = data.decode('utf-8', errors='replace').strip()
                            if text:
                                print(color(f"[Bruce] {text}", Colors.CYAN))
                        except:
                            pass
            except serial.SerialException:
                break
            except Exception:
                pass
            time.sleep(0.01)
    
    def send_command(self, cmd: str, wait_response: float = 0.5) -> bool:
        """Send a command to Bruce."""
        if not self.serial or not self.serial.is_open:
            print(color("Not connected!", Colors.RED))
            return False
        
        try:
            # Add newline if not present
            if not cmd.endswith('\n'):
                cmd += '\n'
            
            self.serial.write(cmd.encode('utf-8'))
            self.serial.flush()
            print(color(f"[Sent] {cmd.strip()}", Colors.GREEN))
            time.sleep(wait_response)
            return True
        except serial.SerialException as e:
            print(color(f"Send failed: {e}", Colors.RED))
            return False
    
    # =========================================================================
    # High-level commands
    # =========================================================================
    
    def wifi_on(self):
        """Turn WiFi on (connect to known network or start AP)."""
        return self.send_command("wifi on")
    
    def wifi_off(self):
        """Turn WiFi off."""
        return self.send_command("wifi off")
    
    def wifi_add(self, ssid: str, password: str):
        """Add a WiFi network."""
        return self.send_command(f'wifi add "{ssid}" "{password}"')
    
    def webui(self, ap_mode: bool = True):
        """Start WebUI."""
        if ap_mode:
            return self.send_command("webui")
        else:
            return self.send_command("webui -noAp")
    
    def evil_portal(self, ssid: str = "Free WiFi", channel: int = 6, 
                    deauth: bool = False, verify: bool = False):
        """
        Start Evil Portal attack.
        
        Args:
            ssid: The fake AP name to broadcast
            channel: WiFi channel (1-13)
            deauth: Enable deauth attack on nearby networks
            verify: Verify captured passwords against real network
        """
        cmd = f'evilportal "{ssid}" -c {channel}'
        if deauth:
            cmd += " -d"
        if verify:
            cmd += " -v"
        return self.send_command(cmd, wait_response=1.0)
    
    def sniffer(self):
        """Start WiFi sniffer."""
        return self.send_command("sniffer")
    
    def arp_scan(self):
        """Scan for hosts on the network."""
        return self.send_command("arp")
    
    def ir_send(self, protocol: str, address: str, command: str):
        """Send an IR command."""
        return self.send_command(f'irsend -p {protocol} -a {address} -c {command}')
    
    def rf_send(self, frequency: int, data: str):
        """Send RF signal."""
        return self.send_command(f'rfsend -f {frequency} -d {data}')
    
    def set_brightness(self, level: int):
        """Set screen brightness (0-100)."""
        return self.send_command(f'brightness {level}')
    
    def reboot(self):
        """Reboot the device."""
        return self.send_command("reboot")
    
    def get_info(self):
        """Get device info."""
        return self.send_command("info")


def list_ports() -> list:
    """List available serial ports."""
    ports = serial.tools.list_ports.comports()
    return [(p.device, p.description) for p in ports]


def print_banner():
    """Print the banner."""
    banner = """
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║    ██████╗ ██████╗ ██╗   ██╗ ██████╗███████╗                 ║
║    ██╔══██╗██╔══██╗██║   ██║██╔════╝██╔════╝                 ║
║    ██████╔╝██████╔╝██║   ██║██║     █████╗                   ║
║    ██╔══██╗██╔══██╗██║   ██║██║     ██╔══╝                   ║
║    ██████╔╝██║  ██║╚██████╔╝╚██████╗███████╗                 ║
║    ╚═════╝ ╚═╝  ╚═╝ ╚═════╝  ╚═════╝╚══════╝                 ║
║                                                              ║
║              Serial Controller v1.0                          ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
"""
    print(color(banner, Colors.CYAN))


def print_help():
    """Print available commands."""
    help_text = """
╔══════════════════════════════════════════════════════════════╗
║                     Available Commands                        ║
╠══════════════════════════════════════════════════════════════╣
║  WiFi Commands:                                               ║
║    wifi on              - Connect to WiFi / Start AP          ║
║    wifi off             - Disconnect WiFi                     ║
║    wifi add <ssid> <pw> - Add WiFi credentials                ║
║    webui                - Start WebUI in AP mode              ║
║    webui sta            - Start WebUI in station mode         ║
║                                                               ║
║  Attack Commands:                                             ║
║    evilportal [ssid]    - Start Evil Portal                   ║
║    evilportal [ssid] -d - Evil Portal with deauth             ║
║    sniffer              - Start WiFi sniffer                  ║
║    arp                  - Scan network for hosts              ║
║                                                               ║
║  IR Commands:                                                 ║
║    irsend <proto> <addr> <cmd> - Send IR command              ║
║                                                               ║
║  RF Commands:                                                 ║
║    rfsend <freq> <data> - Send RF signal                      ║
║                                                               ║
║  System Commands:                                             ║
║    info                 - Get device info                     ║
║    reboot               - Reboot device                       ║
║    brightness <0-100>   - Set brightness                      ║
║                                                               ║
║  Controller Commands:                                         ║
║    help                 - Show this help                      ║
║    exit / quit          - Exit controller                     ║
║    raw <command>        - Send raw serial command             ║
╚══════════════════════════════════════════════════════════════╝
"""
    print(color(help_text, Colors.YELLOW))


def interactive_mode(controller: BruceController):
    """Run interactive command mode."""
    print_help()
    print(color("\nType 'help' for commands, 'exit' to quit.\n", Colors.GREEN))
    
    while True:
        try:
            cmd = input(color("bruce> ", Colors.BOLD)).strip()
            
            if not cmd:
                continue
            
            # Parse command
            parts = cmd.split(maxsplit=1)
            command = parts[0].lower()
            args = parts[1] if len(parts) > 1 else ""
            
            if command in ('exit', 'quit', 'q'):
                break
            elif command == 'help':
                print_help()
            elif command == 'raw':
                controller.send_command(args)
            elif command == 'wifi':
                if args.startswith('on'):
                    controller.wifi_on()
                elif args.startswith('off'):
                    controller.wifi_off()
                elif args.startswith('add'):
                    # Parse: wifi add ssid password
                    add_parts = args.split()[1:]
                    if len(add_parts) >= 2:
                        controller.wifi_add(add_parts[0], add_parts[1])
                    else:
                        print(color("Usage: wifi add <ssid> <password>", Colors.RED))
                else:
                    controller.send_command(f"wifi {args}")
            elif command == 'webui':
                if args == 'sta':
                    controller.webui(ap_mode=False)
                else:
                    controller.webui(ap_mode=True)
            elif command == 'evilportal':
                # Parse optional arguments
                ssid = "Free WiFi"
                channel = 6
                deauth = '-d' in args
                verify = '-v' in args
                
                # Extract SSID (everything before flags)
                arg_parts = args.replace('-d', '').replace('-v', '').strip()
                if arg_parts:
                    # Check for channel flag
                    if '-c' in arg_parts:
                        idx = arg_parts.index('-c')
                        ssid_part = arg_parts[:idx].strip()
                        channel_part = arg_parts[idx+2:].strip().split()[0]
                        if ssid_part:
                            ssid = ssid_part.strip('"\'')
                        channel = int(channel_part)
                    else:
                        ssid = arg_parts.strip('"\'')
                
                controller.evil_portal(ssid, channel, deauth, verify)
            elif command == 'sniffer':
                controller.sniffer()
            elif command == 'arp':
                controller.arp_scan()
            elif command == 'info':
                controller.get_info()
            elif command == 'reboot':
                controller.reboot()
            elif command == 'brightness':
                try:
                    level = int(args)
                    controller.set_brightness(level)
                except ValueError:
                    print(color("Usage: brightness <0-100>", Colors.RED))
            elif command == 'irsend':
                controller.send_command(f"irsend {args}")
            elif command == 'rfsend':
                controller.send_command(f"rfsend {args}")
            else:
                # Send as raw command
                controller.send_command(cmd)
                
        except KeyboardInterrupt:
            print("\n")
            break
        except EOFError:
            break


def select_port() -> Optional[str]:
    """Interactive port selection."""
    ports = list_ports()
    
    if not ports:
        print(color("No serial ports found!", Colors.RED))
        return None
    
    print(color("\nAvailable ports:", Colors.YELLOW))
    for i, (port, desc) in enumerate(ports, 1):
        print(f"  {color(str(i), Colors.CYAN)}. {port} - {desc}")
    
    print()
    while True:
        try:
            choice = input(color("Select port (number or name): ", Colors.BOLD)).strip()
            
            if choice.isdigit():
                idx = int(choice) - 1
                if 0 <= idx < len(ports):
                    return ports[idx][0]
            else:
                # Check if it's a valid port name
                for port, _ in ports:
                    if port.upper() == choice.upper():
                        return port
                # Maybe they typed the full path
                if choice.startswith('/dev/') or choice.upper().startswith('COM'):
                    return choice
            
            print(color("Invalid selection. Try again.", Colors.RED))
        except KeyboardInterrupt:
            return None


def main():
    """Main entry point."""
    # Enable ANSI colors on Windows
    if sys.platform == 'win32':
        os.system('')
    
    print_banner()
    
    # Get port from argument or prompt
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = select_port()
        if not port:
            print(color("No port selected. Exiting.", Colors.RED))
            return
    
    print(color(f"\nConnecting to {port}...", Colors.YELLOW))
    
    controller = BruceController(port)
    if not controller.connect():
        return
    
    print(color(f"Connected to Bruce on {port}!", Colors.GREEN))
    print(color("Device output will appear with [Bruce] prefix.\n", Colors.DIM))
    
    try:
        interactive_mode(controller)
    finally:
        controller.disconnect()
    
    print(color("\nGoodbye! 👋", Colors.CYAN))


if __name__ == "__main__":
    main()
