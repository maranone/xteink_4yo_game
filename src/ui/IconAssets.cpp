#include "IconAssets.h"
#include "IconAssetsData.h"

#include <EInkDisplay.h>

#include <cstring>

namespace puzzle::ui {
namespace {

EInkDisplay* gDisplay = nullptr;

}  // namespace

void initIconAssets(EInkDisplay& display) {
  gDisplay = &display;
}

const IconAssetEntry* findIconAsset(const char* iconId) {
  if (!iconId) return nullptr;
  for (size_t i = 0; i < kIconAssetCount; ++i) {
    if (strcmp(kIconAssetTable[i].iconId, iconId) == 0) return &kIconAssetTable[i];
  }
  return nullptr;
}

bool hasIconAsset(const char* iconId) {
  return findIconAsset(iconId) != nullptr;
}

bool hasGrayscaleIconAsset(const char* iconId) {
  const IconAssetEntry* entry = findIconAsset(iconId);
  return entry && entry->wordBytes == 192 * 192 / 4;
}

bool drawIconAsset(const char* iconId, IconAssetVariant variant, IconRenderPlane plane,
                   int16_t cx, int16_t cy, uint8_t scale) {
  if (!gDisplay || !iconId) return false;
  const IconAssetEntry* entry = findIconAsset(iconId);
  if (!entry || scale == 0 || scale > 4) return false;

  const bool countVariant = variant == IconAssetVariant::kCount;
  const int16_t dim = countVariant ? 32 : 192;
  const uint16_t expectedBytes = countVariant ? 32 * 32 / 8 : 192 * 192 / 4;
  const uint8_t* data = countVariant ? entry->countData : entry->wordData;
  const uint16_t bytes = countVariant ? entry->countBytes : entry->wordBytes;
  if (!data || bytes != expectedBytes) return false;

  const int16_t outputDim = static_cast<int16_t>(dim * scale);
  const int16_t left = static_cast<int16_t>(cx - outputDim / 2);
  const int16_t top = static_cast<int16_t>(cy - outputDim / 2);
  const uint16_t screenW = gDisplay->getDisplayWidth();
  const uint16_t screenH = gDisplay->getDisplayHeight();
  const uint16_t widthBytes = gDisplay->getDisplayWidthBytes();
  uint8_t* framebuffer = gDisplay->getFrameBuffer();

  for (int16_t sy = 0; sy < dim; ++sy) {
    for (int16_t sx = 0; sx < dim; ++sx) {
      const uint16_t pixel = static_cast<uint16_t>(sy * dim + sx);
      uint8_t value = 0;
      if (countVariant) {
        const uint8_t packed = pgm_read_byte(data + pixel / 8);
        value = (packed & (0x80 >> (pixel % 8))) ? 3 : 0;
      } else {
        const uint8_t packed = pgm_read_byte(data + pixel / 4);
        value = static_cast<uint8_t>((packed >> ((3 - pixel % 4) * 2)) & 0x03);
      }

      bool setWhiteBit = false;
      switch (plane) {
        case IconRenderPlane::kBlackWhite:
          setWhiteBit = value == 0;  // all gray/count black pixels are black in the base pass
          break;
        case IconRenderPlane::kGrayLsb:
          setWhiteBit = countVariant ? true : value == 2;  // dark gray only
          break;
        case IconRenderPlane::kGrayMsb:
          setWhiteBit = countVariant ? true : (value == 1 || value == 2);  // light and dark gray
          break;
      }
      for (uint8_t dy = 0; dy < scale; ++dy) {
        const int16_t y = static_cast<int16_t>(top + sy * scale + dy);
        if (y < 0 || y >= static_cast<int16_t>(screenH)) continue;
        for (uint8_t dx = 0; dx < scale; ++dx) {
          const int16_t x = static_cast<int16_t>(left + sx * scale + dx);
          if (x < 0 || x >= static_cast<int16_t>(screenW)) continue;
          const uint32_t index = static_cast<uint32_t>(y) * widthBytes + static_cast<uint16_t>(x) / 8;
          const uint8_t mask = static_cast<uint8_t>(0x80 >> (static_cast<uint16_t>(x) % 8));
          if (setWhiteBit) framebuffer[index] |= mask;
          else framebuffer[index] &= static_cast<uint8_t>(~mask);
        }
      }
    }
  }
  return true;
}

}  // namespace puzzle::ui
