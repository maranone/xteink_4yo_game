"""Convert source PNGs into native e-ink assets.

Drop one image per icon into assets/icon_sources/<icon_id>.png. The build
script runs this converter before bundling, so new and changed sources are
picked up automatically.

Outputs:
  <icon_id>.word.gray.bin  - 192x192 packed 2-bit grayscale for word mode.
  <icon_id>.count.1bit.bin - 32x32 packed 1-bit black/white for counting mode.
"""

import re
import sys
import argparse
from pathlib import Path

import cv2
import numpy as np
from PIL import Image


ROOT = Path(__file__).parent.parent
SOURCE_DIR = ROOT / "assets" / "icon_sources"
OUTPUT_DIR = ROOT / "assets" / "icons"
PREVIEW_DIR = ROOT / "assets" / "icon_previews"
WORD_SIZE = 192
COUNT_SIZE = 32


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert source PNGs into firmware-ready word/count e-ink assets."
    )
    parser.add_argument(
        "--source-dir",
        type=Path,
        default=SOURCE_DIR,
        help="Primary icon source directory. Defaults to assets/icon_sources.",
    )
    parser.add_argument(
        "--fallback-source-dir",
        type=Path,
        action="append",
        default=[],
        help=(
            "Optional fallback source directory. Sources from --source-dir override "
            "fallbacks with the same filename stem. Can be supplied more than once."
        ),
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=OUTPUT_DIR,
        help="Packed asset output directory. Defaults to assets/icons.",
    )
    parser.add_argument(
        "--preview-dir",
        type=Path,
        default=PREVIEW_DIR,
        help="Preview PNG output directory. Defaults to assets/icon_previews.",
    )
    return parser.parse_args()


def resolve_repo_path(path: Path) -> Path:
    return path if path.is_absolute() else ROOT / path


