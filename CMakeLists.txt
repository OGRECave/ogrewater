cmake_minimum_required(VERSION 3.10)

project(OgreWater)

# specify which version and components you need
find_package(OGRE REQUIRED COMPONENTS Bites Terrain CONFIG)

if(OGRE_STATIC)
    find_package(ZLIB)
    find_package(SDL2)
    find_package(assimp REQUIRED CONFIG)
    find_package(pugixml REQUIRED CONFIG)

    add_library(fix::assimp INTERFACE IMPORTED)
    set_target_properties(fix::assimp PROPERTIES
        INTERFACE_LINK_LIBRARIES "${ASSIMP_LIBRARIES};pugixml"
        INTERFACE_LINK_DIRECTORIES "${ASSIMP_LIBRARY_DIRS}"
    )
endif()

file(GLOB SRCS OgreWater/*cpp)
add_library(OgreWater ${SRCS})
target_link_libraries(OgreWater PUBLIC OgreMain)
target_include_directories(OgreWater PUBLIC OgreWater)

add_executable(OgreWaterApp WIN32 OWApplication.cpp OWMain.cpp)
target_link_libraries(OgreWaterApp OgreWater OgreBites OgreTerrain)

configure_file("resources.cfg.in" "${PROJECT_BINARY_DIR}/resources.cfg" @ONLY)
