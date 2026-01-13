#!/usr/bin/env python3
"""
Bruce Firmware Flasher
A user-friendly CLI tool to build and flash Bruce firmware to supported devices.

Usage:
    python bruce_flasher.py

Requirements:
    - PlatformIO Core (pio) must be installed and in PATH
    - Python 3.7+
"""

import os
import sys
import subprocess
import re
from pathlib import Path
from typing import Optional, Tuple

# ANSI color codes for terminal output
class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    DIM = '\033[2m'
    PURPLE = '\033[35m'

def color_text(text: str, color: str) -> str:
    """Apply color to text if terminal supports it."""
    if sys.platform == 'win32':
        # Enable ANSI colors on Windows
        os.system('')
    return f"{color}{text}{Colors.ENDC}"

# Device definitions with categories for better organization
# Format: (env_name, display_name, category)
DEVICES = [
    # M5Stack Devices
    ("m5stack-cardputer", "M5Stack Cardputer", "M5Stack"),
    ("m5stack-cplus2", "M5StickC Plus2", "M5Stack"),
    ("m5stack-cplus1_1", "M5StickC Plus1.1", "M5Stack"),
    ("LAUNCHER_m5stack-cplus1_1", "M5StickC Plus1.1 (Launcher)", "M5Stack"),
    ("m5stack-core2", "M5Stack Core2", "M5Stack"),
    ("m5stack-core4mb", "M5Stack Core (4MB)", "M5Stack"),
    ("m5stack-core16mb", "M5Stack Core (16MB)", "M5Stack"),
    ("m5stack-cores3", "M5Stack CoreS3", "M5Stack"),

    # Lilygo T-Embed Series
    ("lilygo-t-embed-cc1101", "Lilygo T-Embed CC1101", "Lilygo T-Embed"),
    ("lilygo-t-embed", "Lilygo T-Embed", "Lilygo T-Embed"),

    # Lilygo T-Deck Series
    ("lilygo-t-deck", "Lilygo T-Deck", "Lilygo T-Deck"),
    ("lilygo-t-deck-pro", "Lilygo T-Deck Plus", "Lilygo T-Deck"),

    # Lilygo T-Display Series
    ("lilygo-t-display-s3", "Lilygo T-Display S3", "Lilygo T-Display"),
    ("lilygo-t-display-s3-touch", "Lilygo T-Display S3 Touch", "Lilygo T-Display"),
    ("lilygo-t-display-s3-mmc", "Lilygo T-Display S3 MMC", "Lilygo T-Display"),
    ("lilygo-t-display-s3-touch-mmc", "Lilygo T-Display S3 Touch MMC", "Lilygo T-Display"),
    ("lilygo-t-display-S3-pro", "Lilygo T-Display S3 Pro", "Lilygo T-Display"),
    ("lilygo-t-display-ttgo", "Lilygo T-Display TTGO", "Lilygo T-Display"),

    # Other Lilygo Devices
    ("lilygo-t-watch-s3", "Lilygo T-Watch S3", "Lilygo Other"),
    ("lilygo-t-hmi", "Lilygo T-HMI", "Lilygo Other"),
    ("lilygo-t-lora-pager", "Lilygo T-Lora Pager", "Lilygo Other"),

    # CYD (Cheap Yellow Display) Series
    ("CYD-2432S028", "CYD-2432S028", "CYD"),
    ("CYD-2USB", "CYD-2USB", "CYD"),
    ("CYD-2432W328C", "CYD-2432W328C", "CYD"),
    ("CYD-2432W328C_2", "CYD-2432W328C V2 / S024 Capacitive", "CYD"),
    ("CYD-2432W328R-or-S024R", "CYD-2432W328R / S024R", "CYD"),
    ("LAUNCHER_CYD-2432S028", "CYD-2432S028 (Launcher)", "CYD"),
    ("LAUNCHER_CYD-2USB", "CYD-2USB (Launcher)", "CYD"),
    ("LAUNCHER_CYD-2432W328C", "CYD-2432W328C (Launcher)", "CYD"),
    ("LAUNCHER_CYD-2432W328R-or-S024R", "CYD-2432W328R / S024R (Launcher)", "CYD"),

    # Marauder Devices
    ("Marauder-Mini", "Marauder Mini", "Marauder"),
    ("Marauder-v7", "Marauder v7", "Marauder"),
    ("Marauder-V4-V6", "Marauder V4-V6", "Marauder"),
    ("Marauder-v61", "Marauder V6.1", "Marauder"),
    ("Awok-Mini", "Awok Mini", "Marauder"),
    ("Awok-Touch", "Awok Touch", "Marauder"),
    ("WaveSentry-R1", "WaveSentry R1", "Marauder"),
    ("LAUNCHER_Marauder-Mini", "Marauder Mini (Launcher)", "Marauder"),
    ("LAUNCHER_Marauder-v7", "Marauder v7 (Launcher)", "Marauder"),
    ("LAUNCHER_Marauder-V4-V6", "Marauder V4-V6 (Launcher)", "Marauder"),
    ("LAUNCHER_Marauder-v61", "Marauder V6.1 (Launcher)", "Marauder"),
    ("LAUNCHER_WaveSentry-R1", "WaveSentry R1 (Launcher)", "Marauder"),

    # Phantom
    ("Phantom_S024R", "Phantom S024R", "Phantom"),
    ("LAUNCHER_Phantom_S024R", "Phantom S024R (Launcher)", "Phantom"),

    # Generic ESP32 Boards
    ("esp32-s3-devkitc-1", "ESP32-S3 DevKitC-1", "ESP32 Generic"),
    ("esp32-s3-devkitc-1-psram", "ESP32-S3 DevKitC-1 (PSRAM)", "ESP32 Generic"),
    ("esp32-c5", "ESP32-C5", "ESP32 Generic"),
    ("esp32-c5-tft", "ESP32-C5 TFT", "ESP32 Generic"),

    # Other/Community Boards
    ("smoochiee-board", "Smoochiee Board", "Community"),
    ("xk404", "XK404", "Community"),
]

