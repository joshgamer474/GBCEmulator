PROJECT(GBCEmulator_bin)
cmake_minimum_required(VERSION 2.8.12)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(SDL2)
add_definitions(-DSDL_DRAW)

set(GBC_HEADERS
    src/GBCEmulator.h
    src/CartridgeReader.h
    src/ColorPalette.h
    src/CPU.h
    src/GPU.h
    src/Joypad.h
    src/MBC.h
    src/Memory.h
    src/Tile.h)

set(GBC_SOURCE
    src/GBCEmulator.cpp
    src/CartridgeReader.cpp
    src/ColorPalette.cpp
    src/CPU.cpp
    src/GPU.cpp
    src/Joypad.cpp
    src/MBC.cpp
    src/Memory.cpp
    src/Tile.cpp)

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

# Create GBCEmulator executable
add_executable(GBCEmulator_bin ${GBC_RUN_SOURCE})

target_link_libraries(GBCEmulator_bin GBCEmulator)

include_directories(${CONAN_INCLUDE_DIRS}
    "src/")
    
# Rename GBCEmulator_bin to GBCEmulator
SET_TARGET_PROPERTIES(GBCEmulator_bin PROPERTIES OUTPUT_NAME GBCEmulator)

