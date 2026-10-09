cmake_minimum_required(VERSION 3.16)
project(GBCEmulator_bin VERSION 0.2.0 LANGUAGES CXX)

set(LIB_NAME GBCEmulator)

# Set cpp std
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

#include(${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake)

# Find dependencies
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(libzip REQUIRED)
find_package(spdlog REQUIRED)
#find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(SDL3 CONFIG REQUIRED)
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
  src/JoypadGeneric.h
  src/JoypadInputInterface.h
  src/JoypadXInput.h
  src/MBC.h
  src/Memory.h
  src/ScreenInterface.h
  src/SDLWindow.h
  src/SerialTransfer.h
  src/Tile.h
  src/RollingAvg.h
  src/Util.h)

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
  src/JoypadGeneric.cpp
  src/JoypadXInput.cpp
  src/MBC.cpp
  src/Memory.cpp
  src/SDLWindow.cpp
  src/SerialTransfer.cpp
  src/Tile.cpp
  src/RollingAvg.cpp
  src/Util.cpp)

set(GBC_RUN_SOURCE
  src/main.cpp)

set(DEPS_INCLUDES
  ${libzip_INCLUDE_DIRS}
  ${PNG_INCLUDE_DIRS}
  ${spdlog_INCLUDE_DIRS}
  ${SDL3_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS})

set(DEPS_LIBRARIES
  libzip::zip
  PNG::PNG
  spdlog::spdlog
  SDL3::SDL3
  ZLIB::ZLIB)

# Create GBCEmulator lib
option(BUILD_FLUTTER_FFI "Build the C API shared library for Dart FFI" OFF)
if(BUILD_SHARED_LIBS)
  message("Building as SHARED")
  add_library(${LIB_NAME} SHARED)
else()
  message("Building as STATIC")
  add_library(${LIB_NAME} STATIC)
endif()

# Include source code
target_sources(${LIB_NAME}
  PRIVATE ${GBC_SOURCE}
  PUBLIC FILE_SET HEADERS
  BASE_DIRS ${CMAKE_SOURCE_DIR}/src
  FILES ${GBC_HEADERS})

target_include_directories(${LIB_NAME} PUBLIC
  ${DEPS_INCLUDES}
  ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${LIB_NAME}
  ${DEPS_LIBRARIES})

if(BUILD_FLUTTER_FFI)
  set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  add_library(gbcemulator_ffi SHARED
    flutter/gbcemulator/src/gbcemulator.cpp
    flutter/gbcemulator/src/controller_input.cpp)
  target_compile_definitions(gbcemulator_ffi PRIVATE GBC_FFI_BUILD)
  target_link_libraries(gbcemulator_ffi PRIVATE ${LIB_NAME})

  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(gbcemulator_ffi PROPERTIES
      FRAMEWORK TRUE
      MACOSX_FRAMEWORK_IDENTIFIER "com.childers.gbcemulator.ffi"
      MACOSX_FRAMEWORK_BUNDLE_VERSION "${PROJECT_VERSION}"
      MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${PROJECT_VERSION}"
      PUBLIC_HEADER "flutter/gbcemulator/src/gbcemulator.h"
      INSTALL_NAME_DIR "@rpath")
  endif()

  # MacOS
  if(APPLE)
    target_link_options(gbcemulator_ffi PRIVATE "-Wl,-headerpad_max_install_names")
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/gbc-ffi-$<CONFIG>.txt"
      CONTENT "$<TARGET_FILE:gbcemulator_ffi>\n")
  endif()

  # Windows
  if(WIN32)
    if(MINGW AND CMAKE_CROSSCOMPILING)
      target_link_libraries(gbcemulator_ffi PRIVATE xinput1_4)
    else()
      target_link_libraries(gbcemulator_ffi PRIVATE xinput)
    endif()

    add_custom_command(TARGET gbcemulator_ffi POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy -t "$<TARGET_FILE_DIR:gbcemulator_ffi>"
        "$<TARGET_RUNTIME_DLLS:gbcemulator_ffi>"
      COMMAND_EXPAND_LISTS)
  endif()

  # Install options
  install(TARGETS gbcemulator_ffi
    FRAMEWORK DESTINATION lib
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
endif()

# Add std::experimental::filesystem
if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

if(LINUX AND BUILD_UNIT_TEST)
  target_link_libraries(${LIB_NAME}
    stdc++fs)
endif()

include_directories(${CONAN_INCLUDE_DIRS}
  "src/")

if(NOT BUILD_LIB_ONLY)
  # Create GBCEmulator executable
  add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
    ${GBC_RUN_SOURCE})

  target_link_libraries(${PROJECT_NAME}
    ${LIB_NAME})

  if(WIN32)
    if(MINGW AND CMAKE_CROSSCOMPILING)
      target_link_libraries(${LIB_NAME}
        xinput1_4)
    else()
      target_link_libraries(${LIB_NAME}
        xinput)
    endif()
  endif(WIN32)

  # Rename GBCEmulator_bin to GBCEmulator
  SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${LIB_NAME})
endif(NOT BUILD_LIB_ONLY)