# Category order for display
CATEGORY_ORDER = [
    "M5Stack",
    "Lilygo T-Embed",
    "Lilygo T-Deck",
    "Lilygo T-Display",
    "Lilygo Other",
    "CYD",
    "Marauder",
    "Phantom",
    "ESP32 Generic",
    "Community",
]

# PlatformIO command to use (autodetected)
PIO_CMD = ["pio"]


def clear_screen():
    """Clear the terminal screen."""
    os.system('cls' if os.name == 'nt' else 'clear')


def print_banner():
    """Print the Bruce Flasher banner."""
    banner = """
╔══════════════════════════════════════════════════════════════════════════════╗
║                                                                              ║
║    ██████╗ ██████╗ ██╗   ██╗ ██████╗███████╗                                 ║
║    ██╔══██╗██╔══██╗██║   ██║██╔════╝██╔════╝                                 ║
║    ██████╔╝██████╔╝██║   ██║██║     █████╗                                   ║
║    ██╔══██╗██╔══██╗██║   ██║██║     ██╔══╝                                   ║
║    ██████╔╝██║  ██║╚██████╔╝╚██████╗███████╗                                 ║
║    ╚═════╝ ╚═╝  ╚═╝ ╚═════╝  ╚═════╝╚══════╝                                 ║
║                                                                              ║
║                     F I R M W A R E   F L A S H E R                          ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
"""
    print(color_text(banner, Colors.PURPLE))


def print_separator(char: str = "─", length: int = 80):
    """Print a separator line."""
    print(color_text(char * length, Colors.DIM))


