#-------------------------------------------------------------------
# This file is part of the CMake build system for shiny
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find shiny and shiny.OgrePlatform
# Once done this will define
#
#  SHINY_FOUND - system has shiny
#  SHINY_PLATFORM_FOUND - system has shiny.OgrePlatform
#  SHINY_INCLUDE_DIRS - the shiny include directory
#  SHINY_PLATFORM_INCLUDE_DIRS - the shiny.OgrePlatform include directory
#  SHINY_LIBRARIES - the libraries needed to use
#                    shiny with OgrePlatform if SHINY_PLATFORM_FOUND or
#                    shiny without OgrePlatform if NOT SHINY_PLATFORM_FOUND.
#
# $SHINYDIR is an environment variable used for finding shiny.

FIND_PATH(SHINY_INCLUDE_DIRS shiny/Extra/core.h
    PATHS
    $ENV{SHINYDIR}
    /usr/local
    /usr
    PATH_SUFFIXES include
    )

FIND_PATH(SHINY_PLATFORM_INCLUDE_DIRS shiny/Platforms/Ogre/OgrePlatform.hpp
    PATHS
    $ENV{SHINYDIR}
    /usr/local
    /usr
    PATH_SUFFIXES include
    )

FIND_LIBRARY(SHINY_LIBRARY
    NAMES shiny
    PATHS
    $ENV{SHINYDIR}
    /usr/local
    /usr
    PATH_SUFFIXES lib
    )

FIND_LIBRARY(SHINY_PLATFORM_LIBRARY
    NAMES shiny.OgrePlatform
    PATHS
    $ENV{SHINYDIR}
    /usr/local/lib${LIB_SUFFIX}
    /usr/lib${LIB_SUFFIX}
    PATH_SUFFIXES OGRE
    )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SHINY DEFAULT_MSG SHINY_LIBRARY SHINY_INCLUDE_DIRS)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SHINY_PLATFORM DEFAULT_MSG SHINY_PLATFORM_LIBRARY SHINY_PLATFORM_INCLUDE_DIRS)

IF (SHINY_FOUND)
    IF (SHINY_PLATFORM_FOUND)
        SET(SHINY_LIBRARIES ${SHINY_LIBRARY} ${SHINY_PLATFORM_LIBRARY})
    ELSE (SHINY_PLATFORM_FOUND)
        SET(SHINY_LIBRARIES ${SHINY_LIBRARY})
    ENDIF (SHINY_PLATFORM_FOUND)
ENDIF (SHINY_FOUND)

MARK_AS_ADVANCED(SHINY_LIBRARY SHINY_LIBRARIES SHINY_INCLUDE_DIRS)
