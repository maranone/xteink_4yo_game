#include <Arduino.h>
#include <BatteryMonitor.h>
#include <EInkDisplay.h>
#include <InputManager.h>
#include <SDCardManager.h>
#include <SPI.h>
#include <cstdlib>
#include <driver/gpio.h>
#include <esp_random.h>
#include <esp_sleep.h>
#include <esp_system.h>

#include "engine/ChallengeGenerator.h"
#include "engine/FilePaths.h"
#include "engine/PuzzleState.h"
#include "engine/ThemeData.h"
#include "ui/IconAssets.h"
#include "ui/Primitives.h"

using namespace puzzle;
using namespace puzzle::ui;

namespace {

constexpr int8_t EPD_SCLK = 8;
constexpr int8_t SPI_MISO = 7;
constexpr int8_t EPD_MOSI = 10;
constexpr int8_t EPD_CS = 21;
constexpr int8_t EPD_DC = 4;
constexpr int8_t EPD_RST = 5;
constexpr int8_t EPD_BUSY = 6;
constexpr uint8_t BAT_GPIO0 = 0;

EInkDisplay display(EPD_SCLK, EPD_MOSI, EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);
InputManager input;
#ifndef XTEINK_PANEL_X3
BatteryMonitor battery(BAT_GPIO0);
#endif
SDCardManager& sd = SdMan;

ThemeData activeTheme;
PuzzleState state;
Challenge currentChallenge;
Rng rng(1);

bool choiceEliminated[kChoiceCount] = {false, false, false, false};
uint8_t wrongAttemptsThisChallenge = 0;
uint32_t screenDrawCount = 0;
constexpr uint16_t kFullRefreshEvery = 24;  // "every 20-30 screens" per spec, to clear e-ink ghosting

// Kids may pause longer between presses than an idle-game player checking
// in - a more generous timeout than xteink-idle-engine's 20s.
constexpr unsigned long kAwakeTimeoutMs = 120000;
unsigned long gLastActivityTime = 0;
bool gGameLoaded = false;

// The device's button cluster, top to bottom per the printed manual, is
// Next Page / Previous Page / OK / Back - all 4 double as answer choices
// here, in that same physical order, so slot i's on-screen label sits
// directly across from the button that answers it (see drawButtonColumn).
constexpr uint8_t kSlotButtons[kChoiceCount] = {InputManager::BTN_RIGHT, InputManager::BTN_LEFT,
                                                  InputManager::BTN_CONFIRM, InputManager::BTN_BACK};
uint16_t gContentW = 0;       // main content width; the right-edge button gutter is excluded from it
uint16_t gButtonGutterW = 0;  // right-edge column width reserved for the 4 answer-choice labels
uint16_t gButtonColX = 0;     // = gContentW; left edge of the button gutter

uint16_t gImageTop = 0, gImageBottom = 0;
uint16_t gChallengeTop = 0, gChallengeBottom = 0;

// Mirrors assets/themes/animals.json. esptool/serial flashing has no path
// to the SD card (only the running firmware can write to it), so this
// embedded copy self-syncs onto the card on boot whenever its "version"
// differs from what's there - see syncEmbeddedTheme(). Bump "version" here
// whenever assets/themes/animals.json changes, or a reflash won't actually
// refresh the on-card content (same pattern xteink-idle-engine uses for its
// flagship game).
constexpr const char* kEmbeddedAnimalsTheme = R"json({
  "themeId": "animals",
  "themeName": "Animals",
  "version": "1.3.2",
  "wordPool": [
    {
      "word": "CAT",
      "iconId": "cat"
    },
    {
      "word": "DOG",
      "iconId": "dog"
    },
    {
      "word": "PIG",
      "iconId": "pig"
    },
    {
      "word": "COW",
      "iconId": "cow"
    },
    {
      "word": "OWL",
      "iconId": "owl"
    },
    {
      "word": "FOX",
      "iconId": "fox"
    },
    {
      "word": "HEN",
      "iconId": "hen"
    },
    {
      "word": "FROG",
      "iconId": "frog"
    },
    {
      "word": "FISH",
      "iconId": "fish"
    },
    {
      "word": "CAR",
      "iconId": "car"
    },
    {
      "word": "BUS",
      "iconId": "bus"
    },
    {
      "word": "CUP",
      "iconId": "cup"
    },
    {
      "word": "BOOK",
      "iconId": "book"
    },
    {
      "word": "SPOON",
      "iconId": "spoon"
    },
    {
      "word": "CAKE",
      "iconId": "cake"
    },
    {
      "word": "BED",
      "iconId": "bed"
    },
    {
      "word": "HAND",
      "iconId": "hand"
    },
    {
      "word": "BEE",
      "iconId": "bee"
    },
    {
      "word": "BOAT",
      "iconId": "boat"
    },
    {
      "word": "BONE",
      "iconId": "bone"
    },
    {
      "word": "BOWL",
      "iconId": "bowl"
    },
    {
      "word": "BIRD",
      "iconId": "bird"
    },
    {
      "word": "DOLL",
      "iconId": "doll"
    },
    {
      "word": "DUCK",
      "iconId": "duck"
    },
    {
      "word": "EGG",
      "iconId": "egg"
    },
    {
      "word": "FERN",
      "iconId": "fern"
    },
    {
      "word": "GATE",
      "iconId": "gate"
    },
    {
      "word": "GOAT",
      "iconId": "goat"
    },
    {
      "word": "HAM",
      "iconId": "ham"
    },
    {
      "word": "JAR",
      "iconId": "jar"
    },
    {
      "word": "LEAF",
      "iconId": "leaf"
    },
    {
      "word": "MOON",
      "iconId": "moon"
    },
    {
      "word": "MUG",
      "iconId": "mug"
    },
    {
      "word": "PEAR",
      "iconId": "pear"
    },
    {
      "word": "ROSE",
      "iconId": "rose"
    },
    {
      "word": "SHIP",
      "iconId": "ship"
    },
    {
      "word": "SOAP",
      "iconId": "soap"
    },
    {
      "word": "BAT",
      "iconId": "bat"
    },
    {
      "word": "BELL",
      "iconId": "bell"
    },
    {
      "word": "BUN",
      "iconId": "bun"
    },
    {
      "word": "CAP",
      "iconId": "cap"
    },
    {
      "word": "COAT",
      "iconId": "coat"
    },
    {
      "word": "COMB",
      "iconId": "comb"
    },
    {
      "word": "DESK",
      "iconId": "desk"
    },
    {
      "word": "DISH",
      "iconId": "dish"
    },
    {
      "word": "DOOR",
      "iconId": "door"
    },
    {
      "word": "FORK",
      "iconId": "fork"
    },
    {
      "word": "HAT",
      "iconId": "hat"
    },
    {
      "word": "KITE",
      "iconId": "kite"
    },
    {
      "word": "LAMP",
      "iconId": "lamp"
    },
    {
      "word": "MASK",
      "iconId": "mask"
    },
    {
      "word": "NET",
      "iconId": "net"
    },
    {
      "word": "PAN",
      "iconId": "pan"
    },
    {
      "word": "PEN",
      "iconId": "pen"
    },
    {
      "word": "RING",
      "iconId": "ring"
    },
    {
      "word": "ANT",
      "iconId": "ant"
    },
    {
      "word": "AXE",
      "iconId": "axe"
    },
    {
      "word": "BAG",
      "iconId": "bag"
    },
    {
      "word": "BEAN",
      "iconId": "bean"
    },
    {
      "word": "BOOT",
      "iconId": "boot"
    },
    {
      "word": "COIN",
      "iconId": "coin"
    },
    {
      "word": "CORN",
      "iconId": "corn"
    },
    {
      "word": "DRUM",
      "iconId": "drum"
    },
    {
      "word": "ICE",
      "iconId": "ice"
    },
    {
      "word": "KEY",
      "iconId": "key"
    },
    {
      "word": "MILK",
      "iconId": "milk"
    },
    {
      "word": "BEEF",
      "iconId": "beef"
    },
    {
      "word": "CANE",
      "iconId": "cane"
    },
    {
      "word": "BUG",
      "iconId": "bug"
    },
    {
      "word": "FAN",
      "iconId": "fan"
    },
    {
      "word": "FIG",
      "iconId": "fig"
    },
    {
      "word": "JAM",
      "iconId": "jam"
    },
    {
      "word": "LID",
      "iconId": "lid"
    },
    {
      "word": "MAP",
      "iconId": "map"
    },
    {
      "word": "MUD",
      "iconId": "mud"
    },
    {
      "word": "POT",
      "iconId": "pot"
    },
    {
      "word": "NEST",
      "iconId": "nest"
    },
    {
      "word": "PILL",
      "iconId": "pill"
    },
    {
      "word": "APE",
      "iconId": "ape"
    },
    {
      "word": "CRAB",
      "iconId": "crab"
    },
    {
      "word": "DEER",
      "iconId": "deer"
    },
    {
      "word": "DOVE",
      "iconId": "dove"
    },
    {
      "word": "LARK",
      "iconId": "lark"
    },
    {
      "word": "MOTH",
      "iconId": "moth"
    },
    {
      "word": "PRAWN",
      "iconId": "prawn"
    },
    {
      "word": "SEAL",
      "iconId": "seal"
    },
    {
      "word": "WORM",
      "iconId": "worm"
    },
    {
      "word": "PEA",
      "iconId": "pea"
    },
    {
      "word": "GRAPE",
      "iconId": "grape"
    },
    {
      "word": "LEMON",
      "iconId": "lemon"
    },
    {
      "word": "MELON",
      "iconId": "melon"
    },
    {
      "word": "PLUM",
      "iconId": "plum"
    },
    {
      "word": "BREAD",
      "iconId": "bread"
    },
    {
      "word": "RICE",
      "iconId": "rice"
    },
    {
      "word": "PIE",
      "iconId": "pie"
    },
    {
      "word": "TACO",
      "iconId": "taco"
    },
    {
      "word": "KNIFE",
      "iconId": "knife"
    },
    {
      "word": "PLATE",
      "iconId": "plate"
    },
    {
      "word": "MAT",
      "iconId": "mat"
    },
    {
      "word": "BELT",
      "iconId": "belt"
    },
    {
      "word": "GLOVE",
      "iconId": "glove"
    },
    {
      "word": "SHIRT",
      "iconId": "shirt"
    },
    {
      "word": "SKIRT",
      "iconId": "skirt"
    },
    {
      "word": "SOCK",
      "iconId": "sock"
    },
    {
      "word": "VEST",
      "iconId": "vest"
    },
    {
      "word": "PANT",
      "iconId": "pant"
    },
    {
      "word": "CLOAK",
      "iconId": "cloak"
    },
    {
      "word": "SLIP",
      "iconId": "slip"
    },
    {
      "word": "SHOE",
      "iconId": "shoe"
    },
    {
      "word": "SCARF",
      "iconId": "scarf"
    },
    {
      "word": "HOE",
      "iconId": "hoe"
    },
    {
      "word": "SAW",
      "iconId": "saw"
    },
    {
      "word": "ROPE",
      "iconId": "rope"
    },
    {
      "word": "SEED",
      "iconId": "seed"
    },
    {
      "word": "HOSE",
      "iconId": "hose"
    },
    {
      "word": "NAIL",
      "iconId": "nail"
    },
    {
      "word": "PLOW",
      "iconId": "plow"
    },
    {
      "word": "TIGER",
      "iconId": "tiger"
    },
    {
      "word": "ZEBRA",
      "iconId": "zebra"
    },
    {
      "word": "TURTLE",
      "iconId": "turtle"
    },
    {
      "word": "PANDA",
      "iconId": "panda"
    },
    {
      "word": "RABBIT",
      "iconId": "rabbit"
    },
    {
      "word": "PARROT",
      "iconId": "parrot"
    },
    {
      "word": "FLAMINGO",
      "iconId": "flamingo"
    },
    {
      "word": "PENGUIN",
      "iconId": "penguin"
    },
    {
      "word": "SHARK",
      "iconId": "shark"
    },
    {
      "word": "WHALE",
      "iconId": "whale"
    },
    {
      "word": "SPIDER",
      "iconId": "spider"
    },
    {
      "word": "LADYBUG",
      "iconId": "ladybug"
    },
    {
      "word": "BANANA",
      "iconId": "banana"
    },
    {
      "word": "CARROT",
      "iconId": "carrot"
    },
    {
      "word": "CHERRY",
      "iconId": "cherry"
    },
    {
      "word": "CHEESE",
      "iconId": "cheese"
    },
    {
      "word": "CUCUMBER",
      "iconId": "cucumber"
    },
    {
      "word": "GARLIC",
      "iconId": "garlic"
    },
    {
      "word": "KIWI",
      "iconId": "kiwi"
    },
    {
      "word": "MANGO",
      "iconId": "mango"
    },
    {
      "word": "MUSHROOM",
      "iconId": "mushroom"
    },
    {
      "word": "ORANGE",
      "iconId": "orange"
    },
    {
      "word": "POTATO",
      "iconId": "potato"
    },
    {
      "word": "BATH",
      "iconId": "bath"
    },
    {
      "word": "BRUSH",
      "iconId": "brush"
    },
    {
      "word": "CHAIR",
      "iconId": "chair"
    },
    {
      "word": "CLOSET",
      "iconId": "closet"
    },
    {
      "word": "CLOCK",
      "iconId": "clock"
    },
    {
      "word": "KETTLE",
      "iconId": "kettle"
    },
    {
      "word": "MIRROR",
      "iconId": "mirror"
    },
    {
      "word": "SINK",
      "iconId": "sink"
    },
    {
      "word": "TOWEL",
      "iconId": "towel"
    },
    {
      "word": "JACKET",
      "iconId": "jacket"
    },
    {
      "word": "PANTS",
      "iconId": "pants"
    },
    {
      "word": "SHORTS",
      "iconId": "shorts"
    },
    {
      "word": "SNEAKER",
      "iconId": "sneaker"
    },
    {
      "word": "SWEATER",
      "iconId": "sweater"
    },
    {
      "word": "SLIPPER",
      "iconId": "slipper"
    },
    {
      "word": "BERET",
      "iconId": "beret"
    },
    {
      "word": "TIE",
      "iconId": "tie"
    },
    {
      "word": "BEANIE",
      "iconId": "beanie"
    },
    {
      "word": "MITTENS",
      "iconId": "mittens"
    },
    {
      "word": "HAMMER",
      "iconId": "hammer"
    },
    {
      "word": "SHOVEL",
      "iconId": "shovel"
    },
    {
      "word": "WRENCH",
      "iconId": "wrench"
    },
    {
      "word": "DRIL",
      "iconId": "dril"
    },
    {
      "word": "PLIERS",
      "iconId": "pliers"
    },
    {
      "word": "RULER",
      "iconId": "ruler"
    },
    {
      "word": "TROWEL",
      "iconId": "trowel"
    },
    {
      "word": "BUCKET",
      "iconId": "bucket"
    },
    {
      "word": "SICKLE",
      "iconId": "sickle"
    },
    {
      "word": "LANTERN",
      "iconId": "lantern"
    },
    {
      "word": "LEVEL",
      "iconId": "level"
    },
    {
      "word": "SPADE",
      "iconId": "spade"
    },
    {
      "word": "BALLOON",
      "iconId": "balloon"
    },
    {
      "word": "BLOCK",
      "iconId": "block"
    },
    {
      "word": "GUITAR",
      "iconId": "guitar"
    },
    {
      "word": "HARP",
      "iconId": "harp"
    },
    {
      "word": "HORN",
      "iconId": "horn"
    },
    {
      "word": "PIANO",
      "iconId": "piano"
    },
    {
      "word": "PUZZLE",
      "iconId": "puzzle"
    },
    {
      "word": "ROLLER",
      "iconId": "roller"
    },
    {
      "word": "SLIDE",
      "iconId": "slide"
    },
    {
      "word": "TRAIN",
      "iconId": "train"
    },
    {
      "word": "AMBULANCE",
      "iconId": "ambulance"
    },
    {
      "word": "BICYCLE",
      "iconId": "bicycle"
    },
    {
      "word": "CAB",
      "iconId": "cab"
    },
    {
      "word": "JET",
      "iconId": "jet"
    },
    {
      "word": "LORRY",
      "iconId": "lorry"
    },
    {
      "word": "PLANE",
      "iconId": "plane"
    },
    {
      "word": "ROCKET",
      "iconId": "rocket"
    },
    {
      "word": "SCOOTER",
      "iconId": "scooter"
    },
    {
      "word": "SUBMARINE",
      "iconId": "submarine"
    },
    {
      "word": "TRACTOR",
      "iconId": "tractor"
    },
    {
      "word": "EYE",
      "iconId": "eye"
    },
    {
      "word": "NOSE",
      "iconId": "nose"
    },
    {
      "word": "EAR",
      "iconId": "ear"
    },
    {
      "word": "MOUTH",
      "iconId": "mouth"
    },
    {
      "word": "ARM",
      "iconId": "arm"
    },
    {
      "word": "LEG",
      "iconId": "leg"
    },
    {
      "word": "FOOT",
      "iconId": "foot"
    },
    {
      "word": "KNEE",
      "iconId": "knee"
    },
    {
      "word": "ELBOW",
      "iconId": "elbow"
    },
    {
      "word": "TOE",
      "iconId": "toe"
    },
    {
      "word": "CHEEK",
      "iconId": "cheek"
    },
    {
      "word": "NECK",
      "iconId": "neck"
    },
    {
      "word": "CRAYON",
      "iconId": "crayon"
    },
    {
      "word": "GLUE",
      "iconId": "glue"
    },
    {
      "word": "ERASER",
      "iconId": "eraser"
    },
    {
      "word": "PENCIL",
      "iconId": "pencil"
    },
    {
      "word": "SCISSORS",
      "iconId": "scissors"
    },
    {
      "word": "PAPER",
      "iconId": "paper"
    },
    {
      "word": "FOLDER",
      "iconId": "folder"
    },
    {
      "word": "STAPLER",
      "iconId": "stapler"
    },
    {
      "word": "CLIP",
      "iconId": "clip"
    },
    {
      "word": "MARKER",
      "iconId": "marker"
    },
    {
      "word": "COMPASS",
      "iconId": "compass"
    },
    {
      "word": "SHARPENER",
      "iconId": "sharpener"
    },
    {
      "word": "FLOWER",
      "iconId": "flower"
    },
    {
      "word": "CLOUD",
      "iconId": "cloud"
    }
  ],
  "countObjects": [
    {
      "iconId": "apple",
      "label": "Apple"
    },
    {
      "iconId": "star",
      "label": "Star"
    },
    {
      "iconId": "lion",
      "label": "Lion"
    },
    {
      "iconId": "sun",
      "label": "Sun"
    },
    {
      "iconId": "ball",
      "label": "Ball"
    },
    {
      "iconId": "tree",
      "label": "Tree"
    },
    {
      "iconId": "frog",
      "label": "Frog"
    },
    {
      "iconId": "fish",
      "label": "Fish"
    },
    {
      "iconId": "cup",
      "label": "Cup"
    },
    {
      "iconId": "cake",
      "label": "Cake"
    }
  ],
  "difficultyTiers": [
    {
      "wordLen": [
        3,
        255
      ],
      "blankPositions": [
        "first",
        "middle",
        "last"
      ],
      "count": [
        1,
        5
      ],
      "sum": [
        1,
        5
      ],
      "advanceAt": 0.75
    },
    {
      "wordLen": [
        3,
        255
      ],
      "blankPositions": [
        "first",
        "middle",
        "last"
      ],
      "count": [
        1,
        8
      ],
      "sum": [
        2,
        10
      ],
      "advanceAt": 0.85
    }
  ],
  "completionPicture": {
    "iconId": "lion",
    "label": "Lion"
  }
})json";

