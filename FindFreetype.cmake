# Additional modules
include(FindPackageHandleStandardArgs)

if (WIN32)
    # Find include files
    message("Windows is not supported yet")
else()
    # Find include files
    find_path(
            FREETYPE_INCLUDE_DIR1
            NAMES ft2build.h
            PATHS
            /usr/local/include/freetype2
            DOC "The directory where ft2build.h resides")
    
	find_path(
            FREETYPE_INCLUDE_DIR2
            NAMES freetype/config/ftheader.h
            PATHS
            /usr/local/include/freetype2
            DOC "The directory where freetype/config/ftheader.h resides")
			
    # Find library files
    find_library(
            FREETYPE_LIBRARY
            NAMES libfreetype.dylib
            PATHS
            /usr/local/lib
            DOC "The Freetype library")
endif()

# Handle REQUIRD argument, define *_FOUND variable
find_package_handle_standard_args(Freetype DEFAULT_MSG FREETYPE_INCLUDE_DIR1 FREETYPE_INCLUDE_DIR2 FREETYPE_LIBRARY)

if (FREETYPE_FOUND)
    set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY})
    set(FREETYPE_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR1} ${FREETYPE_INCLUDE_DIR2})
endif()

# Hide some variables
mark_as_advanced(FREETYPE_INCLUDE_DIR FREETYPE_LIBRARY)