PROJECT(GBCEmulator_mobile)
cmake_minimum_required(VERSION 2.8.12)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

set(GBC_HEADERS
    bgcolors.h
    SDLWindowAndroid.h)

set(GBC_SOURCE
    SDLWindowAndroid.cpp)

set(GBC_RUN_SOURCE
    main.cpp)

# Create GBCEmulator lib
if(BUILD_SHARED_LIBS)
    message("Building as SHARED")
    add_library(${PROJECT_NAME} SHARED ${GBC_SOURCE} ${GBC_HEADERS})
else()
    message("Building as STATIC")
    add_library(${PROJECT_NAME} STATIC ${GBC_SOURCE} ${GBC_HEADERS})
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CONAN_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME}
    ${CONAN_TARGETS})

# Add std::experimental::filesystem
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()
if(LINUX AND BUILD_UNIT_TEST)
    target_link_libraries(${PROJECT_NAME}
        stdc++fs)
endif(LINUX)

include_directories(${CONAN_INCLUDE_DIRS}
    ./)

if(NOT BUILD_LIB_ONLY)
    # Create GBCEmulator executable
    add_executable(${PROJECT_NAME}_bin
        ${GBC_RUN_SOURCE})

    target_link_libraries(${PROJECT_NAME}_bin
        GBCEmulator
        ${PROJECT_NAME})

    if(WIN32)
        message("************************************")
        message("LINKING XINPUT")
        target_link_libraries(${PROJECT_NAME}_bin
            XInput)
    endif(WIN32)
endif(NOT BUILD_LIB_ONLY)

# Copy button pngs to build folder
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/mobile/assets
    ${CMAKE_BINARY_DIR})