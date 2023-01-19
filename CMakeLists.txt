cmake_minimum_required(VERSION 3.10)

project(OgreWater)

# specify which version and components you need
find_package(OGRE 13 REQUIRED COMPONENTS Bites Terrain CONFIG)

file(GLOB SRCS OgreWater/*cpp)
add_executable(OgreWater WIN32 ${SRCS})
target_link_libraries(OgreWater OgreBites OgreTerrain)

configure_file("resources.cfg.in" "${PROJECT_BINARY_DIR}/resources.cfg" @ONLY)