void computeLayout() {
  // EInkDisplay::displayWindow requires x and w to be byte-aligned (8px) -
  // round the gutter width down to a multiple of 8 so the windowed refresh
  // of just the button column stays valid on both X4 (800px) and X3 (792px).
  gButtonGutterW = static_cast<uint16_t>((gW / 5) / 8 * 8);
  gContentW = static_cast<uint16_t>(gW - gButtonGutterW);
  gButtonColX = gContentW;

  gImageTop = 0;
  // The progress-icon grid was removed. Give the image area most of the
  // content height so count challenges can use native 96x96 images in a
  // multi-row grid without scaling them down.
  gImageBottom = static_cast<uint16_t>(gH * 0.80f);
  gChallengeTop = gImageBottom;
  gChallengeBottom = gH;
}

// Writes via a temp file + rename instead of SDCardManager::writeFile(),
// which deletes the existing file before writing the replacement - a power
// loss mid-write would otherwise corrupt save progress or theme content.
// Ported from xteink-idle-engine's writeFileAtomic.
bool writeFileAtomic(const String& finalPath, const char* content) {
  const String tmpPath = finalPath + ".tmp";
  FsFile f = sd.open(tmpPath.c_str(), O_RDWR | O_CREAT | O_TRUNC);
  if (!f) {
    Serial.printf("[FS] failed to open %s for write\n", tmpPath.c_str());
    return false;
  }
  f.print(content);
  f.close();
  if (sd.exists(finalPath.c_str())) sd.remove(finalPath.c_str());
  if (!sd.rename(tmpPath.c_str(), finalPath.c_str())) {
    Serial.printf("[FS] failed to commit %s\n", finalPath.c_str());
    return false;
  }
  return true;
}