def check_platformio() -> bool:
    """Check if PlatformIO is installed and accessible."""
    global PIO_CMD

    # Try default 'pio' command
    try:
        result = subprocess.run(
            ["pio", "--version"],
            capture_output=True,
            text=True,
            timeout=10
        )
        if result.returncode == 0:
            PIO_CMD = ["pio"]
            return True
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass

    # Try 'python -m platformio' (for cases where pio not in PATH)
    try:
        result = subprocess.run(
            [sys.executable, "-m", "platformio", "--version"],
            capture_output=True,
            text=True,
            timeout=10
        )
        if result.returncode == 0:
            PIO_CMD = [sys.executable, "-m", "platformio"]
            return True
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass

    return False


def get_connected_ports() -> list:
    """Get list of connected serial ports."""
    try:
        cmd = PIO_CMD + ["device", "list", "--serial"]
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30
        )
        if result.returncode == 0:
            # Parse the output to extract port information
            ports = []
            current_port = None
            for line in result.stdout.split('\n'):
                if line.startswith('/') or (os.name == 'nt' and line.startswith('COM')):
                    current_port = line.strip()
                elif 'Description:' in line and current_port:
                    desc = line.split('Description:')[1].strip()
                    ports.append((current_port, desc))
                    current_port = None
            return ports
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass
    return []


def display_devices_by_category() -> list:
    """Display devices organized by category and return flattened list."""
    devices_by_category = {}
    for env_name, display_name, category in DEVICES:
        if category not in devices_by_category:
            devices_by_category[category] = []
        devices_by_category[category].append((env_name, display_name))

    flattened_list = []
    index = 1

    for category in CATEGORY_ORDER:
        if category in devices_by_category:
            print(f"\n  {color_text(f'▶ {category}', Colors.YELLOW + Colors.BOLD)}")
            print_separator("─", 60)

            for env_name, display_name in devices_by_category[category]:
                # Mark launcher versions with special indicator
                if "LAUNCHER" in env_name:
                    indicator = color_text("⚡", Colors.YELLOW)
                    name_display = color_text(display_name, Colors.DIM)
                else:
                    indicator = color_text("●", Colors.GREEN)
                    name_display = display_name

                print(f"    {color_text(f'{index:3}', Colors.CYAN)}. {indicator} {name_display}")
                flattened_list.append((env_name, display_name))
                index += 1

    return flattened_list


def get_user_choice(max_choice: int) -> Optional[int]:
    """Get user's device choice with input validation."""
    print()
    print_separator()
    print(f"\n  {color_text('⚡ Launcher versions', Colors.YELLOW)} are optimized for M5Launcher compatibility")
    print()

    while True:
        try:
            user_input = input(
                f"  {color_text('→', Colors.GREEN)} Enter device number "
                f"({color_text('1', Colors.CYAN)}-{color_text(str(max_choice), Colors.CYAN)}) "
                f"or {color_text('q', Colors.RED)} to quit: "
            ).strip().lower()

            if user_input in ('q', 'quit', 'exit'):
                return None

            choice = int(user_input)
            if 1 <= choice <= max_choice:
                return choice
            else:
                print(color_text(f"    ⚠ Please enter a number between 1 and {max_choice}", Colors.YELLOW))
        except ValueError:
            print(color_text("    ⚠ Invalid input. Please enter a number.", Colors.YELLOW))


