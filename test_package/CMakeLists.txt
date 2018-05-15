PROJECT(GBCEmulatorTestPackage)
cmake_minimum_required(VERSION 2.8.12)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(UNIT_TEST_TEMPLATE_HEADERS \ 
        src/individual_rom_test.h)

set(UNIT_TEST_TEMPLATE_SOURCE \ 
        src/individual_rom_test.cpp)      

set(UNIT_TEST_SOURCE \ 
        src/test_unit_test.cpp \ 
        src/blargg_unit_tests/cpu_instrs/01-special.cpp)


find_package("boost")
if(BOOST_FOUND)
    include_directories(${BOOST_INCLUDE_DIRS})
endif()

add_executable(GBCEmulatorTestPackage ${UNIT_TEST_SOURCE} ${UNIT_TEST_TEMPLATE_SOURCE} ${UNIT_TEST_TEMPLATE_HEADERS})
target_link_libraries(GBCEmulatorTestPackage CONAN_PKG::GBCEmulator CONAN_PKG::SDL CONAN_PKG::spdlog CONAN_PKG::boost)