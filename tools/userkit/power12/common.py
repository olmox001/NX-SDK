#!/usr/bin/env python3
"""Shared infrastructure for the NEXS Power12 build gate."""
from __future__ import annotations
import json
import os
import subprocess
from pathlib import Path

ROOT = Path(os.environ["NEXS_POWER12_PROJECT_ROOT"]).resolve()
POLICY_PATH = Path(os.environ["NEXS_POWER12_POLICY"]).resolve()

def sdk_root() -> Path:
    configured = os.environ.get("NEXS_SDK_ROOT", "")
    if not configured:
        raise RuntimeError("NEXS_SDK_ROOT is required")
    root = Path(configured).resolve()
    if not (root / "include/api").is_dir() or not (root / "include/contract").is_dir():
        raise RuntimeError(f"invalid NX-SDK root: {root}")
    return root


def sdk_extension_include_dirs(root: Path) -> list[Path]:
    config = root / "config" / "extension-paths.mk"
    if not config.is_file():
        raise RuntimeError(f"missing NX-SDK extension path catalog: {config}")
    paths: list[Path] = []
    collecting = False
    for raw in config.read_text().splitlines():
        line = raw.strip()
        if line.startswith("NXSDK_EXTENSION_INCLUDE_REL :="):
            collecting = True
            line = line.split(":=", 1)[1].strip()
        elif not collecting:
            continue
        token = line.rstrip("\\").strip()
        if token:
            paths.append(root / token)
        if not line.endswith("\\"):
            break
    if not paths:
        raise RuntimeError(f"empty NX-SDK extension include catalog: {config}")
    return paths

def sdk_userkit_include_dirs(root: Path) -> list[Path]:
    config = root / "config" / "userkit-paths.mk"
    if not config.is_file():
        raise RuntimeError(f"missing NX-SDK USERKIT path catalog: {config}")
    paths: list[Path] = []
    collecting = False
    for raw in config.read_text().splitlines():
        line = raw.strip()
        if line.startswith("NXSDK_USERKIT_INCLUDE_REL :="):
            collecting = True
            line = line.split(":=", 1)[1].strip()
        elif not collecting:
            continue
        token = line.rstrip("\\").strip()
        if token:
            paths.append(root / token)
        if not line.endswith("\\"):
            break
    if not paths:
        raise RuntimeError(f"empty NX-SDK USERKIT include catalog: {config}")
    return paths

def load_policy() -> dict:
    return json.loads(POLICY_PATH.read_text())

def relpath(path: Path) -> str:
    return path.resolve().relative_to(ROOT.resolve()).as_posix()

def excluded(rel: str, policy: dict) -> bool:
    return any(rel == p.rstrip("/") or rel.startswith(p)
               for p in policy["excluded_prefixes"])

def owned_tree_files(policy: dict) -> list[Path]:
    found: dict[str, Path] = {}
    for root_name in policy["source_roots"]:
        base = ROOT / root_name
        if not base.exists():
            continue
        for base_dir, dir_names, file_names in os.walk(base):
            base_path = Path(base_dir)
            rel_dir = relpath(base_path)
            dir_names[:] = [name for name in dir_names
                            if not excluded(f"{rel_dir}/{name}/", policy)]
            for name in file_names:
                path = base_path / name
                rel = relpath(path)
                if not excluded(rel, policy):
                    found[rel] = path
    for pattern in policy["first_party_root_globs"]:
        for path in ROOT.glob(pattern):
            rel = relpath(path)
            if path.is_file() and not excluded(rel, policy):
                found[rel] = path
    return [found[key] for key in sorted(found)]

def owned_files(policy: dict, suffixes: tuple[str, ...] | None = None) -> list[Path]:
    wanted = set(suffixes or policy["extensions"])
    return [path for path in owned_tree_files(policy) if path.suffix in wanted]

