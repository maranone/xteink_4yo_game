#include "ChallengeGenerator.h"

#include <algorithm>
#include <vector>

namespace puzzle {
namespace {

const DifficultyTier& tierOrFallback(const ThemeData& theme, uint8_t tierIndex) {
  if (tierIndex >= theme.difficultyTiers.size()) return theme.difficultyTiers.back();
  return theme.difficultyTiers[tierIndex];
}

std::vector<const WordEntry*> wordsForTier(const ThemeData& theme, const DifficultyTier& tier) {
  std::vector<const WordEntry*> out;
  for (const auto& w : theme.wordPool) {
    const size_t len = w.word.size();
    if (len >= tier.wordLenMin && len <= tier.wordLenMax) out.push_back(&w);
  }
  if (out.empty()) {
    // No word in the pool fits this tier's length range (content authoring
    // gap) - fall back to the whole pool rather than crashing on an empty
    // candidate list.
    for (const auto& w : theme.wordPool) out.push_back(&w);
  }
  return out;
}

size_t pickBlankPosition(const std::string& word, const DifficultyTier& tier, Rng& rng) {
  const size_t len = word.size();
  const std::string& choice = tier.blankPositions[rng.range(0, static_cast<uint32_t>(tier.blankPositions.size() - 1))];
  if (choice == "first") return 0;
  if (choice == "middle" && len >= 3) return rng.range(1, static_cast<uint32_t>(len - 2));
  return len - 1;  // "last", or "middle" requested on a too-short word
}

// Picks a letter not in alreadyUsed (size n) and not in word. Guarded so a
// pathological pool (e.g. a word using most of the alphabet) can't loop
// forever - falls back to whatever the loop landed on.
char randomLetterExcluding(Rng& rng, const std::string& word, const char* alreadyUsed, uint8_t n) {
  char c;
  int guard = 0;
  do {
    c = static_cast<char>('A' + rng.range(0, 25));
    bool clash = word.find(c) != std::string::npos;
    for (uint8_t i = 0; i < n && !clash; i++) clash = (c == alreadyUsed[i]);
    guard++;
    if (!clash) break;
  } while (guard < 50);
  return c;
}

void shuffleChoices(std::string values[kChoiceCount], uint8_t correctIndex, std::string outChoices[kChoiceCount],
                     uint8_t& outCorrectIndex, Rng& rng) {
  uint8_t order[kChoiceCount];
  for (uint8_t i = 0; i < kChoiceCount; i++) order[i] = i;
  for (int i = kChoiceCount - 1; i > 0; i--) {
    const uint8_t j = static_cast<uint8_t>(rng.range(0, i));
    std::swap(order[i], order[j]);
  }
  for (uint8_t slot = 0; slot < kChoiceCount; slot++) {
    outChoices[slot] = values[order[slot]];
    if (order[slot] == correctIndex) outCorrectIndex = slot;
  }
}

Challenge generateWordChallenge(const ThemeData& theme, const DifficultyTier& tier, Rng& rng) {
  const auto candidates = wordsForTier(theme, tier);
  const WordEntry& entry = *candidates[rng.range(0, static_cast<uint32_t>(candidates.size() - 1))];
  const size_t blankPos = pickBlankPosition(entry.word, tier, rng);
  const char correctLetter = entry.word[blankPos];

  char wrongLetters[kChoiceCount - 1];
  for (uint8_t i = 0; i < kChoiceCount - 1; i++) {
    char used[kChoiceCount] = {correctLetter};
    for (uint8_t j = 0; j < i; j++) used[1 + j] = wrongLetters[j];
    wrongLetters[i] = randomLetterExcluding(rng, entry.word, used, static_cast<uint8_t>(1 + i));
  }

  std::string values[kChoiceCount];
  values[0] = std::string(1, correctLetter);
  for (uint8_t i = 0; i < kChoiceCount - 1; i++) values[1 + i] = std::string(1, wrongLetters[i]);

  Challenge challenge;
  challenge.type = Challenge::Type::WordMissingLetter;
  challenge.iconId = entry.iconId;
  challenge.promptPrefix = entry.word.substr(0, blankPos);
  challenge.promptSuffix = entry.word.substr(blankPos + 1);
  shuffleChoices(values, 0, challenge.choices, challenge.correctChoiceIndex, rng);
  return challenge;
}

Challenge generateCountChallenge(const ThemeData& theme, const DifficultyTier& tier, Rng& rng) {
  const CountObject& obj = theme.countObjects[rng.range(0, static_cast<uint32_t>(theme.countObjects.size() - 1))];
  const uint8_t correctCount = static_cast<uint8_t>(rng.range(tier.countMin, tier.countMax));

  uint8_t wrongCounts[kChoiceCount - 1];
  for (uint8_t i = 0; i < kChoiceCount - 1; i++) {
    uint8_t candidate = correctCount;
    for (int guard = 0; guard < 50; guard++) {
      const int32_t delta = static_cast<int32_t>(rng.range(1, 2 + i)) * (rng.range(0, 1) ? 1 : -1);
      const int32_t v = static_cast<int32_t>(correctCount) + delta;
      if (v < 1 || v > 255 || v == correctCount) continue;
      bool clash = false;
      for (uint8_t j = 0; j < i && !clash; j++) clash = (wrongCounts[j] == v);
      if (!clash) {
        candidate = static_cast<uint8_t>(v);
        break;
      }
    }
    if (candidate == correctCount) candidate = static_cast<uint8_t>(correctCount + i + 1);  // never loops forever
    wrongCounts[i] = candidate;
  }

  std::string values[kChoiceCount];
  values[0] = std::to_string(correctCount);
  for (uint8_t i = 0; i < kChoiceCount - 1; i++) values[1 + i] = std::to_string(wrongCounts[i]);

  Challenge challenge;
  challenge.type = Challenge::Type::CountObjects;
  challenge.iconId = obj.iconId;
  challenge.iconRepeatCount = correctCount;
  shuffleChoices(values, 0, challenge.choices, challenge.correctChoiceIndex, rng);
  return challenge;
}

}  // namespace

uint32_t Rng::next() {
  // xorshift32
  state_ ^= state_ << 13;
  state_ ^= state_ >> 17;
  state_ ^= state_ << 5;
  return state_;
}

uint32_t Rng::range(uint32_t minInclusive, uint32_t maxInclusive) {
  if (maxInclusive <= minInclusive) return minInclusive;
  return minInclusive + (next() % (maxInclusive - minInclusive + 1));
}

Challenge generateChallenge(const ThemeData& theme, uint8_t tierIndex, Rng& rng) {
  const DifficultyTier& tier = tierOrFallback(theme, tierIndex);
  // Alternate challenge type by coin flip so both word and count practice
  // show up across a session, rather than picking once per theme.
  const bool wantWord = theme.countObjects.empty() || rng.range(0, 1) == 0;
  if (wantWord && !theme.wordPool.empty()) return generateWordChallenge(theme, tier, rng);
  return generateCountChallenge(theme, tier, rng);
}

}  // namespace puzzle