def select_port(ports: list) -> Optional[str]:
    """Let user select a serial port or use auto-detection."""
    if not ports:
        print(color_text("\n  ⚠ No serial ports detected!", Colors.YELLOW))
        print("    Make sure your device is connected and drivers are installed.")
        user_input = input(
            f"\n  {color_text('→', Colors.GREEN)} Press Enter to let PlatformIO auto-detect "
            f"or {color_text('q', Colors.RED)} to quit: "
        ).strip().lower()
        return "" if user_input not in ('q', 'quit', 'exit') else None

    print(f"\n  {color_text('Connected Devices:', Colors.BOLD)}")
    print_separator("─", 60)

    for i, (port, desc) in enumerate(ports, 1):
        print(f"    {color_text(f'{i}', Colors.CYAN)}. {color_text(port, Colors.GREEN)} - {desc}")

    print(f"    {color_text('0', Colors.CYAN)}. Auto-detect (let PlatformIO choose)")
    print()

    while True:
        try:
            user_input = input(
                f"  {color_text('→', Colors.GREEN)} Select port "
                f"({color_text('0', Colors.CYAN)}-{color_text(str(len(ports)), Colors.CYAN)}) "
                f"or {color_text('q', Colors.RED)} to quit: "
            ).strip().lower()

            if user_input in ('q', 'quit', 'exit'):
                return None

            choice = int(user_input)
            if choice == 0:
                return ""  # Auto-detect
            elif 1 <= choice <= len(ports):
                return ports[choice - 1][0]
            else:
                print(color_text(f"    ⚠ Please enter a number between 0 and {len(ports)}", Colors.YELLOW))
        except ValueError:
            print(color_text("    ⚠ Invalid input. Please enter a number.", Colors.YELLOW))


def select_action() -> Optional[str]:
    """Let user choose between build, upload, or build+upload."""
    print(f"\n  {color_text('Select Action:', Colors.BOLD)}")
    print_separator("─", 60)
    print(f"    {color_text('1', Colors.CYAN)}. {color_text('Build & Flash', Colors.GREEN)} - Build firmware and upload to device")
    print(f"    {color_text('2', Colors.CYAN)}. {color_text('Build Only', Colors.YELLOW)} - Just compile the firmware")
    print(f"    {color_text('3', Colors.CYAN)}. {color_text('Flash Only', Colors.BLUE)} - Upload existing firmware (skip build)")
    print()

    while True:
        user_input = input(
            f"  {color_text('→', Colors.GREEN)} Select action "
            f"({color_text('1', Colors.CYAN)}-{color_text('3', Colors.CYAN)}) "
            f"or {color_text('q', Colors.RED)} to quit: "
        ).strip().lower()

        if user_input in ('q', 'quit', 'exit'):
            return None

        if user_input == '1':
            return 'build_upload'
        elif user_input == '2':
            return 'build'
        elif user_input == '3':
            return 'upload'
        else:
            print(color_text("    ⚠ Please enter 1, 2, or 3.", Colors.YELLOW))


