#!/usr/bin/env python3
"""Run the unchanged Power12 auditor set against an explicit USERKIT project."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

AUDITS = ("audit_build.py", "audit_limits.py", "audit_ast.py")
HERE = Path(__file__).resolve().parent


def digest_inputs(project: Path, policy: Path) -> str:
    digest = hashlib.sha256()
    paths = [policy, project / "Makefile", project / "catalog.mk", project / "catalog.json"]
    for root_name in ("sbin", "services", "tests"):
        root = project / root_name
        if root.is_dir():
            paths.extend(path for path in root.rglob("*") if path.is_file())
    paths.extend(HERE.glob("*.py"))
    for path in sorted(set(paths), key=lambda item: item.as_posix()):
        if not path.is_file():
            continue
        digest.update(path.as_posix().encode())
        digest.update(b"\0")
        digest.update(hashlib.sha256(path.read_bytes()).digest())
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-root", type=Path, required=True)
    parser.add_argument("--policy", type=Path, required=True)
    parser.add_argument("--sdk-root", type=Path, required=True)
    parser.add_argument("--report-dir", type=Path, required=True)
    parser.add_argument("--max-print", type=int, default=80)
    args = parser.parse_args()
    project = args.project_root.resolve()
    policy = args.policy.resolve()
    env = os.environ.copy()
    env["NEXS_POWER12_PROJECT_ROOT"] = str(project)
    env["NEXS_POWER12_POLICY"] = str(policy)
    env["NEXS_SDK_ROOT"] = str(args.sdk_root.resolve())
    args.report_dir.mkdir(parents=True, exist_ok=True)
    statuses = {}
    tests = subprocess.run([sys.executable, str(HERE / "test_auditors.py")],
                           cwd=project, env=env, text=True,
                           capture_output=True, check=False)
    sys.stdout.write(tests.stdout)
    sys.stderr.write(tests.stderr)
    statuses["mutation_tests"] = tests.returncode
    for script in AUDITS:
        report = args.report_dir / script.replace(".py", ".json")
        run = subprocess.run([sys.executable, str(HERE / script),
                              "--json", str(report), "--max-print", str(args.max_print)],
                             cwd=project, env=env, text=True,
                             capture_output=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        statuses[script] = run.returncode
    manifest = {
        "profile": json.loads(policy.read_text())["profile"],
        "source_digest": digest_inputs(project, policy),
        "verified": not any(code != 0 for code in statuses.values()),
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "statuses": statuses,
    }
    (args.report_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    failed = [name for name, code in statuses.items() if code != 0]
    if failed:
        print("Power12 gate: FAIL — " + ", ".join(failed))
        return 1
    print("Power12 gate: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
