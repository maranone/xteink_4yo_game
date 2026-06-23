#pragma once

#include <cstdint>
#include <string>

#include "ThemeData.h"

namespace puzzle {

// 4 choices, one per physical button (the device's button cluster is
// Next Page / Previous Page / OK / Back, all usable as answer slots).
constexpr uint8_t kChoiceCount = 4;

struct Challenge {
  enum class Type { WordMissingLetter, CountObjects };
  Type type = Type::WordMissingLetter;

  std::string iconId;        // icon to draw (the word's icon, or the count object's icon)
  std::string promptPrefix;  // text shown before the blank (word challenges only)
  std::string promptSuffix;  // text shown after the blank (word challenges only)
  uint8_t iconRepeatCount = 1;  // how many copies of iconId to draw (count challenges)

  std::string choices[kChoiceCount];  // already shuffled; exactly one is correct
  uint8_t correctChoiceIndex = 0;
};

// Small, self-contained PRNG (xorshift32) instead of Arduino's random() so
// this engine layer stays Arduino-independent and unit-testable, matching
// xteink-idle-engine's engine/ layer (see its test/native/test_engine.cpp).
// Seed once at startup from a real entropy source (e.g. esp_random()).
class Rng {
 public:
  explicit Rng(uint32_t seed) : state_(seed ? seed : 1) {}
  void reseed(uint32_t seed) { state_ = seed ? seed : 1; }
  uint32_t next();
  uint32_t range(uint32_t minInclusive, uint32_t maxInclusive);

 private:
  uint32_t state_;
};

// Generates one challenge from theme's word/count pools, constrained by
// theme.difficultyTiers[tierIndex] (clamped to a valid index). Exactly one
// of the kChoiceCount choices is correct; positions are shuffled via rng.
Challenge generateChallenge(const ThemeData& theme, uint8_t tierIndex, Rng& rng);

}  // namespace puzzle
