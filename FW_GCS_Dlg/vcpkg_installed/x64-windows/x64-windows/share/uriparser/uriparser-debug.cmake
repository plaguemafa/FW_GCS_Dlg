#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "uriparser::uriparser" for configuration "Debug"
set_property(TARGET uriparser::uriparser APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(uriparser::uriparser PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/uriparser.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/uriparser.dll"
  )

list(APPEND _cmake_import_check_targets uriparser::uriparser )
list(APPEND _cmake_import_check_files_for_uriparser::uriparser "${_IMPORT_PREFIX}/debug/lib/uriparser.lib" "${_IMPORT_PREFIX}/debug/bin/uriparser.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
