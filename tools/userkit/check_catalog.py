#!/usr/bin/env python3
from pathlib import Path
import argparse
import json
import re
import sys


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdk-root", type=Path, required=True)
    parser.add_argument("--project-root", type=Path, required=True)
    return parser.parse_args()


def mk_list(text: str, name: str) -> list[str]:
    logical = re.sub(r"\\\n[ \t]*", " ", text)
    match = re.search(rf"^{re.escape(name)}[ \t]*:=[ \t]*(.+)$", logical, re.MULTILINE)
    return match.group(1).split() if match else []


def interface_headers(text: str) -> list[str]:
    match = re.search(r"^NXSDK_EXTENSION_INTERFACE_HEADERS[ \t]*:=[ \t]*(.*)$", text, re.MULTILINE)
    return match.group(1).split() if match else []


def header_exists(sdk: Path, header: str) -> bool:
    roots = [sdk / "include/api", sdk / "include/contract", sdk / "lib/extensions"]
    for root in roots:
        if root.is_dir() and any(path.is_file() for path in root.rglob(header)):
            return True
    return False


def sdk_project_mk(sdk: Path, project: str) -> Path | None:
    matches = list((sdk / "lib/extensions/USERKIT").rglob("project.mk"))
    for path in matches:
        text = path.read_text()
        if re.search(rf"^NXSDK_EXTENSION_PROJECT[ \t]*:=[ \t]*{re.escape(project)}$", text, re.MULTILINE):
            return path
    return None


def service_dir(project_root: Path, project: str, example: bool) -> Path:
    if example:
        return project_root / "services/example" / project
    return project_root / "services" / project


def validate_sdk_project(sdk: Path, project: str, errors: list[str]) -> None:
    project_mk = sdk_project_mk(sdk, project)
    if project_mk is None:
        errors.append(f"{project}: missing SDK USERKIT extension project")
        return
    text = project_mk.read_text()
    if "NXSDK_EXTENSION_CLASS := USERKIT" not in text:
        errors.append(f"{project}: SDK extension class is not USERKIT")
    headers = interface_headers(text)
    if not headers:
        errors.append(f"{project}: SDK extension exposes no interface headers")
    for header in headers:
        if not header_exists(sdk, header):
            errors.append(f"{project}: interface header not found: {header}")


def main() -> int:
    args = parse_args()
    root = args.project_root.resolve()
    sdk = args.sdk_root.resolve()
    catalog = json.loads((root / "catalog.json").read_text())
    mk_text = (root / "catalog.mk").read_text()
    base = catalog.get("services", [])
    examples = catalog.get("examples", [])
    sbin = catalog.get("sbin", [])
    errors: list[str] = []

    base_ids = [item.get("project", "") for item in base]
    example_ids = [item.get("project", "") for item in examples]
    sbin_ids = [item.get("project", "") for item in sbin]
    if mk_list(mk_text, "NEXS_USERKIT_SERVICE_PROJECTS") != base_ids:
        errors.append("base service catalog.mk mismatch")
    if mk_list(mk_text, "NEXS_USERKIT_EXAMPLE_PROJECTS") != example_ids:
        errors.append("example service catalog.mk mismatch")
    if mk_list(mk_text, "NEXS_USERKIT_SBIN_PROJECTS") != sbin_ids:
        errors.append("sbin catalog.mk mismatch")

    forbidden = {"nxenvinit", "nxui", "nxbar", "nxlauncher", "nxshell"}
    leaked = sorted(forbidden.intersection(base_ids + example_ids))
    if leaked:
        errors.append("non-service projects in USERKIT: " + ", ".join(leaked))

    for item, example in [(entry, False) for entry in base] + [(entry, True) for entry in examples]:
        project = item.get("project", "")
        executable = item.get("executable", "")
        directory = service_dir(root, project, example)
        for rel in ("project.mk", "src/main.c", "tests/project_test.py", "tests/runtime.c"):
            if not (directory / rel).is_file():
                errors.append(f"{project}: missing {rel}")
        if executable != f"/sys/services/{project}":
            errors.append(f"{project}: noncanonical executable {executable}")
        validate_sdk_project(sdk, project, errors)

    for item in sbin:
        project = item.get("project", "")
        executable = item.get("executable", "")
        directory = root / "sbin" / project
        for rel in ("project.mk", "src/main.c", "tests/project_test.py"):
            if not (directory / rel).is_file():
                errors.append(f"{project}: missing {rel}")
        if executable != f"/sbin/{project}":
            errors.append(f"{project}: noncanonical executable {executable}")
        validate_sdk_project(sdk, project, errors)

    if not (root / "sbin/common/nxinit_runtime.c").is_file():
        errors.append("missing shared sbin init runtime")
    validate_sdk_project(sdk, "sbin-common", errors)

    if errors:
        for error in errors:
            print("USERKIT catalog FAIL: " + error, file=sys.stderr)
        return 1
    print(f"USERKIT catalog PASS: {len(base)} base services, {len(examples)} examples, {len(sbin)} sbin projects")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
