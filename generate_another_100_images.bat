@echo off
setlocal

set PROJECT_DIR=%~dp0
cd /d "%PROJECT_DIR%"

echo === Generate another 100 child-friendly words with no maximum word length ===
echo This is resumable. Re-run this file after an interruption to continue the active batch.
echo.

python -u tools\generate_word_collection.py --new-batch --count 100 --no-word-length-cap %*
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
