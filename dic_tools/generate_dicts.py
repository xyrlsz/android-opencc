#!/usr/bin/env python3
"""Generate ALL .ocd2 dictionaries and copy configs for android-opencc.

Requires OpenCC CLI tools installed on your system.

Usage:
    python3 generate_dicts.py                              # Auto-detect tools from PATH
    python3 generate_dicts.py --opencc path/to/opencc      # Specify opencc path explicitly
"""

import sys
import subprocess
import shutil
import argparse
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
JNI_OPENCC = PROJECT_ROOT / "lib-opencc-android" / "src" / "main" / "jni" / "OpenCC"
OPENCC_DICT_DIR = JNI_OPENCC / "data" / "dictionary"
OPENCC_CONFIG_DIR = JNI_OPENCC / "data" / "config"
OPENCC_SCRIPTS_DIR = JNI_OPENCC / "data" / "scripts"
ASSETS_DIR = PROJECT_ROOT / "lib-opencc-android" / "src" / "main" / "assets" / "openccdata"

# ── All raw .txt source → .ocd2 target mappings ──────────────────────────
# From upstream data/CMakeLists.txt: DICTS_RAW
ALL_DICT_MAP = {
    "CJK_Compatibility_Ideographs.txt": "CJK_Compatibility_Ideographs.ocd2",
    "STCharacters.txt":                "STCharacters.ocd2",
    "STPhrases.txt":                   "STPhrases.ocd2",
    "TSCharacters.txt":                "TSCharacters.ocd2",
    "TSPhrases.txt":                   "TSPhrases.ocd2",
    "TWPhrases.txt":                   "TWPhrases.ocd2",
    "TWPhrasesRev.txt":                "TWPhrasesRev.ocd2",
    "TWVariantsPhrases.txt":           "TWVariantsPhrases.ocd2",
    "TWVariants.txt":                  "TWVariants.ocd2",
    "TWVariantsRevPhrases.txt":        "TWVariantsRevPhrases.ocd2",
    "HKVariantsPhrases.txt":           "HKVariantsPhrases.ocd2",
    "HKVariants.txt":                  "HKVariants.ocd2",
    "HKVariantsRevPhrases.txt":        "HKVariantsRevPhrases.ocd2",
    "HKPhrases.txt":                   "HKPhrases.ocd2",
    "HKPhrasesRev.txt":                "HKPhrasesRev.ocd2",
    "JPShinjitaiCharacters.txt":       "JPShinjitaiCharacters.ocd2",
    "JPShinjitaiPhrases.txt":          "JPShinjitaiPhrases.ocd2",
}

# Generated dictionaries (reverse / derived) that need extra steps
# reverse.py: swap key↔value → TWVariantsRev, HKVariantsRev, JPShinjitaiCharactersRev
REVERSE_GENERATED = [
    ("TWVariants.txt",              "TWVariantsRev.txt"),
    ("HKVariants.txt",              "HKVariantsRev.txt"),
    ("JPShinjitaiCharacters.txt",   "JPShinjitaiCharactersRev.txt"),
]

# extract_tofu_risk.py: extract @tofu-risk annotations → TSCharactersExt.txt
TOFU_GENERATED = [
    ("TSCharacters.txt", "TSCharactersExt.txt"),
]

# Generated .txt → .ocd2 (after generation step)
GENERATED_TXT_MAP = {
    "TWVariantsRev.txt":              "TWVariantsRev.ocd2",
    "HKVariantsRev.txt":              "HKVariantsRev.ocd2",
    "JPShinjitaiCharactersRev.txt":   "JPShinjitaiCharactersRev.ocd2",
    "TSCharactersExt.txt":            "TSCharactersExt.ocd2",
}

# STPhrases_GeneratedFromRegionalPhrases needs opencc CLI tool.
# generate_st_phrases_from_regional_phrases.py converts HK/TW regional
# phrase keys to Simplified Chinese via opencc, then outputs ST entries.
ST_PHRASES_GENERATED_SRC = "STPhrases_GeneratedFromRegionalPhrases.txt"
ST_PHRASES_GENERATED_DST = "STPhrases_GeneratedFromRegionalPhrases.ocd2"

# JPVariants.ocd2 / JPVariantsRev.ocd2 are Android-specific custom files
# with no upstream source — preserved as-is.
KEEP_ANDROID_ONLY = {
    "JPVariants.ocd2",
    "JPVariantsRev.ocd2",
}


# ── Helpers ──────────────────────────────────────────────────────────────

def find_on_path(name):
    """Find an executable on PATH, checking common extensions on Windows."""
    exe = shutil.which(name)
    if exe:
        return Path(exe)
    if sys.platform == "win32":
        for ext in [".exe", ".bat", ".cmd"]:
            exe = shutil.which(name + ext)
            if exe:
                return Path(exe)
    return None


