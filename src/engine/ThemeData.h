#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace puzzle {

struct WordEntry {
  std::string word;
  std::string iconId;
};

struct CountObject {
  std::string iconId;
  std::string label;
};

// One difficulty tier's generation constraints. Tiers are ordered easy to
// hard; the engine only ever moves forward through them (one-way ratchet)
// based on a rolling success-rate window - see ChallengeGenerator.
struct DifficultyTier {
  uint8_t wordLenMin = 3;
  uint8_t wordLenMax = 3;
  std::vector<std::string> blankPositions;  // "first" | "middle" | "last"
  uint8_t countMin = 1;
  uint8_t countMax = 5;
  uint8_t sumMin = 1;
  uint8_t sumMax = 5;
  float advanceAt = 0.75f;  // rolling correct-on-first-try rate to advance past this tier
};

struct CompletionPicture {
  std::string iconId;
  std::string label;
};

struct ThemeData {
  std::string themeId;
  std::string themeName;
  std::string version;  // compared against the embedded copy to decide whether to self-sync onto SD
  std::vector<WordEntry> wordPool;
  std::vector<CountObject> countObjects;
  std::vector<DifficultyTier> difficultyTiers;
  CompletionPicture completionPicture;
};

struct ThemeResult {
  bool ok = false;
  ThemeData theme;
  std::string error;
};

ThemeResult parseThemeJson(const std::string& json);

}  // namespace puzzle
