PROJECT(GBCEmulator)
cmake_minimum_required(VERSION 2.8.12)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(GBC_HEADERS \ 
        src/CartridgeReader.h \ 
        src/CPU.h \ 
        src/GPU.h \ 
        src/Joypad.h \ 
        src/MBC.h \ 
        src/Memory.h \ 
        src/Tile.h)
        
set(GBC_SOURCE \ 
        src/GBCEmulator.cpp \ 
        src/CartridgeReader.cpp \ 
        src/CPU.cpp \ 
        src/GPU.cpp \ 
        src/Joypad.cpp \ 
        src/MBC.cpp \ 
        src/Memory.cpp \ 
        src/Tile.cpp)
        
find_package("SDL")

if(SDL_FOUND)
    include_directories(${SDL_INCLUDE_DIRS})
endif()

add_executable(GBCEmulator ${GBC_HEADERS} ${GBC_SOURCE})
target_link_libraries(GBCEmulator CONAN_PKG::SDL CONAN_PKG::spdlog)