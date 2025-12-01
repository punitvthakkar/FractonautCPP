#!/bin/bash
set -e

# ========== CONFIGURATION ==========
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build-android"

# Android paths (adjust to your installation)
export ANDROID_HOME="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
export ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-$ANDROID_HOME/ndk/26.1.10909125}"
export JAVA_HOME="${JAVA_HOME:-/opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home}"

# Qt paths - **ADJUST THESE TO YOUR INSTALLATION!**
QT_VERSION="${QT_VERSION:-6.7.2}"  # Change to your Qt version
QT_HOST_PATH="${QT_HOST_PATH:-/opt/homebrew/opt/qt@6}"
QT_ANDROID_PATH="${QT_ANDROID_PATH:-$HOME/Qt/$QT_VERSION/android_arm64_v8a}"

# Build configuration
ANDROID_ABI="${ANDROID_ABI:-arm64-v8a}"  # Use armeabi-v7a for 32-bit
ANDROID_PLATFORM="${ANDROID_PLATFORM:-android-26}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

# ========== COLORS ==========
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# ========== VALIDATION ==========
echo -e "${YELLOW}Validating environment...${NC}"

if [ ! -d "$ANDROID_HOME" ]; then
    echo -e "${RED}ERROR: Android SDK not found at $ANDROID_HOME${NC}"
    echo "Please install Android SDK or set ANDROID_HOME environment variable"
    echo "Visit: https://developer.android.com/studio#command-line-tools-only"
    exit 1
fi

if [ ! -d "$ANDROID_NDK_ROOT" ]; then
    echo -e "${RED}ERROR: Android NDK not found at $ANDROID_NDK_ROOT${NC}"
    echo "Install with: sdkmanager \"ndk;26.1.10909125\""
    exit 1
fi

if [ ! -d "$JAVA_HOME" ]; then
    echo -e "${RED}ERROR: Java JDK not found at $JAVA_HOME${NC}"
    echo "Install with: brew install openjdk@17"
    exit 1
fi

if [ ! -d "$QT_ANDROID_PATH" ]; then
    echo -e "${RED}ERROR: Qt for Android not found at $QT_ANDROID_PATH${NC}"
    echo ""
    echo "Please install Qt for Android from: https://www.qt.io/download"
    echo "Or set QT_ANDROID_PATH environment variable to your Qt Android installation"
    echo ""
    echo "Common locations:"
    echo "  - ~/Qt/6.7.2/android_arm64_v8a"
    echo "  - ~/Qt/6.x.x/android_arm64_v8a"
    exit 1
fi

echo -e "${GREEN}✓ Environment validated${NC}"
echo "  Android SDK: $ANDROID_HOME"
echo "  Android NDK: $ANDROID_NDK_ROOT"
echo "  Java: $JAVA_HOME"
echo "  Qt Android: $QT_ANDROID_PATH"
echo ""

# ========== BUILD ==========
echo -e "${YELLOW}Building for Android ($ANDROID_ABI)...${NC}"

# Clean and create build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Running CMake configuration..."
cmake \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI="$ANDROID_ABI" \
  -DANDROID_PLATFORM="$ANDROID_PLATFORM" \
  -DANDROID_STL=c++_shared \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_FIND_ROOT_PATH="$QT_ANDROID_PATH" \
  -DQT_HOST_PATH="$QT_HOST_PATH" \
  -DCMAKE_PREFIX_PATH="$QT_ANDROID_PATH" \
  -DANDROID_SDK_ROOT="$ANDROID_HOME" \
  -DANDROID_NDK="$ANDROID_NDK_ROOT" \
  ..

echo -e "${GREEN}✓ CMake configuration complete${NC}"

# Build
echo "Building project..."
cmake --build . --parallel $(sysctl -n hw.ncpu)

echo -e "${GREEN}✓ Build complete${NC}"

# ========== PACKAGE APK ==========
echo -e "${YELLOW}Packaging APK...${NC}"

DEPLOYMENT_JSON="android-Fractonaut-deployment-settings.json"

if [ ! -f "$DEPLOYMENT_JSON" ]; then
    echo -e "${RED}ERROR: Deployment settings not found: $DEPLOYMENT_JSON${NC}"
    echo "Make sure CMake generated this file"
    exit 1
fi

"$QT_ANDROID_PATH/bin/androiddeployqt" \
  --input "$DEPLOYMENT_JSON" \
  --output android-build \
  --android-platform android-34 \
  --jdk "$JAVA_HOME" \
  --gradle

# Copy APK to project root
APK_DEBUG="android-build/build/outputs/apk/debug/android-build-debug.apk"
APK_RELEASE="android-build/build/outputs/apk/release/android-build-release-unsigned.apk"

if [ -f "$APK_DEBUG" ]; then
    cp "$APK_DEBUG" "$PROJECT_DIR/Fractonaut.apk"
    echo -e "${GREEN}✓ APK created: $PROJECT_DIR/Fractonaut.apk${NC}"
fi

if [ -f "$APK_RELEASE" ]; then
    cp "$APK_RELEASE" "$PROJECT_DIR/Fractonaut-release.apk"
    echo -e "${GREEN}✓ Release APK created: $PROJECT_DIR/Fractonaut-release.apk${NC}"
fi

# Show file size
if [ -f "$PROJECT_DIR/Fractonaut.apk" ]; then
    SIZE=$(du -h "$PROJECT_DIR/Fractonaut.apk" | cut -f1)
    echo ""
    echo "=========================================="
    echo -e "${GREEN}Build successful!${NC}"
    echo "APK: Fractonaut.apk ($SIZE)"
    echo "=========================================="
    echo ""
    echo "To install on your Android device:"
    echo "1. Enable Developer Options and USB Debugging on your phone"
    echo "2. Connect device via USB"
    echo "3. Run: adb install Fractonaut.apk"
    echo ""
    echo "Or transfer Fractonaut.apk to your phone and install manually"
    echo ""
fi
