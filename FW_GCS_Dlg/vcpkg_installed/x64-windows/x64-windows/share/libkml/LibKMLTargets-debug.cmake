get_filename_component(VCPKG_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)
#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "kmlbase" for configuration "Debug"
set_property(TARGET kmlbase APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(kmlbase PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX"
  IMPORTED_LOCATION_DEBUG "${VCPKG_IMPORT_PREFIX}/debug/lib/kmlbase.lib"
  )

list(APPEND _cmake_import_check_targets kmlbase )
list(APPEND _cmake_import_check_files_for_kmlbase "${VCPKG_IMPORT_PREFIX}/debug/lib/kmlbase.lib" )

# Import target "kmldom" for configuration "Debug"
set_property(TARGET kmldom APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(kmldom PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${VCPKG_IMPORT_PREFIX}/debug/lib/kmldom.lib"
  )

list(APPEND _cmake_import_check_targets kmldom )
list(APPEND _cmake_import_check_files_for_kmldom "${VCPKG_IMPORT_PREFIX}/debug/lib/kmldom.lib" )

# Import target "kmlxsd" for configuration "Debug"
set_property(TARGET kmlxsd APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(kmlxsd PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${VCPKG_IMPORT_PREFIX}/debug/lib/kmlxsd.lib"
  )

list(APPEND _cmake_import_check_targets kmlxsd )
list(APPEND _cmake_import_check_files_for_kmlxsd "${VCPKG_IMPORT_PREFIX}/debug/lib/kmlxsd.lib" )

# Import target "kmlengine" for configuration "Debug"
set_property(TARGET kmlengine APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(kmlengine PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${VCPKG_IMPORT_PREFIX}/debug/lib/kmlengine.lib"
  )

list(APPEND _cmake_import_check_targets kmlengine )
list(APPEND _cmake_import_check_files_for_kmlengine "${VCPKG_IMPORT_PREFIX}/debug/lib/kmlengine.lib" )

# Import target "kmlconvenience" for configuration "Debug"
set_property(TARGET kmlconvenience APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(kmlconvenience PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${VCPKG_IMPORT_PREFIX}/debug/lib/kmlconvenience.lib"
  )

list(APPEND _cmake_import_check_targets kmlconvenience )
list(APPEND _cmake_import_check_files_for_kmlconvenience "${VCPKG_IMPORT_PREFIX}/debug/lib/kmlconvenience.lib" )

# Import target "kmlregionator" for configuration "Debug"
set_property(TARGET kmlregionator APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(kmlregionator PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${VCPKG_IMPORT_PREFIX}/debug/lib/kmlregionator.lib"
  )

list(APPEND _cmake_import_check_targets kmlregionator )
list(APPEND _cmake_import_check_files_for_kmlregionator "${VCPKG_IMPORT_PREFIX}/debug/lib/kmlregionator.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
