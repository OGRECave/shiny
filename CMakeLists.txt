cmake_minimum_required(VERSION 2.8)

option(SHINY_BUILD_OGRE_PLATFORM "build the Ogre platform" ON)

set(SHINY_LIBRARY "shiny")
set(SHINY_OGREPLATFORM_LIBRARY "shiny.OgrePlatform")

# Sources
file(GLOB SOURCE_FILES Main/*.cpp )

# In Debug mode, write the shader sources to the current directory
if (DEFINED CMAKE_BUILD_TYPE)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_definitions(-DSHINY_WRITE_SHADER_DEBUG)
    endif()
endif()

if (NOT SHINY_USE_WAVE_SYSTEM_INSTALL)
    file(GLOB SOURCE_FILES2 Preprocessor/*.cpp)
    list(APPEND SOURCE_FILES ${SOURCE_FILES2})

    # Don't use thread-safe boost::wave. Results in a huge speed-up for the preprocessor.
    add_definitions(-DBOOST_WAVE_SUPPORT_THREADING=0)

else()
    # use system install
endif()

file(GLOB OGRE_PLATFORM_SOURCE_FILES Platforms/Ogre/*.cpp)

add_library(${SHINY_LIBRARY} STATIC ${SOURCE_FILES})

if (SHINY_BUILD_OGRE_PLATFORM)
    add_library(${SHINY_OGREPLATFORM_LIBRARY} STATIC ${OGRE_PLATFORM_SOURCE_FILES})
endif()


link_directories(${CMAKE_CURRENT_BINARY_DIR})
