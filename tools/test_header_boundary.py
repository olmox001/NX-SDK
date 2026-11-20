#!/usr/bin/env python3
"""Mutation tests for the NX-SDK public-header boundary gate."""

from __future__ import annotations

import tempfile
from pathlib import Path

from check_header_boundary import scan_public_headers


def write_tree(
    root: Path, api_text: str, contract_text: str = "",
    libgraphics_text: str = ""
) -> None:
    (root / "include/api").mkdir(parents=True)
    (root / "include/contract").mkdir(parents=True)
    (root / "lib/extensions/LIBGRAPHICS/include").mkdir(parents=True)
    (root / "include/api/example.h").write_text(api_text)
    (root / "include/contract/example.h").write_text(contract_text)
    (root / "lib/extensions/LIBGRAPHICS/include/example.h").write_text(
        libgraphics_text
    )


def check_clean_public_dependencies() -> int:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        write_tree(
            root,
            "#include <stdint.h>\n#include <surface.h>\n",
            "#include <stddef.h>\n",
        )
        if scan_public_headers(root):
            print("FAIL: public/standard dependency was rejected")
            return 1
    return 0


def check_kernel_dependency_fails() -> int:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        write_tree(root, "#include <kernel/types.h>\n")
        findings = scan_public_headers(root)
        if len(findings) != 1 or findings[0][2] != "kernel/types.h":
            print("FAIL: kernel dependency was not rejected")
            return 1
    return 0


def check_parent_escape_fails() -> int:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        write_tree(root, '#include "../private.h"\n')
        findings = scan_public_headers(root)
        if len(findings) != 1 or findings[0][2] != "../private.h":
            print("FAIL: parent-directory escape was not rejected")
            return 1
    return 0


def check_private_sdk_path_fails() -> int:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        write_tree(root, "#include <lib/private.h>\n")
        findings = scan_public_headers(root)
        if len(findings) != 1 or findings[0][2] != "lib/private.h":
            print("FAIL: private SDK path was not rejected")
            return 1
    return 0


def check_libgraphics_kernel_dependency_fails() -> int:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        write_tree(
            root,
            "#include <stdint.h>\n",
            libgraphics_text="#include <kernel/graphics.h>\n",
        )
        findings = scan_public_headers(root)
        if len(findings) != 1 or findings[0][2] != "kernel/graphics.h":
            print("FAIL: LIBGRAPHICS kernel dependency was not rejected")
            return 1
    return 0


def main() -> int:
    checks = (
        check_clean_public_dependencies,
        check_kernel_dependency_fails,
        check_parent_escape_fails,
        check_private_sdk_path_fails,
        check_libgraphics_kernel_dependency_fails,
    )
    for check in checks:
        if check() != 0:
            return 1
    print(f"NX-SDK public-header boundary tests: PASS ({len(checks)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
