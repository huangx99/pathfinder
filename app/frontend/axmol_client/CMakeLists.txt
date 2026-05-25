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
set(AX_ENABLE_3D OFF CACHE BOOL "Pathfinder sandbox is 2D only" FORCE)
set(AX_ENABLE_AUDIO OFF CACHE BOOL "Disable audio for Sandbox client" FORCE)
set(AX_ENABLE_HTTP OFF CACHE BOOL "Sandbox client runs local backend runtime directly" FORCE)
set(AX_ENABLE_WEBSOCKET OFF CACHE BOOL "Disable websocket for Sandbox client" FORCE)
set(AX_ENABLE_MEDIA OFF CACHE BOOL "Disable media for Sandbox client" FORCE)
set(AX_ENABLE_PHYSICS_3D OFF CACHE BOOL "Disable 3D physics" FORCE)
set(AX_ENABLE_NAVMESH OFF CACHE BOOL "NPC logic stays backend-owned" FORCE)

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
    Source/app/AppDelegate.cpp
    Source/runtime/EngineLocalClient.cpp
    Source/scene/SandboxScene.cpp
    Source/world/SandboxMapLayer.cpp
    Source/ui/UiStyle.cpp
    Source/ui/ToolPalettePanel.cpp
    Source/ui/PixelUI.cpp
    Source/ui/EventLogPanel.cpp
    Source/ui/PlaybackControlPanel.cpp
    Source/ui/AgentInfoPanel.cpp
    Source/procedural/GrassTile.cpp
    Source/procedural/SoilTile.cpp
    Source/procedural/WaterTile.cpp
    Source/procedural/SelectionTile.cpp
    Source/procedural/Player.cpp
    Source/procedural/RedBerry.cpp
    Source/procedural/DecayedRedBerry.cpp
    Source/procedural/BitterLeaf.cpp
    Source/procedural/StoneFlake.cpp
    Source/procedural/Wood.cpp
    proj.linux/main.cpp
)

add_executable(${APP_NAME} ${PATHFINDER_AXMOL_SOURCES})
target_include_directories(${APP_NAME} PRIVATE Source)
target_link_libraries(${APP_NAME} PRIVATE
    ${_AX_CORE_LIB}
    pathfinder_client_runtime_host
)
target_compile_definitions(${APP_NAME} PRIVATE PATHFINDER_REPO_ROOT="${PATHFINDER_REPO_DIR}")

if(LINUX)
    set_target_properties(${APP_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
