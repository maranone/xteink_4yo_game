@echo off
setlocal

set PROJECT_DIR=%~dp0
set PORT=COM3

cd /d "%PROJECT_DIR%"

echo === Preparing dual-resolution icon assets from Pexels override folder ===
echo Primary:  assets\icon_sources_pexels
echo Fallback: assets\icon_sources
python tools\prepare_icons.py --source-dir assets\icon_sources_pexels --fallback-source-dir assets\icon_sources --preview-dir assets\icon_previews_pexels
if errorlevel 1 (
    echo.
    echo Icon preparation failed - see errors above.
    pause
    exit /b 1
)

echo.
echo === Bundling icon assets into firmware ===
py -3.10 tools\bundle_icons.py
if errorlevel 1 (
    echo.
    echo Icon bundling failed - see errors above.
    pause
    exit /b 1
)

echo.
echo === Building firmware ===
py -3.10 -m platformio run
if errorlevel 1 (
    echo.
    echo Build failed - see errors above.
    pause
    exit /b 1
)

echo.
echo === Checking device on %PORT% ===
python -m esptool --chip esp32c3 --port %PORT% chip-id
if errorlevel 1 (
    echo.
    echo Could not reach the device on %PORT%. Check the USB cable/port and try again.
    pause
    exit /b 1
)

echo.
echo === Flashing both OTA slots ===
python -m esptool --chip esp32c3 --port %PORT% --baud 921600 write_flash ^
  --flash_mode dio ^
  --flash_freq 80m ^
  --flash_size 16MB ^
  0x10000 .pio\build\x4\firmware.bin ^
  0x650000 .pio\build\x4\firmware.bin

if errorlevel 1 (
    echo.
    echo Flash failed - see errors above.
    pause
    exit /b 1
)

echo.
echo === Done - Pexels-override firmware flashed successfully ===
pause
