#!/usr/bin/env bash
# Tail adb logcat for PastViewer. Handy for debugging network/SSL and crashes.
# Usage: ./deployScripts/android/adb-log-app.sh [logcat args...]
# Examples:
#   ./deployScripts/android/adb-log-app.sh
#   ./deployScripts/android/adb-log-app.sh -s  # brief format
#   ./deployScripts/android/adb-log-app.sh *:V  # verbose (then filter in terminal)

set -euo pipefail

PACKAGE="org.qtproject.PastViewer"

# Optional: clear logcat first
CLEAR="${CLEAR:-0}"
for arg in "$@"; do
  if [[ "$arg" == "--clear" ]]; then
    CLEAR=1
    break
  fi
done

if ! command -v adb &>/dev/null; then
  echo "Error: adb not found. Add Android SDK platform-tools to PATH." >&2
  exit 1
fi

if ! adb devices | grep -q 'device$'; then
  echo "Error: no device/emulator connected (run: adb devices)" >&2
  exit 1
fi

if [[ "$CLEAR" == "1" ]]; then
  adb logcat -c
fi

# Filter logcat to this app's process (by PID). If app not running, tail all logs so you can start the app and see output.
run_logcat() {
  local pid
  pid=$(adb shell pidof "$PACKAGE" 2>/dev/null || true)
  if [[ -n "$pid" ]]; then
    adb logcat --pid="$pid" "$@"
  else
    echo "App not running. Start PastViewer on the device; this will show all logcat until you Ctrl+C." >&2
    echo "Tip: run again when the app is already running to filter by PID only." >&2
    adb logcat "$@"
  fi
}

# Remove --clear from args we pass to logcat
args=()
for arg in "$@"; do
  [[ "$arg" != "--clear" ]] && args+=("$arg")
done

run_logcat "${args[@]}"
