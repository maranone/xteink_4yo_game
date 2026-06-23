"""Generate a resumable vocabulary/image batch using Ollama and local SDXL.

Default use (via generate_100_images.bat):
  python tools/generate_word_collection.py

An active plan is always resumed. After it completes, pass --new-batch to
archive it and create another batch. Source PNGs, packed grayscale assets,
the generated C++ header, theme JSON, embedded theme, and firmware build are
all produced locally. Nothing is copied to the SD card.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import time
import warnings
import zlib
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
SOURCE_DIR = ROOT / "assets" / "icon_sources"
THEME_PATH = ROOT / "assets" / "themes" / "animals.json"
MAIN_CPP = ROOT / "src" / "main.cpp"
PLAN_PATH = ROOT / "assets" / "icon_generation_plan.json"
HISTORY_DIR = ROOT / "assets" / "icon_generation_history"
LAST_OLLAMA_RESPONSE = ROOT / "assets" / "ollama_last_vocabulary_response.json"
DEFAULT_EBOOKCREATOR = Path.home() / "Documents" / "ebookcreator"
DEFAULT_OLLAMA_MODEL = "gemma4-local:26b-a4b-it-q4_k_m"

CATEGORY_PASSES = (
    "animals, birds, fish, and insects",
    "fruit, vegetables, and simple foods",
    "household objects and kitchen objects",
    "clothes and wearable items",
    "tools, garden objects, and workshop objects",
    "toys, games, and musical instruments",
    "vehicles and transport objects",
    "body parts that can be shown alone in an educational photograph",
    "school, desk, and craft objects",
    "plants, weather objects, and simple natural objects",
    "farm objects and pet-related objects",
    "bathroom, bedroom, and cleaning objects",
)

SYSTEM_PROMPT = """You curate vocabulary for a young child's word-learning game.
Return strict JSON only. Choose visually concrete, common, age-appropriate nouns
that can be represented by one unmistakable isolated photograph. Avoid abstract
ideas, verbs, adjectives, brands, people, weapons, frightening subjects, duplicate
meanings, homographs, and words whose image would require a scene or multiple
objects. Every word must be singular, alphabetic, at least 3 letters long, and
must obey any maximum length specified by the user prompt."""

NEGATIVE_PROMPT = (
    "multiple subjects, several objects, person, people, face, portrait, character, mascot, "
    "scene, room, interior, landscape, background clutter, text, letters, watermark, logo, "
    "blurry, illustration, comic, anime, drawing, sketch, icon, emoji, line art"
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--count", type=int, default=100, help="words in a newly created batch (default: 100)")
    parser.add_argument("--new-batch", action="store_true", help="archive a completed plan and create another batch")
    parser.add_argument(
        "--no-word-length-cap",
        action="store_true",
        help="allow child-friendly words of any length (minimum remains 3 letters)",
    )
    parser.add_argument(
        "--ollama-model",
        default=DEFAULT_OLLAMA_MODEL,
        help=f"Ollama model for vocabulary planning (default: {DEFAULT_OLLAMA_MODEL})",
    )
    parser.add_argument("--ebookcreator", type=Path, default=DEFAULT_EBOOKCREATOR)
    parser.add_argument("--skip-build", action="store_true", help="prepare/bundle assets but do not compile firmware")
    args = parser.parse_args()
    if args.count < 1 or args.count > 500:
        parser.error("--count must be between 1 and 500")
    return args


def run(command: list[str]) -> None:
    print(f"\n> {' '.join(command)}", flush=True)
    subprocess.run(command, cwd=ROOT, check=True)


def load_theme() -> dict[str, Any]:
    return json.loads(THEME_PATH.read_text(encoding="utf-8"))


def existing_ids(theme: dict[str, Any]) -> set[str]:
    ids = {str(item["iconId"]).lower() for item in theme.get("wordPool", [])}
    ids.update(str(item["iconId"]).lower() for item in theme.get("countObjects", []))
    ids.update(path.stem.lower() for path in SOURCE_DIR.glob("*.png"))
    return ids


def validate_candidate(
    item: Any, used: set[str], no_word_length_cap: bool
) -> dict[str, str] | None:
    if not isinstance(item, dict):
        return None
    word = str(item.get("word", "")).strip().upper()
    pattern = r"[A-Z]{3,}" if no_word_length_cap else r"[A-Z]{3,5}"
    if not re.fullmatch(pattern, word):
        return None
    icon_id = word.lower()
    if icon_id in used:
        return None
    description = " ".join(str(item.get("description", "")).split())
    if not description:
        description = f"one single {word.lower()}"
    return {"word": word, "iconId": icon_id, "description": description}


def request_candidates(
    client: Any,
    needed: int,
    used: set[str],
    pass_number: int,
    no_word_length_cap: bool,
) -> list[dict[str, str]]:
    # Small responses are more reliable in Ollama JSON mode and checkpoint
    # quickly. A 100-word collection is intentionally built in several passes.
    request_count = min(needed, 12)
    category = CATEGORY_PASSES[(pass_number - 1) % len(CATEGORY_PASSES)]
    avoid = ", ".join(sorted(used))
    length_rule = (
        "Words must have at least 3 letters. There is no maximum length; include useful longer words."
        if no_word_length_cap
        else "Every word must contain exactly 3 to 5 letters."
    )
    validation_regex = "^[A-Z]{3,}$" if no_word_length_cap else "^[A-Z]{3,5}$"
    prompt = f"""Propose {request_count} new vocabulary entries.
