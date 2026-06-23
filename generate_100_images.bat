@echo off
setlocal

set PROJECT_DIR=%~dp0
cd /d "%PROJECT_DIR%"

echo === Generate word list with Ollama, images with local SDXL, and firmware assets ===
echo This is resumable. Re-run this file after an interruption to continue the active batch.
echo.

python -u tools\generate_word_collection.py %*
if errorlevel 1 (
    echo.
    echo Collection generation failed - see errors above.
    pause
    exit /b 1
)

echo.
echo === Done ===
echo The generated firmware is .pio\build\x4\firmware.bin
echo Run build_and_flash.bat when you are ready to flash the device.
pause
