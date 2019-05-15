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
    src/Debugger.h
    src/GBCEmulator.h
    src/GPU.h
    src/Joypad.h
    src/JoypadInputInterface.h
    src/JoypadXInput.h
    src/MBC.h
    src/Memory.h
    src/ScreenInterface.h
    src/SDLWindow.h
    src/Tile.h)

set(GBC_SOURCE
    src/APU.cpp
    src/AudioNoise.cpp
    src/AudioSquare.cpp
    src/AudioWave.cpp
    src/CartridgeReader.cpp
    src/ColorPalette.cpp
    src/CPU.cpp
    src/Debugger.cpp
    src/GPU.cpp
    src/GBCEmulator.cpp
    src/Joypad.cpp
    src/JoypadXInput.cpp
    src/MBC.cpp
    src/Memory.cpp
    src/SDLWindow.cpp
    src/Tile.cpp)

set(GBC_RUN_SOURCE
    src/main.cpp)

set(IMGUI_MISC_SOURCE
    ${CMAKE_BINARY_DIR}/misc/bindings/imgui_impl_opengl3.cpp
    ${CMAKE_BINARY_DIR}/misc/bindings/imgui_impl_sdl.cpp)

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
    ${CONAN_TARGETS}
    ${CONAN_LIB})

# Add std::experimental::filesystem
if(UNIX)
    target_link_libraries(GBCEmulator
        stdc++fs)
endif(UNIX)

add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLEW 1)

# Create GBCEmulator executable
add_executable(GBCEmulator_bin
    ${GBC_RUN_SOURCE}
    ${IMGUI_MISC_SOURCE})

target_link_libraries(GBCEmulator_bin
    GBCEmulator)

if(WIN32)
message("************************************")
message("LINKING XINPUT")
target_link_libraries(GBCEmulator_bin
    XInput)
endif(WIN32)

include_directories(${CONAN_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/misc/bindings
    "src/")

# Rename GBCEmulator_bin to GBCEmulator
SET_TARGET_PROPERTIES(GBCEmulator_bin PROPERTIES OUTPUT_NAME GBCEmulator)

# Enable unit testing
enable_testing()

# Add unit tests
add_subdirectory(test_package)