void saveProgress() {
  sd.ensureDirectoryExists(kThemesRoot);
  writeFileAtomic(kStateFile, serializeState(state).c_str());
}

bool shouldResetGameStateFromBootReason() {
  const esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("[PUZZLE] reset reason: %d\n", static_cast<int>(reason));
  return reason == ESP_RST_EXT;
}

void resetGameStateForFreshStart() {
  const std::string themeId = activeTheme.themeId;
  state = {};
  state.currentThemeId = themeId;
  saveProgress();
  Serial.println("[PUZZLE] external reset detected; game achievements/progress cleared");
}

void syncEmbeddedTheme() {
  const auto embedded = parseThemeJson(kEmbeddedAnimalsTheme);
  if (!embedded.ok) return;
  const String dir = String(kThemesRoot) + "/animals";
  const String path = dir + "/theme.json";
  const String onDiskJson = sd.readFile(path.c_str());
  if (!onDiskJson.isEmpty()) {
    const auto onDisk = parseThemeJson(onDiskJson.c_str());
    if (onDisk.ok && onDisk.theme.version == embedded.theme.version) return;
  }
  sd.ensureDirectoryExists(kThemesRoot);
  sd.ensureDirectoryExists(dir.c_str());
  if (writeFileAtomic(path, kEmbeddedAnimalsTheme)) {
    Serial.printf("[THEME] synced embedded animals theme (version %s)\n", embedded.theme.version.c_str());
  }
}