def check_tools(opencc_override=None):
    """Check that required OpenCC tools are available.

    Returns (opencc_dict_path, opencc_path).
    opencc_path may be None if not found (only needed for ST phrases).
    """
    opencc_dict_path = find_on_path("opencc_dict")
    if not opencc_dict_path:
        print("\nERROR: 'opencc_dict' not found on PATH.", file=sys.stderr)
        print("", file=sys.stderr)
        print("  Please install OpenCC CLI first:", file=sys.stderr)
        print("    Windows: scoop install opencc", file=sys.stderr)
        print("    macOS:   brew install opencc", file=sys.stderr)
        print("    Linux:   apt install opencc  (or use your distro's package manager)", file=sys.stderr)
        print("", file=sys.stderr)
        print("  Or download from: https://github.com/BYVoid/OpenCC/releases", file=sys.stderr)
        sys.exit(1)

    print(f"  Found opencc_dict: {opencc_dict_path}")

    opencc_path = None
    if opencc_override:
        opencc_path = Path(opencc_override)
        if not opencc_path.exists():
            print(f"  WARNING: Specified opencc path not found: {opencc_path}", file=sys.stderr)
            opencc_path = None
    else:
        opencc_path = find_on_path("opencc")

    if opencc_path:
        print(f"  Found opencc:      {opencc_path}")
    else:
        print(f"  (opencc CLI not found — will skip STPhrases_GeneratedFromRegionalPhrases)")

    return opencc_dict_path, opencc_path


def run(cmd, cwd=None):
    """Run a command and check exit code."""
    print(f"  RUN: {' '.join(str(c) for c in cmd)}")
    result = subprocess.run(cmd, cwd=cwd)
    if result.returncode != 0:
        sys.exit(result.returncode)


def convert_to_ocd2(opencc_dict_exe, src_path, dst_path):
    """Convert a single .txt → .ocd2 using opencc_dict."""
    run([str(opencc_dict_exe), "-f", "text", "-t", "ocd2",
         "-i", str(src_path), "-o", str(dst_path)])



# ── Generation steps ─────────────────────────────────────────────────────

def generate_reverse_dicts(work_dir):
    """Generate reverse .txt files using reverse.py."""
    generated = 0
    for src_name, dst_name in REVERSE_GENERATED:
        src_path = OPENCC_DICT_DIR / src_name
        dst_path = work_dir / dst_name
        if not src_path.exists():
            print(f"  SKIP reverse: source not found: {src_name}")
            continue
        if dst_path.exists() and src_path.stat().st_mtime <= dst_path.stat().st_mtime:
            print(f"  SKIP reverse: {dst_name} is up to date")
            continue
        print(f"  Reversing {src_name} -> {dst_name} ...")
        run([sys.executable, str(OPENCC_SCRIPTS_DIR / "reverse.py"),
             str(src_path), str(dst_path)])
        generated += 1
    return generated


def generate_tofu_dicts(work_dir):
    """Generate TSCharactersExt.txt using extract_tofu_risk.py."""
    generated = 0
    for src_name, dst_name in TOFU_GENERATED:
        src_path = OPENCC_DICT_DIR / src_name
        dst_path = work_dir / dst_name
        if not src_path.exists():
            print(f"  SKIP tofu: source not found: {src_name}")
            continue
        if dst_path.exists() and src_path.stat().st_mtime <= dst_path.stat().st_mtime:
            print(f"  SKIP tofu: {dst_name} is up to date")
            continue
        print(f"  Extracting tofu risk {src_name} -> {dst_name} ...")
        run([sys.executable, str(OPENCC_SCRIPTS_DIR / "extract_tofu_risk.py"),
             str(src_path), str(dst_path)])
        generated += 1
    return generated


def generate_raw_dicts(opencc_dict_exe, force=False):
    """Generate .ocd2 from all raw .txt sources directly into assets."""
    print(f"\n{'='*60}")
    print("  Step 1: Generate .ocd2 from raw .txt sources")
    print(f"{'='*60}")
    generated = 0
    skipped = 0
    for src_name, dst_name in ALL_DICT_MAP.items():
        src_path = OPENCC_DICT_DIR / src_name
        dst_path = ASSETS_DIR / dst_name
        if not src_path.exists():
            print(f"  SKIP: source not found: {src_name}")
            continue
        if not force and dst_path.exists() and src_path.stat().st_mtime <= dst_path.stat().st_mtime:
            skipped += 1
            continue
        print(f"  {src_name} -> {dst_name} ...")
        convert_to_ocd2(opencc_dict_exe, src_path, dst_path)
        generated += 1
    print(f"  Generated: {generated}, up-to-date: {skipped}")
    return generated


