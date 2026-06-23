#include "PuzzleState.h"

#include <ArduinoJson.h>

namespace puzzle {

void recordOutcome(PuzzleState& state, uint8_t wrongAttempts) {
  state.successWindow[state.successWindowIndex] = wrongAttempts;
  state.successWindowIndex = (state.successWindowIndex + 1) % kSuccessWindowSize;
  if (state.successWindowFilled < kSuccessWindowSize) state.successWindowFilled++;
}

float rollingSuccessRate(const PuzzleState& state) {
  if (state.successWindowFilled == 0) return 0.0f;
  uint8_t firstTryCount = 0;
  for (uint8_t i = 0; i < state.successWindowFilled; i++) {
    if (state.successWindow[i] == 0) firstTryCount++;
  }
  return static_cast<float>(firstTryCount) / static_cast<float>(state.successWindowFilled);
}

void addPrize(PuzzleState& state, const std::string& iconId) {
  if (state.completedPictures.size() >= kMaxCollectedPrizes) {
    state.completedPictures.erase(state.completedPictures.begin());
  }
  state.completedPictures.push_back(iconId);
}

std::string serializeState(const PuzzleState& state) {
  JsonDocument doc;
  doc["currentThemeId"] = state.currentThemeId;
  doc["currentPictureProgress"] = state.currentPictureProgress;
  doc["difficultyTier"] = state.difficultyTier;
  doc["successWindowIndex"] = state.successWindowIndex;
  doc["successWindowFilled"] = state.successWindowFilled;
  JsonArray window = doc["successWindow"].to<JsonArray>();
  for (uint8_t i = 0; i < kSuccessWindowSize; i++) window.add(state.successWindow[i]);
  JsonArray pieceIcons = doc["currentPieceIcons"].to<JsonArray>();
  for (uint8_t i = 0; i < kPiecesPerPicture; i++) pieceIcons.add(state.currentPieceIcons[i]);
  JsonArray completed = doc["completedPictures"].to<JsonArray>();
  for (const auto& iconId : state.completedPictures) completed.add(iconId);

  std::string out;
  serializeJsonPretty(doc, out);
  return out;
}

bool parseStateJson(const std::string& json, PuzzleState& state, std::string& error) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    error = err.c_str();
    return false;
  }
  state = {};
  state.currentThemeId = doc["currentThemeId"] | "";
  state.currentPictureProgress = doc["currentPictureProgress"] | 0;
  state.difficultyTier = doc["difficultyTier"] | 0;
  state.successWindowIndex = doc["successWindowIndex"] | 0;
  state.successWindowFilled = doc["successWindowFilled"] | 0;
  uint8_t i = 0;
  for (JsonVariantConst v : doc["successWindow"].as<JsonArrayConst>()) {
    if (i >= kSuccessWindowSize) break;
    state.successWindow[i++] = v.as<uint8_t>();
  }
  uint8_t pieceIdx = 0;
  for (JsonVariantConst v : doc["currentPieceIcons"].as<JsonArrayConst>()) {
    if (pieceIdx >= kPiecesPerPicture) break;
    const char* s = v | "";
    state.currentPieceIcons[pieceIdx++] = s ? s : "";
  }
  for (JsonVariantConst v : doc["completedPictures"].as<JsonArrayConst>()) {
    const char* s = v | "";
    if (s) state.completedPictures.emplace_back(s);
  }
  return true;
}

}  // namespace puzzle
