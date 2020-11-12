cmake_minimum_required(VERSION 3.18)

project(vulkan-experiments)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(vulkan-experiments src/main.cpp)

add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
target_link_libraries(vulkan-experiments PRIVATE glfw)

add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
target_link_libraries(vulkan-experiments PRIVATE glm)