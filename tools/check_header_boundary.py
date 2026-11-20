#!/usr/bin/env python3
"""Reject private repository dependencies from NX-SDK public headers."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

PUBLIC_DIRS = (
    "include/api",
    "include/contract",
    "lib/extensions/LIBGRAPHICS/include",
)
FORBIDDEN_PREFIXES = (
    "kernel/",
    "HAL/",
    "user/",
    "system/",
    "lib/",
    "tools/",
)
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')


def include_is_private(name: str) -> bool:
    if name.startswith("/") or name.startswith("../"):
        return True
    return name.startswith(FORBIDDEN_PREFIXES)


def scan_public_headers(root: Path) -> list[tuple[str, int, str]]:
    findings: list[tuple[str, int, str]] = []
    for relative in PUBLIC_DIRS:
        base = root / relative
        if not base.is_dir():
            findings.append((relative, 0, "<missing public header directory>"))
            continue
        for path in sorted(base.rglob("*.h")):
            for line_number, raw in enumerate(
                path.read_text(errors="replace").splitlines(), 1
            ):
                match = INCLUDE_RE.match(raw)
                if match and include_is_private(match.group(1)):
                    findings.append(
                        (path.relative_to(root).as_posix(), line_number, match.group(1))
                    )
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify NX-SDK public headers do not import private repository paths."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="NX-SDK repository root",
    )
    args = parser.parse_args()
    findings = scan_public_headers(args.root.resolve())
    if findings:
        for path, line, include in findings:
            print(f"{path}:{line}: forbidden public-header include: {include}")
        return 1
    print("NX-SDK public-header boundary: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
