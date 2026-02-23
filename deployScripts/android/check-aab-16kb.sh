#!/usr/bin/env bash
# Check an Android App Bundle (AAB) for 16 KB page size alignment.
# All native .so files must have LOAD segment alignment 2**14 (16384) for Google Play.
# Usage: ./check-aab-16kb.sh [path/to/app.aab]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Default AAB path (release bundle from this project's build)
DEFAULT_AAB="${REPO_ROOT}/build-android-release/android-build/build/outputs/bundle/release/android-build-release.aab"
AAB="${1:-$DEFAULT_AAB}"

if [[ ! -f "$AAB" ]]; then
  echo "Error: AAB not found: $AAB" >&2
  echo "Usage: $0 [path/to/app.aab]" >&2
  exit 1
fi

# Find llvm-objdump (prefer ANDROID_NDK_ROOT, then ANDROID_HOME/ndk)
if [[ -n "${ANDROID_NDK_ROOT:-}" ]]; then
  NDK="$ANDROID_NDK_ROOT"
elif [[ -n "${ANDROID_HOME:-}" ]]; then
  # Use latest NDK under SDK
  NDK=$(find "$ANDROID_HOME/ndk" -maxdepth 1 -type d -name "[0-9]*" 2>/dev/null | sort -V | tail -n1)
else
  NDK="/Users/$(whoami)/Library/Android/sdk/ndk/29.0.14206865"
fi

case "$(uname -s)" in
  Darwin)  PREBUILT="darwin-x86_64" ;;
  Linux)   PREBUILT="linux-x86_64" ;;
  *)       PREBUILT="darwin-x86_64" ;;
esac
OBJDUMP="${NDK}/toolchains/llvm/prebuilt/${PREBUILT}/bin/llvm-objdump"
if [[ ! -x "$OBJDUMP" ]]; then
  # Fallback: try the other common prebuilt
  if [[ "$PREBUILT" == "darwin-x86_64" ]]; then OBJDUMP="${NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-objdump"; else OBJDUMP="${NDK}/toolchains/llvm/prebuilt/darwin-x86_64/bin/llvm-objdump"; fi
fi
if [[ ! -x "$OBJDUMP" ]]; then
  echo "Error: llvm-objdump not found at $OBJDUMP. Set ANDROID_NDK_ROOT or ANDROID_HOME." >&2
  exit 1
fi

WORK_DIR=$(mktemp -d)
trap 'rm -rf "$WORK_DIR"' EXIT

echo "Checking: $AAB"
echo "Unpacking AAB..."
unzip -q "$AAB" -d "$WORK_DIR/aab"

LIB_DIR="$WORK_DIR/aab/base/lib/arm64-v8a"
if [[ ! -d "$LIB_DIR" ]]; then
  echo "Error: No arm64-v8a libs in AAB (missing base/lib/arm64-v8a)." >&2
  exit 1
fi

bad=0
ok=0
total=0

for f in "$LIB_DIR"/*.so; do
  [[ -f "$f" ]] || continue
  total=$((total + 1))
  name=$(basename "$f")
  align=$("$OBJDUMP" -p "$f" 2>/dev/null | grep "LOAD" | head -1)
  if echo "$align" | grep -q "2\*\*12\|2\*\*13"; then
    bad=$((bad + 1))
    echo "BAD (4KB): $name"
  elif echo "$align" | grep -q "2\*\*14"; then
    ok=$((ok + 1))
    echo "OK (16KB): $name"
  else
    echo "? $name"
  fi
done

echo "---"
echo "Summary: $ok OK (16KB), $bad BAD (4KB), total .so: $total"
if [[ $bad -eq 0 ]]; then
  echo "Result: All libs are 16 KB aligned."
  exit 0
else
  echo "Result: $bad lib(s) are not 16 KB aligned. Rebuild with 16 KB Qt/NDK and 16 KB dependencies (OpenSSL, Sentry, etc.)."
  exit 1
fi
