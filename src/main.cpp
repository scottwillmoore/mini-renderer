#include <cstdlib>
#include <cstdint>

#include <iostream>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <vulkan/vulkan.hpp>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const char* APP_NAME = "Mini Renderer";
const uint32_t APP_VERSION = 1;

const char* ENGINE_NAME = "Mini Renderer";
const uint32_t ENGINE_VERSION = 1;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	auto window = glfwCreateWindow(WIDTH, HEIGHT, APP_NAME, nullptr, nullptr);

	try {
		vk::ApplicationInfo applicationInfo(APP_NAME, APP_VERSION, ENGINE_NAME, ENGINE_VERSION, VK_API_VERSION_1_2);
		vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo);
		vk::Instance instance = vk::createInstance(instanceCreateInfo);

		std::vector<vk::ExtensionProperties> extensionProperties = vk::enumerateInstanceExtensionProperties();
		for (auto const& extension : extensionProperties) {
			std::cout << extension.extensionName << ": " << extension.specVersion << std::endl;
		}
	}
	catch (vk::SystemError& error) {
		std::cout << "vk::SystemError: " << error.what() << std::endl;
		exit(EXIT_FAILURE);
	}
	catch (std::exception& error) {
		std::cout << "std::exception: " << error.what() << std::endl;
		exit(EXIT_FAILURE);
	}
	catch (...) {
		std::cout << "An unknown error occured" << std::endl;
		exit(EXIT_FAILURE);
	}

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return EXIT_SUCCESS;
}
