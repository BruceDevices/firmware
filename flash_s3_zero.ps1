param(
    [string]$BinFile = "Bruce-esp32-s3-zero-4mb.bin",
    [string]$Port = "COM3"
)

if (-not (Test-Path $BinFile)) {
    Write-Error "Binary '$BinFile' not found. Usage: .\flash_s3_zero.ps1 -BinFile Bruce-esp32-s3-zero-4mb.bin -Port COMx"
    exit 1
}

Write-Host "=== ESP32-S3-Zero 4MB Bruce Flasher ==="
Write-Host "1. Connect ESP32-S3-Zero via a data-capable USB-C cable."
Write-Host "2. Hold BOOT, tap RST, then release BOOT to enter ROM download mode."
Read-Host "Press Enter when ready to flash $BinFile to $Port"

esptool.py --chip esp32s3 --port $Port --baud 460800 `
  write_flash -z --flash_mode qio --flash_size 4MB `
  0x0 $BinFile

Write-Host ""
Write-Host "Flash complete! Tap RST once on the board to boot Bruce."
