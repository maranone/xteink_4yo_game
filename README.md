# Xteink Puzzle Game

A kids' learning game for the Xteink X4/X3 e-ink device. Solve small word/number challenges to earn puzzle pieces; complete a 12-piece picture to unlock the next one.

See [INSTALL.md](INSTALL.md) for build/flash instructions - same hardware and toolchain as the sibling `xteink-idle-engine` project, but this is a separate firmware (one device, one firmware at a time).

## How it plays

- A challenge shows: a procedurally-drawn icon, and either a word with one letter blanked out (`CA_`) or a "how many?" count of icons.
- Three buttons (`LEFT` / `CONFIRM` / `RIGHT`) each show one answer choice, one correct and two wrong, shuffled every round.
- A wrong press **removes that choice** rather than ending the round - by the third remaining button, only the correct answer is left, so progress is always guaranteed, never lost.
- Correct on the 1st/2nd/3rd try earns 3/2/1 puzzle pieces respectively.
- 12 pieces completes the picture (a "Lion Completed!"-style banner), adds it to the permanent collection, and starts a fresh picture.
- Difficulty increases automatically (longer words, wider count ranges) once the rolling correct-on-first-try rate is high enough - it never decreases.

## What's reused from `xteink-idle-engine`

This is a separate codebase, not a shared library, but it deliberately mirrors several patterns already proven on this hardware:
- The 1-bit drawing primitives and bitmap font (`src/ui/Primitives.cpp`).
- The windowed-partial-refresh vs. periodic-full-refresh pattern: a wrong-answer elimination only repaints the button row via `EInkDisplay::displayWindow`; a new challenge repaints the whole screen; full ghost-busting `FULL_REFRESH` happens periodically and on picture/theme completion.
- The atomic temp-file + rename write pattern (`writeFileAtomic` in `src/main.cpp`), since `SDCardManager::writeFile()`'s delete-then-write is corruption-prone on power loss.
- The GPIO13 battery-latch deep-sleep sequence, required on this exact hardware for the Power button to cleanly sleep/wake instead of behaving like an uncontrolled hard reset.
- An embedded copy of the active theme's JSON that self-syncs onto the SD card on boot (version-compared), so a plain firmware reflash over the COM port also refreshes game content without a separate upload step.

Unlike the idle engine, there's no offline/idle simulation here (it's an active play session, not a background-progress game), so sleeping only arms a Power-button wakeup - no timer wakeup.

## Status

MVP scope: one theme (Animals - `CAT`/`DOG`/`PIG`, completion picture `Lion`), word missing-letter and count-objects challenge types only. Farm/Vehicles/Dinosaurs/Sea Animals/Space, and the missing-number/addition/subtraction challenge types, are designed for (see the content schema in `assets/themes/animals.json` and `src/engine/ChallengeGenerator.cpp`) but not yet authored/implemented - adding a theme means writing one more `theme.json` and a handful of new icons in `src/ui/IconLibrary.cpp`, no engine changes needed.
