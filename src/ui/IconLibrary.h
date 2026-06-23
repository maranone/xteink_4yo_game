#pragma once

#include <cstdint>

namespace puzzle::ui {

// Draws a procedural icon centered at (cx, cy) sized roughly to fit a
// (2*size) x (2*size) box. No bitmap assets - this hardware has no
// runtime PNG/JPEG decode and a tight enough heap that one isn't worth
// adding (see xteink-idle-engine's drawProceduralCoffeeArt, which hit the
// same constraint for its art screen). Unknown iconId draws nothing.
void drawIcon(const char* iconId, int16_t cx, int16_t cy, int16_t size);

}  // namespace puzzle::ui
