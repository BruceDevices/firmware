#!/usr/bin/env python3
# ---@IncursioHack---
import os
import subprocess
import shutil
from pathlib import Path
from http.server import HTTPServer, SimpleHTTPRequestHandler
from datetime import datetime

# --- 1. Check if PlatformIO CLI an esptool is installed ---
if shutil.which("pio") is None:
    print("PlatformIO not found. Installing via pip --user...")
    subprocess.run(["python3", "-m", "pip", "install", "--user", "platformio"], check=True)
    os.environ["PATH"] = f"{os.environ['HOME']}/.local/bin:{os.environ['PATH']}"

if shutil.which("esptool") is None:
    print("Esptool not found. Installing via pip --user...")
    subprocess.run(["python3", "-m", "pip", "install", "--user", "esptool"], check=True)
    os.environ["PATH"] = f"{os.environ['HOME']}/.local/bin:{os.environ['PATH']}"

# --- 2. Read default_envs from platformio.ini as raw text ---
ini_file = Path("platformio.ini")
if not ini_file.exists():
    print("platformio.ini not found")
    exit(1)

envs = []
in_platformio_section = False
in_default_envs = False

with open(ini_file) as f:
    for line in f:
        line_strip = line.strip()
        if line_strip.startswith("[platformio]"):
            in_platformio_section = True
            in_default_envs = False
            continue
        elif line_strip.startswith("[") and line_strip != "[platformio]":
            in_platformio_section = False
            in_default_envs = False
        if in_platformio_section:
            if line_strip.startswith("default_envs"):
                in_default_envs = True
                parts = line_strip.split("=", 1)
                if len(parts) > 1:
                    val = parts[1].strip()
                    if val:
                        envs.append(val)
            elif in_default_envs:
                # Stop if a line looks like a config directive instead of a board
                if "=" in line_strip:
                    break
                if line_strip and not line_strip.lower().startswith(";uncomment"):
                    envs.append(line_strip)

# --- 3. Clean env names and remove leading ';' ---
clean_envs = []
for e in envs:
    e_clean = e.lstrip("; ").strip()
    # Skip lines that are not board names (simple heuristic)
    if e_clean and not e_clean.lower().startswith("uncomment") and not e_clean.lower().startswith("boards") and not e_clean.lower().startswith("build_cache_dir") and not e_clean.lower().startswith("cache_dir"):
        clean_envs.append(e_clean)

if not clean_envs:
    print("No environments (boards) found in platformio.ini")
    exit(1)

# --- 4. Display menu for environment selection ---
print("Select the board/environment for build:")
for i, e in enumerate(clean_envs):
    print(f"{i}) {e}")

try:
    choice = int(input("Number: "))
    env = clean_envs[choice]
except (ValueError, IndexError):
    print("Invalid choice")
    exit(1)

print(f"Building for {env}...")

# --- 5. Run PlatformIO build ---
res = subprocess.run(["pio", "run", "-e", env])
if res.returncode != 0:
    print("Build failed")
    exit(1)

# --- 6. Copy firmware.bin to temporary folder ---
bin_path = Path(f".pio/build/{env}/firmware.bin")
if not bin_path.exists():
    print(f"Error: {bin_path} not found")
    exit(1)

merged_bin_path = Path(f"Bruce-{env}.bin")
if not merged_bin_path.exists():
    print(f"Error: {merged_bin_path} not found")
    exit(1)

tmp_dir = Path("/tmp/fwtest")
tmp_dir.mkdir(exist_ok=True)
dest = tmp_dir / "firmware.bin"
shutil.copy(bin_path, dest)
dest = Path(f"/tmp/fwtest/Bruce-{env}.bin")
shutil.copy(merged_bin_path, dest)

mod_time = datetime.fromtimestamp(dest.stat().st_mtime)
print(f"Firmware ready: {dest} (last modified: {mod_time})")

# --- 7. Start HTTP server with download support and directory listing with date/time ---
os.chdir(tmp_dir)
print("HTTP server started at /tmp/fwtest:8000")
print("Open the PORTS tab in Codespaces and set port 8000 to Public")

