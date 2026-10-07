cmake_minimum_required(VERSION 3.21)

function(run_checked)
  execute_process(COMMAND ${ARGV} RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "${ARGV}\n${output}\n${error}")
  endif()
endfunction()

if(NOT EXISTS "${GBC_MANIFEST}")
  message(FATAL_ERROR "Missing ${GBC_MANIFEST}. Reconfigure the native build with Release and BUILD_FLUTTER_FFI=ON.")
endif()
file(READ "${GBC_MANIFEST}" native_library)
string(STRIP "${native_library}" native_library)
file(GET_RUNTIME_DEPENDENCIES
  LIBRARIES "${native_library}"
  RESOLVED_DEPENDENCIES_VAR dependencies
  UNRESOLVED_DEPENDENCIES_VAR unresolved
  CONFLICTING_DEPENDENCIES_PREFIX conflicts
  PRE_EXCLUDE_REGEXES "^/System/Library/" "^/usr/lib/"
  POST_EXCLUDE_REGEXES "^/System/Library/" "^/usr/lib/")
if(unresolved OR conflicts_FILENAMES)
  message(FATAL_ERROR "Native dependency resolution failed. Unresolved: ${unresolved}; conflicts: ${conflicts_FILENAMES}")
endif()
set(libraries "${native_library}" ${dependencies})
file(MAKE_DIRECTORY "${GBC_FRAMEWORKS}")
separate_arguments(architectures UNIX_COMMAND "${GBC_ARCHS}")
set(names)
foreach(library IN LISTS libraries)
  get_filename_component(name "${library}" NAME)
  if(name IN_LIST names)
    message(FATAL_ERROR "Native libraries have conflicting bundle names: ${name}")
  endif()
  if(library MATCHES "\\.framework/")
    message(FATAL_ERROR "Native framework dependency ${library} is unsupported; configure this dependency as a dylib.")
  endif()
  list(APPEND names "${name}")
  # Fail before copying/signing if a thin native dependency cannot serve the app.
  if(architectures)
    execute_process(COMMAND /usr/bin/lipo "${library}" -verify_arch ${architectures}
      RESULT_VARIABLE architecture_result)
    if(NOT architecture_result EQUAL 0)
      message(FATAL_ERROR "${library} does not contain all app architectures (${GBC_ARCHS}). Rebuild the native core and dependencies for those architectures, or change ARCHS in macos/Runner/Configs/AppInfo.xcconfig.")
    endif()
  endif()
  file(COPY_FILE "${library}" "${GBC_FRAMEWORKS}/${name}")
  file(CHMOD "${GBC_FRAMEWORKS}/${name}"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endforeach()

foreach(name IN LISTS names)
  set(bundled "${GBC_FRAMEWORKS}/${name}")
  execute_process(COMMAND /usr/bin/otool -L "${bundled}"
    OUTPUT_VARIABLE links COMMAND_ERROR_IS_FATAL ANY)
  string(REPLACE "\n" ";" lines "${links}")
  foreach(line IN LISTS lines)
    if(line MATCHES "^[ \t]+(.+) \\(compatibility version")
      set(original "${CMAKE_MATCH_1}")
      get_filename_component(dependency_name "${original}" NAME)
      if(dependency_name IN_LIST names)
        run_checked(/usr/bin/install_name_tool -change "${original}"
          "@loader_path/${dependency_name}" "${bundled}")
      elseif(NOT original MATCHES "^(/System/Library/|/usr/lib/)")
        message(FATAL_ERROR "Unbundled native dependency: ${original} in ${name}")
      endif()
    endif()
  endforeach()
  run_checked(/usr/bin/install_name_tool -id "@rpath/${name}" "${bundled}")
  if(NOT GBC_SIGNING_ALLOWED STREQUAL "NO")
    if(NOT GBC_SIGN_IDENTITY)
      set(GBC_SIGN_IDENTITY "-")
    endif()
    run_checked(/usr/bin/codesign --force --sign "${GBC_SIGN_IDENTITY}" "${bundled}")
  endif()
endforeach()
message(STATUS "Bundled native libraries: ${names}")
