# Qt Cross-Platform Development Rules

> A comprehensive guide for building cross-platform applications with Qt and C++.
> Target platforms: Windows, macOS, iOS, Android

---

## Table of Contents

1. [Project Structure](#project-structure)
2. [CMake Configuration](#cmake-configuration)
3. [Qt Widgets vs QML](#qt-widgets-vs-qml)
4. [Memory Management](#memory-management)
5. [Signals and Slots](#signals-and-slots)
6. [Threading](#threading)
7. [Cross-Platform Guidelines](#cross-platform-guidelines)
8. [Mobile Development (iOS/Android)](#mobile-development)
9. [Deployment](#deployment)
10. [Performance](#performance)
11. [Common Pitfalls](#common-pitfalls)
12. [Coding Conventions](#coding-conventions)

---

## Project Structure

### Recommended Directory Layout

```
MyApp/
├── CMakeLists.txt              # Root CMake configuration
├── src/
│   ├── CMakeLists.txt          # Source CMake
│   ├── main.cpp                # Application entry point
│   ├── core/                   # Platform-independent business logic
│   │   ├── CMakeLists.txt
│   │   ├── engine.cpp
│   │   └── engine.h
│   ├── ui/                     # UI layer
│   │   ├── CMakeLists.txt
│   │   ├── mainwindow.cpp      # Qt Widgets (if using)
│   │   ├── mainwindow.h
│   │   └── qml/                # QML files (if using)
│   │       ├── main.qml
│   │       └── components/
│   └── platform/               # Platform-specific code
│       ├── android/
│       ├── ios/
│       ├── macos/
│       └── windows/
├── resources/
│   ├── resources.qrc           # Qt Resource file
│   ├── icons/
│   ├── images/
│   └── fonts/
├── tests/
│   ├── CMakeLists.txt
│   └── test_engine.cpp
├── android/                    # Android manifest and gradle
│   └── AndroidManifest.xml
├── ios/                        # iOS specific files
│   └── Info.plist
└── docs/
```

### Rules

- **ALWAYS** separate core business logic from UI code
- **NEVER** put platform-specific code in core modules
- **ALWAYS** use Qt Resource System (.qrc) for bundling assets
- **PREFER** relative paths within the project

---

## CMake Configuration

### Root CMakeLists.txt Template

```cmake
cmake_minimum_required(VERSION 3.16)

project(MyApp 
    VERSION 1.0.0 
    LANGUAGES CXX
    DESCRIPTION "Cross-platform Qt Application"
)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt Configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt - prefer Qt6, fallback to Qt5
find_package(Qt6 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Quick 
    QuickControls2
    Network
    Sql
    REQUIRED
)

# Platform detection
if(ANDROID)
    message(STATUS "Building for Android")
    add_definitions(-DPLATFORM_ANDROID)
elseif(IOS)
    message(STATUS "Building for iOS")
    add_definitions(-DPLATFORM_IOS)
elseif(APPLE)
    message(STATUS "Building for macOS")
    add_definitions(-DPLATFORM_MACOS)
elseif(WIN32)
    message(STATUS "Building for Windows")
    add_definitions(-DPLATFORM_WINDOWS)
elseif(UNIX)
    message(STATUS "Building for Linux")
    add_definitions(-DPLATFORM_LINUX)
endif()

# Add subdirectories
add_subdirectory(src)

# Enable testing
enable_testing()
add_subdirectory(tests)
```

### Rules

- **ALWAYS** set `CMAKE_AUTOMOC`, `CMAKE_AUTORCC`, `CMAKE_AUTOUIC` to ON
- **ALWAYS** use `target_link_libraries` with PRIVATE/PUBLIC/INTERFACE
- **NEVER** use `link_libraries()` (global scope)
- **PREFER** `find_package(Qt6 ...)` over `find_package(Qt5 ...)`
- **ALWAYS** specify minimum CMake version (3.16+ for Qt6)

---

## Qt Widgets vs QML

### Decision Matrix

| Factor | Use Qt Widgets | Use QML |
|--------|---------------|---------|
| Target Platform | Desktop only | Mobile + Desktop |
| UI Complexity | Forms, dialogs | Animated, fluid UIs |
| Team Experience | C++ heavy | Comfortable with declarative |
| Performance | CPU-bound UIs | GPU-accelerated UIs |
| Native Look | Required | Custom look acceptable |
| Development Speed | Slower | Faster iteration |

### Rules

- **USE QML** for mobile applications (iOS/Android)
- **USE QML** when you need animations and modern UI
- **USE Widgets** for traditional desktop applications
- **NEVER** mix Widgets and QML in the same window (use one or the other)
- **ALWAYS** expose C++ logic to QML via registered types or context properties

### Exposing C++ to QML

```cpp
// engine.h
#ifndef ENGINE_H
#define ENGINE_H

#include <QObject>
#include <QString>

class Engine : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)

public:
    explicit Engine(QObject *parent = nullptr);

    QString status() const;
    void setStatus(const QString &status);
    int progress() const;

    Q_INVOKABLE void startProcess();
    Q_INVOKABLE void stopProcess();

signals:
    void statusChanged();
    void progressChanged();
    void processCompleted(bool success);

private:
    QString m_status;
    int m_progress = 0;
};

#endif
```

```cpp
// main.cpp - Registering for QML
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "engine.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    // Method 1: Register as instantiable type
    qmlRegisterType<Engine>("com.myapp.core", 1, 0, "Engine");

    // Method 2: Register singleton
    qmlRegisterSingletonInstance("com.myapp.core", 1, 0, "AppEngine", new Engine(&app));

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    return app.exec();
}
```

```qml
// main.qml - Using C++ from QML
import QtQuick
import QtQuick.Controls
import com.myapp.core 1.0

ApplicationWindow {
    visible: true
    width: 400
    height: 600

    // Using singleton
    Label {
        text: AppEngine.status
    }

    Button {
        text: "Start"
        onClicked: AppEngine.startProcess()
    }

    // Or instantiate
    Engine {
        id: localEngine
        onProcessCompleted: (success) => {
            console.log("Completed:", success)
        }
    }
}
```

---

## Memory Management

### Qt Object Ownership Rules

```cpp
// CORRECT: Parent takes ownership
QWidget *parent = new QWidget();
QPushButton *button = new QPushButton("Click", parent);  // parent deletes button

// CORRECT: Explicitly set parent later
QWidget *widget = new QWidget();
widget->setParent(parentWidget);  // parentWidget now owns widget

// CORRECT: Using smart pointers for non-QObject or ownerless objects
auto data = std::make_unique<MyDataClass>();

// WRONG: Memory leak - no parent, no delete
QPushButton *button = new QPushButton("Click");  // LEAKED!

// WRONG: Double delete
QWidget *widget = new QWidget(parent);
delete widget;  // Parent will also try to delete!
```

### Rules

- **ALWAYS** pass a parent to QObject-derived classes when possible
- **NEVER** manually delete a QObject that has a parent
- **USE** `deleteLater()` for objects in signal/slot chains
- **PREFER** `std::unique_ptr` for non-QObject classes
- **NEVER** store QObject pointers in STL containers without careful ownership consideration
- **USE** `QPointer<T>` for safe weak references to QObjects

### Safe Pointer Usage

```cpp
// QPointer - becomes null when object is deleted
QPointer<QWidget> safePtr = someWidget;
if (safePtr) {  // Always check before use
    safePtr->show();
}

// QScopedPointer - RAII for QObjects without parent
QScopedPointer<QTemporaryFile> tempFile(new QTemporaryFile());
```

---

## Signals and Slots

### Modern Syntax (Preferred)

```cpp
// PREFERRED: Compile-time checked connections
connect(sender, &Sender::valueChanged, receiver, &Receiver::updateValue);

// With lambda
connect(button, &QPushButton::clicked, this, [this]() {
    handleClick();
});

// With lambda capturing values
connect(button, &QPushButton::clicked, this, [this, id = item.id]() {
    processItem(id);
});
```

### Rules

- **ALWAYS** use new-style `connect()` syntax (not SIGNAL/SLOT macros)
- **ALWAYS** include receiver object in lambda connects for proper cleanup
- **NEVER** connect to lambdas without a context object
- **USE** `Qt::QueuedConnection` for cross-thread signals
- **ALWAYS** disconnect or ensure sender/receiver lifetime management

### Cross-Thread Signals

```cpp
// Automatic queued connection when crossing threads
connect(workerThread, &Worker::resultReady, 
        this, &MainWindow::handleResult);  // Auto-detects threads

// Explicit queued connection
connect(sender, &Sender::signal, 
        receiver, &Receiver::slot, 
        Qt::QueuedConnection);
```

### Thread-Safe Signal Emission Rules

- Signals are **thread-safe** to emit
- Slots execute in the **receiver's thread** (with QueuedConnection)
- **NEVER** pass pointers to stack objects across threads

---

## Threading

### QThread Correct Usage

```cpp
// CORRECT: Worker object approach
class Worker : public QObject {
    Q_OBJECT
public slots:
    void doWork() {
        // Heavy processing here
        emit resultReady(result);
    }
signals:
    void resultReady(const QString &result);
};

// In main code
QThread *thread = new QThread(this);
Worker *worker = new Worker();  // No parent!
worker->moveToThread(thread);

connect(thread, &QThread::started, worker, &Worker::doWork);
connect(worker, &Worker::resultReady, this, &MyClass::handleResult);
connect(worker, &Worker::resultReady, thread, &QThread::quit);
connect(thread, &QThread::finished, worker, &Worker::deleteLater);
connect(thread, &QThread::finished, thread, &QThread::deleteLater);

thread->start();
```

### Rules

- **NEVER** subclass QThread (except for special cases)
- **ALWAYS** use worker object + `moveToThread()` pattern
- **NEVER** create GUI objects in non-GUI threads
- **USE** `QMutex`/`QMutexLocker` for shared data access
- **PREFER** `QtConcurrent::run()` for simple async tasks
- **USE** `QFuture` and `QFutureWatcher` for async results

### Simple Async with QtConcurrent

```cpp
#include <QtConcurrent>

// Fire and forget
QtConcurrent::run([]() {
    heavyComputation();
});

// With result
QFuture<int> future = QtConcurrent::run([]() {
    return computeValue();
});

QFutureWatcher<int> *watcher = new QFutureWatcher<int>(this);
connect(watcher, &QFutureWatcher<int>::finished, this, [this, watcher]() {
    int result = watcher->result();
    watcher->deleteLater();
});
watcher->setFuture(future);
```

---

## Cross-Platform Guidelines

### Platform Detection

```cpp
// Compile-time detection
#if defined(Q_OS_WIN)
    // Windows-specific code
#elif defined(Q_OS_MACOS)
    // macOS-specific code
#elif defined(Q_OS_IOS)
    // iOS-specific code
#elif defined(Q_OS_ANDROID)
    // Android-specific code
#elif defined(Q_OS_LINUX)
    // Linux-specific code
#endif

// Runtime detection (less common)
#include <QSysInfo>
QString os = QSysInfo::productType();
```

### Rules

- **ALWAYS** use Qt path functions, never hardcoded paths
- **ALWAYS** use `/` as path separator (Qt converts automatically)
- **NEVER** assume file system case sensitivity
- **USE** `QStandardPaths` for standard locations
- **USE** `QDir::separator()` only when displaying to users

### Standard Paths

```cpp
#include <QStandardPaths>

// App data storage
QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

// Documents folder
QString docsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

// Cache
QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

// Temporary files
QString tempPath = QDir::tempPath();
```

### File Handling

```cpp
// CORRECT: Platform-independent path handling
QString path = QDir::cleanPath(baseDir + "/" + subDir + "/" + fileName);

// CORRECT: Using QDir
QDir dir(basePath);
QString filePath = dir.filePath("subdir/file.txt");

// CORRECT: Check existence
if (QFile::exists(filePath)) {
    // ...
}

// WRONG: Hardcoded Windows paths
QString path = "C:\\Users\\...";  // NEVER DO THIS
```

---

## Mobile Development

### iOS Specific

#### Requirements
- macOS with Xcode installed
- Apple Developer Account ($99/year)
- Qt for iOS kit installed

#### Info.plist Keys

```xml
<!-- ios/Info.plist -->
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "...">
<plist version="1.0">
<dict>
    <key>CFBundleDisplayName</key>
    <string>MyApp</string>
    <key>CFBundleIdentifier</key>
    <string>com.mycompany.myapp</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    
    <!-- Permissions -->
    <key>NSCameraUsageDescription</key>
    <string>This app needs camera access for...</string>
    <key>NSPhotoLibraryUsageDescription</key>
    <string>This app needs photo library access for...</string>
    <key>NSLocationWhenInUseUsageDescription</key>
    <string>This app needs location for...</string>
</dict>
</plist>
```

#### CMake for iOS

```cmake
if(IOS)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0")
    set_target_properties(MyApp PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/ios/Info.plist
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "YOUR_TEAM_ID"
    )
endif()
```

### Android Specific

#### Requirements
- Android SDK and NDK
- Java JDK 11+
- Qt for Android kit installed

#### AndroidManifest.xml

```xml
<!-- android/AndroidManifest.xml -->
<?xml version="1.0"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.mycompany.myapp"
    android:versionCode="1"
    android:versionName="1.0">
    
    <!-- Permissions -->
    <uses-permission android:name="android.permission.INTERNET"/>
    <uses-permission android:name="android.permission.CAMERA"/>
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE"/>
    
    <application
        android:name="org.qtproject.qt.android.bindings.QtApplication"
        android:label="MyApp"
        android:icon="@drawable/icon">
        
        <activity
            android:name="org.qtproject.qt.android.bindings.QtActivity"
            android:configChanges="orientation|screenSize|screenLayout|..."
            android:screenOrientation="unspecified">
            
            <intent-filter>
                <action android:name="android.intent.action.MAIN"/>
                <category android:name="android.intent.category.LAUNCHER"/>
            </intent-filter>
        </activity>
    </application>
</manifest>
```

#### CMake for Android

```cmake
if(ANDROID)
    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android")
    
    set_target_properties(MyApp PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR ${ANDROID_PACKAGE_SOURCE_DIR}
        QT_ANDROID_TARGET_SDK_VERSION 33
        QT_ANDROID_MIN_SDK_VERSION 24
    )
endif()
```

### Mobile UI Rules

- **ALWAYS** use QML for mobile UIs
- **ALWAYS** design for touch (44pt minimum touch targets)
- **ALWAYS** handle screen rotation
- **ALWAYS** handle safe areas (notches, home indicators)
- **USE** `Qt.platform.os` in QML for platform checks
- **TEST** on real devices, not just simulators

### QML Safe Area Handling

```qml
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    visible: true

    // Safe area margins
    property real safeTop: Qt.platform.os === "ios" ? 44 : 0
    property real safeBottom: Qt.platform.os === "ios" ? 34 : 0

    header: ToolBar {
        topPadding: root.safeTop
        // ...
    }

    footer: TabBar {
        bottomPadding: root.safeBottom
        // ...
    }
}
```

---

## Deployment

### Windows Deployment

```bash
# Use windeployqt
cd build/release
windeployqt.exe MyApp.exe --qmldir /path/to/qml

# Creates all necessary DLLs alongside executable
```

### macOS Deployment

```bash
# Use macdeployqt
macdeployqt MyApp.app -qmldir=/path/to/qml -dmg

# Creates self-contained .app bundle and .dmg
```

### iOS Deployment

```bash
# Build in Qt Creator with iOS kit
# Or via command line:
cmake --build . --target MyApp -- -sdk iphoneos

# Open generated .xcodeproj in Xcode for signing and archive
```

### Android Deployment

```bash
# Use androiddeployqt (called automatically by Qt Creator)
# Or manually:
androiddeployqt --input android-deployment-settings.json \
                --output android-build \
                --android-platform android-33 \
                --gradle

# Creates .apk or .aab in android-build/build/outputs/
```

### Rules

- **ALWAYS** test deployment on clean machines
- **ALWAYS** use the deploy tools (windeployqt, macdeployqt, etc.)
- **NEVER** manually copy Qt DLLs
- **ALWAYS** include QML modules if using QML
- **TEST** all features after deployment

---

## Performance

### General Rules

- **AVOID** blocking the main/GUI thread
- **USE** asynchronous operations for I/O
- **PROFILE** before optimizing
- **CACHE** expensive computations
- **USE** `QElapsedTimer` for timing

### QML Performance

```qml
// GOOD: Use integer IDs for model data
// GOOD: Avoid JavaScript in tight loops
// GOOD: Use Loader for conditional content

// GOOD: Lazy loading
Loader {
    active: tabIndex === 2
    sourceComponent: ExpensiveComponent {}
}

// GOOD: Use shader effects sparingly
// GOOD: Prefer opacity animation over visible toggling

// BAD: Complex bindings
property int value: someFunction(a, b, c) + otherFunction(d, e)  // Recalculates constantly

// GOOD: Break complex bindings
property int intermediateValue: someFunction(a, b, c)
property int value: intermediateValue + otherFunction(d, e)
```

### C++ Performance

```cpp
// GOOD: Reserve container capacity
QVector<MyClass> items;
items.reserve(expectedSize);

// GOOD: Use move semantics
QString processData(QString data) {
    // ...
    return std::move(result);  // Or just return result (RVO)
}

// GOOD: Pass large objects by const reference
void processItems(const QVector<MyClass> &items);

// GOOD: Use string concatenation efficiently
QString result = QString("%1 - %2 - %3").arg(a).arg(b).arg(c);

// BAD: Repeated concatenation
QString result = a + " - " + b + " - " + c;  // Multiple allocations
```

---

## Common Pitfalls

### 1. Forgetting Q_OBJECT Macro

```cpp
// WRONG: Missing Q_OBJECT
class MyClass : public QObject {
    // signals and slots won't work!
};

// CORRECT
class MyClass : public QObject {
    Q_OBJECT  // REQUIRED for signals/slots
public:
    // ...
};
```

### 2. Wrong Thread Access

```cpp
// WRONG: Accessing GUI from worker thread
void Worker::doWork() {
    label->setText("Done");  // CRASH or undefined behavior
}

// CORRECT: Emit signal, let main thread update GUI
void Worker::doWork() {
    emit workDone("Done");  // Connected to GUI slot
}
```

### 3. Modifying Container While Iterating

```cpp
// WRONG
for (auto it = list.begin(); it != list.end(); ++it) {
    if (shouldRemove(*it)) {
        list.erase(it);  // UNDEFINED BEHAVIOR
    }
}

// CORRECT
list.erase(std::remove_if(list.begin(), list.end(), shouldRemove), list.end());

// Or with indices (reverse iteration)
for (int i = list.size() - 1; i >= 0; --i) {
    if (shouldRemove(list[i])) {
        list.removeAt(i);
    }
}
```

### 4. Lambda Connection Without Context

```cpp
// WRONG: No context - won't disconnect when receiver dies
connect(sender, &Sender::signal, [this]() {
    doSomething();  // 'this' might be deleted!
});

// CORRECT: With context object
connect(sender, &Sender::signal, this, [this]() {
    doSomething();
});
```

### 5. Blocking Event Loop

```cpp
// WRONG: Blocks entire UI
void MainWindow::onButtonClick() {
    QThread::sleep(5);  // UI frozen for 5 seconds!
    updateUI();
}

// CORRECT: Use async
void MainWindow::onButtonClick() {
    QtConcurrent::run([this]() {
        heavyWork();
        QMetaObject::invokeMethod(this, "updateUI", Qt::QueuedConnection);
    });
}
```

---

## Coding Conventions

### Naming Conventions

```cpp
// Classes: PascalCase
class MyClassName;

// Functions and methods: camelCase
void myFunction();
void calculateTotal();

// Member variables: m_ prefix + camelCase
QString m_userName;
int m_itemCount;

// Constants: ALL_CAPS or k prefix
const int MAX_ITEMS = 100;
const int kDefaultTimeout = 5000;

// Enums: PascalCase with scoped enums preferred
enum class Status {
    Idle,
    Running,
    Completed,
    Failed
};

// Signals: past tense or state description
signals:
    void dataChanged();
    void downloadCompleted();
    void errorOccurred(const QString &message);

// Slots: verb or action
public slots:
    void processData();
    void handleError(const QString &message);
    void updateDisplay();
```

### File Organization

```cpp
// header.h
#ifndef MYCLASS_H
#define MYCLASS_H

#include <QObject>
#include <QString>

// Forward declarations (prefer over includes in headers)
class QNetworkReply;

class MyClass : public QObject {
    Q_OBJECT

public:
    explicit MyClass(QObject *parent = nullptr);
    ~MyClass() override;

    // Public API
    void doSomething();

signals:
    void somethingHappened();

public slots:
    void handleEvent();

private:
    void helperMethod();
    
    QString m_data;
};

#endif // MYCLASS_H
```

### Include Order

```cpp
// 1. Corresponding header (for .cpp files)
#include "myclass.h"

// 2. Qt headers
#include <QObject>
#include <QString>
#include <QVector>

// 3. Other library headers
#include <third_party/library.h>

// 4. Project headers
#include "core/engine.h"
#include "utils/helpers.h"

// 5. Standard library headers
#include <memory>
#include <vector>
```

---

## Quick Reference

### Essential Qt Modules

| Module | Purpose | Include |
|--------|---------|---------|
| Core | Base classes, containers, threads | `<QtCore>` |
| Gui | GUI primitives | `<QtGui>` |
| Widgets | Desktop widgets | `<QtWidgets>` |
| Quick | QML engine | `<QtQuick>` |
| QuickControls2 | QML UI controls | `<QtQuickControls2>` |
| Network | HTTP, TCP, UDP | `<QtNetwork>` |
| Sql | Database access | `<QtSql>` |
| Concurrent | Threading utilities | `<QtConcurrent>` |

### Essential QML Imports

```qml
import QtQuick                  // Core QML
import QtQuick.Controls         // UI Controls
import QtQuick.Layouts          // Layout managers
import QtQuick.Window           // Window management
import QtQuick.Dialogs          // File dialogs, etc.
import Qt.labs.platform         // Native dialogs
```

### Build Commands Cheat Sheet

```bash
# Configure (Desktop)
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64

# Configure (Android)
cmake -B build-android \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/android_arm64_v8a \
    -DANDROID_ABI=arm64-v8a

# Build
cmake --build build

# Deploy (Windows)
windeployqt build/Release/MyApp.exe --qmldir src/qml

# Deploy (macOS)
macdeployqt build/MyApp.app -qmldir=src/qml

# Deploy (Android) - usually via Qt Creator
```

---

## Checklist Before Release

- [ ] Tested on all target platforms
- [ ] No compiler warnings (treat warnings as errors)
- [ ] Memory leak check (use AddressSanitizer or Valgrind)
- [ ] UI tested at different DPI scales
- [ ] All strings internationalized (`tr()` or `qsTr()`)
- [ ] Error handling for all I/O operations
- [ ] Network timeout handling
- [ ] Graceful offline behavior
- [ ] App icons for all platforms and sizes
- [ ] Splash screen (if applicable)
- [ ] Permissions requested properly (mobile)
- [ ] Deployment tested on clean systems
- [ ] Version numbers updated
- [ ] Release notes prepared

---

*Last updated: 2024*