def generate_derived_dicts(opencc_dict_exe, work_dir, force=False):
    """Generate derived .txt (reverse/tofu) then convert to .ocd2."""
    print(f"\n{'='*60}")
    print("  Step 2: Generate derived dictionaries (reverse + tofu)")
    print(f"{'='*60}")

    # Generate reverse .txt
    n_rev = generate_reverse_dicts(work_dir)
    print(f"  Reverse files generated: {n_rev}")

    # Generate tofu-risk .txt
    n_tofu = generate_tofu_dicts(work_dir)
    print(f"  Tofu-risk files generated: {n_tofu}")

    # Convert generated .txt → .ocd2
    print(f"\n  Converting generated .txt -> .ocd2 ...")
    gen_count = 0
    for src_name, dst_name in GENERATED_TXT_MAP.items():
        src_path = work_dir / src_name
        dst_path = ASSETS_DIR / dst_name
        if not src_path.exists():
            print(f"  SKIP: generated source not found: {src_name}")
            continue
        if not force and dst_path.exists() and src_path.stat().st_mtime <= dst_path.stat().st_mtime:
            continue
        print(f"  {src_name} -> {dst_name} ...")
        convert_to_ocd2(opencc_dict_exe, src_path, dst_path)
        gen_count += 1
    print(f"  Generated: {gen_count}")

    return n_rev + n_tofu + gen_count


def generate_st_phrases_from_regional(work_dir, opencc_path, force=False):
    """Generate STPhrases_GeneratedFromRegionalPhrases.txt using opencc CLI."""
    print(f"\n{'='*60}")
    print("  Step 3: Generate STPhrases_GeneratedFromRegionalPhrases")
    print(f"{'='*60}")

    script = OPENCC_SCRIPTS_DIR / "generate_st_phrases_from_regional_phrases.py"
    if not script.exists():
        print(f"  ERROR: script not found: {script}", file=sys.stderr)
        return 0

    if not Path(opencc_path).exists():
        print(f"  ERROR: opencc CLI not found: {opencc_path}", file=sys.stderr)
        return 0

    # Input files
    hk_phrases = OPENCC_DICT_DIR / "HKPhrases.txt"
    tw_phrases = OPENCC_DICT_DIR / "TWPhrases.txt"
    dst_txt = work_dir / ST_PHRASES_GENERATED_SRC
    config_path = OPENCC_CONFIG_DIR / "t2s.json"

    if not hk_phrases.exists():
        print(f"  SKIP: source not found: HKPhrases.txt")
        return 0
    if not tw_phrases.exists():
        print(f"  SKIP: source not found: TWPhrases.txt")
        return 0
    if not config_path.exists():
        print(f"  SKIP: config not found: t2s.json")
        return 0

    # Check if up-to-date
    src_mtime = max(hk_phrases.stat().st_mtime, tw_phrases.stat().st_mtime)
    if not force and dst_txt.exists() and src_mtime <= dst_txt.stat().st_mtime:
        print(f"  SKIP: {ST_PHRASES_GENERATED_SRC} is up to date")
        return 0

    print(f"  Generating {ST_PHRASES_GENERATED_SRC} ...")
    run([
        sys.executable, str(script),
        "--input", str(hk_phrases),
        "--input", str(tw_phrases),
        "--output", str(dst_txt),
        "--opencc", str(opencc_path),
        "--config", str(config_path),
        "--dict-dir", str(ASSETS_DIR),
    ])
    print(f"  -> {dst_txt}")
    return 1


def generate_st_phrases_ocd2(opencc_dict_exe, work_dir, force=False):
    """Convert STPhrases_GeneratedFromRegionalPhrases.txt → .ocd2."""
    src_path = work_dir / ST_PHRASES_GENERATED_SRC
    dst_path = ASSETS_DIR / ST_PHRASES_GENERATED_DST
    if not src_path.exists():
        print(f"  SKIP: {ST_PHRASES_GENERATED_SRC} not found, skipping ocd2 conversion")
        return 0
    if not force and dst_path.exists() and src_path.stat().st_mtime <= dst_path.stat().st_mtime:
        return 0
    print(f"  Converting {ST_PHRASES_GENERATED_SRC} -> {ST_PHRASES_GENERATED_DST} ...")
    convert_to_ocd2(opencc_dict_exe, src_path, dst_path)
    return 1