def isolate_subject(rgb: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """Remove a plain studio background without flattening subject detail."""
    h, w = rgb.shape[:2]
    max_work_size = 320
    scale = min(1.0, max_work_size / max(h, w))
    work_w = max(8, round(w * scale))
    work_h = max(8, round(h * scale))
    reduced = cv2.resize(rgb, (work_w, work_h), interpolation=cv2.INTER_AREA)
    work = cv2.cvtColor(reduced, cv2.COLOR_RGB2BGR)
    mask = np.zeros((work_h, work_w), np.uint8)
    margin_x = max(2, work_w // 50)
    margin_y = max(2, work_h // 50)
    rect = (margin_x, margin_y, work_w - 2 * margin_x, work_h - 2 * margin_y)
    bg_model = np.zeros((1, 65), np.float64)
    fg_model = np.zeros((1, 65), np.float64)
    cv2.grabCut(work, mask, rect, bg_model, fg_model, 3, cv2.GC_INIT_WITH_RECT)
    foreground = np.where(
        (mask == cv2.GC_FGD) | (mask == cv2.GC_PR_FGD), 255, 0
    ).astype("uint8")

    kernel = np.ones((5, 5), np.uint8)
    foreground = cv2.morphologyEx(foreground, cv2.MORPH_CLOSE, kernel)
    foreground = cv2.GaussianBlur(foreground, (5, 5), 0)
    foreground = cv2.resize(foreground, (w, h), interpolation=cv2.INTER_LINEAR)
    return rgb, foreground


def fit_subject(rgb: np.ndarray, alpha: np.ndarray, source_name: str, size: int) -> tuple[Image.Image, bool]:
    points = cv2.findNonZero((alpha >= 96).astype("uint8"))
    if points is None:
        # Do not abort a hundred-image batch because segmentation rejected one
        # source. Preserve the full centered studio image and flag it for human
        # review; replacing that source PNG and rerunning will regenerate it.
        print(f"warning: {source_name}: background removal found no isolated subject; using full image")
        alpha = np.full(rgb.shape[:2], 255, dtype=np.uint8)
        x, y, w, h = 0, 0, rgb.shape[1], rgb.shape[0]
        used_fallback = True
    else:
        x, y, w, h = cv2.boundingRect(points)
        used_fallback = False
    pad = max(8, int(max(w, h) * 0.06))
    x0, y0 = max(0, x - pad), max(0, y - pad)
    x1, y1 = min(rgb.shape[1], x + w + pad), min(rgb.shape[0], y + h + pad)

    crop_rgb = rgb[y0:y1, x0:x1]
    crop_alpha = alpha[y0:y1, x0:x1].astype(np.float32) / 255.0
    composed = crop_rgb * crop_alpha[..., None] + 255.0 * (1.0 - crop_alpha[..., None])
    image = Image.fromarray(composed.astype("uint8"), "RGB")
    inset = max(4, size // 12)
    image.thumbnail((size - inset, size - inset), Image.Resampling.LANCZOS)
    canvas = Image.new("RGB", (size, size), "white")
    canvas.paste(image, ((size - image.width) // 2, (size - image.height) // 2))
    return canvas, used_fallback


def quantize_grayscale(image: Image.Image) -> np.ndarray:
    gray = np.asarray(image.convert("L"), dtype=np.float32)

    # The panel only gives us four useful tones, so use the full tonal range of
    # each icon before packing it. SDXL studio-style sources often come out very
    # light; fixed thresholds then waste most pixels as white/light-gray. Stretch
    # the visible subject region and apply a small ordered dither so rounded
    # shapes keep mid-tone detail after 2-bit quantization.
    subject = gray < 248
    if np.any(subject):
        lo, hi = np.percentile(gray[subject], (3, 97))
        if hi - lo >= 12:
            gray = (gray - lo) * (235.0 / (hi - lo))
            gray = np.clip(gray, 0, 255)

    # Slight contrast curve: preserve white background, make object detail more
    # legible on the e-ink panel, but avoid turning the whole object black.
    gray = 255.0 * np.power(np.clip(gray / 255.0, 0.0, 1.0), 1.12)

    bayer4 = np.array(
        [
            [0, 8, 2, 10],
            [12, 4, 14, 6],
            [3, 11, 1, 9],
            [15, 7, 13, 5],
        ],
        dtype=np.float32,
    )
    h, w = gray.shape
    dither = (np.tile(bayer4, ((h + 3) // 4, (w + 3) // 4))[:h, :w] - 7.5) * 5.0
    gray = np.clip(gray + dither, 0, 255)

    packed_values = np.zeros(gray.shape, dtype=np.uint8)
    packed_values[gray < 232] = 1
    packed_values[gray < 156] = 2
    packed_values[gray < 76] = 3
    return packed_values


def quantize_1bit(image: Image.Image) -> np.ndarray:
    gray = np.asarray(image.convert("L"), dtype=np.float32)
    subject = gray < 248
    if np.any(subject):
        lo, hi = np.percentile(gray[subject], (5, 95))
        if hi - lo >= 12:
            gray = np.clip((gray - lo) * (255.0 / (hi - lo)), 0, 255)
    h, w = gray.shape
    bayer4 = np.array(
        [
            [0, 8, 2, 10],
            [12, 4, 14, 6],
            [3, 11, 1, 9],
            [15, 7, 13, 5],
        ],
        dtype=np.float32,
    )
    threshold = 150 + (np.tile(bayer4, ((h + 3) // 4, (w + 3) // 4))[:h, :w] - 7.5) * 8.0
    return (gray < threshold).astype(np.uint8)


def pack_2bit(values: np.ndarray) -> bytes:
    flat = values.reshape(-1)
    if len(flat) % 4:
        raise ValueError("pixel count must be divisible by four")
    packed = (
        (flat[0::4] << 6)
        | (flat[1::4] << 4)
        | (flat[2::4] << 2)
        | flat[3::4]
    )
    return packed.astype("uint8").tobytes()


def pack_1bit(values: np.ndarray) -> bytes:
    flat = values.reshape(-1)
    if len(flat) % 8:
        raise ValueError("pixel count must be divisible by eight")
    packed = np.zeros(len(flat) // 8, dtype=np.uint8)
    for bit in range(8):
        packed |= (flat[bit::8].astype(np.uint8) & 0x01) << (7 - bit)
    return packed.tobytes()


def save_gray_preview(values: np.ndarray, path: Path) -> None:
    palette = np.array([255, 170, 85, 0], dtype=np.uint8)
    Image.fromarray(palette[values], "L").save(path)


def save_1bit_preview(values: np.ndarray, path: Path) -> None:
    Image.fromarray(np.where(values, 0, 255).astype(np.uint8), "L").save(path)


def main() -> None:
    args = parse_args()
    source_dir = resolve_repo_path(args.source_dir)
    fallback_dirs = [resolve_repo_path(path) for path in args.fallback_source_dir]
    output_dir = resolve_repo_path(args.output_dir)
    preview_dir = resolve_repo_path(args.preview_dir)
    warning_path = preview_dir / "_review_warnings.txt"

    if not source_dir.is_dir():
        print(f"error: source directory does not exist: {source_dir}", file=sys.stderr)
        raise SystemExit(1)

    merged_sources: dict[str, Path] = {}
    for fallback_dir in fallback_dirs:
        if not fallback_dir.is_dir():
            print(f"error: fallback source directory does not exist: {fallback_dir}", file=sys.stderr)
            raise SystemExit(1)
        for source in sorted(fallback_dir.glob("*.png")):
            merged_sources[source.stem] = source

    for source in sorted(source_dir.glob("*.png")):
        merged_sources[source.stem] = source

    sources = [merged_sources[key] for key in sorted(merged_sources)]
    if not sources:
        print(f"error: no source PNGs found in {source_dir}", file=sys.stderr)
        raise SystemExit(1)

    output_dir.mkdir(parents=True, exist_ok=True)
    preview_dir.mkdir(parents=True, exist_ok=True)
    expected_outputs: set[Path] = set()
    converted = 0
    converter_mtime = Path(__file__).stat().st_mtime

    for source in sources:
        icon_id = source.stem
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", icon_id):
            print(f"error: invalid icon id in filename {source.name!r}", file=sys.stderr)
            raise SystemExit(1)
        word_output = output_dir / f"{icon_id}.word.gray.bin"
        count_output = output_dir / f"{icon_id}.count.1bit.bin"
        word_preview = preview_dir / f"{icon_id}.word.png"
        count_preview = preview_dir / f"{icon_id}.count.png"
        review_marker = preview_dir / f"{icon_id}.review.txt"
        expected_outputs.add(word_output)
        expected_outputs.add(count_output)
        if (
            word_output.exists()
            and count_output.exists()
            and word_preview.exists()
            and count_preview.exists()
            and word_output.stat().st_mtime >= max(source.stat().st_mtime, converter_mtime)
            and count_output.stat().st_mtime >= max(source.stat().st_mtime, converter_mtime)
            and word_preview.stat().st_mtime >= max(source.stat().st_mtime, converter_mtime)
            and count_preview.stat().st_mtime >= max(source.stat().st_mtime, converter_mtime)
        ):
            continue

        rgb = np.asarray(Image.open(source).convert("RGB"))
        rgb, alpha = isolate_subject(rgb)
        word_fitted, used_fallback = fit_subject(rgb, alpha, source.name, WORD_SIZE)
        count_fitted, _ = fit_subject(rgb, alpha, source.name, COUNT_SIZE)
        if used_fallback:
            review_marker.write_text(
                f"{source.name}: no isolated foreground detected; full image used\n",
                encoding="utf-8",
            )
        elif review_marker.exists():
            review_marker.unlink()

        word_values = quantize_grayscale(word_fitted)
        word_data = pack_2bit(word_values)
        if len(word_data) != WORD_SIZE * WORD_SIZE // 4:
            raise ValueError(f"unexpected word packed size for {source.name}: {len(word_data)}")
        word_output.write_bytes(word_data)
        save_gray_preview(word_values, word_preview)

        count_values = quantize_1bit(count_fitted)
        count_data = pack_1bit(count_values)
        if len(count_data) != COUNT_SIZE * COUNT_SIZE // 8:
            raise ValueError(f"unexpected count packed size for {source.name}: {len(count_data)}")
        count_output.write_bytes(count_data)
        save_1bit_preview(count_values, count_preview)
        converted += 1
        print(
            f"converted {source.name} -> {word_output.name} ({len(word_data)} bytes), "
            f"{count_output.name} ({len(count_data)} bytes)"
        )

    # Old outputs must not remain silently bundled after their source is removed.
    for stale in list(output_dir.glob("*.word.gray.bin")) + list(output_dir.glob("*.count.1bit.bin")):
        if stale not in expected_outputs:
            stale.unlink()
            print(f"removed stale output {stale.name}")

    for stale in output_dir.glob("*.gray.bin"):
        if stale.name.endswith(".word.gray.bin"):
            continue
        stale.unlink()
        print(f"removed obsolete output {stale.name}")

    review_warnings = [
        marker.read_text(encoding="utf-8").strip()
        for marker in sorted(preview_dir.glob("*.review.txt"))
    ]
    if review_warnings:
        warning_path.write_text("\n".join(review_warnings) + "\n", encoding="utf-8")
        print(f"review warnings written to {warning_path}")
    elif warning_path.exists():
        warning_path.unlink()

    print(
        f"{len(sources)} dual-resolution icons ready "
        f"({converted} converted, {len(sources) - converted} unchanged)"
    )


if __name__ == "__main__":
    main()
