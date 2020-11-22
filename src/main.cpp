#include <iostream>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <vulkan/vulkan.hpp>

int main()
{
	std::cout << glfwGetVersionString() << std::endl;
	std::cout << glm::to_string(glm::vec3(1, 2, 3)) << std::endl;
	std::cout << "Hello, world!" << std::endl;

	return 0;
}
