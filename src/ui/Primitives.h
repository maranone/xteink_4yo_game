#pragma once

#include <cstdint>

class EInkDisplay;

namespace puzzle::ui {

// Must be called once in setup() after display.begin(), before any drawing.
void initPrimitives(EInkDisplay& display);

// Current panel resolution, set by initPrimitives - read-only elsewhere.
extern uint16_t gW, gH;

void setPixel(uint16_t x, uint16_t y, bool black);
void clearRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h);
void drawHLine(uint16_t y, uint8_t thickness = 1);
void drawVLine(uint16_t x, uint16_t y0, uint16_t y1);
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness = 2);
void drawEllipseOutline(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint8_t thickness = 2);
void fillEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry);
void drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, float fraction);

// Tiny built-in bitmap font (A-Z, 0-9, basic punctuation) - same glyph set
// xteink-idle-engine uses, ported here since each firmware is its own
// binary with no shared runtime library between them.
uint16_t textWidth(const char* text, uint8_t scale);
void drawText(uint16_t x, uint16_t y, const char* text, uint8_t scale = 3);

}  // namespace puzzle::ui
