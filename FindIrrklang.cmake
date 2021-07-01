#
# Find Irrklang
#
# This module defines the following variables:
# - IRRKLANG_INCLUDE_DIRS
# - IRRKLANG_LIBRARIES
# - IRRKLANG_FOUND
#

# Additional modules
include(FindPackageHandleStandardArgs)

if (WIN32)
    # Find include files
    message("Windows is not supported yet")
else()
    # Find include files
    find_path(
            IRRKLANG_INCLUDE_DIR
            NAMES irrKlang.h
            PATHS
            ext/irrklang/include
            DOC "The directory where irrklang/irrKlang.h resides")

    # Find library files
    find_library(
            IRRKLANG_LIBRARY
            NAMES libirrklang.dylib 
            PATHS
            ext/irrklang/bin/mac
            DOC "The Irrklang library")
endif()

# Handle REQUIRD argument, define *_FOUND variable
find_package_handle_standard_args(Irrklang DEFAULT_MSG IRRKLANG_INCLUDE_DIR IRRKLANG_LIBRARY)

if (IRRKLANG_FOUND)
    set(IRRKLANG_LIBRARIES ${IRRKLANG_LIBRARY})
    set(IRRKLANG_INCLUDE_DIRS ${IRRKLANG_INCLUDE_DIR})
    #add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
    #  PRE_BUILD
    #  COMMAND sudo cp -n "${CMAKE_SOURCE_DIR}/ext/irrklang/bin/mac/libirrklang.dylib" "/usr/local/lib/"
    #)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
      PRE_BUILD
      COMMAND cp "${CMAKE_SOURCE_DIR}/ext/irrklang/bin/mac/ikpMP3.dylib" "${CMAKE_BINARY_DIR}"
    )

    add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
      PRE_BUILD
      COMMAND cp "${CMAKE_SOURCE_DIR}/ext/irrklang/bin/mac/ikpMP3.dylib" "${CMAKE_BINARY_DIR}/Debug"
    )
endif()

# Hide some variables
mark_as_advanced(IRRKLANG_INCLUDE_DIR IRRKLANG_LIBRARY)
