# Xteink Puzzle Game export

This folder is a compact export for sharing/backing up the game source code
plus a ready-to-flash firmware binary.

## Ready firmware

`firmware/firmware.bin` is the compiled game firmware. The icon/image assets
are embedded into this binary; they do not need to be copied to the SD card.

## Source included

- `src/` - firmware/game source
- `tools/` - icon preparation, bundling, and generation helpers
- `platformio.ini`, `partitions.csv`, and helper `.bat` files

Image source material, build outputs, `.pio`, `.git`, previews, caches, and
generated intermediate packed icon files are intentionally not included.

The current assets are still inside `firmware.bin`; they are just not included
as separate source-material files in this compact export.
