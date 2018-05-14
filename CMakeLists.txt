PROJECT(GBCEmulator_bin)
cmake_minimum_required(VERSION 2.8.12)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(GBC_HEADERS \ 
        src/GBCEmulator.h \ 
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
        
set(GBC_RUN_SOURCE \ 
        src/main.cpp)

# Create GBCEmulator lib
if(BUILD_SHARED_LIBS)
    message("Building as SHARED")
    add_library(GBCEmulator SHARED ${GBC_SOURCE})
else()
    message("Building as STATIC")
    add_library(GBCEmulator STATIC ${GBC_SOURCE})
endif()
target_link_libraries(GBCEmulator ${CONAN_TARGETS})

# Create GBCEmulator executable
add_executable(GBCEmulator_bin ${GBC_RUN_SOURCE})
target_link_libraries(GBCEmulator_bin GBCEmulator ${CONAN_TARGETS})
# Rename GBCEmulator_bin to GBCEmulator
SET_TARGET_PROPERTIES(GBCEmulator_bin PROPERTIES OUTPUT_NAME GBCEmulator)
