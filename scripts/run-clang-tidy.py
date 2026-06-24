#!/usr/bin/env python3
import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path


SOURCE_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".mm"}


def resolve_compile_file(entry):
    file_path = Path(entry["file"])
    if file_path.is_absolute():
        return file_path.resolve()

    directory = Path(entry.get("directory", "."))
    return (directory / file_path).resolve()


def is_relative_to(path, parent):
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def find_clang_tidy(explicit_path):
    candidates = [
        explicit_path,
        os.environ.get("CLANG_TIDY"),
        shutil.which("clang-tidy"),
        "/opt/homebrew/opt/llvm/bin/clang-tidy",
        "/usr/local/opt/llvm/bin/clang-tidy",
    ]

    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return candidate

    raise FileNotFoundError("clang-tidy was not found. Set CLANG_TIDY or install LLVM.")


def main():
    parser = argparse.ArgumentParser(description="Run clang-tidy on project translation units.")
    parser.add_argument("--build-dir", default="build", help="CMake build directory containing compile_commands.json")
    parser.add_argument("--clang-tidy", default=None, help="Path to clang-tidy binary")
    parser.add_argument(
        "--source-root",
        action="append",
        default=["src", "tests"],
        help="Source root to lint. Can be passed multiple times.",
    )
    args = parser.parse_args()

    workspace = Path.cwd().resolve()
    build_dir = (workspace / args.build_dir).resolve()
    compile_commands = build_dir / "compile_commands.json"
    if not compile_commands.is_file():
        print(f"error: compile database not found: {compile_commands}", file=sys.stderr)
        return 1

    source_roots = [(workspace / root).resolve() for root in args.source_root]
    with compile_commands.open(encoding="utf-8") as handle:
        entries = json.load(handle)

    files = sorted(
        {
            path
            for entry in entries
            for path in [resolve_compile_file(entry)]
            if path.suffix in SOURCE_EXTENSIONS and any(is_relative_to(path, root) for root in source_roots)
        }
    )

    if not files:
        print("error: no project source files found in compile_commands.json", file=sys.stderr)
        return 1

    clang_tidy = find_clang_tidy(args.clang_tidy)
    print(f"Running {clang_tidy} on {len(files)} translation units", flush=True)

    failed = []
    for file_path in files:
        print(f"clang-tidy: {file_path.relative_to(workspace)}", flush=True)
        result = subprocess.run(
            [clang_tidy, "-p", str(build_dir), "--quiet", str(file_path)],
            check=False,
        )
        if result.returncode != 0:
            failed.append(file_path)

    if failed:
        print("\nclang-tidy failed for:", file=sys.stderr)
        for file_path in failed:
            print(f"  {file_path.relative_to(workspace)}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
