PROJECT(GBCEmulator_bin)
cmake_minimum_required(VERSION 2.8.12)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(SDL2)
add_definitions(-DSDL_DRAW)

set(GBC_HEADERS
    src/APU.h
    src/AudioSquare.h
    src/AudioWave.h
    src/AudioNoise.h
    src/CartridgeReader.h
    src/ColorPalette.h
    src/CPU.h
    src/GBCEmulator.h
    src/GetUniqueColorPalette.h
    src/GPU.h
    src/Joypad.h
    src/JoypadInputInterface.h
    src/JoypadXInput.h
    src/MBC.h
    src/Memory.h
    src/ScreenInterface.h
    src/SDLWindow.h
    src/SerialTransfer.h
    src/Tile.h
	src/RollingAvg.h)

set(GBC_SOURCE
    src/APU.cpp
    src/AudioNoise.cpp
    src/AudioSquare.cpp
    src/AudioWave.cpp
    src/CartridgeReader.cpp
    src/ColorPalette.cpp
    src/CPU.cpp
    src/GBCEmulator.cpp
    src/GetUniqueColorPalette.cpp
    src/GPU.cpp
    src/Joypad.cpp
    src/JoypadXInput.cpp
    src/MBC.cpp
    src/Memory.cpp
    src/SDLWindow.cpp
    src/SerialTransfer.cpp
    src/Tile.cpp
	src/RollingAvg.cpp)

set(GBC_RUN_SOURCE
    src/main.cpp)

# Create GBCEmulator lib
if(BUILD_SHARED_LIBS)
    message("Building as SHARED")
    add_library(GBCEmulator SHARED ${GBC_SOURCE} ${GBC_HEADERS})
else()
    message("Building as STATIC")
    add_library(GBCEmulator STATIC ${GBC_SOURCE} ${GBC_HEADERS})
endif()

target_include_directories(GBCEmulator PUBLIC
    ${CONAN_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(GBCEmulator
    ${CONAN_TARGETS})

# Add std::experimental::filesystem
if(UNIX AND BUILD_UNIT_TEST)
    target_link_libraries(GBCEmulator
        stdc++fs)
endif(UNIX)

include_directories(${CONAN_INCLUDE_DIRS}
    "src/")

if(NOT BUILD_LIB_ONLY)
# Create GBCEmulator executable
add_executable(GBCEmulator_bin ${GBC_RUN_SOURCE})

target_link_libraries(GBCEmulator_bin
    GBCEmulator)

if(WIN32)
message("************************************")
message("LINKING XINPUT")
target_link_libraries(GBCEmulator_bin
    XInput)
endif(WIN32)

# Rename GBCEmulator_bin to GBCEmulator
SET_TARGET_PROPERTIES(GBCEmulator_bin PROPERTIES OUTPUT_NAME GBCEmulator)
endif(NOT BUILD_LIB_ONLY)

# Enable unit testing
enable_testing()

if(BUILD_UNIT_TEST)
# Add unit tests
add_subdirectory(test_package)
endif(BUILD_UNIT_TEST)