def run_platformio(env_name: str, action: str, port: str = "") -> Tuple[bool, str]:
    """Run PlatformIO with the specified environment and action."""
    # Change to firmware directory
    script_dir = Path(__file__).parent.resolve()
    user_profile = os.environ.get("USERPROFILE") or os.environ.get("HOME")

    # 1. Environment Setup (Modify PATH)
    # Windows: .platformio\penv\Scripts
    # Linux/Termux/Mac: .platformio/penv/bin
    if user_profile:
        # Check for Windows Scripts folder
        penv_scripts_win = os.path.join(user_profile, ".platformio", "penv", "Scripts")
        if os.path.exists(penv_scripts_win) and penv_scripts_win not in os.environ["PATH"]:
            os.environ["PATH"] += os.pathsep + penv_scripts_win

        # Check for Unix/Termux bin folder
        penv_bin_unix = os.path.join(user_profile, ".platformio", "penv", "bin")
        if os.path.exists(penv_bin_unix) and penv_bin_unix not in os.environ["PATH"]:
            os.environ["PATH"] += os.pathsep + penv_bin_unix

    # 2. Cleanup (Remove-Item...)
    # Only do this heavy cleanup if we are building/uploading to ensure clean state as requested
    if action in ('build_upload', 'build'):
        print(f"\n  {color_text('Cleanup (Standard Procedure)', Colors.BOLD)}")
        print_separator("─", 60)

        if user_profile:
            packages_dir = Path(user_profile) / ".platformio" / "packages"
            # Find dirs matching framework-arduinoespressif32*
            targets = list(packages_dir.glob("framework-arduinoespressif32*"))

            if targets:
                print(f"  {color_text('Performing deep clean...', Colors.YELLOW)}")
                print(f"  {color_text('This ensures the flashing works, but takes time.', Colors.YELLOW)}")
                print(f"  {color_text('Take a shower 🚿, this might take a while!', Colors.CYAN)}")

                for target in targets:
                    try:
                        print(f"  Removing: {target.name}...")
                        def remove_readonly(func, path, excinfo):
                            os.chmod(path, stat.S_IWRITE)
                            func(path)
                        shutil.rmtree(target, onerror=remove_readonly)
                    except Exception as e:
                        print(f"  {color_text(f'Warning cleaning {target.name}: {e}', Colors.DIM)}")
            else:
                print(f"  {color_text('Clean.', Colors.DIM)}")

    # 3. Execution with Max Speed
    # Use python -m platformio directly for reliability
    cmd = [sys.executable, "-m", "platformio", "run", "-e", env_name]

    # Add parallel build to MAXIMIZE SPEED
    cpu_count = os.cpu_count() or 4
    if action in ('build_upload', 'build'):
        cmd.extend(["-j", str(cpu_count)])

    if action in ('build_upload', 'upload'):
        cmd.extend(["-t", "upload"])
        if port:
            cmd.extend(["--upload-port", port])

    if action == 'upload':
        cmd.extend(["-t", "nobuild"])

    print(f"\n  {color_text('Running:', Colors.BOLD)} {' '.join(cmd)}")
    print_separator()
    print()

    try:
        # Run the command with bufsize=0 to get unbuffered output
        # Use simple Popen to avoid encoding issues with text=True on some systems
        process = subprocess.Popen(
            cmd,
            cwd=str(script_dir),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )

        output_lines = []

        # Read byte-by-byte or line-by-line to avoid buffering delays
        while True:
            line = process.stdout.readline()
            if not line and process.poll() is not None:
                break
            if line:
                # Decode safely
                decoded_line = line.decode('utf-8', errors='replace')
                print(decoded_line, end='')
                output_lines.append(decoded_line)

        process.wait()

        return process.returncode == 0, ''.join(output_lines)

    except KeyboardInterrupt:
        print(color_text("\n\n  ⚠ Operation cancelled by user.", Colors.YELLOW))
        if 'process' in locals():
            process.kill()
        return False, "Cancelled by user"
    except Exception as e:
        return False, str(e)