Focus this pass on: {category}.
Do not use any of these existing words or IDs: {avoid}
{length_rule}

Return exactly this JSON shape:
{{"words":[{{"word":"PEAR","description":"one whole ripe pear with its stem"}}]}}

The description must specify exactly one centered subject for a clean studio
product photograph. Keep it literal and short. Do not put style instructions
in the description.

Before returning JSON, silently verify every word against {validation_regex}.
Reject your own candidate if it does not match that expression."""
    result = client.generate_json(SYSTEM_PROMPT, prompt, max_retries=2, max_tokens=4096)
    LAST_OLLAMA_RESPONSE.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    raw_words = result.get("words", []) if isinstance(result, dict) else []
    accepted: list[dict[str, str]] = []
    rejected: list[str] = []
    for raw in raw_words:
        candidate = validate_candidate(raw, used, no_word_length_cap)
        if candidate is None:
            if isinstance(raw, dict):
                rejected.append(str(raw.get("word", "<missing>")))
            else:
                rejected.append("<non-object>")
            continue
        accepted.append(candidate)
        used.add(candidate["iconId"])
        if len(accepted) >= needed:
            break
    if rejected:
        print(f"  rejected by length/duplicate validation: {', '.join(rejected[:12])}", flush=True)
    return accepted


def save_plan(plan: dict[str, Any]) -> None:
    PLAN_PATH.write_text(json.dumps(plan, indent=2) + "\n", encoding="utf-8")


def continue_planning(plan: dict[str, Any], client: Any) -> dict[str, Any]:
    target = int(plan["requestedCount"])
    entries = plan.setdefault("entries", [])
    used = existing_ids(load_theme())
    used.update(str(entry["iconId"]).lower() for entry in entries)
    no_word_length_cap = bool(plan.get("noWordLengthCap", False))
    attempts = 0
    while len(entries) < target:
        attempts += 1
        if attempts > 60:
            raise RuntimeError(f"Ollama supplied only {len(entries)} valid unique words after 60 attempts")
        category = CATEGORY_PASSES[(attempts - 1) % len(CATEGORY_PASSES)]
        print(
            f"\nOllama vocabulary pass: {len(entries)}/{target} accepted "
            f"(focus: {category})",
            flush=True,
        )
        try:
            accepted = request_candidates(
                client, target - len(entries), used, attempts, no_word_length_cap
            )
        except ValueError as exc:
            # The Ollama client has already logged the raw response. Keep the
            # checkpoint and retry a fresh small pass rather than losing work.
            print(f"Ollama pass failed ({exc}); checkpoint retained, retrying.", flush=True)
            continue
        if not accepted:
            print("Ollama pass contained no valid new words; retrying.", flush=True)
            continue
        entries.extend(accepted)
        plan["ollamaModel"] = client.model
        save_plan(plan)
        print(f"Checkpoint saved: {len(entries)}/{target} words", flush=True)

    plan["status"] = "generating"
    save_plan(plan)
    return plan


def create_plan(args: argparse.Namespace, client: Any) -> dict[str, Any]:
    plan = {
        "version": 1,
        "createdAt": time.strftime("%Y-%m-%dT%H:%M:%S"),
        "requestedCount": args.count,
        "ollamaModel": client.model,
        "noWordLengthCap": args.no_word_length_cap,
        "status": "planning",
        "entries": [],
    }
    print(f"Saved generation plan: {PLAN_PATH}", flush=True)
    save_plan(plan)
    return continue_planning(plan, client)


def archive_completed_plan() -> None:
    if not PLAN_PATH.exists():
        return
    plan = json.loads(PLAN_PATH.read_text(encoding="utf-8"))
    if plan.get("status") != "complete":
        print("An unfinished generation plan already exists; resuming it instead of starting another.", flush=True)
        return
    HISTORY_DIR.mkdir(parents=True, exist_ok=True)
    stamp = re.sub(r"[^0-9]", "", str(plan.get("createdAt", ""))) or str(int(time.time()))
    destination = HISTORY_DIR / f"icon_generation_plan_{stamp}.json"
    PLAN_PATH.replace(destination)
    print(f"Archived completed plan to {destination}", flush=True)


def product_prompt(entry: dict[str, str]) -> str:
    return (
        f"clean studio product photograph of {entry['description']}, seamless plain white background, "
        "exactly one subject, centered, fully visible, large in frame, generous even padding, "
        "soft even studio lighting, recognizable silhouette, nothing else in frame"
    )


def generate_sources(plan: dict[str, Any], pipe: Any, settings: Any) -> None:
    SOURCE_DIR.mkdir(parents=True, exist_ok=True)
    entries = plan["entries"]
    total = len(entries)
    for index, entry in enumerate(entries, 1):
        destination = SOURCE_DIR / f"{entry['iconId']}.png"
        if destination.exists() and destination.stat().st_size > 0:
            print(f"[{index}/{total}] skip existing {destination.name}", flush=True)
            continue
        seed = zlib.crc32(f"puzzle_photo:{entry['iconId']}".encode("utf-8")) % (2**31)
        print(f"[{index}/{total}] SDXL generating {entry['word']} (seed={seed})", flush=True)
        image = pipe.generate(
            positive=product_prompt(entry),
            negative=NEGATIVE_PROMPT,
            seed=seed,
            steps=settings.SDXL_STEPS,
            cfg=6.5,
            width=settings.SDXL_GEN_WIDTH,
            height=settings.SDXL_GEN_HEIGHT,
        )
        image.save(destination)
        print(f"  saved {destination}", flush=True)


def bump_patch(version: str) -> str:
    match = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", version)
    if not match:
        return "1.0.1"
    major, minor, patch = map(int, match.groups())
    return f"{major}.{minor}.{patch + 1}"


def update_theme_and_embedded_copy(plan: dict[str, Any]) -> None:
    theme = load_theme()
    present = {str(item["iconId"]).lower() for item in theme.get("wordPool", [])}
    added = 0
    for entry in plan["entries"]:
        if entry["iconId"] in present:
            continue
        theme.setdefault("wordPool", []).append({"word": entry["word"], "iconId": entry["iconId"]})
        present.add(entry["iconId"])
        added += 1
    tier_changed = False
    if plan.get("noWordLengthCap"):
        for tier in theme.get("difficultyTiers", []):
            word_len = tier.get("wordLen")
            if isinstance(word_len, list) and len(word_len) == 2:
                if word_len[1] != 255:
                    word_len[1] = 255
                    tier_changed = True
    if added or tier_changed:
        theme["version"] = bump_patch(str(theme.get("version", "1.0.0")))
        json_text = json.dumps(theme, indent=2)
        THEME_PATH.write_text(json_text + "\n", encoding="utf-8")

        cpp = MAIN_CPP.read_text(encoding="utf-8")
        marker = 'constexpr const char* kEmbeddedAnimalsTheme = R"json('
        start = cpp.index(marker) + len(marker)
        end = cpp.index(')json";', start)
        MAIN_CPP.write_text(cpp[:start] + json_text + cpp[end:], encoding="utf-8", newline="\n")
        print(
            f"Added {added} words to theme; version is now {theme['version']}",
            flush=True,
        )
    else:
        print("Theme already contains every planned word", flush=True)


def main() -> None:
    args = parse_args()
    ebookcreator = args.ebookcreator.resolve()
    if not (ebookcreator / "config.py").is_file() or not (ebookcreator / "sdxl" / "pipeline.py").is_file():
        raise FileNotFoundError(f"ebookcreator SDXL project not found at {ebookcreator}")
    sys.path.insert(0, str(ebookcreator))

    # This workstation currently has a harmless requests dependency-version
    # warning. It does not affect Ollama calls and would otherwise obscure the
    # long-running batch's useful progress output.
    warnings.filterwarnings("ignore", message=r"urllib3 .* doesn't match a supported version.*")

    import config as settings  # type: ignore
    from llm.ollama_client import OllamaClient  # type: ignore

    class NoThinkOllamaClient(OllamaClient):
        """Ollama JSON client that explicitly disables the model's thinking channel.

        gpt-oss and Gemma reasoning models can spend the entire num_predict
        budget on hidden thinking and return an empty message. The Ollama chat
        API's think=false option makes that budget available to the JSON answer.
        """

        def _call(self, system_prompt: str, user_prompt: str, max_tokens: int = 4096) -> str:
            import requests

            payload = {
                "model": self.model,
                "messages": [
                    {"role": "system", "content": system_prompt},
                    {"role": "user", "content": user_prompt},
                ],
                "format": "json",
                "think": False,
                "stream": False,
                "options": {"num_predict": max_tokens, "num_ctx": settings.OLLAMA_NUM_CTX},
            }
            started = time.time()
            response = requests.post(self.endpoint, json=payload, timeout=self.timeout)
            response.raise_for_status()
            data = response.json()
            content = str(data.get("message", {}).get("content", ""))
            elapsed = time.time() - started
            print(f"  done: {len(content)} output chars in {elapsed:.0f}s (thinking disabled)", flush=True)
            return content

    if args.new_batch:
        archive_completed_plan()

    client = NoThinkOllamaClient(model=args.ollama_model)
    if PLAN_PATH.exists():
        plan = json.loads(PLAN_PATH.read_text(encoding="utf-8"))
        print(f"Resuming plan with {len(plan.get('entries', []))} entries: {PLAN_PATH}", flush=True)
        if plan.get("status") == "planning":
            plan["ollamaModel"] = client.model
            save_plan(plan)
            plan = continue_planning(plan, client)
    else:
        plan = create_plan(args, client)

    if plan.get("status") == "complete":
        print("This plan is already complete. Use --new-batch to create another collection.", flush=True)
        return

    missing_sources = [
        entry
        for entry in plan["entries"]
        if not (SOURCE_DIR / f"{entry['iconId']}.png").is_file()
    ]
    if missing_sources:
        # Ollama and SDXL compete for VRAM. Explicitly unload Ollama before loading SDXL.
        client.unload_model()
        from sdxl.pipeline import SDXLPipeline  # type: ignore

        print(f"Loading local SDXL pipeline: {settings.SDXL_MODEL_ID}", flush=True)
        pipe = SDXLPipeline(settings.SDXL_MODEL_ID, settings.SDXL_DEVICE, settings.SDXL_DTYPE)
        generate_sources(plan, pipe, settings)
        del pipe
    else:
        print("All planned source PNGs already exist; skipping SDXL load.", flush=True)

    run([sys.executable, "tools/prepare_icons.py"])
    run(["py", "-3.10", "tools/bundle_icons.py"])
    update_theme_and_embedded_copy(plan)
    if not args.skip_build:
        run(["py", "-3.10", "-m", "platformio", "run"])

    plan["status"] = "complete"
    plan["completedAt"] = time.strftime("%Y-%m-%dT%H:%M:%S")
    save_plan(plan)
    print("\nCollection complete. All sources, grayscale assets, theme data, and firmware are ready.", flush=True)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nInterrupted. Re-run the batch file to resume.", file=sys.stderr)
        raise SystemExit(130)
