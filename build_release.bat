@echo off
cd /d "%~dp0"
call pio run -e lilygo-t-embed-cc1101 -t build-firmware -j 2 > build_release_log.txt 2>&1
echo ENV1_EXIT=%ERRORLEVEL% >> build_release_log.txt
call pio run -e m5stack-sticks3 -t build-firmware -j 2 >> build_release_log.txt 2>&1
echo ENV2_EXIT=%ERRORLEVEL% >> build_release_log.txt
call pio run -e m5stack-cardputer -t build-firmware -j 2 >> build_release_log.txt 2>&1
echo ENV3_EXIT=%ERRORLEVEL% >> build_release_log.txt
call pio run -e esp32-c5-tft -t build-firmware -j 2 >> build_release_log.txt 2>&1
echo ENV4_EXIT=%ERRORLEVEL% >> build_release_log.txt
echo EXITCODE=%ERRORLEVEL% >> build_release_log.txt