bool loadTheme() {
  const String path = String(kThemesRoot) + "/animals/theme.json";
  const String json = sd.readFile(path.c_str());
  if (!json.isEmpty()) {
    const auto result = parseThemeJson(json.c_str());
    if (result.ok) {
      activeTheme = result.theme;
      return true;
    }
    Serial.printf("[THEME] invalid %s: %s\n", path.c_str(), result.error.c_str());
  }
  const auto embedded = parseThemeJson(kEmbeddedAnimalsTheme);
  if (!embedded.ok) return false;
  activeTheme = embedded.theme;
  return true;
}

// Only generated assets are used. Word-hunt/prize screens use the native
// 192x192 grayscale word asset; count challenges use tiny 32x32 1-bit assets.
void drawThemeIcon(const std::string& iconId, IconAssetVariant variant, int16_t cx, int16_t cy, uint8_t scale,
                   IconRenderPlane plane = IconRenderPlane::kBlackWhite) {
  drawIconAsset(iconId.c_str(), variant, plane, cx, cy, scale);
}

void drawChallengeImages(IconRenderPlane plane) {
  if (currentChallenge.type == Challenge::Type::CountObjects) {
    const uint8_t n = currentChallenge.iconRepeatCount;
    const uint8_t cols = n <= 4 ? n : 4;
    const uint8_t rows = static_cast<uint8_t>((n + cols - 1) / cols);
    constexpr int16_t kCellW = 96;
    constexpr int16_t kCellH = 96;
    const int16_t gridW = static_cast<int16_t>(cols * kCellW);
    const int16_t gridH = static_cast<int16_t>(rows * kCellH);
    const int16_t gridX = static_cast<int16_t>((gContentW - gridW) / 2);
    const int16_t gridY = static_cast<int16_t>((gImageBottom - gridH) / 2);
    for (uint8_t i = 0; i < n; i++) {
      const uint8_t col = static_cast<uint8_t>(i % cols);
      const uint8_t row = static_cast<uint8_t>(i / cols);
      drawThemeIcon(currentChallenge.iconId, IconAssetVariant::kCount,
                    static_cast<int16_t>(gridX + col * kCellW + kCellW / 2),
                    static_cast<int16_t>(gridY + row * kCellH + kCellH / 2), 2, plane);
    }
  } else {
    const int16_t iconCy = static_cast<int16_t>((gImageTop + gImageBottom) / 2);
    drawThemeIcon(currentChallenge.iconId, IconAssetVariant::kWord, gContentW / 2, iconCy, 1, plane);
  }
}

