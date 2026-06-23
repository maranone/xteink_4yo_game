#include "ThemeData.h"

#include <ArduinoJson.h>

namespace puzzle {
namespace {

std::string asString(JsonVariantConst value) {
  const char* text = value | "";
  return text ? std::string(text) : std::string();
}

}  // namespace

ThemeResult parseThemeJson(const std::string& json) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) return {false, {}, err.c_str()};

  ThemeData theme;
  theme.themeId = asString(doc["themeId"]);
  theme.themeName = asString(doc["themeName"]);
  theme.version = asString(doc["version"]);

  for (JsonObjectConst item : doc["wordPool"].as<JsonArrayConst>()) {
    theme.wordPool.push_back({asString(item["word"]), asString(item["iconId"])});
  }

  for (JsonObjectConst item : doc["countObjects"].as<JsonArrayConst>()) {
    theme.countObjects.push_back({asString(item["iconId"]), asString(item["label"])});
  }

  for (JsonObjectConst item : doc["difficultyTiers"].as<JsonArrayConst>()) {
    DifficultyTier tier;
    JsonArrayConst wordLen = item["wordLen"].as<JsonArrayConst>();
    if (wordLen.size() == 2) {
      tier.wordLenMin = wordLen[0].as<uint8_t>();
      tier.wordLenMax = wordLen[1].as<uint8_t>();
    }
    for (JsonVariantConst pos : item["blankPositions"].as<JsonArrayConst>()) {
      const char* s = pos | "";
      tier.blankPositions.push_back(s ? s : "");
    }
    JsonArrayConst count = item["count"].as<JsonArrayConst>();
    if (count.size() == 2) {
      tier.countMin = count[0].as<uint8_t>();
      tier.countMax = count[1].as<uint8_t>();
    }
    JsonArrayConst sum = item["sum"].as<JsonArrayConst>();
    if (sum.size() == 2) {
      tier.sumMin = sum[0].as<uint8_t>();
      tier.sumMax = sum[1].as<uint8_t>();
    }
    tier.advanceAt = item["advanceAt"] | 0.75f;
    theme.difficultyTiers.push_back(tier);
  }

  theme.completionPicture.iconId = asString(doc["completionPicture"]["iconId"]);
  theme.completionPicture.label = asString(doc["completionPicture"]["label"]);

  if (theme.themeId.empty()) return {false, {}, "missing themeId"};
  if (theme.wordPool.empty()) return {false, {}, "wordPool must not be empty"};
  if (theme.difficultyTiers.empty()) return {false, {}, "difficultyTiers must not be empty"};
  for (auto& tier : theme.difficultyTiers) {
    if (tier.blankPositions.empty()) tier.blankPositions.push_back("last");
  }

  return {true, theme, ""};
}

}  // namespace puzzle
