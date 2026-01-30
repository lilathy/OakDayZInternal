#!/usr/bin/env python3
"""
Remove comments from project source files. Preserves string literals and
preprocessor directives. Supports C/C++/C

Usage:
  python remove_comments.py [--dry-run] [--backup] [--include-third-party] [root_dir]

  --dry-run              Show what would be changed without writing (recommended first)
  --backup               Create .bak backup before modifying (recommended first run)
  --include-third-party  Also process external/ghidra/minhook/kdmapper (default: skip)
"""

import argparse
import os
import re
import sys
from pathlib import Path


EXCLUDE_DIRS = {
    "ghidra_12.0.1_PUBLIC_20260114",
    "ghidra_12.0.1_PUBLIC",
    "minhook-master",
    "kdmapper-master",
    "external",
    "DayZ-DMA-Radar-master",
    "node_modules",
    ".git",
}


C_STYLE_EXT = {".cpp", ".c", ".h", ".hpp", ".cs", ".hlsl", ".hxx", ".cxx"}


HASH_LINE_EXT = {".ps1", ".py", ".sh", ".bat", ".cmake"}


CMAKE_FILES = {"CMakeLists.txt"}


XAML_EXT = {".xaml"}


def remove_c_style_comments(content: str) -> str:
    """
    Remove // and /* */ comments. Respects double/single-quoted strings
    and escape sequences. Does not touch 
    """
    result = []
    i = 0
    n = len(content)
    state = "normal"  

    while i < n:
        if state == "normal":
            if i < n - 1 and content[i] == "/" and content[i + 1] == "/":
                state = "line_comment"
                i += 2
                continue
            if i < n - 1 and content[i] == "/" and content[i + 1] == "*":
                state = "block_comment"
                i += 2
                continue
            if content[i] == '"':
                state = "double_str"
                result.append(content[i])
                i += 1
                continue
            if content[i] == "'":
                state = "single_str"
                result.append(content[i])
                i += 1
                continue
            result.append(content[i])
            i += 1
            continue

        if state == "line_comment":
            if content[i] == "\n":
                result.append("\n")
                state = "normal"
            i += 1
            continue

        if state == "block_comment":
            if i < n - 1 and content[i] == "*" and content[i + 1] == "/":
                state = "normal"
                result.append(" ")
                i += 2
            else:
                if content[i] == "\n":
                    result.append("\n")
                else:
                    result.append(" ")
                i += 1
            continue

        if state == "double_str":
            if content[i] == "\\":
                result.append(content[i])
                i += 1
                if i < n:
                    result.append(content[i])
                    i += 1
            elif content[i] == '"':
                result.append(content[i])
                i += 1
                state = "normal"
            else:
                result.append(content[i])
                i += 1
            continue

        if state == "single_str":
            if content[i] == "\\":
                result.append(content[i])
                i += 1
                if i < n:
                    result.append(content[i])
                    i += 1
            elif content[i] == "'":
                result.append(content[i])
                i += 1
                state = "normal"
            else:
                result.append(content[i])
                i += 1
            continue

        i += 1

    return "".join(result)


def remove_hash_line_comments_simple(content: str) -> str:
    """
    Remove 
    """
    lines = content.split("\n")
    out = []
    for line in lines:
        stripped = line.lstrip()
        if stripped.startswith("#!"):
            out.append(line)
            continue
        if stripped.startswith("#"):
            out.append("")
            continue

        i = 0
        new_line = []
        state = "normal"
        n = len(line)
        while i < n:
            if state == "normal":
                if line[i] in '"\'':
                    state = line[i]
                    new_line.append(line[i])
                    i += 1
                    continue
                if line[i] == "#":
                    break
                new_line.append(line[i])
                i += 1
                continue
            if state in '"\'':
                q = state
                new_line.append(line[i])
                if line[i] == "\\" and i + 1 < n:
                    new_line.append(line[i + 1])
                    i += 2
                    continue
                if line[i] == q:
                    state = "normal"
                i += 1
                continue
            i += 1
        out.append("".join(new_line))
    return "\n".join(out)


def remove_xml_comments(content: str) -> str:
    """Remove XML/HTML comments: <!-- ... -->"""
    result = []
    i = 0
    n = len(content)
    state = "normal"

    while i < n:
        if state == "normal":
            if i < n - 4 and content[i : i + 4] == "<!--":
                state = "comment"
                i += 4
                continue
            result.append(content[i])
            i += 1
            continue
        if state == "comment":
            if i < n - 3 and content[i : i + 3] == "-->":
                state = "normal"
                result.append(" ")
                i += 3
            else:
                if content[i] == "\n":
                    result.append("\n")
                else:
                    result.append(" ")
                i += 1
            continue
        i += 1
    return "".join(result)


def process_file(
    path: Path, root: Path, dry_run: bool, backup: bool, include_third_party: bool
) -> bool:
    """Process one file. Returns True if file was modified."""
    try:
        rel = path.relative_to(root)
    except ValueError:
        rel = path
    for skip in EXCLUDE_DIRS:
        if skip in path.parts and not include_third_party:
            return False

    ext = path.suffix.lower()
    name = path.name

    if ext in C_STYLE_EXT:
        stripper = remove_c_style_comments
    elif ext in HASH_LINE_EXT or name in CMAKE_FILES:
        stripper = remove_hash_line_comments_simple
    elif ext in XAML_EXT:
        stripper = remove_xml_comments
    else:
        return False

    try:
        raw = path.read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        print(f"  Skip {rel}: {e}", file=sys.stderr)
        return False

    new_content = stripper(raw)
    if new_content == raw:
        return False

    if dry_run:
        print(f"  Would strip comments: {rel}")
        return True

    if backup:
        path.with_suffix(path.suffix + ".bak").write_text(raw, encoding="utf-8")

    path.write_text(new_content, encoding="utf-8")
    print(f"  Stripped comments: {rel}")
    return True


def main():
    parser = argparse.ArgumentParser(
        description="Remove comments from project source files (C/C++, C#, HLSL, PS1, XAML, CMake)."
    )
    parser.add_argument(
        "root_dir",
        nargs="?",
        default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        help="Project root (default: parent of scripts/)",
    )
    parser.add_argument("--dry-run", action="store_true", help="Only show what would be changed")
    parser.add_argument("--backup", action="store_true", help="Create .bak backup before modifying")
    parser.add_argument(
        "--include-third-party",
        action="store_true",
        help="Also process external/ghidra/minhook/kdmapper (default: skip)",
    )
    args = parser.parse_args()

    root = Path(args.root_dir).resolve()
    if not root.is_dir():
        print(f"Not a directory: {root}", file=sys.stderr)
        sys.exit(1)

    print(f"Root: {root}")
    print("Dry run: " + ("yes" if args.dry_run else "no"))
    if args.dry_run:
        print("Run without --dry-run to apply changes.")
    print()

    count = 0
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if process_file(path, root, args.dry_run, args.backup, args.include_third_party):
            count += 1

    print()
    print(f"Done. {'Would modify' if args.dry_run else 'Modified'} {count} file(s).")


if __name__ == "__main__":
    main()
