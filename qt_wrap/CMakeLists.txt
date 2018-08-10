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


set(GBC_HEADERS
    ../src/GBCEmulator.h
    ../src/CartridgeReader.h
    ../src/CPU.h
    ../src/GPU.h
    ../src/Joypad.h
    ../src/MBC.h
    ../src/Memory.h
    ../src/Tile.h)
        
set(GBC_SOURCE
    ../src/GBCEmulator.cpp
    ../src/CartridgeReader.cpp
    ../src/CPU.cpp
    ../src/GPU.cpp
    ../src/Joypad.cpp
    ../src/MBC.cpp
    ../src/Memory.cpp
    ../src/Tile.cpp)

set(QT_UIS
    src/mainwindow.ui
    src/debuggerwindow.ui)
        
set(QT_HEADERS
    src/mainwindow.h
    src/emuview.h
    src/debuggerwindow.h
    src/hexwidget.h)
        
set(QT_SOURCE
    src/mainwindow.cpp
    src/emuview.cpp
    src/debuggerwindow.cpp
    src/hexwidget.cpp)
        
set(QT_RUN_SOURCE
    src/main.cpp) 

qt5_wrap_ui(QT_UI_GENERATED_HEADERS ${QT_UIS})
    
add_executable(GBCEmulator_qt WIN32 
    ${QT_RUN_SOURCE}
    ${QT_SOURCE}
    ${QT_HEADERS}
    ${QT_UI_GENERATED_HEADERS}
    ${GBC_SOURCE}
    ${GBC_HEADERS})
include_directories(${CONAN_INCLUDE_DIRS} "src/")
target_include_directories(GBCEmulator_qt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GBCEmulator_qt ${CONAN_LIB_DIRS} CONAN_PKG::GBCEmulator Qt5::Widgets)