class BinDownloadHandler(SimpleHTTPRequestHandler):
    """HTTP handler that forces .bin download and shows last-modified date in directory listing."""

    _RANGE_BUFFER_SIZE = 64 * 1024

    def _parse_range_header(self, range_header: str, file_size: int) -> tuple[int, int]:
        if not range_header.startswith("bytes="):
            raise ValueError

        range_spec = range_header.split("=", 1)[1].split(",", 1)[0].strip()
        if "-" not in range_spec:
            raise ValueError

        start_str, end_str = range_spec.split("-", 1)
        if not start_str:
            try:
                suffix_length = int(end_str)
            except ValueError:
                raise ValueError from None
            if suffix_length <= 0:
                raise ValueError
            if suffix_length >= file_size:
                return 0, file_size - 1
            return file_size - suffix_length, file_size - 1

        try:
            start = int(start_str)
        except ValueError:
            raise ValueError from None
        if start >= file_size or start < 0:
            raise ValueError

        if end_str:
            try:
                end = int(end_str)
            except ValueError:
                raise ValueError from None
            if end < start:
                raise ValueError
            end = min(end, file_size - 1)
        else:
            end = file_size - 1

        return start, end

    def send_head(self):
        if not self.path.endswith(".bin"):
            return super().send_head()

        file_path = Path(self.translate_path(self.path))
        if not file_path.exists() or not file_path.is_file():
            self.send_error(404, "File not found")
            return None

        stat_result = file_path.stat()
        file_size = stat_result.st_size
        range_header = self.headers.get("Range")
        status_code = 200
        range_tuple = None
        self._range = None

        if range_header:
            range_header = range_header.strip()
            try:
                range_tuple = self._parse_range_header(range_header, file_size)
            except ValueError:
                self.send_response(416, "Requested Range Not Satisfiable")
                self.send_header("Content-Range", f"bytes */{file_size}")
                self.send_header("Accept-Ranges", "bytes")
                self.end_headers()
                return None
            status_code = 206

        if range_tuple:
            range_start, range_end = range_tuple
        else:
            range_start, range_end = 0, file_size - 1

        self.send_response(status_code)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Disposition", f"attachment; filename={os.path.basename(file_path)}")
        self.send_header("Last-Modified", self.date_time_string(stat_result.st_mtime))
        self.send_header("Accept-Ranges", "bytes")
        if status_code == 206:
            self.send_header("Content-Range", f"bytes {range_start}-{range_end}/{file_size}")
        content_length = range_end - range_start + 1
        self.send_header("Content-Length", str(content_length))
        self.end_headers()

        file_obj = file_path.open("rb")
        file_obj.seek(range_start)
        if status_code == 206:
            self._range = (range_start, range_end)
        return file_obj

    def copyfile(self, source, outputfile):
        range_info = getattr(self, "_range", None)
        if not range_info:
            return super().copyfile(source, outputfile)

        start, end = range_info
        remaining = end - start + 1
        try:
            while remaining > 0:
                chunk = source.read(min(self._RANGE_BUFFER_SIZE, remaining))
                if not chunk:
                    break
                outputfile.write(chunk)
                remaining -= len(chunk)
        finally:
            self._range = None
        return None

    def list_directory(self, path):
        """Override directory listing to include last-modified date."""
        try:
            entries = sorted(os.listdir(path))
        except OSError:
            self.send_error(404, "No permission to list directory")
            return None

        r = []
        displaypath = os.path.relpath(path, Path.cwd())
        r.append(f"<html><head><title>Directory listing for {displaypath}</title></head>")
        r.append("<body>")
        r.append(f"<h2>Directory listing for {displaypath}</h2>")
        r.append("<hr><ul>")
        for name in entries:
            fullname = os.path.join(path, name)
            display_name = name
            link_name = name
            if os.path.isdir(fullname):
                display_name += "/"
                link_name += "/"
            mod_time = datetime.fromtimestamp(os.stat(fullname).st_mtime).strftime("%Y-%m-%d %H:%M:%S")
            r.append(f'<li><a href="{link_name}">{display_name}</a> (last modified: {mod_time})</li>')
        r.append("</ul><hr></body></html>")
        encoded = "\n".join(r).encode("utf-8", "surrogateescape")
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(encoded)))
        self.end_headers()
        return self.wfile.write(encoded)

try:
    HTTPServer(("0.0.0.0", 8000), BinDownloadHandler).serve_forever()
except KeyboardInterrupt:
    print("\nHTTP server stopped")

## curl -vL --range 32768-33536 -o parcial_file.bin https://super-spork-57j65pp59wp3744w-8000.app.github.dev/firmware.bin