// Image + challenge text band. Redrawn whenever a new challenge is generated.
void drawImageAndChallenge() {
  clearRect(0, gImageTop, gContentW, static_cast<uint16_t>(gChallengeBottom - gImageTop));
  drawChallengeImages(IconRenderPlane::kBlackWhite);

  if (currentChallenge.type == Challenge::Type::WordMissingLetter) {
    // The missing letter is a drawn box, not an underscore - clearer at a
    // glance for a young reader that there's exactly one letter to find,
    // and exactly where in the word it goes.
    uint8_t scale = 7;
    uint16_t prefixW = 0, suffixW = 0, boxW = 0, boxH = 0, gap = 0, totalW = 0;
    do {
      gap = static_cast<uint16_t>(2 * scale);
      prefixW = textWidth(currentChallenge.promptPrefix.c_str(), scale);
      suffixW = textWidth(currentChallenge.promptSuffix.c_str(), scale);
      boxW = static_cast<uint16_t>(5 * scale);
      boxH = static_cast<uint16_t>(7 * scale);
      totalW = static_cast<uint16_t>(prefixW + (prefixW > 0 ? gap : 0) + boxW +
                                     (suffixW > 0 ? gap : 0) + suffixW);
      if (totalW <= gContentW - 20 || scale == 2) break;
      scale--;
    } while (true);
    const uint16_t textY = static_cast<uint16_t>(
        gChallengeTop + (gChallengeBottom - gChallengeTop - boxH) / 2);
    uint16_t x = static_cast<uint16_t>(gContentW / 2 - totalW / 2);

    if (prefixW > 0) {
      drawText(x, textY, currentChallenge.promptPrefix.c_str(), scale);
      x = static_cast<uint16_t>(x + prefixW + gap);
    }
    for (uint16_t xx = 0; xx < boxW; xx++) {
      setPixel(static_cast<uint16_t>(x + xx), textY, true);
      setPixel(static_cast<uint16_t>(x + xx), static_cast<uint16_t>(textY + boxH - 1), true);
    }
    for (uint16_t yy = 0; yy < boxH; yy++) {
      setPixel(x, static_cast<uint16_t>(textY + yy), true);
      setPixel(static_cast<uint16_t>(x + boxW - 1), static_cast<uint16_t>(textY + yy), true);
    }
    if (suffixW > 0) {
      x = static_cast<uint16_t>(x + boxW + gap);
      drawText(x, textY, currentChallenge.promptSuffix.c_str(), scale);
    }
  } else {
    constexpr uint8_t scale = 5;
    const uint16_t textY = static_cast<uint16_t>(
        gChallengeTop + (gChallengeBottom - gChallengeTop) / 2 - 17);
    const char* msg = "HOW MANY?";
    const uint16_t tw = textWidth(msg, scale);
    drawText(static_cast<uint16_t>(gContentW / 2 - tw / 2), textY, msg, scale);
  }
}

