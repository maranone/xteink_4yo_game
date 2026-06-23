#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace puzzle {

constexpr uint8_t kPiecesPerPicture = 12;
constexpr uint8_t kSuccessWindowSize = 10;
constexpr uint8_t kMaxCollectedPrizes = 25;  // oldest dropped once full - see addPrize()

// Persisted progress. Never reset by content changes (e.g. re-syncing the
// embedded theme JSON onto the SD card) - only by completing pieces/themes.
struct PuzzleState {
  std::string currentThemeId;
  uint8_t currentPictureProgress = 0;  // 0..kPiecesPerPicture

  // Slot i holds the icon id of whichever challenge earned that piece
  // (empty = not solved yet, drawn as "?"). A single correct answer can
  // fill more than one slot at once (the 1st/2nd/3rd/4th-try reward is
  // 4/3/2/1 pieces), all stamped with that challenge's icon. Reset to all
  // empty whenever a picture completes.
  std::string currentPieceIcons[kPiecesPerPicture];

  uint8_t difficultyTier = 0;  // index into the active theme's difficultyTiers

  // Ring buffer of recent challenge outcomes, used to decide when to
  // advance difficultyTier. Value is the number of wrong presses before
  // getting it right (0 = correct on the 1st try) - slots beyond
  // successWindowFilled are unwritten.
  uint8_t successWindow[kSuccessWindowSize] = {0};
  uint8_t successWindowIndex = 0;
  uint8_t successWindowFilled = 0;

  // One entry per completed picture - the icon of whichever challenge
  // happened to complete it (not a fixed per-theme mascot, so the prize
  // varies). Drawn as the "prizes" grid on the sleep/status screen.
  std::vector<std::string> completedPictures;
};

// Appends iconId, dropping the oldest entry first if already at
// kMaxCollectedPrizes (keeps the most recent prizes, not the first ones).
void addPrize(PuzzleState& state, const std::string& iconId);

void recordOutcome(PuzzleState& state, uint8_t wrongAttempts);
float rollingSuccessRate(const PuzzleState& state);  // fraction of window that's a first-try correct (wrongAttempts==0)

std::string serializeState(const PuzzleState& state);
bool parseStateJson(const std::string& json, PuzzleState& state, std::string& error);

}  // namespace puzzle