def copy_configs(force=False):
    """Copy all .json config files from data/config to assets/openccdata."""
    print(f"\n{'='*60}")
    print("  Step 4: Copy config files from data/config/")
    print(f"{'='*60}")
    copied = 0
    skipped = 0
    if not ASSETS_DIR.exists():
        ASSETS_DIR.mkdir(parents=True)
    for f in sorted(OPENCC_CONFIG_DIR.glob("*.json")):
        dst = ASSETS_DIR / f.name
        if not force and dst.exists() and f.stat().st_mtime <= dst.stat().st_mtime:
            skipped += 1
            continue
        print(f"  {f.name}")
        shutil.copy2(f, dst)
        copied += 1
    print(f"  Copied: {copied}, up-to-date: {skipped}")

    # Show assets configs that are NOT from upstream (keep custom ones)
    upstream_configs = {f.name for f in OPENCC_CONFIG_DIR.glob("*.json")}
    assets_configs = {f.name for f in ASSETS_DIR.glob("*.json")}
    custom = assets_configs - upstream_configs
    if custom:
        print(f"  Custom configs kept (no upstream source): {', '.join(sorted(custom))}")
    return copied


def report_unresolved(opencc_available=False):
    """Report dictionaries that could not be auto-generated."""
    print(f"\n{'='*60}")
    print("  Step 5: Summary of remaining dictionaries")
    print(f"{'='*60}")

    # Check what we have in assets
    assets_ocd2 = {f.name for f in ASSETS_DIR.glob("*.ocd2")}

    # All expected .ocd2 from raw sources
    expected = set(ALL_DICT_MAP.values())
    # Plus generated
    expected.update(GENERATED_TXT_MAP.values())
    # Plus Android-only custom ones
    expected.update(KEEP_ANDROID_ONLY)
    # Plus STPhrases_GeneratedFromRegionalPhrases (added by --opencc)
    expected.add(ST_PHRASES_GENERATED_DST)

    missing = expected - assets_ocd2
    extra = assets_ocd2 - expected

    if missing:
        print(f"\n  [FAIL] MISSING .ocd2 files (could not be auto-generated):")
        for d in sorted(missing):
            print(f"     - {d}")
    else:
        print(f"\n  [OK] All expected .ocd2 files are present.")

    if extra:
        print(f"\n  [i] Extra .ocd2 files in assets (maybe unused):")
        for d in sorted(extra):
            print(f"     - {d}")

    # Check STPhrases_GeneratedFromRegionalPhrases
    stp = ASSETS_DIR / ST_PHRASES_GENERATED_DST
    if stp.exists():
        print(f"\n  [OK] {ST_PHRASES_GENERATED_DST} is present.")
    elif opencc_available:
        print(f"\n  [FAIL] {ST_PHRASES_GENERATED_DST} is MISSING (generation failed).")
    else:
        print(f"\n  [i] {ST_PHRASES_GENERATED_DST} was skipped (opencc CLI not available).")


# ── Main ─────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Build ALL OpenCC .ocd2 dictionaries and copy configs for android-opencc")
    parser.add_argument("--build-dir", default="build",
                        help="Working directory for intermediate files (default: build)")
    parser.add_argument("--opencc",
                        help="Path to opencc CLI (for STPhrases_GeneratedFromRegionalPhrases)")
    parser.add_argument("--force", action="store_true",
                        help="Force regenerate even if up-to-date")
    parser.add_argument("--no-config-copy", action="store_true",
                        help="Skip copying config files")
    args = parser.parse_args()

    work_dir = Path(args.build_dir).resolve()
    work_dir.mkdir(parents=True, exist_ok=True)

    # ── Check for required OpenCC tools ──
    print(f"{'='*60}")
    print("  Checking for OpenCC CLI tools...")
    print(f"{'='*60}")
    opencc_dict_exe, opencc_exe = check_tools(opencc_override=args.opencc)

    # ── Ensure assets directory exists ──
    ASSETS_DIR.mkdir(parents=True, exist_ok=True)

    # ── Generate all dictionaries ──
    generate_raw_dicts(opencc_dict_exe, force=args.force)
    generate_derived_dicts(opencc_dict_exe, work_dir, force=args.force)

    # ── Generate STPhrases_GeneratedFromRegionalPhrases (needs opencc CLI) ──
    if opencc_exe:
        n_st = generate_st_phrases_from_regional(work_dir, opencc_exe, force=args.force)
        if n_st:
            generate_st_phrases_ocd2(opencc_dict_exe, work_dir, force=args.force)
    else:
        print(f"\n  (Skipping STPhrases_GeneratedFromRegionalPhrases: opencc CLI not found)")
        print(f"  Tip: Install OpenCC (e.g. 'scoop install opencc') and re-run, or use --opencc")

    # ── Copy config files ──
    if not args.no_config_copy:
        copy_configs(force=args.force)

    # ── Summary ──
    report_unresolved(opencc_available=(opencc_exe is not None))

    print(f"\n{'='*60}")
    print("  [OK] All done!")
    print(f"{'='*60}")
    print(f"  Dictionaries in: {ASSETS_DIR}")
    print(f"  Configs in:      {ASSETS_DIR}")
    print(f"  (Intermediate .txt in: {work_dir})")


if __name__ == "__main__":
    main()