def main():
    """Main entry point for Bruce Flasher."""
    clear_screen()
    print_banner()

    # Check PlatformIO
    print(f"  {color_text('Checking requirements...', Colors.DIM)}")
    if not check_platformio():
        print(color_text("\n  ✗ PlatformIO not found!", Colors.RED))
        print("    Please install PlatformIO Core: https://docs.platformio.org/en/latest/core/installation.html")
        print("    Or install it via pip: pip install platformio")
        sys.exit(1)
    print(f"  {color_text('✓ PlatformIO found', Colors.GREEN)}")

    # Check we're in the right directory
    script_dir = Path(__file__).parent.resolve()
    platformio_ini = script_dir / "platformio.ini"
    if not platformio_ini.exists():
        print(color_text(f"\n  ✗ platformio.ini not found in {script_dir}", Colors.RED))
        print("    Please run this script from the firmware directory.")
        sys.exit(1)
    print(f"  {color_text('✓ Found platformio.ini', Colors.GREEN)}")

    # Display devices
    print(f"\n  {color_text('Select Your Device:', Colors.BOLD + Colors.UNDERLINE)}")
    device_list = display_devices_by_category()

    # Get device choice
    choice = get_user_choice(len(device_list))
    if choice is None:
        print(color_text("\n  Goodbye! 👋\n", Colors.CYAN))
        sys.exit(0)

    selected_env, selected_name = device_list[choice - 1]

    clear_screen()
    print_banner()
    print(f"  {color_text('Selected Device:', Colors.BOLD)} {color_text(selected_name, Colors.GREEN)}")
    print(f"  {color_text('Environment:', Colors.DIM)} {selected_env}")

    # Select action
    action = select_action()
    if action is None:
        print(color_text("\n  Goodbye! 👋\n", Colors.CYAN))
        sys.exit(0)

    # Select port if uploading
    port = ""
    if action in ('build_upload', 'upload'):
        print(f"\n  {color_text('Scanning for connected devices...', Colors.DIM)}")
        ports = get_connected_ports()
        port = select_port(ports)
        if port is None:
            print(color_text("\n  Goodbye! 👋\n", Colors.CYAN))
            sys.exit(0)

    # Confirm
    clear_screen()
    print_banner()

    action_text = {
        'build_upload': 'Build & Flash',
        'build': 'Build Only',
        'upload': 'Flash Only'
    }[action]

    print(f"\n  {color_text('╔══ Summary ══════════════════════════════════════════════════╗', Colors.CYAN)}")
    print(f"  {color_text('║', Colors.CYAN)} Device:     {color_text(selected_name, Colors.GREEN):<48} {color_text('║', Colors.CYAN)}")
    print(f"  {color_text('║', Colors.CYAN)} Environment: {selected_env:<47} {color_text('║', Colors.CYAN)}")
    print(f"  {color_text('║', Colors.CYAN)} Action:      {color_text(action_text, Colors.YELLOW):<47} {color_text('║', Colors.CYAN)}")
    if action in ('build_upload', 'upload'):
        port_display = port if port else "Auto-detect"
        print(f"  {color_text('║', Colors.CYAN)} Port:        {port_display:<47} {color_text('║', Colors.CYAN)}")
    print(f"  {color_text('╚══════════════════════════════════════════════════════════════╝', Colors.CYAN)}")

    print()
    confirm = input(
        f"  {color_text('→', Colors.GREEN)} Press {color_text('Enter', Colors.CYAN)} to continue "
        f"or {color_text('q', Colors.RED)} to quit: "
    ).strip().lower()

    if confirm in ('q', 'quit', 'exit'):
        print(color_text("\n  Goodbye! 👋\n", Colors.CYAN))
        sys.exit(0)

    # Run PlatformIO
    print()
    print_separator("═", 80)
    print(color_text(f"\n  🦈 Starting {action_text}...\n", Colors.GREEN + Colors.BOLD))
    print_separator("═", 80)

    success, output = run_platformio(selected_env, action, port)

    print()
    print_separator("═", 80)

    if success:
        print(color_text("\n  ✓ SUCCESS!", Colors.GREEN + Colors.BOLD))
        if action in ('build_upload', 'upload'):
            print(color_text("    Your device should now be running Bruce firmware!", Colors.GREEN))
            print(color_text("    If the device doesn't respond, try pressing the reset button.", Colors.DIM))
        elif action == 'build':
            # Find the output binary
            # Updated to check build_output folder
            bin_path = script_dir / "build_output" / selected_env / f"Bruce-{selected_env}.bin"
            if not bin_path.exists():
                # Fallback to old location just in case
                bin_path = script_dir / f"Bruce-{selected_env}.bin"

            if bin_path.exists():
                print(f"    Firmware binary: {color_text(str(bin_path), Colors.CYAN)}")
                print(color_text(f"    Location: {bin_path.parent}", Colors.DIM))

            print(color_text("    Use 'Flash Only' option to upload to your device.", Colors.DIM))
    else:
        print(color_text("\n  ✗ FAILED!", Colors.RED + Colors.BOLD))
        print(color_text("    Check the output above for error details.", Colors.RED))
        if action in ('build_upload', 'upload'):
            print(color_text("    Common issues:", Colors.YELLOW))
            print(color_text("    - Device not in bootloader mode (hold BOOT button while connecting)", Colors.DIM))
            print(color_text("    - Wrong COM port selected", Colors.DIM))
            print(color_text("    - Missing USB drivers", Colors.DIM))

    print()
    input(f"  Press {color_text('Enter', Colors.CYAN)} to exit...")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print(color_text("\n\n  Operation cancelled. Goodbye! 👋\n", Colors.CYAN))
        sys.exit(0)
