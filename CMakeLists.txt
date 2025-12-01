cmake_minimum_required(VERSION 3.16)

project(Fractonaut 
    VERSION 1.0.0 
    LANGUAGES CXX
    DESCRIPTION "Native C++ Qt fractal explorer with infinite zoom"
)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt Configuration - CRITICAL for Qt to work properly
set(CMAKE_AUTOMOC ON)  # Automatically run moc on Q_OBJECT classes
set(CMAKE_AUTORCC ON)  # Automatically compile .qrc resource files
set(CMAKE_AUTOUIC OFF) # We won't be using Qt Designer .ui files

# Find Qt6 - Prefer Qt6, specify all needed components
find_package(Qt6 COMPONENTS 
    Core 
    Gui 
    Widgets 
    OpenGL
    OpenGLWidgets
    REQUIRED
)

# Platform specific settings
if(APPLE AND NOT ANDROID)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")  # macOS Big Sur minimum
    message(STATUS "Building for macOS with deployment target ${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif()

if(ANDROID)
    message(STATUS "Building for Android")
    set(ANDROID_MIN_SDK_VERSION 26)
    set(ANDROID_TARGET_SDK_VERSION 34)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/rendering/FractalGLWidget.cpp
    src/rendering/ShaderManager.cpp
)

set(HEADERS
    include/Constants.h
    src/rendering/FractalGLWidget.h
    src/rendering/ShaderManager.h
)

# Qt Resource file
set(RESOURCES
    resources/resources.qrc
)

# Create executable
if(ANDROID)
    add_library(Fractonaut SHARED
        ${SOURCES}
        ${HEADERS}
        ${RESOURCES}
    )
else()
    add_executable(Fractonaut MACOSX_BUNDLE
        ${SOURCES}
        ${HEADERS}
        ${RESOURCES}
    )
endif()

# Link Qt libraries
target_link_libraries(Fractonaut PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

# macOS bundle configuration
if(APPLE AND NOT ANDROID)
    set_target_properties(Fractonaut PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Fractonaut"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.fractonaut.app"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_INFO_STRING "Infinite fractal explorer"
    )
endif()

# Android configuration
if(ANDROID)
    set_target_properties(Fractonaut PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android"
        QT_ANDROID_MIN_SDK_VERSION ${ANDROID_MIN_SDK_VERSION}
        QT_ANDROID_TARGET_SDK_VERSION ${ANDROID_TARGET_SDK_VERSION}
    )
endif()

# Installation rules
install(TARGETS Fractonaut
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
