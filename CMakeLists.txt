cmake_minimum_required(VERSION 2.8)

option(SHINY_BUILD_OGRE_PLATFORM "build the Ogre platform" ON)

set(SHINY_LIBRARY "shiny")
set(SHINY_OGREPLATFORM_LIBRARY "shiny.OgrePlatform")

# Sources
file(GLOB SOURCE_FILES Main/*.cpp Preprocessor/*.cpp)
file(GLOB OGRE_PLATFORM_SOURCE_FILES Platforms/Ogre/*.cpp)

# Don't use thread-safe boost::wave. Results in a huge speed-up for the preprocessor.
add_definitions(-DBOOST_WAVE_SUPPORT_THREADING=0)

add_library(${SHINY_LIBRARY} STATIC ${SOURCE_FILES})

set_target_properties(${SHINY_LIBRARY} PROPERTIES COMPILE_FLAGS "-DMCPP_LIB")

if (SHINY_BUILD_OGRE_PLATFORM)
    add_library(${SHINY_OGREPLATFORM_LIBRARY} STATIC ${OGRE_PLATFORM_SOURCE_FILES})
endif()


link_directories(${CMAKE_CURRENT_BINARY_DIR})
