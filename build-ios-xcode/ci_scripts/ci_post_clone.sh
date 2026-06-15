#!/bin/sh -e

# Generates build-ios-xcode/PastViewer.xcodeproj for Xcode Cloud.
# Configure secrets in App Store Connect → Xcode Cloud → Environment:
#   OSM_API_KEY, SENTRY_DSN, APPLE_TEAM_ID, APPLE_PROVISION_PROFILE_NAME
# Optional overrides:
#   QT_VERSION, QT_IOS, QT_MACOS

echo "[ci_post_clone] PastViewer iOS Xcode project generation"

REPO_ROOT="${CI_PRIMARY_REPOSITORY_PATH:-$(cd "$(dirname "$0")/../.." && pwd)}"
BUILD_DIR="${REPO_ROOT}/build-ios-xcode"
QT_VERSION="${QT_VERSION:-6.10.2}"
QT_BASE="${QT_BASE:-$HOME/Qt/$QT_VERSION}"

require_env() {
    if [ -z "$(eval "printf '%s' \"\${$1:-}\"")" ]; then
        echo "[ci_post_clone] ERROR: $1 is not set (add it in Xcode Cloud environment variables)" >&2
        exit 1
    fi
}

pip_user_bin() {
    python3 -m site --user-base 2>/dev/null | sed 's/$/\/bin/'
}

ensure_pip_user_path() {
    _pip_bin="$(pip_user_bin)"
    if [ -d "$_pip_bin" ]; then
        export PATH="$_pip_bin:$PATH"
    fi
}

require_env OSM_API_KEY
require_env SENTRY_DSN
require_env APPLE_TEAM_ID
require_env APPLE_PROVISION_PROFILE_NAME

APPLE_APP_REVERSED_DOMAIN="${APPLE_APP_REVERSED_DOMAIN:-com.dv.pastviewer}"

install_conan() {
    ensure_pip_user_path
    if command -v conan >/dev/null 2>&1; then
        return
    fi
    echo "[ci_post_clone] Installing Conan"
    python3 -m pip install --user conan
    ensure_pip_user_path
    if ! command -v conan >/dev/null 2>&1; then
        echo "[ci_post_clone] ERROR: conan not found after pip install (PATH=$(pip_user_bin))" >&2
        exit 1
    fi
    conan profile detect
}

install_qt() {
    if [ -n "${QT_IOS:-}" ] && [ -n "${QT_MACOS:-}" ]; then
        echo "[ci_post_clone] Using QT_IOS=$QT_IOS QT_MACOS=$QT_MACOS"
        return
    fi

    echo "[ci_post_clone] Installing Qt $QT_VERSION via aqtinstall"
    python3 -m pip install --user aqtinstall

    DESKTOP_MODULES="qtlanguageserver qtmultimedia qtlocation qtpositioning qtshadertools"
    IOS_MODULES="qtmultimedia qtlocation qtpositioning qtshadertools"

    python3 -m aqt install-qt mac desktop "$QT_VERSION" clang_64 -O "$HOME/Qt" -m $DESKTOP_MODULES
    python3 -m aqt install-qt mac ios "$QT_VERSION" ios -O "$HOME/Qt" -m $IOS_MODULES

    QT_MACOS="$QT_BASE/macos"
    QT_IOS="$QT_BASE/ios"
    export QT_MACOS QT_IOS
}

ensure_ios_conan_profile() {
    PROFILE="${REPO_ROOT}/profiles/ios_device"
    if [ -f "$PROFILE" ]; then
        return
    fi

    echo "[ci_post_clone] Creating default Conan profile at profiles/ios_device"
    mkdir -p "${REPO_ROOT}/profiles"
    cat >"$PROFILE" <<'EOF'
[settings]
os=iOS
os.version=16.0
os.sdk=iphoneos
arch=armv8
compiler=apple-clang
compiler.cppstd=20
compiler.libcxx=libc++
build_type=Release
EOF
}

install_cmake() {
    if command -v cmake >/dev/null 2>&1; then
        cmake_version=$(cmake --version | sed -n '1s/cmake version //p')
        echo "[ci_post_clone] Found cmake $cmake_version"
        return
    fi
    echo "[ci_post_clone] Installing CMake via Homebrew"
    brew install cmake
}

install_conan
install_qt
install_cmake

QT_MACOS="${QT_MACOS:-$QT_BASE/macos}"
QT_IOS="${QT_IOS:-$QT_BASE/ios}"

if [ ! -f "$QT_IOS/lib/cmake/Qt6/qt.toolchain.cmake" ]; then
    echo "[ci_post_clone] ERROR: Qt iOS toolchain not found at $QT_IOS" >&2
    exit 1
fi

if [ ! -f "$QT_MACOS/bin/qt-cmake" ] && [ ! -d "$QT_MACOS/lib/cmake/Qt6" ]; then
    echo "[ci_post_clone] ERROR: Qt macOS host installation not found at $QT_MACOS" >&2
    exit 1
fi

ensure_ios_conan_profile

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

ensure_pip_user_path
echo "[ci_post_clone] Installing Conan dependencies"
CLANG_MAJOR=$(clang --version | sed -n 's/.*clang version \([0-9]*\).*/\1/p')

echo "[ci_post_clone] Testing curl.se connectivity..."
curl -fsSI https://curl.haxx.se/ca/cacert.pem || echo "curl.se unreachable"
nslookup curl.haxx.se

conan install "$REPO_ROOT" \
    --profile="${REPO_ROOT}/profiles/ios_device" \
    --output-folder="$BUILD_DIR" \
    --build=missing \
    -s build_type=Release \
    -s os=iOS \
    -s os.version=16.0 \
    -s os.sdk=iphoneos \
    -s arch=armv8 \
    -s compiler.cppstd=20 \
    -s compiler.version="$CLANG_MAJOR"

if command -v brew >/dev/null 2>&1 && ! command -v protoc >/dev/null 2>&1; then
    echo "[ci_post_clone] Installing protobuf (needed by some Qt tooling)"
    brew install protobuf
    export PATH="$(brew --prefix protobuf)/bin:$PATH"
fi

echo "[ci_post_clone] Configuring CMake Xcode project"
cmake "$REPO_ROOT" \
    -G Xcode \
    -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$QT_IOS/lib/cmake/Qt6/qt.toolchain.cmake" \
    -DQT_HOST_PATH="$QT_MACOS" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0 \
    -DOSM_API_KEY="$OSM_API_KEY" \
    -DAPPLE_TEAM_ID="$APPLE_TEAM_ID" \
    -DAPPLE_APP_REVERSED_DOMAIN="$APPLE_APP_REVERSED_DOMAIN" \
    -DAPPLE_PROVISION_PROFILE_NAME="$APPLE_PROVISION_PROFILE_NAME" \
    -DSENTRY_DSN="$SENTRY_DSN"

if [ ! -f "$BUILD_DIR/PastViewer.xcodeproj/project.pbxproj" ]; then
    echo "[ci_post_clone] ERROR: PastViewer.xcodeproj was not generated" >&2
    exit 1
fi

echo "[ci_post_clone] Done: $BUILD_DIR/PastViewer.xcodeproj"
