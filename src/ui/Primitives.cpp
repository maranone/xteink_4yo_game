#include "Primitives.h"

#include <Arduino.h>
#include <EInkDisplay.h>

#include <cstdlib>

namespace puzzle::ui {

uint16_t gW = 0, gH = 0;

namespace {
EInkDisplay* gDisplay = nullptr;

uint8_t glyphBits(char c, uint8_t row) {
  if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  switch (c) {
    case 'A': { static const uint8_t g[] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}; return g[row]; }
    case 'B': { static const uint8_t g[] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}; return g[row]; }
    case 'C': { static const uint8_t g[] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}; return g[row]; }
    case 'D': { static const uint8_t g[] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}; return g[row]; }
    case 'E': { static const uint8_t g[] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}; return g[row]; }
    case 'F': { static const uint8_t g[] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}; return g[row]; }
    case 'G': { static const uint8_t g[] = {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}; return g[row]; }
    case 'H': { static const uint8_t g[] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}; return g[row]; }
    case 'I': { static const uint8_t g[] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}; return g[row]; }
    case 'J': { static const uint8_t g[] = {0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C}; return g[row]; }
    case 'K': { static const uint8_t g[] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}; return g[row]; }
    case 'L': { static const uint8_t g[] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}; return g[row]; }
    case 'M': { static const uint8_t g[] = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}; return g[row]; }
    case 'N': { static const uint8_t g[] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}; return g[row]; }
    case 'O': { static const uint8_t g[] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}; return g[row]; }
    case 'P': { static const uint8_t g[] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}; return g[row]; }
    case 'Q': { static const uint8_t g[] = {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}; return g[row]; }
    case 'R': { static const uint8_t g[] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}; return g[row]; }
    case 'S': { static const uint8_t g[] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}; return g[row]; }
    case 'T': { static const uint8_t g[] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}; return g[row]; }
    case 'U': { static const uint8_t g[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}; return g[row]; }
    case 'V': { static const uint8_t g[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}; return g[row]; }
    case 'W': { static const uint8_t g[] = {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}; return g[row]; }
    case 'X': { static const uint8_t g[] = {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}; return g[row]; }
    case 'Y': { static const uint8_t g[] = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}; return g[row]; }
    case 'Z': { static const uint8_t g[] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}; return g[row]; }
    case '0': { static const uint8_t g[] = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}; return g[row]; }
    case '1': { static const uint8_t g[] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}; return g[row]; }
    case '2': { static const uint8_t g[] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}; return g[row]; }
    case '3': { static const uint8_t g[] = {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}; return g[row]; }
    case '4': { static const uint8_t g[] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}; return g[row]; }
    case '5': { static const uint8_t g[] = {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}; return g[row]; }
    case '6': { static const uint8_t g[] = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}; return g[row]; }
    case '7': { static const uint8_t g[] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}; return g[row]; }
    case '8': { static const uint8_t g[] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}; return g[row]; }
    case '9': { static const uint8_t g[] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}; return g[row]; }
    case ':': return row == 2 || row == 4 ? 0x04 : 0x00;
    case '.': return row == 6 ? 0x04 : 0x00;
    case '-': return row == 3 ? 0x1F : 0x00;
    case '/': return row == 0 ? 0x01 : row == 6 ? 0x10 : (0x01 << (row > 3 ? 6 - row : row));
    case '?': { static const uint8_t g[] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}; return g[row]; }
    default: return 0x00;
  }
}

void drawChar(char c, uint16_t x, uint16_t y, uint8_t scale) {
  for (uint8_t row = 0; row < 7; row++) {
    const uint8_t bits = glyphBits(c, row);
    for (uint8_t col = 0; col < 5; col++) {
      if ((bits & (0x10 >> col)) == 0) continue;
      for (uint8_t dy = 0; dy < scale; dy++) {
        for (uint8_t dx = 0; dx < scale; dx++) {
          setPixel(x + col * scale + dx, y + row * scale + dy, true);
        }
      }
    }
  }
}

}  // namespace

void initPrimitives(EInkDisplay& display) {
  gDisplay = &display;
  gW = display.getDisplayWidth();
  gH = display.getDisplayHeight();
}

void setPixel(uint16_t x, uint16_t y, bool black) {
  if (x >= gW || y >= gH || !gDisplay) return;
  const uint16_t widthBytes = gDisplay->getDisplayWidthBytes();
  uint8_t* fb = gDisplay->getFrameBuffer();
  const uint32_t index = y * widthBytes + x / 8;
  const uint8_t mask = 0x80 >> (x % 8);
  if (black) {
    fb[index] &= ~mask;
  } else {
    fb[index] |= mask;
  }
}

void clearRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h) {
  for (uint16_t y = y0; y < y0 + h && y < gH; y++) {
    for (uint16_t x = x0; x < x0 + w && x < gW; x++) setPixel(x, y, false);
  }
}

void drawHLine(uint16_t y, uint8_t thickness) {
  for (uint8_t t = 0; t < thickness; t++) {
    for (uint16_t x = 0; x < gW; x++) setPixel(x, y + t, true);
  }
}

void drawVLine(uint16_t x, uint16_t y0, uint16_t y1) {
  for (uint16_t y = y0; y < y1; y++) setPixel(x, y, true);
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness) {
  const int16_t dx = x1 - x0;
  const int16_t dy = y1 - y0;
  const int16_t steps = max(static_cast<int16_t>(abs(dx)), static_cast<int16_t>(abs(dy)));
  const int8_t half = static_cast<int8_t>(thickness / 2);
  for (int16_t i = 0; i <= steps; i++) {
    const int16_t x = steps == 0 ? x0 : x0 + dx * i / steps;
    const int16_t y = steps == 0 ? y0 : y0 + dy * i / steps;
    for (int8_t tx = -half; tx <= half; tx++) {
      for (int8_t ty = -half; ty <= half; ty++) {
        const int16_t px = x + tx, py = y + ty;
        if (px >= 0 && py >= 0) setPixel(static_cast<uint16_t>(px), static_cast<uint16_t>(py), true);
      }
    }
    if (steps == 0) break;
  }
}

void drawEllipseOutline(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint8_t thickness) {
  const int16_t rxIn = rx > thickness ? rx - thickness : 0;
  const int16_t ryIn = ry > thickness ? ry - thickness : 0;
  const int32_t outer = static_cast<int32_t>(rx) * rx * ry * ry;
  const int32_t inner = static_cast<int32_t>(rxIn) * rxIn * ryIn * ryIn;
  for (int16_t dy = -ry; dy <= ry; dy++) {
    for (int16_t dx = -rx; dx <= rx; dx++) {
      const int32_t v = static_cast<int32_t>(dx) * dx * ry * ry + static_cast<int32_t>(dy) * dy * rx * rx;
      if (v <= outer && v >= inner) {
        const int16_t x = cx + dx, y = cy + dy;
        if (x >= 0 && y >= 0) setPixel(static_cast<uint16_t>(x), static_cast<uint16_t>(y), true);
      }
    }
  }
}

void fillEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry) {
  const int32_t outer = static_cast<int32_t>(rx) * rx * ry * ry;
  for (int16_t dy = -ry; dy <= ry; dy++) {
    for (int16_t dx = -rx; dx <= rx; dx++) {
      const int32_t v = static_cast<int32_t>(dx) * dx * ry * ry + static_cast<int32_t>(dy) * dy * rx * rx;
      if (v <= outer) {
        const int16_t x = cx + dx, y = cy + dy;
        if (x >= 0 && y >= 0) setPixel(static_cast<uint16_t>(x), static_cast<uint16_t>(y), true);
      }
    }
  }
}

void drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, float fraction) {
  if (fraction < 0.0f) fraction = 0.0f;
  if (fraction > 1.0f) fraction = 1.0f;
  for (uint16_t i = 0; i < w; i++) {
    setPixel(x + i, y, true);
    setPixel(x + i, y + h - 1, true);
  }
  for (uint16_t j = 0; j < h; j++) {
    setPixel(x, y + j, true);
    setPixel(x + w - 1, y + j, true);
  }
  const uint16_t fillW = w > 2 ? static_cast<uint16_t>((w - 2) * fraction) : 0;
  for (uint16_t j = 1; j + 1 < h; j++) {
    for (uint16_t i = 1; i < 1 + fillW; i++) setPixel(x + i, y + j, true);
  }
}

uint16_t textWidth(const char* text, uint8_t scale) {
  uint16_t width = 0;
  for (const char* p = text; *p; p++) width += (*p == ' ') ? 4 * scale : 6 * scale;
  return width;
}

void drawText(uint16_t x, uint16_t y, const char* text, uint8_t scale) {
  uint16_t cursor = x;
  while (*text) {
    if (*text == ' ') {
      cursor += 4 * scale;
    } else {
      drawChar(*text, cursor, y, scale);
      cursor += 6 * scale;
    }
    text++;
    if (cursor > gW - 6 * scale) break;
  }
}

}  // namespace puzzle::ui
