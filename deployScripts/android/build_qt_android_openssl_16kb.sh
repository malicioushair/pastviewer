#!/usr/bin/env bash
# Build Qt 6.10 for Android with OpenSSL TLS support and 16 KB page alignment.
# Prereqs: Android NDK 29, Android SDK, ninja, cmake. OpenSSL for Android from
# this repo's ext/android_openssl (ssl_3 with arm64-v8a libs + include).
#
# macOS workaround: the script applies a patch to Qt's QtCompressMimeDatabase.cmake
# so that on Darwin it uses external gzip instead of CMake's file(ARCHIVE_CREATE),
# avoiding "Could not open extended attribute file" during the mime database step.
#
# Usage:
#   ./deployScripts/android/build_qt_android_openssl_16kb.sh
#
# Options (env):
#   QT_SOURCE_DIR     - Where to clone Qt (default: ../qt6-src relative to repo)
#   QT_HOST_BUILD    - Host build dir (default: ../qt6-host-build)
#   QT_ANDROID_BUILD - Android build dir (default: ../qt6-android-build)
#   INSTALL_PREFIX   - Install Qt Android here (default: $HOME/Qt/6.10.2/android_arm64_v8a_openssl_16kb)
#   ANDROID_NDK      - NDK path (default: $HOME/Library/Android/sdk/ndk/29.0.14206865)
#   ANDROID_SDK      - SDK path (default: $HOME/Library/Android/sdk)
#   DO_CLONE=1       - Clone Qt source if missing (default: 1)
#   DO_HOST=1        - Build host tools (default: 1)
#   DO_ANDROID=1     - Configure and build Android Qt (default: 1)
#   DO_INSTALL=1     - Install after build (default: 1)
#   DO_CLEAN=0       - If 1, remove host and Android build dirs so new submodules are picked up (default: 0)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
OPENSSL_ANDROID="$REPO_ROOT/ext/android_openssl"
OPENSSL_INCLUDE="$OPENSSL_ANDROID/ssl_3/include"
OPENSSL_LIB_DIR="$OPENSSL_ANDROID/ssl_3/arm64-v8a"

# Defaults
QT_SOURCE_DIR="${QT_SOURCE_DIR:-$(cd "$REPO_ROOT/.." && pwd)/qt6-src}"
QT_HOST_BUILD="${QT_HOST_BUILD:-$(cd "$REPO_ROOT/.." && pwd)/qt6-host-build}"
QT_ANDROID_BUILD="${QT_ANDROID_BUILD:-$(cd "$REPO_ROOT/.." && pwd)/qt6-android-build}"
INSTALL_PREFIX="${INSTALL_PREFIX:-$HOME/Qt/6.10.2/android_arm64_v8a_openssl_16kb}"
ANDROID_NDK="${ANDROID_NDK:-$HOME/Library/Android/sdk/ndk/29.0.14206865}"
ANDROID_SDK="${ANDROID_SDK:-$HOME/Library/Android/sdk}"
DO_CLONE="${DO_CLONE:-1}"
DO_HOST="${DO_HOST:-1}"
DO_ANDROID="${DO_ANDROID:-1}"
DO_INSTALL="${DO_INSTALL:-1}"
DO_CLEAN="${DO_CLEAN:-0}"
QT_TAG="${QT_TAG:-v6.10.2}"

# Submodules required for PastViewer: Quick, QuickLayouts, QuickControls2, Location, Positioning, PositioningQuick, Multimedia
QT_SUBMODULES="qtbase qtshadertools qtdeclarative qtpositioning qtlocation qtmultimedia"


if [[ ! -d "$OPENSSL_INCLUDE" || ! -f "$OPENSSL_LIB_DIR/libssl_3.so" ]]; then
  echo "Error: OpenSSL for Android not found. Need $OPENSSL_INCLUDE and $OPENSSL_LIB_DIR/libssl_3.so" >&2
  exit 1
fi
if [[ ! -d "$ANDROID_NDK" ]]; then
  echo "Error: ANDROID_NDK not found: $ANDROID_NDK" >&2
  exit 1
fi
if [[ ! -d "$ANDROID_SDK" ]]; then
  echo "Error: ANDROID_SDK not found: $ANDROID_SDK" >&2
  exit 1
fi

echo "=== Qt Android build (OpenSSL + 16 KB) ==="
echo "  QT_SOURCE_DIR=$QT_SOURCE_DIR"
echo "  QT_HOST_BUILD=$QT_HOST_BUILD"
echo "  QT_ANDROID_BUILD=$QT_ANDROID_BUILD"
echo "  INSTALL_PREFIX=$INSTALL_PREFIX"
echo "  OpenSSL: $OPENSSL_ANDROID (ssl_3, arm64-v8a)"
echo "  16 KB: -Wl,-z,max-page-size=16384"
echo ""

# --- Clean build dirs so new submodules are picked up ---
if [[ "$DO_CLEAN" == "1" ]]; then
  echo "Cleaning host and Android build dirs (DO_CLEAN=1)..."
  rm -rf "$QT_HOST_BUILD" "$QT_ANDROID_BUILD"
  echo "Done cleaning."
