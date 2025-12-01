# Building Fractonaut for Android

This guide explains how to build Fractonaut as an Android APK on macOS.

## Prerequisites

### 1. Install Android Command Line Tools

```bash
# Create Android SDK directory
mkdir -p ~/Library/Android/sdk
cd ~/Library/Android/sdk

# Download Android command line tools (macOS)
# Visit: https://developer.android.com/studio#command-line-tools-only
# Or use direct link:
curl -O https://dl.google.com/android/repository/commandlinetools-mac-11076708_latest.zip
unzip commandlinetools-mac-11076708_latest.zip
mkdir -p cmdline-tools/latest
mv cmdline-tools/* cmdline-tools/latest/ 2>/dev/null || true

# Add to PATH (add these to ~/.zshrc or ~/.bash_profile)
export ANDROID_HOME=$HOME/Library/Android/sdk
export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin
export PATH=$PATH:$ANDROID_HOME/platform-tools
export PATH=$PATH:$ANDROID_HOME/build-tools/34.0.0
```

### 2. Install Required Android SDK Components

```bash
# Accept licenses
sdkmanager --licenses

# Install required components
sdkmanager "platform-tools" "platforms;android-34" "build-tools;34.0.0"
sdkmanager "ndk;26.1.10909125"
sdkmanager "cmake;3.22.1"
```

### 3. Install Java JDK

```bash
# Install OpenJDK 17 (required for Android Gradle Plugin 8.x)
brew install openjdk@17

# Add to PATH
export JAVA_HOME=/opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home
export PATH=$JAVA_HOME/bin:$PATH
```

### 4. Install Qt for Android

**Option A: Using Qt Online Installer (Recommended)**
1. Download Qt Online Installer from https://www.qt.io/download-qt-installer
2. Install Qt 6.x with the following components:
   - Qt 6.x for Android (arm64-v8a, armabi-v7a, x86_64)
   - CMake
   - Ninja

**Option B: Using Homebrew (if available)**
```bash
brew install qt@6
```

### 5. Set Qt Environment Variables

```bash
# Add to ~/.zshrc or ~/.bash_profile
export QT_HOST_PATH=/opt/homebrew/opt/qt@6
export QT_ANDROID_PATH=$HOME/Qt/6.x.x/android_arm64_v8a  # Adjust version
export PATH=$QT_ANDROID_PATH/bin:$PATH
```

## Building the APK

### Method 1: Using Qt's androiddeployqt Tool (Recommended)

```bash
#!/bin/bash

# Configuration
PROJECT_DIR="/Users/punitthakkar/Documents/Personal/Code/2025/FractonautCPP"
BUILD_DIR="$PROJECT_DIR/build-android"
ANDROID_ARCH="arm64-v8a"  # or "armeabi-v7a" for 32-bit

# Android SDK/NDK paths
export ANDROID_HOME="$HOME/Library/Android/sdk"
export ANDROID_NDK_ROOT="$ANDROID_HOME/ndk/26.1.10909125"
export JAVA_HOME="/opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home"

# Qt paths (adjust to your installation)
export QT_HOST_PATH="/opt/homebrew/opt/qt@6"
export QT_ANDROID_PATH="$HOME/Qt/6.7.2/android_arm64_v8a"  # Adjust version

# Clean and create build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake for Android
cmake \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=$ANDROID_ARCH \
  -DANDROID_PLATFORM=android-26 \
  -DANDROID_STL=c++_shared \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_FIND_ROOT_PATH="$QT_ANDROID_PATH" \
  -DQT_HOST_PATH="$QT_HOST_PATH" \
  -DCMAKE_PREFIX_PATH="$QT_ANDROID_PATH" \
  -DANDROID_SDK_ROOT="$ANDROID_HOME" \
  -DANDROID_NDK="$ANDROID_NDK_ROOT" \
  ..

# Build
cmake --build . --parallel $(sysctl -n hw.ncpu)

# Deploy to APK
$QT_ANDROID_PATH/bin/androiddeployqt \
  --input android-Fractonaut-deployment-settings.json \
  --output android-build \
  --android-platform android-34 \
  --jdk $JAVA_HOME \
  --gradle

# Copy APK to project root
cp android-build/build/outputs/apk/debug/android-build-debug.apk \
   "$PROJECT_DIR/Fractonaut-debug.apk"

echo "✓ APK created: $PROJECT_DIR/Fractonaut-debug.apk"
```

### Method 2: Quick Build Script

Save this as `build_android.sh` in your project root:

```bash
#!/bin/bash
set -e

# ========== CONFIGURATION ==========
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build-android"

# Android paths (adjust to your installation)
export ANDROID_HOME="$HOME/Library/Android/sdk"
export ANDROID_NDK_ROOT="$ANDROID_HOME/ndk/26.1.10909125"
export JAVA_HOME="/opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home"

# Qt paths (ADJUST THESE TO YOUR INSTALLATION!)
QT_VERSION="6.7.2"  # Change to your Qt version
QT_HOST_PATH="/opt/homebrew/opt/qt@6"
QT_ANDROID_PATH="$HOME/Qt/$QT_VERSION/android_arm64_v8a"

# Build configuration
ANDROID_ABI="arm64-v8a"  # Use armeabi-v7a for 32-bit
ANDROID_PLATFORM="android-26"
BUILD_TYPE="Release"

# ========== VALIDATION ==========
echo "Validating environment..."

if [ ! -d "$ANDROID_HOME" ]; then
    echo "ERROR: Android SDK not found at $ANDROID_HOME"
    exit 1
fi

if [ ! -d "$ANDROID_NDK_ROOT" ]; then
    echo "ERROR: Android NDK not found at $ANDROID_NDK_ROOT"
    exit 1
fi

if [ ! -d "$QT_ANDROID_PATH" ]; then
    echo "ERROR: Qt for Android not found at $QT_ANDROID_PATH"
    echo "Please install Qt for Android or update QT_ANDROID_PATH"
    exit 1
fi

echo "✓ Environment validated"

# ========== BUILD ==========
echo "Building for Android ($ANDROID_ABI)..."

# Clean and create build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
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

echo "✓ CMake configuration complete"

# Build
cmake --build . --parallel $(sysctl -n hw.ncpu)

echo "✓ Build complete"

# ========== PACKAGE APK ==========
echo "Packaging APK..."

DEPLOYMENT_JSON="android-Fractonaut-deployment-settings.json"

if [ ! -f "$DEPLOYMENT_JSON" ]; then
    echo "ERROR: Deployment settings not found: $DEPLOYMENT_JSON"
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
    cp "$APK_DEBUG" "$PROJECT_DIR/Fractonaut-debug.apk"
    echo "✓ Debug APK created: $PROJECT_DIR/Fractonaut-debug.apk"
fi

if [ -f "$APK_RELEASE" ]; then
    cp "$APK_RELEASE" "$PROJECT_DIR/Fractonaut-release.apk"
    echo "✓ Release APK created: $PROJECT_DIR/Fractonaut-release.apk"
fi

# Show file size
if [ -f "$PROJECT_DIR/Fractonaut-debug.apk" ]; then
    SIZE=$(du -h "$PROJECT_DIR/Fractonaut-debug.apk" | cut -f1)
    echo ""
    echo "=========================================="
    echo "Build successful!"
    echo "APK: Fractonaut-debug.apk ($SIZE)"
    echo "=========================================="
    echo ""
    echo "To install on your Android device:"
    echo "1. Enable Developer Options and USB Debugging"
    echo "2. Connect device via USB"
    echo "3. Run: adb install Fractonaut-debug.apk"
    echo ""
    echo "Or copy the APK to your phone and install manually"
fi
```

## Quick Start Commands

```bash
# 1. Navigate to project
cd /Users/punitthakkar/Documents/Personal/Code/2025/FractonautCPP

# 2. Make build script executable
chmod +x build_android.sh

# 3. Edit the script and adjust Qt paths
nano build_android.sh  # Update QT_VERSION and QT_ANDROID_PATH

# 4. Run the build
./build_android.sh

# 5. Install on connected device
adb install Fractonaut-debug.apk

# Or transfer to phone and install manually
```

## Troubleshooting

### "Qt for Android not found"
- Install Qt for Android using Qt Maintenance Tool or Qt Online Installer
- Update `QT_ANDROID_PATH` in the build script

### "Android SDK/NDK not found"
```bash
# Install via sdkmanager
sdkmanager "ndk;26.1.10909125"
sdkmanager "platform-tools" "platforms;android-34"
```

### "Gradle build failed"
```bash
# Update Gradle wrapper
cd android-build
./gradlew wrapper --gradle-version=8.4
```

### "Java version incompatible"
```bash
# Ensure Java 17 is in use
java -version  # Should show 17.x.x
export JAVA_HOME=/opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home
```

## Notes

- **GPU Precision**: Android devices typically support native double precision (GL_ARB_gpu_shader_fp64), so you should get 15-17 decimal digits of precision
- **Performance**: Arm64 (arm64-v8a) is recommended for modern Android devices
- **Testing**: Use `adb logcat` to see debug output from the app
- **File Size**: APK will be 20-40MB due to Qt libraries

## Signing for Release

To create a signed APK for Google Play:

```bash
# 1. Create keystore (one time)
keytool -genkey -v -keystore fractonaut-release.keystore \
  -alias fractonaut -keyalg RSA -keysize 2048 -validity 10000

# 2. Build release APK
./build_android.sh

# 3. Sign APK
jarsigner -verbose -sigalg SHA256withRSA -digestalg SHA-256 \
  -keystore fractonaut-release.keystore \
  Fractonaut-release.apk fractonaut

# 4. Verify signature
jarsigner -verify -verbose Fractonaut-release.apk
```
