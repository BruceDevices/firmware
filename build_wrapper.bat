@echo off
cd /d "%~dp0"
call pio run -e lilygo-t-embed-cc1101 -t build-firmware -j 2 > build_log.txt 2>&1
echo EXITCODE=%ERRORLEVEL% >> build_log.txt
