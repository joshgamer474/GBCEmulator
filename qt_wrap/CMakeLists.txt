PROJECT(GBCEmulator_qt)
cmake_minimum_required(VERSION 2.8.12)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Qt5 REQUIRED Widgets)
find_package(SDL2)

set(GBC_HEADERS
    ../src/APU.h
    ../src/AudioSquare.h
    ../src/AudioWave.h
    ../src/AudioNoise.h
    ../src/CartridgeReader.h
    ../src/ColorPalette.h
    ../src/CPU.h
    ../src/GBCEmulator.h
    ../src/GPU.h
    ../src/Joypad.h
    ../src/JoypadInputInterface.h
    ../src/JoypadXInput.h
    ../src/MBC.h
    ../src/Memory.h
    ../src/ScreenInterface.h
    ../src/SDLWindow.h
    ../src/Tile.h)

set(GBC_SOURCE
    ../src/APU.cpp
    ../src/AudioNoise.cpp
    ../src/AudioSquare.cpp
    ../src/AudioWave.cpp
    ../src/CartridgeReader.cpp
    ../src/ColorPalette.cpp
    ../src/CPU.cpp
    ../src/GPU.cpp
    ../src/GBCEmulator.cpp
    ../src/Joypad.cpp
    ../src/JoypadXInput.cpp
    ../src/MBC.cpp
    ../src/Memory.cpp
    ../src/SDLWindow.cpp
    ../src/Tile.cpp)

set(QT_UIS
    src/mainwindow.ui
    src/audiodebuggerwindow.ui
    src/debuggerwindow.ui
    src/vramwindow.ui)
        
set(QT_HEADERS
    src/mainwindow.h
    src/emuview.h
    src/debuggerwindow.h
    src/audiodebuggerwindow.h
    src/hexwidget.h
    src/vramwindow.h)
        
set(QT_SOURCE
    src/mainwindow.cpp
    src/emuview.cpp
    src/audiodebuggerwindow.cpp
    src/debuggerwindow.cpp
    src/hexwidget.cpp
    src/vramwindow.cpp)
        
set(QT_RUN_SOURCE
    src/main.cpp) 

qt5_wrap_ui(QT_UI_GENERATED_HEADERS ${QT_UIS})
    
#add_executable(GBCEmulator_qt WIN32
add_executable(GBCEmulator_qt
    ${QT_RUN_SOURCE}
    ${QT_SOURCE}
    ${QT_HEADERS}
    ${QT_UI_GENERATED_HEADERS}
    ${GBC_SOURCE}
    ${GBC_HEADERS})
    
include_directories(${CONAN_INCLUDE_DIRS}
    "src/"
    ${CONAN_INCLUDE_DIRS_QT}/QtCore
    ${CONAN_INCLUDE_DIRS_QT}/QtGui
    ${CONAN_INCLUDE_DIRS_QT}/QtWidgets)
    
target_include_directories(GBCEmulator_qt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(GBCEmulator_qt
    ${CONAN_TARGETS}
    Qt5::Widgets)

if(WIN32)
    target_link_libraries(GBCEmulator_qt
    XInput)
endif(WIN32)