################################
# Android Only
# Copy ndk used libc++_shared.so to lib
################################
if(CMAKE_NM AND ANDROID_ABI)
  get_filename_component(NDKPARENT_DIR ${CMAKE_NM} DIRECTORY)
  get_filename_component(NDKPARENT_DIR ${NDKPARENT_DIR} DIRECTORY)
  set(NDKLIBPATH "${NDKPARENT_DIR}/sysroot/usr/lib")
  message("NDKLIBPATH ${NDKLIBPATH}")
  # x86
  if(${ANDROID_ABI} STREQUAL x86)
    set(USENDKLIBPATH ${NDKLIBPATH}/i686-linux-android)
  endif()
  # x86_64
  if(${ANDROID_ABI} STREQUAL x86_64)
    set(USENDKLIBPATH ${NDKLIBPATH}/x86_64-linux-android)
  endif()
  # armv7
  if(${ANDROID_ABI} STREQUAL armeabi-v7a)
    set(USENDKLIBPATH ${NDKLIBPATH}/arm-linux-androideabi)
  endif()
  # armv8
  if(${ANDROID_ABI} STREQUAL arm64-v8a)
    set(USENDKLIBPATH ${NDKLIBPATH}/aarch64-linux-android)
  endif()
  # Copy libc++_shared.so to build/lib
  if (USENDKLIBPATH)
    message("Copying ${USENDKLIBPATH}/libc++_shared.so to ${CMAKE_BINARY_DIR}/lib")
    file(COPY ${USENDKLIBPATH}/libc++_shared.so
      DESTINATION ${CMAKE_BINARY_DIR}/lib)
  endif()
endif()

# Enable unit testing
enable_testing()

option(BUILD_CONTROLLER_TEST "Build SDL virtual-gamepad regression test" OFF)
if(BUILD_CONTROLLER_TEST)
  add_executable(controller_input_test
    flutter/gbcemulator/test/controller_input_test.cpp
    flutter/gbcemulator/src/controller_input.cpp)
  target_link_libraries(controller_input_test PRIVATE ${LIB_NAME})
  add_test(NAME controller_input COMMAND controller_input_test)
endif()

if(BUILD_UNIT_TEST)
  # Add unit tests
  add_subdirectory(test_package)
endif(BUILD_UNIT_TEST)

if(BUILD_QT_GUI)
  add_subdirectory(qt_wrap)
endif(BUILD_QT_GUI)

# Install
if (BUILD_LIB_ONLY)
  install(TARGETS ${LIB_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)
else()
  if(APPLE)
    set(LIB_DEST "Applications/${LIB_NAME}.app/Contents/Frameworks")
    set_target_properties(${PROJECT_NAME} PROPERTIES
      INSTALL_RPATH "@executable_path/../Frameworks")
  else()
    set(LIB_DEST "lib")
  endif()

  # MinGW cross compile
  set(RUNTIME_SEARCH_DIRECTORIES)
  set(RUNTIME_LIB_EXCLUDES)
  if(MINGW AND CMAKE_CROSSCOMPILING)
    # Add runtime DLL paths
    list(APPEND RUNTIME_SEARCH_DIRECTORIES
      DIRECTORIES
        "/usr/x86_64-w64-mingw32/bin")

    # Exclude unneeded DLLs
    list(APPEND RUNTIME_LIB_EXCLUDES
      PRE_EXCLUDE_REGEXES
        "^api-ms-win-.*"
        "^ext-ms-win-.*"
        "^(advapi32|gdi32|imm32|kernel32|oleaut32|setupapi|shell32|user32|version|winmm|bcrypt|ole32)\\.dll$"
        "^xinput1_4\\.dll$")

    # Fix cross compile cmake_install.cmake error
    install(CODE [[
      set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
      set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")
      set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND
          "/usr/bin/x86_64-w64-mingw32-objdump")
    ]])
  endif()

  # Install application and collect runtime dependencies.
  install(TARGETS ${PROJECT_NAME}
    RUNTIME_DEPENDENCY_SET ${PROJECT_NAME}-dependencies
    BUNDLE DESTINATION Applications
    RUNTIME DESTINATION bin
    FRAMEWORK DESTINATION ${LIB_DEST}
    LIBRARY DESTINATION ${LIB_DEST}
  )

  install(RUNTIME_DEPENDENCY_SET ${PROJECT_NAME}-dependencies
    ${RUNTIME_SEARCH_DIRECTORIES}
    ${RUNTIME_LIB_EXCLUDES}
    RUNTIME DESTINATION bin
    FRAMEWORK DESTINATION ${LIB_DEST}
    LIBRARY DESTINATION ${LIB_DEST}
  )

  if (WIN32)
    add_custom_command (
      TARGET "${PROJECT_NAME}" POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy -t "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        "$<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>" USES_TERMINAL COMMAND_EXPAND_LISTS)
  endif()

endif()

# Package
if(NOT BUILD_LIB_ONLY)
  include(${CMAKE_SOURCE_DIR}/packaging/CMakeLists.txt)
endif()
