@echo off
setlocal

set PROJECT_DIR=%~dp0
set PORT=COM3
set FIRMWARE=%PROJECT_DIR%firmware.bin

if not exist "%FIRMWARE%" (
    set FIRMWARE=%PROJECT_DIR%firmware\firmware.bin
)

if not exist "%FIRMWARE%" (
    set FIRMWARE=%PROJECT_DIR%.pio\build\x4\firmware.bin
)

cd /d "%PROJECT_DIR%"

if not exist "%FIRMWARE%" (
    echo.
    echo Could not find firmware.bin.
    echo Expected one of:
    echo   %PROJECT_DIR%firmware.bin
    echo   %PROJECT_DIR%firmware\firmware.bin
    echo   %PROJECT_DIR%.pio\build\x4\firmware.bin
    pause
    exit /b 1
)

echo === Using firmware ===
echo %FIRMWARE%

echo.
echo === Checking device on %PORT% ===
python -m esptool --chip esp32c3 --port %PORT% chip-id
if errorlevel 1 (
    echo.
    echo Could not reach the device on %PORT%. Check the USB cable/port and try again.
    echo If your device is on a different COM port, edit PORT near the top of this file.
    pause
    exit /b 1
)

echo.
echo === Flashing both OTA slots ===
python -m esptool --chip esp32c3 --port %PORT% --baud 921600 write_flash ^
  --flash_mode dio ^
  --flash_freq 80m ^
  --flash_size 16MB ^
  0x10000 "%FIRMWARE%" ^
  0x650000 "%FIRMWARE%"

if errorlevel 1 (
    echo.
    echo Flash failed - see errors above.
    pause
    exit /b 1
)

echo.
echo === Done - firmware flashed successfully ===
pause
