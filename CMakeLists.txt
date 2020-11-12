cmake_minimum_required(VERSION 3.18)

set(EXECUTABLE_NAME VulkanExperiments)

project(${EXECUTABLE_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_executable(${EXECUTABLE_NAME} src/main.cpp)

target_include_directories(${EXECUTABLE_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})

target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(${EXECUTABLE_NAME} PRIVATE glfw)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE glm)