// Vertical button-label column along the right edge, one slot per physical
// button (top to bottom - see kSlotButtons), positioned so each label sits
// right across from the real button that answers it. No key-name caption
// (NEXT/PREV/OK/BACK) needed once the physical buttons and on-screen slots
// line up - the position itself is the label. This is what gets a small
// windowed refresh after a wrong-answer elimination - the image/challenge
// text/progress bar are all untouched in the framebuffer and don't need
// redrawing.
void drawButtonColumn() {
  clearRect(gButtonColX, 0, gButtonGutterW, gH);
  drawVLine(gButtonColX, 0, gH);
  const uint16_t slotH = static_cast<uint16_t>(gH / kChoiceCount);
  constexpr uint8_t choiceScale = 5;
  for (uint8_t slot = 0; slot < kChoiceCount; slot++) {
    const uint16_t slotTop = static_cast<uint16_t>(slot * slotH);
    if (slot > 0) {
      // Divider confined to the button gutter only - drawHLine spans the
      // full screen width, which would cut across the content area too.
      for (uint16_t x = gButtonColX; x < gW; x++) setPixel(x, slotTop, true);
    }

    if (choiceEliminated[slot]) continue;
    const String label = "[" + String(currentChallenge.choices[slot].c_str()) + "]";
    const uint16_t labelW = textWidth(label.c_str(), choiceScale);
    drawText(static_cast<uint16_t>(gButtonColX + (gButtonGutterW - labelW) / 2),
              static_cast<uint16_t>(slotTop + slotH / 2 - 17), label.c_str(), choiceScale);
  }
}

bool shouldFullRefresh() {
  return (screenDrawCount % kFullRefreshEvery) == 0;
}

void drawChallengeFrame() {
  display.clearScreen(0xFF);
  drawImageAndChallenge();
  drawButtonColumn();
}

void applyChallengeGrayscale() {
  display.clearScreen(0x00);
  drawChallengeImages(IconRenderPlane::kGrayLsb);
  display.copyGrayscaleLsbBuffers(display.getFrameBuffer());

  display.clearScreen(0x00);
  drawChallengeImages(IconRenderPlane::kGrayMsb);
  display.copyGrayscaleMsbBuffers(display.getFrameBuffer());
  display.displayGrayBuffer(true);

  // Grayscale reuses the single framebuffer. Rebuild the normal BW frame and
  // rebase both controller RAM planes so later button-only updates stay valid.
  drawChallengeFrame();
  display.cleanupGrayscaleBuffers(display.getFrameBuffer());
}

// New challenge generated: image, challenge text, and buttons redraw together.
void renderChallenge() {
  drawChallengeFrame();
  const bool grayscale = currentChallenge.type == Challenge::Type::WordMissingLetter &&
                         hasGrayscaleIconAsset(currentChallenge.iconId.c_str());
  display.displayBuffer(shouldFullRefresh() ? EInkDisplay::FULL_REFRESH : EInkDisplay::HALF_REFRESH,
                        !grayscale);
  if (grayscale) applyChallengeGrayscale();
  screenDrawCount++;
}

// Wrong-answer elimination used to redraw only the button column via a
// windowed partial refresh. That interacts poorly with the grayscale overlay
// state on this panel and can invert/blacken the main content area. Redraw the
// full current challenge instead; the eliminated answer remains hidden because
// choiceEliminated[] is already updated before this is called.
void renderButtonsOnly() {
  renderChallenge();
}

void drawPictureCompletedIcon(const std::string& prizeIconId, IconRenderPlane plane) {
  const int16_t cx = gW / 2;
  const int16_t cy = static_cast<int16_t>(gH / 2 - gH / 10);
  drawThemeIcon(prizeIconId, IconAssetVariant::kWord, cx, cy, 1, plane);
}

