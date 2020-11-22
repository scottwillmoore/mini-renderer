cmake_minimum_required(VERSION 3.18)

project(MiniRenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE glm)
