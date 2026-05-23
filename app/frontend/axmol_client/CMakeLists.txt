cmake_minimum_required(VERSION 3.22...4.3)
project(PathfinderAxmolClient LANGUAGES CXX)

set(APP_NAME PathfinderAxmolClient)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(PATHFINDER_REPO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(AXMOL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/axmol" CACHE PATH "Bundled Axmol source root")
if(NOT EXISTS "${AXMOL_ROOT}/axmol/CMakeLists.txt")
    message(FATAL_ERROR "Axmol source not found at ${AXMOL_ROOT}. Expected app/frontend/third_party/axmol.")
endif()

set(_AX_ROOT "${AXMOL_ROOT}" CACHE INTERNAL "Axmol root" FORCE)
set(ENV{AX_ROOT} "${AXMOL_ROOT}")
list(APPEND CMAKE_MODULE_PATH "${AXMOL_ROOT}/cmake/Modules")
include(AXBuildSet)

set(AX_BUILD_TESTS OFF CACHE BOOL "Do not build Axmol tests for Pathfinder client" FORCE)
set(AX_ENABLE_EXT_LUA OFF CACHE BOOL "Disable Lua extension" FORCE)
set(AX_ENABLE_EXT_LIVE2D OFF CACHE BOOL "Disable Live2D extension" FORCE)
set(AX_ENABLE_EXT_EFFEKSEER OFF CACHE BOOL "Disable Effekseer extension" FORCE)
set(AX_ENABLE_3D OFF CACHE BOOL "Pathfinder Axmol client starts as 2D only" FORCE)
set(AX_ENABLE_AUDIO OFF CACHE BOOL "Disable audio for first Pathfinder client smoke test" FORCE)
set(AX_ENABLE_HTTP OFF CACHE BOOL "Disable Axmol HTTP; Pathfinder transport is separate" FORCE)
set(AX_ENABLE_WEBSOCKET OFF CACHE BOOL "Disable Axmol websocket; Pathfinder transport is separate" FORCE)
set(AX_ENABLE_MEDIA OFF CACHE BOOL "Disable media for first Pathfinder client smoke test" FORCE)
set(AX_ENABLE_PHYSICS_3D OFF CACHE BOOL "Disable 3D physics" FORCE)
set(AX_ENABLE_NAVMESH OFF CACHE BOOL "Disable engine navmesh; gameplay pathing stays backend-owned" FORCE)

add_subdirectory("${AXMOL_ROOT}/axmol" "${CMAKE_BINARY_DIR}/axmol")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_subdirectory("${PATHFINDER_REPO_DIR}/backend" "${CMAKE_BINARY_DIR}/pathfinder_backend")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    get_property(_pathfinder_targets DIRECTORY "${PATHFINDER_REPO_DIR}/backend" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_target ${_pathfinder_targets})
        if(_target MATCHES "^pathfinder_")
            target_compile_options(${_target} PRIVATE -fchar8_t)
        endif()
    endforeach()
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PATHFINDER_AXMOL_SOURCES
    Source/AppDelegate.cpp
    Source/MainScene.cpp
    Source/local/LocalClientRuntime.cpp
    Source/ui/ClientUiFormat.cpp
    Source/procedural/BerryBush.cpp
    Source/procedural/CampFire.cpp
    Source/procedural/Companion.cpp
    Source/procedural/ForestTile.cpp
    Source/procedural/GrassTile.cpp
    Source/procedural/LooseStone.cpp
    Source/procedural/PathDot.cpp
    Source/procedural/Axe.cpp
    Source/procedural/Player.cpp
    Source/procedural/RockyTile.cpp
    Source/procedural/RedBerry.cpp
    Source/procedural/SelectionTile.cpp
    Source/procedural/StoneFlake.cpp
    Source/procedural/StonePile.cpp
    Source/procedural/Torch.cpp
    Source/procedural/Tree.cpp
    Source/procedural/WaterTile.cpp
    Source/procedural/Wolf.cpp
    Source/procedural/Wood.cpp
    proj.linux/main.cpp
)

add_executable(${APP_NAME} ${PATHFINDER_AXMOL_SOURCES})
target_include_directories(${APP_NAME} PRIVATE Source)
target_link_libraries(${APP_NAME} PRIVATE
    ${_AX_CORE_LIB}
    pathfinder_client_http
)
target_compile_definitions(${APP_NAME} PRIVATE PATHFINDER_REPO_ROOT="${PATHFINDER_REPO_DIR}")

if(LINUX)
    set_target_properties(${APP_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
