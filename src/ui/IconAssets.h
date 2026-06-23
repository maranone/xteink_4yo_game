#pragma once

#include <cstdint>

class EInkDisplay;

namespace puzzle::ui {

// Must be called once in setup(), after the display is ready and before any
// drawIconAsset() call.
void initIconAssets(EInkDisplay& display);

enum class IconRenderPlane : uint8_t { kBlackWhite, kGrayLsb, kGrayMsb };
enum class IconAssetVariant : uint8_t { kWord, kCount };

// Looks up an embedded icon and draws one pass of it centered at (cx, cy).
// Word assets are native 192x192 four-level grayscale. Count assets are native
// 32x32 one-bit black/white. Grayscale display refreshes render word assets in
// three planes: black/white base, gray LSB, then gray MSB.
// Returns false when iconId is not bundled, allowing callers to fall back to
// the procedural IconLibrary.
bool drawIconAsset(const char* iconId, IconAssetVariant variant, IconRenderPlane plane,
                   int16_t cx, int16_t cy,
                   uint8_t scale = 1);
bool hasIconAsset(const char* iconId);
bool hasGrayscaleIconAsset(const char* iconId);

}  // namespace puzzle::ui