void drawPictureCompletedFrame(const std::string& prizeIconId) {
  display.clearScreen(0xFF);
  const int16_t cx = gW / 2;
  const int16_t cy = static_cast<int16_t>(gH / 2 - gH / 10);
  const int16_t iconSize = static_cast<int16_t>(gH / 5);
  drawPictureCompletedIcon(prizeIconId, IconRenderPlane::kBlackWhite);
  constexpr uint8_t scale = 5;
  const char* msg = "PRIZE WON!";
  const uint16_t tw = textWidth(msg, scale);
  drawText(static_cast<uint16_t>(gW / 2 - tw / 2), static_cast<uint16_t>(cy + iconSize + 30), msg, scale);
}

// The prize is whichever icon was being practiced on the challenge that
// just completed the picture - not a fixed per-theme mascot, so it varies.
void renderPictureCompleted(const std::string& prizeIconId) {
  drawPictureCompletedFrame(prizeIconId);
  const bool grayscale = hasGrayscaleIconAsset(prizeIconId.c_str());
  display.displayBuffer(EInkDisplay::FULL_REFRESH, !grayscale);
  if (grayscale) {
    display.clearScreen(0x00);
    drawPictureCompletedIcon(prizeIconId, IconRenderPlane::kGrayLsb);
    display.copyGrayscaleLsbBuffers(display.getFrameBuffer());
    display.clearScreen(0x00);
    drawPictureCompletedIcon(prizeIconId, IconRenderPlane::kGrayMsb);
    display.copyGrayscaleMsbBuffers(display.getFrameBuffer());
    display.displayGrayBuffer(true);
    drawPictureCompletedFrame(prizeIconId);
    display.cleanupGrayscaleBuffers(display.getFrameBuffer());
  }
  screenDrawCount++;
  delay(2500);  // let the celebration actually be seen before the next challenge overwrites it
}

// Shown full-screen right before sleeping, and left on the panel for the
// whole nap (e-ink keeps whatever was last drawn). Rather than a plain
// "Sleeping..." message, this is a trophy shelf of every picture won so
// far (one icon per completedPictures entry) - a kid glancing at a parked
// device sees their collection, not a blank status word.
void drawStatusIcons(IconRenderPlane plane) {
  const size_t count = state.completedPictures.size();
  if (count == 0) return;
  constexpr uint16_t kCols = 5;
  const uint16_t topY = 90;
  const uint16_t bottomY = static_cast<uint16_t>(gH - 50);
  const uint16_t rows = static_cast<uint16_t>((count + kCols - 1) / kCols);
  const uint16_t cellW = gW / kCols;
  const uint16_t cellH = static_cast<uint16_t>((bottomY - topY) / rows);
  for (size_t i = 0; i < count; i++) {
    const uint16_t col = static_cast<uint16_t>(i % kCols);
    const uint16_t row = static_cast<uint16_t>(i / kCols);
    const int16_t cx = static_cast<int16_t>(col * cellW + cellW / 2);
    const int16_t cy = static_cast<int16_t>(topY + row * cellH + cellH / 2);
    drawThemeIcon(state.completedPictures[i], IconAssetVariant::kCount, cx, cy, 2, plane);
  }
}

void drawStatusFrame() {
  display.clearScreen(0xFF);
  constexpr uint8_t titleScale = 4;
  const char* title = "MY PRIZES";
  const uint16_t titleW = textWidth(title, titleScale);
  drawText(static_cast<uint16_t>(gW / 2 - titleW / 2), 24, title, titleScale);
  drawHLine(static_cast<uint16_t>(70), 2);

  const size_t count = state.completedPictures.size();
  if (count == 0) {
    constexpr uint8_t scale = 3;
    const char* msg = "KEEP PLAYING TO WIN PRIZES!";
    const uint16_t tw = textWidth(msg, scale);
    drawText(static_cast<uint16_t>(gW / 2 - tw / 2), static_cast<uint16_t>(gH / 2 - 12), msg, scale);
  } else {
    drawStatusIcons(IconRenderPlane::kBlackWhite);
  }

  char footer[40];
  snprintf(footer, sizeof(footer), "%u PRIZE%s WON", static_cast<unsigned>(count), count == 1 ? "" : "S");
  constexpr uint8_t footerScale = 3;
  const uint16_t footerW = textWidth(footer, footerScale);
  drawText(static_cast<uint16_t>(gW / 2 - footerW / 2), static_cast<uint16_t>(gH - 36), footer, footerScale);
}

void renderStatusScreen() {
  drawStatusFrame();
  display.displayBuffer(EInkDisplay::FULL_REFRESH, true);
  screenDrawCount++;
}

void newChallenge() {
  currentChallenge = generateChallenge(activeTheme, state.difficultyTier, rng);
  wrongAttemptsThisChallenge = 0;
  for (uint8_t i = 0; i < kChoiceCount; i++) choiceEliminated[i] = false;
  renderChallenge();
}

