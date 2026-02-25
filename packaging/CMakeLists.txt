# Minimal CPack entrypoint for GBCEmulator.

set(CPACK_PACKAGE_NAME "GBCEmulator")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A WIP GameBoy Color emulator")
set(CPACK_PACKAGE_VENDOR "Josh Childers")
set(CPACK_PACKAGE_CONTACT "https://github.com/joshgamer474/GBCEmulator")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "GBCEmulator")

if(NOT DEFINED PROJECT_VERSION)
  message(FATAL_ERROR "PROJECT_VERSION is not defined. Set VERSION in the root project() call.")
endif()
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_FILE_NAME
  "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

if(EXISTS "${CMAKE_SOURCE_DIR}/packaging/LICENSE.txt")
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/packaging/LICENSE.txt")
endif()

if(APPLE)
  include("${CMAKE_SOURCE_DIR}/packaging/macos/package.cmake")
elseif(WIN32)
  include("${CMAKE_SOURCE_DIR}/packaging/windows/package.cmake")
elseif(UNIX)
  include("${CMAKE_SOURCE_DIR}/packaging/linux/package.cmake")
endif()

include(CPack)
