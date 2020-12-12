cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(MiniRenderer)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(MiniRenderer)

set(MiniRenderer_PRECOMPILED_HEADERS
    "src/main.hpp"
)

set(MiniRenderer_HEADERS
    "src/main.hpp"
)

set(MiniRenderer_SOURCES
    "src/main.cpp"
)

target_precompile_headers(MiniRenderer PRIVATE
    ${MiniRenderer_PRECOMPILED_HEADERS}
)

target_sources(MiniRenderer PRIVATE
    ${MiniRenderer_HEADERS}
    ${MiniRenderer_SOURCES}
)

target_include_directories(MiniRenderer PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    "src/"
)

target_link_libraries(MiniRenderer PRIVATE
    ${Vulkan_LIBRARIES}
    glfw
    glm
)