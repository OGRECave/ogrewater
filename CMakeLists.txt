cmake_minimum_required(VERSION 3.10)

project(OgreWater)

# specify which version and components you need
find_package(OGRE 13 REQUIRED COMPONENTS Bites Terrain CONFIG)

file(GLOB SRCS OgreWater/*cpp)
add_library(OgreWater ${SRCS})
target_link_libraries(OgreWater PUBLIC OgreMain)
target_include_directories(OgreWater PUBLIC OgreWater)

add_executable(OgreWaterApp WIN32 OWApplication.cpp OWMain.cpp)
target_link_libraries(OgreWaterApp OgreWater OgreBites OgreTerrain)

configure_file("resources.cfg.in" "${PROJECT_BINARY_DIR}/resources.cfg" @ONLY)