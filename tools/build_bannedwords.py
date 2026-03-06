#!/usr/bin/env python3
"""
Fetches profanity lists from two upstream sources, merges them,
deduplicates, sorts, and writes bannedwords.txt.

Sources:
  - LDNOOBW/List-of-Dirty-Naughty-Obscene-and-Otherwise-Bad-Words
    Multilingual, community maintained, one file per language code.
  - surge-list/profanity-list
    English-focused, single flat file.

Output: bannedwords.txt — one word/phrase per line, sorted, no duplicates.
"""

import urllib.request
import urllib.error
import sys
import os

# ── Source definitions ────────────────────────────────────────────────────────

# LDNOOBW: each language is a separate file at this base URL.
# Raw file list fetched from the GitHub API.
LDNOOBW_API = (
    "https://api.github.com/repos/LDNOOBW/"
    "List-of-Dirty-Naughty-Obscene-and-Otherwise-Bad-Words/contents"
)
LDNOOBW_RAW = (
    "https://raw.githubusercontent.com/LDNOOBW/"
    "List-of-Dirty-Naughty-Obscene-and-Otherwise-Bad-Words/master/{lang}"
)

# surge-list: single flat file
SURGE_RAW = (
    "https://raw.githubusercontent.com/surge-list/"
    "profanity-list/master/word_list"
)

# ── Helpers ───────────────────────────────────────────────────────────────────

def fetch(url, label):
    """Fetch URL, return text. Exits on failure."""
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "tf2vintage-build/1.0"})
        with urllib.request.urlopen(req, timeout=30) as resp:
            return resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        print(f"  WARNING: HTTP {e.code} fetching {label} — skipping", file=sys.stderr)
        return ""
    except Exception as e:
        print(f"  WARNING: Failed to fetch {label}: {e} — skipping", file=sys.stderr)
        return ""


def parse_words(text):
    """Extract non-empty, non-comment lines from a word list."""
    words = set()
    for line in text.splitlines():
        word = line.strip()
        if word and not word.startswith("#"):
            words.add(word.lower())
    return words


def fetch_ldnoobw_languages():
    """
    Fetch the LDNOOBW repo contents listing, find all language files
    (files without an extension, like 'en', 'es', 'zh', etc.),
    and return their names.
    """
    import json
    text = fetch(LDNOOBW_API, "LDNOOBW directory listing")
    if not text:
        return []
    try:
        entries = json.loads(text)
        # Language files are plain files with no extension and short names
        # (e.g. "en", "es", "zh", "ar"). README, LICENSE etc. are excluded.
        langs = [
            e["name"] for e in entries
            if e.get("type") == "file"
            and "." not in e["name"]
            and len(e["name"]) <= 5  # language codes are 2-5 chars
            and e["name"] not in ("LICENSE", "README")
        ]
        return langs
    except (json.JSONDecodeError, KeyError) as e:
        print(f"  WARNING: Could not parse LDNOOBW listing: {e}", file=sys.stderr)
        return []


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    out_path = sys.argv[1] if len(sys.argv) > 1 else "bannedwords.txt"
    combined = set()

    # ── Source 1: LDNOOBW (all language files) ────────────────────────────────
    print("Fetching LDNOOBW language file list...")
    langs = fetch_ldnoobw_languages()
    if langs:
        print(f"  Found {len(langs)} language files: {', '.join(sorted(langs))}")
        for lang in sorted(langs):
            url = LDNOOBW_RAW.format(lang=lang)
            text = fetch(url, f"LDNOOBW/{lang}")
            words = parse_words(text)
            print(f"  [{lang}] {len(words)} words")
            combined |= words
    else:
        print("  WARNING: Could not retrieve LDNOOBW language list", file=sys.stderr)

    # ── Source 2: surge-list ──────────────────────────────────────────────────
    print("Fetching surge-list/profanity-list...")
    text = fetch(SURGE_RAW, "surge-list")
    words = parse_words(text)
    print(f"  {len(words)} words")
    combined |= words

    # ── Merge, sort, write ────────────────────────────────────────────────────
    if not combined:
        print("ERROR: No words fetched from any source — not writing output", file=sys.stderr)
        sys.exit(1)

    sorted_words = sorted(combined)
    print(f"\nTotal unique entries after merge: {len(sorted_words)}")

    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(f"# TF2 Vintage banned word list\n")
        f.write(f"# Auto-generated at build time — do not edit manually.\n")
        f.write(f"# Sources:\n")
        f.write(f"#   https://github.com/LDNOOBW/List-of-Dirty-Naughty-Obscene-and-Otherwise-Bad-Words\n")
        f.write(f"#   https://github.com/surge-list/profanity-list\n")
        f.write(f"# Total entries: {len(sorted_words)}\n")
        f.write("\n")
        f.write("\n".join(sorted_words))
        f.write("\n")

    print(f"Written to: {out_path}")


if __name__ == "__main__":
    main()
