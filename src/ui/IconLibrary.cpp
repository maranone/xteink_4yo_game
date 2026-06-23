#include "IconLibrary.h"

#include <cstring>

#include "Primitives.h"

namespace puzzle::ui {
namespace {

void drawCat(int16_t cx, int16_t cy, int16_t size) {
  const int16_t r = size;
  drawEllipseOutline(cx, cy, r, static_cast<int16_t>(r * 0.85f), 3);
  // ears: two triangles via lines
  drawLine(static_cast<int16_t>(cx - r * 0.7f), static_cast<int16_t>(cy - r * 0.5f),
            static_cast<int16_t>(cx - r * 0.9f), static_cast<int16_t>(cy - r * 1.3f), 2);
  drawLine(static_cast<int16_t>(cx - r * 0.9f), static_cast<int16_t>(cy - r * 1.3f),
            static_cast<int16_t>(cx - r * 0.3f), static_cast<int16_t>(cy - r * 0.7f), 2);
  drawLine(static_cast<int16_t>(cx + r * 0.7f), static_cast<int16_t>(cy - r * 0.5f),
            static_cast<int16_t>(cx + r * 0.9f), static_cast<int16_t>(cy - r * 1.3f), 2);
  drawLine(static_cast<int16_t>(cx + r * 0.9f), static_cast<int16_t>(cy - r * 1.3f),
            static_cast<int16_t>(cx + r * 0.3f), static_cast<int16_t>(cy - r * 0.7f), 2);
  // eyes
  fillEllipse(static_cast<int16_t>(cx - r * 0.35f), static_cast<int16_t>(cy - r * 0.1f), 3, 3);
  fillEllipse(static_cast<int16_t>(cx + r * 0.35f), static_cast<int16_t>(cy - r * 0.1f), 3, 3);
  // nose + whiskers
  fillEllipse(cx, static_cast<int16_t>(cy + r * 0.2f), 3, 2);
  drawLine(static_cast<int16_t>(cx - r * 0.2f), static_cast<int16_t>(cy + r * 0.3f), cx,
            static_cast<int16_t>(cy + r * 0.45f), 2);
  drawLine(static_cast<int16_t>(cx + r * 0.2f), static_cast<int16_t>(cy + r * 0.3f), cx,
            static_cast<int16_t>(cy + r * 0.45f), 2);
  for (int8_t i = -1; i <= 1; i++) {
    drawLine(static_cast<int16_t>(cx - r * 0.5f), static_cast<int16_t>(cy + r * 0.2f + i * 8),
              static_cast<int16_t>(cx - r * 1.1f), static_cast<int16_t>(cy + r * 0.1f + i * 10), 1);
    drawLine(static_cast<int16_t>(cx + r * 0.5f), static_cast<int16_t>(cy + r * 0.2f + i * 8),
              static_cast<int16_t>(cx + r * 1.1f), static_cast<int16_t>(cy + r * 0.1f + i * 10), 1);
  }
}

void drawDog(int16_t cx, int16_t cy, int16_t size) {
  const int16_t r = size;
  drawEllipseOutline(cx, cy, r, static_cast<int16_t>(r * 0.85f), 3);
  // droopy ears: ellipses on either side
  drawEllipseOutline(static_cast<int16_t>(cx - r * 0.9f), static_cast<int16_t>(cy - r * 0.1f),
                       static_cast<int16_t>(r * 0.3f), static_cast<int16_t>(r * 0.55f), 2);
  drawEllipseOutline(static_cast<int16_t>(cx + r * 0.9f), static_cast<int16_t>(cy - r * 0.1f),
                       static_cast<int16_t>(r * 0.3f), static_cast<int16_t>(r * 0.55f), 2);
  fillEllipse(static_cast<int16_t>(cx - r * 0.3f), static_cast<int16_t>(cy - r * 0.1f), 3, 3);
  fillEllipse(static_cast<int16_t>(cx + r * 0.3f), static_cast<int16_t>(cy - r * 0.1f), 3, 3);
  // snout
  drawEllipseOutline(cx, static_cast<int16_t>(cy + r * 0.35f), static_cast<int16_t>(r * 0.35f),
                       static_cast<int16_t>(r * 0.25f), 2);
  fillEllipse(cx, static_cast<int16_t>(cy + r * 0.3f), 3, 2);
}

void drawPig(int16_t cx, int16_t cy, int16_t size) {
  const int16_t r = size;
  fillEllipse(cx, cy, r, static_cast<int16_t>(r * 0.85f));
  // ears
  drawLine(static_cast<int16_t>(cx - r * 0.6f), static_cast<int16_t>(cy - r * 0.6f),
            static_cast<int16_t>(cx - r * 0.9f), static_cast<int16_t>(cy - r * 1.1f), 2);
  drawLine(static_cast<int16_t>(cx + r * 0.6f), static_cast<int16_t>(cy - r * 0.6f),
            static_cast<int16_t>(cx + r * 0.9f), static_cast<int16_t>(cy - r * 1.1f), 2);
  // snout (lighter oval punched out by drawing its outline in white-on-black region isn't
  // supported without grayscale, so just outline it directly over the filled head)
  drawEllipseOutline(cx, static_cast<int16_t>(cy + r * 0.3f), static_cast<int16_t>(r * 0.4f),
                       static_cast<int16_t>(r * 0.25f), 2);
}

void drawApple(int16_t cx, int16_t cy, int16_t size) {
  const int16_t r = size;
  fillEllipse(cx, cy, static_cast<int16_t>(r * 0.85f), r);
  drawLine(cx, static_cast<int16_t>(cy - r), cx, static_cast<int16_t>(cy - r * 1.4f), 2);
  drawLine(cx, static_cast<int16_t>(cy - r * 1.3f), static_cast<int16_t>(cx + r * 0.4f),
            static_cast<int16_t>(cy - r * 1.5f), 2);
}

void drawStar(int16_t cx, int16_t cy, int16_t size) {
  // Sparkle/asterisk star: long cardinal arms, short diagonal arms - avoids
  // needing trig for a true 5-point polygon while still reading as "star"
  // at small icon sizes on a 1-bit display.
  drawLine(cx, static_cast<int16_t>(cy - size), cx, static_cast<int16_t>(cy + size), 3);
  drawLine(static_cast<int16_t>(cx - size), cy, static_cast<int16_t>(cx + size), cy, 3);
  const int16_t d = static_cast<int16_t>(size * 0.6f);
  drawLine(static_cast<int16_t>(cx - d), static_cast<int16_t>(cy - d), static_cast<int16_t>(cx + d),
            static_cast<int16_t>(cy + d), 2);
  drawLine(static_cast<int16_t>(cx - d), static_cast<int16_t>(cy + d), static_cast<int16_t>(cx + d),
            static_cast<int16_t>(cy - d), 2);
}

void drawCow(int16_t cx, int16_t cy, int16_t size) {
  const int16_t r = size;
  drawEllipseOutline(cx, cy, r, static_cast<int16_t>(r * 0.85f), 3);
  // horns: short lines angled outward/up
  drawLine(static_cast<int16_t>(cx - r * 0.4f), static_cast<int16_t>(cy - r * 0.9f),
            static_cast<int16_t>(cx - r * 0.6f), static_cast<int16_t>(cy - r * 1.3f), 2);
  drawLine(static_cast<int16_t>(cx + r * 0.4f), static_cast<int16_t>(cy - r * 0.9f),
            static_cast<int16_t>(cx + r * 0.6f), static_cast<int16_t>(cy - r * 1.3f), 2);
  fillEllipse(static_cast<int16_t>(cx - r * 0.35f), static_cast<int16_t>(cy - r * 0.1f), 3, 3);
  fillEllipse(static_cast<int16_t>(cx + r * 0.35f), static_cast<int16_t>(cy - r * 0.1f), 3, 3);
  // snout + spots
  drawEllipseOutline(cx, static_cast<int16_t>(cy + r * 0.35f), static_cast<int16_t>(r * 0.4f),
                       static_cast<int16_t>(r * 0.25f), 2);
  fillEllipse(static_cast<int16_t>(cx - r * 0.5f), static_cast<int16_t>(cy + r * 0.1f),
               static_cast<int16_t>(r * 0.15f), static_cast<int16_t>(r * 0.15f));
}

void drawOwl(int16_t cx, int16_t cy, int16_t size) {
  const int16_t r = size;
  drawEllipseOutline(cx, cy, r, static_cast<int16_t>(r * 0.9f), 3);
  // ear tufts
  drawLine(static_cast<int16_t>(cx - r * 0.4f), static_cast<int16_t>(cy - r * 0.85f),
            static_cast<int16_t>(cx - r * 0.55f), static_cast<int16_t>(cy - r * 1.25f), 2);
  drawLine(static_cast<int16_t>(cx + r * 0.4f), static_cast<int16_t>(cy - r * 0.85f),
            static_cast<int16_t>(cx + r * 0.55f), static_cast<int16_t>(cy - r * 1.25f), 2);
  // big round eyes - the defining feature
  drawEllipseOutline(static_cast<int16_t>(cx - r * 0.4f), cy, static_cast<int16_t>(r * 0.3f),
                       static_cast<int16_t>(r * 0.3f), 2);
  drawEllipseOutline(static_cast<int16_t>(cx + r * 0.4f), cy, static_cast<int16_t>(r * 0.3f),
                       static_cast<int16_t>(r * 0.3f), 2);
  fillEllipse(static_cast<int16_t>(cx - r * 0.4f), cy, 3, 3);
  fillEllipse(static_cast<int16_t>(cx + r * 0.4f), cy, 3, 3);
  // beak
  drawLine(cx, static_cast<int16_t>(cy + r * 0.15f), static_cast<int16_t>(cx - r * 0.1f),
            static_cast<int16_t>(cy + r * 0.4f), 2);
  drawLine(cx, static_cast<int16_t>(cy + r * 0.15f), static_cast<int16_t>(cx + r * 0.1f),
            static_cast<int16_t>(cy + r * 0.4f), 2);
}

void drawFox(int16_t cx, int16_t cy, int16_t size) {
  const int16_t r = size;
  drawEllipseOutline(cx, cy, r, static_cast<int16_t>(r * 0.85f), 3);
  // pointed ears (sharper triangles than the cat's)
  drawLine(static_cast<int16_t>(cx - r * 0.6f), static_cast<int16_t>(cy - r * 0.6f),
            static_cast<int16_t>(cx - r * 0.75f), static_cast<int16_t>(cy - r * 1.4f), 2);
  drawLine(static_cast<int16_t>(cx - r * 0.75f), static_cast<int16_t>(cy - r * 1.4f),
            static_cast<int16_t>(cx - r * 0.15f), static_cast<int16_t>(cy - r * 0.75f), 2);
  drawLine(static_cast<int16_t>(cx + r * 0.6f), static_cast<int16_t>(cy - r * 0.6f),
            static_cast<int16_t>(cx + r * 0.75f), static_cast<int16_t>(cy - r * 1.4f), 2);
  drawLine(static_cast<int16_t>(cx + r * 0.75f), static_cast<int16_t>(cy - r * 1.4f),
            static_cast<int16_t>(cx + r * 0.15f), static_cast<int16_t>(cy - r * 0.75f), 2);
  fillEllipse(static_cast<int16_t>(cx - r * 0.3f), static_cast<int16_t>(cy - r * 0.1f), 3, 3);
  fillEllipse(static_cast<int16_t>(cx + r * 0.3f), static_cast<int16_t>(cy - r * 0.1f), 3, 3);
  // pointed snout
  drawLine(static_cast<int16_t>(cx - r * 0.2f), static_cast<int16_t>(cy + r * 0.3f), cx,
            static_cast<int16_t>(cy + r * 0.6f), 2);
  drawLine(static_cast<int16_t>(cx + r * 0.2f), static_cast<int16_t>(cy + r * 0.3f), cx,
            static_cast<int16_t>(cy + r * 0.6f), 2);
}

void drawHen(int16_t cx, int16_t cy, int16_t size) {
  const int16_t r = size;
  fillEllipse(cx, static_cast<int16_t>(cy + r * 0.1f), r, static_cast<int16_t>(r * 0.75f));
  drawEllipseOutline(cx, static_cast<int16_t>(cy - r * 0.6f), static_cast<int16_t>(r * 0.4f),
                       static_cast<int16_t>(r * 0.4f), 2);
  // comb on top
  drawLine(static_cast<int16_t>(cx - r * 0.15f), static_cast<int16_t>(cy - r * 0.95f), cx,
            static_cast<int16_t>(cy - r * 1.25f), 2);
  drawLine(cx, static_cast<int16_t>(cy - r * 1.25f), static_cast<int16_t>(cx + r * 0.15f),
            static_cast<int16_t>(cy - r * 0.95f), 2);
  // beak
  drawLine(static_cast<int16_t>(cx + r * 0.35f), static_cast<int16_t>(cy - r * 0.6f),
            static_cast<int16_t>(cx + r * 0.6f), static_cast<int16_t>(cy - r * 0.55f), 2);
  // legs
  drawLine(static_cast<int16_t>(cx - r * 0.3f), static_cast<int16_t>(cy + r * 0.8f),
            static_cast<int16_t>(cx - r * 0.3f), static_cast<int16_t>(cy + r * 1.1f), 2);
  drawLine(static_cast<int16_t>(cx + r * 0.3f), static_cast<int16_t>(cy + r * 0.8f),
            static_cast<int16_t>(cx + r * 0.3f), static_cast<int16_t>(cy + r * 1.1f), 2);
}

void drawSun(int16_t cx, int16_t cy, int16_t size) {
  const int16_t r = static_cast<int16_t>(size * 0.55f);
  static const int16_t kDirX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
  static const int16_t kDirY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
  for (uint8_t i = 0; i < 8; i++) {
    drawLine(static_cast<int16_t>(cx + kDirX[i] * r * 1.1f), static_cast<int16_t>(cy + kDirY[i] * r * 1.1f),
              static_cast<int16_t>(cx + kDirX[i] * r * 1.8f), static_cast<int16_t>(cy + kDirY[i] * r * 1.8f), 2);
  }
  drawEllipseOutline(cx, cy, r, r, 3);
}

void drawBall(int16_t cx, int16_t cy, int16_t size) {
  drawEllipseOutline(cx, cy, size, size, 3);
  drawLine(static_cast<int16_t>(cx - size), cy, static_cast<int16_t>(cx + size), cy, 1);
  drawLine(cx, static_cast<int16_t>(cy - size), cx, static_cast<int16_t>(cy + size), 1);
}

void drawTree(int16_t cx, int16_t cy, int16_t size) {
  drawLine(cx, static_cast<int16_t>(cy + size * 0.3f), cx, static_cast<int16_t>(cy + size * 1.1f), 3);
  fillEllipse(cx, static_cast<int16_t>(cy - size * 0.25f), size, static_cast<int16_t>(size * 0.75f));
}

void drawLionCompleted(int16_t cx, int16_t cy, int16_t size) {
  const int16_t r = static_cast<int16_t>(size * 0.55f);
  // mane: radiating lines around the head circle. Uses an 8-direction
  // lookup instead of sin/cos - cheap and reads fine as a mane at icon
  // scale - so this stays free of a <cmath> dependency.
  static const int16_t kDirX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
  static const int16_t kDirY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
  for (uint8_t i = 0; i < 8; i++) {
    const int16_t tipX = static_cast<int16_t>(cx + kDirX[i] * r * 1.7f);
    const int16_t tipY = static_cast<int16_t>(cy + kDirY[i] * r * 1.7f);
    const int16_t rootX = static_cast<int16_t>(cx + kDirX[i] * r);
    const int16_t rootY = static_cast<int16_t>(cy + kDirY[i] * r);
    drawLine(rootX, rootY, tipX, tipY, 2);
  }
  drawEllipseOutline(cx, cy, r, static_cast<int16_t>(r * 0.9f), 3);
  fillEllipse(static_cast<int16_t>(cx - r * 0.35f), static_cast<int16_t>(cy - r * 0.1f), 3, 3);
  fillEllipse(static_cast<int16_t>(cx + r * 0.35f), static_cast<int16_t>(cy - r * 0.1f), 3, 3);
  fillEllipse(cx, static_cast<int16_t>(cy + r * 0.25f), 4, 3);
}

}  // namespace

void drawIcon(const char* iconId, int16_t cx, int16_t cy, int16_t size) {
  if (!iconId) return;
  if (strcmp(iconId, "cat") == 0) {
    drawCat(cx, cy, size);
  } else if (strcmp(iconId, "dog") == 0) {
    drawDog(cx, cy, size);
  } else if (strcmp(iconId, "pig") == 0) {
    drawPig(cx, cy, size);
  } else if (strcmp(iconId, "apple") == 0) {
    drawApple(cx, cy, size);
  } else if (strcmp(iconId, "star") == 0) {
    drawStar(cx, cy, size);
  } else if (strcmp(iconId, "lion") == 0) {
    drawLionCompleted(cx, cy, size);
  } else if (strcmp(iconId, "cow") == 0) {
    drawCow(cx, cy, size);
  } else if (strcmp(iconId, "owl") == 0) {
    drawOwl(cx, cy, size);
  } else if (strcmp(iconId, "fox") == 0) {
    drawFox(cx, cy, size);
  } else if (strcmp(iconId, "hen") == 0) {
    drawHen(cx, cy, size);
  } else if (strcmp(iconId, "sun") == 0) {
    drawSun(cx, cy, size);
  } else if (strcmp(iconId, "ball") == 0) {
    drawBall(cx, cy, size);
  } else if (strcmp(iconId, "tree") == 0) {
    drawTree(cx, cy, size);
  }
}

}  // namespace puzzle::ui