def source_arches(rel: str, policy: dict) -> list[str]:
    if rel.startswith("kernel/HAL/tests/"):
        return ["host"]
    if "/arch/amd64/" in rel or "/amd64/" in rel:
        return ["amd64"]
    if "/arch/aarch64/" in rel or "/arm64/" in rel or "/aarch64/" in rel:
        return ["aarch64"]
    # Structural rules are source properties. Common files are parsed once;
    # real cross-builds remain the architecture-specific R10 evidence.
    return ["amd64"]

def clang_args(rel: str, arch: str) -> list[str]:
    if arch == "host":
        target_args = []
        arch_define = "-DARCH_AMD64"
        arch_inc = "kernel/HAL/host/arch/amd64/include"
    elif arch == "amd64":
        target_args = ["--target=x86_64-unknown-none-elf"]
        arch_define = "-DARCH_AMD64"
        arch_inc = "kernel/HAL/host/arch/amd64/include"
    else:
        target_args = ["--target=aarch64-unknown-none-elf"]
        arch_define = "-DARCH_AARCH64"
        arch_inc = "kernel/HAL/host/arch/aarch64/include"
    args = [
        "clang", *target_args, "-std=gnu17", "-ffreestanding",
        "-fno-builtin", "-Wno-unknown-warning-option", "-Wno-unused-command-line-argument",
        arch_define,
    ]
    if rel.startswith("kernel/") and arch != "host":
        args += ["-DKERNEL", "-DNX_MDEBUG_BOOT=1"]
    # Host verification of HAL tests must use the same contract/header
    # dependencies as its compiled ISA-specific test executable. Omitting
    # Core ABI and provider includes made valid native-frame tests unparseable;
    # this is a compile-context correction, not an audit exemption.
    sdk = sdk_root()
    include_dirs = [
        sdk / "include/contract",
        sdk / "include/api",
        *sdk_extension_include_dirs(sdk),
        *sdk_userkit_include_dirs(sdk),
        ROOT / "tests/runtime",
    ]
    for item in include_dirs:
        args += ["-I", str(item)]
    return args

def clang_ast(path: Path, arch: str) -> tuple[dict | None, str]:
    rel = relpath(path)
    cmd = clang_args(rel, arch) + [
        "-Xclang", "-ast-dump=json", "-fsyntax-only", str(path)
    ]
    run = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True,
                         check=False, timeout=45)
    if run.returncode != 0:
        return None, run.stderr.strip()
    try:
        return json.loads(run.stdout), ""
    except json.JSONDecodeError as exc:
        return None, f"invalid clang AST JSON: {exc}"

def node_children(node: dict) -> list[dict]:
    return [x for x in node.get("inner", ()) if isinstance(x, dict)]

def walk(node: dict):
    stack = [node]
    while stack:
        current = stack.pop()
        yield current
        stack.extend(reversed(node_children(current)))

def line_for(node: dict, data: bytes) -> int:
    loc = node.get("loc", {})
    if not loc:
        loc = node.get("range", {}).get("begin", {})
    line = loc.get("line")
    if isinstance(line, int):
        return line
    offset = loc.get("offset")
    if isinstance(offset, int) and 0 <= offset <= len(data):
        return data[:offset].count(b"\n") + 1
    return 0

def source_definitions(ast: dict, path: Path) -> list[dict]:
    data = path.read_bytes()
    resolved = path.resolve()
    result = []
    for node in ast.get("inner", ()):
        if not isinstance(node, dict) or node.get("kind") != "FunctionDecl":
            continue
        if node.get("isImplicit"):
            continue
        if not any(x.get("kind") == "CompoundStmt" for x in node_children(node)):
            continue
        loc = node.get("loc", {})
        if loc.get("includedFrom"):
            continue
        file_name = loc.get("file")
        if file_name and Path(file_name).resolve() != resolved:
            continue
        if line_for(node, data) == 0:
            continue
        result.append(node)
    return result

def finding(rule: str, rel: str, line: int, symbol: str, detail: str,
            arch: str = "all") -> dict:
    return {
        "rule": rule, "file": rel, "line": int(line or 0),
        "symbol": symbol, "arch": arch, "detail": detail,
    }