// Reward rule: every correct answer is exactly 1 piece, regardless of how
// many wrong presses it took - simple one-by-one advancement. (A variable
// 4/3/2/1 reward was tried and dropped: stamping several identical-icon
// slots at once from a single answer made the 12-piece grid look like it
// wasn't counting correctly.) Always progress, by construction (a wrong
// press eliminates that choice, so the last remaining button is always
// correct) - wrongAttemptsThisChallenge still feeds the rolling success
// window below, it just no longer scales the reward itself. Difficulty
// tier only ever advances (one-way ratchet) once the rolling correct-on-
// first-try rate crosses the current tier's threshold.
void onCorrectAnswer() {
  const uint8_t slot = state.currentPictureProgress;
  state.currentPieceIcons[slot] = currentChallenge.iconId;
  state.currentPictureProgress = static_cast<uint8_t>(slot + 1);
  recordOutcome(state, wrongAttemptsThisChallenge);

  if (static_cast<size_t>(state.difficultyTier) + 1 < activeTheme.difficultyTiers.size() &&
      rollingSuccessRate(state) >= activeTheme.difficultyTiers[state.difficultyTier].advanceAt) {
    state.difficultyTier++;
  }

  if (state.currentPictureProgress >= kPiecesPerPicture) {
    const std::string prizeIconId = currentChallenge.iconId;
    addPrize(state, prizeIconId);
    state.currentPictureProgress = 0;
    for (auto& icon : state.currentPieceIcons) icon.clear();
    saveProgress();
    renderPictureCompleted(prizeIconId);
  } else {
    saveProgress();
  }
  newChallenge();
}

void onWrongAnswer(uint8_t choiceIndex) {
  choiceEliminated[choiceIndex] = true;
  wrongAttemptsThisChallenge++;
  renderButtonsOnly();
}

void handleChoicePress(uint8_t choiceIndex) {
  if (choiceEliminated[choiceIndex]) return;  // already removed; ignore a stray press on it
  if (choiceIndex == currentChallenge.correctChoiceIndex) {
    onCorrectAnswer();
  } else {
    onWrongAnswer(choiceIndex);
  }
}

// GPIO13 is wired as a battery-latch MOSFET on this exact hardware (ported
// from crosspoint-reader's HalPowerManager::startDeepSleep, confirmed
// against this same Xteink X4 board in the sibling xteink-idle-engine
// project): on battery power "sleep" physically cuts MCU+SD power rather
// than soft-sleeping, and the Power button briefly re-applies it - without
// holding this latch and arming a GPIO wakeup, Power-button "wake" is an
// uncontrolled hard reset rather than a clean resume. Unlike the idle
// engine, there's no timer wakeup here - this game has no offline/idle
// simulation, so only the Power button should ever wake it.
void sleepNow() {
  renderStatusScreen();
  saveProgress();

  while (input.isPressed(InputManager::BTN_POWER)) {
    delay(50);
    input.update();
  }

  constexpr gpio_num_t kBatteryLatchPin = GPIO_NUM_13;
  gpio_set_direction(kBatteryLatchPin, GPIO_MODE_OUTPUT);
  gpio_set_level(kBatteryLatchPin, 0);
  esp_sleep_config_gpio_isolate();
  gpio_deep_sleep_hold_en();
  gpio_hold_en(kBatteryLatchPin);

  pinMode(InputManager::POWER_BUTTON_PIN, INPUT_PULLUP);
  esp_deep_sleep_enable_gpio_wakeup(1ULL << InputManager::POWER_BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}

bool handleButtons() {
  input.update();
  for (uint8_t slot = 0; slot < kChoiceCount; slot++) {
    if (input.wasPressed(kSlotButtons[slot])) handleChoicePress(slot);
  }
  if (input.wasPressed(InputManager::BTN_POWER)) sleepNow();  // does not return
  return input.wasAnyPressed() || input.wasAnyReleased();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.printf("\n[PUZZLE] Xteink Puzzle Game %s\n", PUZZLE_GAME_VERSION);
  input.begin();
#ifdef XTEINK_PANEL_X3
  display.setDisplayX3();
#else
  pinMode(BAT_GPIO0, INPUT);
#endif
  SPI.begin(EPD_SCLK, SPI_MISO, EPD_MOSI, EPD_CS);
  display.begin();
  initPrimitives(display);
  initIconAssets(display);
  computeLayout();
  rng.reseed(esp_random());

  const bool sdReady = sd.begin();
  if (!sdReady) {
    const auto embedded = parseThemeJson(kEmbeddedAnimalsTheme);
    if (!embedded.ok) {
      display.clearScreen(0xFF);
      drawText(gW / 40, gH / 2, "SD card not ready", 4);
      display.displayBuffer(EInkDisplay::FULL_REFRESH, true);
      return;
    }
    activeTheme = embedded.theme;
    gGameLoaded = true;
    newChallenge();
    gLastActivityTime = millis();
    return;
  }

  sd.ensureDirectoryExists(kThemesRoot);
  syncEmbeddedTheme();
  if (!loadTheme()) {
    display.clearScreen(0xFF);
    drawText(gW / 40, gH / 2, "No theme data", 4);
    display.displayBuffer(EInkDisplay::FULL_REFRESH, true);
    return;
  }

  const String stateJson = sd.readFile(kStateFile);
  std::string err;
  if (!stateJson.isEmpty()) parseStateJson(stateJson.c_str(), state, err);
  if (state.currentThemeId.empty()) state.currentThemeId = activeTheme.themeId;
  if (shouldResetGameStateFromBootReason()) resetGameStateForFreshStart();

  gGameLoaded = true;
  newChallenge();
  gLastActivityTime = millis();
}

void loop() {
  if (handleButtons()) gLastActivityTime = millis();
  if (gGameLoaded && millis() - gLastActivityTime >= kAwakeTimeoutMs) {
    sleepNow();
  }
  delay(20);
}
