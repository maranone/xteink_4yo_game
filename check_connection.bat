@echo off
setlocal

set PORT=COM3

echo === Serial ports currently visible to Windows ===
powershell -NoProfile -Command "[System.IO.Ports.SerialPort]::GetPortNames()"

echo.
echo === Trying esptool on %PORT% ===
python -m esptool --chip esp32c3 --port %PORT% chip-id
if errorlevel 1 (
    echo.
    echo Could not reach the device. Try reseating the USB cable, a different cable, or a different port, then run this again.
) else (
    echo.
    echo Device responded - connection is good.
)

pause
