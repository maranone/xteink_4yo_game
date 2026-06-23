# Install Xteink Puzzle Game

These instructions target the Xteink X4 (`ESP32-C3`, 16 MB flash) on Windows PowerShell. Identical workflow to the sibling `xteink-idle-engine` project (same hardware, same toolchain) - see that repo's `INSTALL.md` for more detail if anything here is unclear.

## Requirements

- A USB-C data cable.
- Python 3.10 for PlatformIO builds (the ESP32 platform doesn't support Python 3.14).
- Python with `esptool` installed for flashing.
- The `community-sdk` checkout next to this repository:

  ```text
  C:\Users\waz-tower\Documents\xteink\community-sdk
  C:\Users\waz-tower\Documents\xteink\xteink-puzzle-game
  ```

Install the tools if needed:

```powershell
py -3.10 -m pip install --user platformio
python -m pip install --user esptool
```

## Build

```powershell
cd C:\Users\waz-tower\Documents\xteink\xteink-puzzle-game
py -3.10 -m platformio run
```

Output: `.pio\build\x4\firmware.bin`

## Find The Serial Port

```powershell
[System.IO.Ports.SerialPort]::GetPortNames()
python -m esptool --chip esp32c3 --port COM3 chip-id
```

Substitute the detected port if it isn't `COM3`.

## Flash

This firmware **replaces** whatever firmware (e.g. `xteink-idle-engine`) is currently on the device - one physical chip, one firmware at a time.

```powershell
cd C:\Users\waz-tower\Documents\xteink\xteink-puzzle-game
python -m esptool --chip esp32c3 --port COM3 --baud 921600 write_flash `
  --flash_mode dio `
  --flash_freq 80m `
  --flash_size 16MB `
  0x10000 .pio\build\x4\firmware.bin `
  0x650000 .pio\build\x4\firmware.bin
```

Writes the same firmware to both OTA slots (`app0` at `0x10000`, `app1` at `0x650000`) so the device boots the new build regardless of which OTA slot is active.

## Important Recovery Requirement

Same as `xteink-idle-engine`: this assumes the device already has the 16 MB bootloader + CrossPoint-style partition table. Do not use `--flash_size 4MB`. Do not overwrite the bootloader/partition table during a routine app update.

## Game Content And SD Card

Firmware and theme content are separate:

- Firmware is flashed to the ESP32-C3 (this is the actual game engine/code).
- Theme JSON lives on the SD card under `/puzzles/<theme-id>/theme.json`.
- The firmware also carries an embedded copy of the current theme content and self-syncs it onto the SD card on boot if the on-card copy is missing or out of date (same self-sync pattern as `xteink-idle-engine`'s flagship game) - so a plain reflash refreshes theme content too, without needing a separate content-upload step.
- Save progress lives at `/puzzles/.state.json` and is never touched by the content sync.

Example SD layout:

```text
/puzzles/animals/theme.json
/puzzles/.state.json
```