fi

# --- Clone Qt 6.10.2 (supermodule) and init submodules for Quick, Location, Positioning, Multimedia ---
if [[ "$DO_CLONE" == "1" && ! -f "$QT_SOURCE_DIR/configure" ]]; then
  echo "Cloning Qt $QT_TAG..."
  mkdir -p "$(dirname "$QT_SOURCE_DIR")"
  if [[ ! -d "$QT_SOURCE_DIR" ]]; then
    git clone --depth 1 --branch "$QT_TAG" https://code.qt.io/qt/qt5.git "$QT_SOURCE_DIR"
  fi
  (cd "$QT_SOURCE_DIR" && git submodule update --init --recursive $QT_SUBMODULES)
  echo "Qt source ready (submodules: $QT_SUBMODULES)."
elif [[ -f "$QT_SOURCE_DIR/configure" ]]; then
  # Ensure all required submodules are inited (e.g. repo existed with only qtbase)
  echo "Ensuring Qt submodules: $QT_SUBMODULES"
  (cd "$QT_SOURCE_DIR" && git submodule update --init --recursive $QT_SUBMODULES)
fi

# --- macOS: patch Qt mime DB script to use external gzip (avoids extended-attribute error) ---
PATCH_FILE="${SCRIPT_DIR}/qt6-macos-mime-db-workaround.patch"
QT_MIME_CMAKE="${QT_SOURCE_DIR}/qtbase/src/corelib/QtCompressMimeDatabase.cmake"
if [[ -f "$QT_MIME_CMAKE" && -f "$PATCH_FILE" && "$(uname -s)" == "Darwin" ]]; then
  if ! grep -q "CMAKE_HOST_SYSTEM_NAME STREQUAL \"Darwin\"" "$QT_MIME_CMAKE" 2>/dev/null; then
    echo "Applying macOS workaround (QtCompressMimeDatabase.cmake)..."
    (cd "$QT_SOURCE_DIR" && patch -p1 --forward --dry-run < "$PATCH_FILE" >/dev/null 2>&1) && \
      (cd "$QT_SOURCE_DIR" && patch -p1 --forward < "$PATCH_FILE") || true
  fi
fi

if [[ ! -f "$QT_SOURCE_DIR/configure" ]]; then
  echo "Error: Qt source not found at $QT_SOURCE_DIR (run with DO_CLONE=1 or set QT_SOURCE_DIR)" >&2
  exit 1
fi

# --- Host build (minimal, for host tools) ---
if [[ "$DO_HOST" == "1" ]]; then
  echo "Configuring host Qt (minimal, for host_tools)..."
  mkdir -p "$QT_HOST_BUILD"
  cd "$QT_HOST_BUILD"
  "$QT_SOURCE_DIR/configure" -developer-build -nomake tests -nomake examples \
    -- -DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF
  echo "Building host_tools..."
  cmake --build . --target host_tools --parallel
  echo "Host build done."
fi

# -qt-host-path must be the qtbase dir inside the host build directory
QT_HOST_PATH="$QT_HOST_BUILD/qtbase"
if [[ ! -d "$QT_HOST_PATH" ]]; then
  echo "Error: Host build qtbase dir not found at $QT_HOST_PATH. Run with DO_HOST=1." >&2
  exit 1
fi

# --- Android build with OpenSSL and 16 KB ---
if [[ "$DO_ANDROID" == "1" ]]; then
  echo "Configuring Qt for Android (arm64-v8a, OpenSSL, 16 KB)..."
  mkdir -p "$QT_ANDROID_BUILD"
  cd "$QT_ANDROID_BUILD"
  "$QT_SOURCE_DIR/configure" \
    -prefix "$INSTALL_PREFIX" \
    -qt-host-path "$QT_HOST_PATH" \
    -android-abis arm64-v8a \
    -android-sdk "$ANDROID_SDK" \
    -android-ndk "$ANDROID_NDK" \
    -openssl-linked \
    -release \
    -nomake tests \
    -nomake examples \
    -- \
    -DOPENSSL_INCLUDE_DIR="$OPENSSL_INCLUDE" \
    -DOPENSSL_CRYPTO_LIBRARY="$OPENSSL_LIB_DIR/libcrypto_3.so" \
    -DOPENSSL_SSL_LIBRARY="$OPENSSL_LIB_DIR/libssl_3.so" \
    -DOPENSSL_USE_STATIC_LIBS=OFF \
    -DCMAKE_SHARED_LINKER_FLAGS="-Wl,-z,max-page-size=16384" \
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,-z,max-page-size=16384"

  echo "Building Qt for Android..."
  cmake --build . --parallel
  echo "Android build done."
fi

if [[ "$DO_INSTALL" == "1" && -d "$QT_ANDROID_BUILD" ]]; then
  echo "Installing to $INSTALL_PREFIX..."
  cd "$QT_ANDROID_BUILD"
  cmake --install .
  echo "Installed. Use this prefix for PastViewer Android build (and set QT_ANDROID_OPENSSL_PLUGIN if needed)."
fi

echo "Done."
