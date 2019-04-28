PROJECT(GBCEmulatorTestPackage)
cmake_minimum_required(VERSION 2.8.12)

#include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

set(CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}/test_package)

set(UNIT_TEST_TEMPLATE_HEADERS
    src/Fixtures/ROMTestFixture.h
    src/UnitTests.h)

set(UNIT_TEST_TEMPLATE_SOURCE
    src/Fixtures/ROMTestFixture.cpp)

set(UNIT_TEST_SOURCE
    src/Tests/blargg_cgb_sounds.cpp
    src/Tests/blargg_cpu_instrs.cpp
    src/Tests/blargg_dmg_sounds.cpp
    src/Tests/blargg_halt_bug.cpp
    src/Tests/blargg_instr_timing.cpp
    src/Tests/blargg_interrupt_time.cpp
    src/Tests/blargg_mem_timing.cpp
    src/Tests/blargg_mem_timing_2.cpp
    src/Tests/blargg_oam_bug.cpp)

include_directories(src)

add_executable(GBCEmulatorTestPackage
    ${UNIT_TEST_SOURCE}
    ${UNIT_TEST_TEMPLATE_SOURCE}
    ${UNIT_TEST_TEMPLATE_HEADERS})

target_link_libraries(GBCEmulatorTestPackage
    GBCEmulator
    CONAN_PKG::gtest)

message("CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")

# Copy blarggtest roms dir to build dir
message("Copying dir ${CMAKE_SOURCE_DIR}/test_package/blarggtests ")
add_custom_command(TARGET GBCEmulatorTestPackage PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/test_package/blarggtests $<TARGET_FILE_DIR:GBCEmulatorTestPackage>/blarggtests)

add_test(NAME romUnitTests COMMAND GBCEmulatorTestPackage)