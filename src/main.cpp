#include <cstdlib>
#include <cstdint>
#include <cstring>

#include <algorithm>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

#define USE_VALIDATION

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const char* APP_NAME = "Mini Renderer";
const uint32_t APP_VERSION = 1;

const char* ENGINE_NAME = "Mini Renderer";
const uint32_t ENGINE_VERSION = 1;

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	VkDebugUtilsMessengerCallbackDataEXT const* callbackData,
	void* userData)
{
	std::stringstream message;

	message << "messageSeverity = " << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) << std::endl;
	message << "messageTypes    = " << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes)) << std::endl;

	message << "\tmessageIdName   = " << callbackData->pMessageIdName << std::endl;
	message << "\tmessageIdNumber = " << callbackData->messageIdNumber << std::endl;
	message << "\tmessage         = " << callbackData->pMessage << std::endl;

	std::cout << message.str() << std::endl;

	return VK_FALSE;
}

void run() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	auto window = glfwCreateWindow(WIDTH, HEIGHT, APP_NAME, nullptr, nullptr);

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
	vk::DynamicLoader dynamicLoader;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#endif

	std::vector<char const*> layerNames;
#ifdef USE_VALIDATION
	layerNames.push_back("VK_LAYER_KHRONOS_validation");
#endif

	std::vector<vk::LayerProperties> layerProperties = vk::enumerateInstanceLayerProperties();
	auto hasLayers = std::all_of(layerNames.begin(), layerNames.end(), [&layerProperties](char const* layerName) {
		return std::find_if(layerProperties.begin(), layerProperties.end(), [&layerName](vk::LayerProperties const& layerProperty) {
			return strcmp(layerProperty.layerName, layerName) == 0;
			}) != layerProperties.end();
		});
	if (!hasLayers) {
		throw std::runtime_error("could not find all required layers");
	}


	uint32_t glfwRequiredExtensionsCount = 0;
	const char** glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionsCount);
	std::vector<char const*> extensionNames(glfwRequiredExtensions, glfwRequiredExtensions + glfwRequiredExtensionsCount);
#ifdef USE_VALIDATION
	extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	vk::ApplicationInfo applicationInfo(APP_NAME, APP_VERSION, ENGINE_NAME, ENGINE_VERSION, VK_API_VERSION_1_2);
	vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo, layerNames, extensionNames);

#ifdef USE_VALIDATION
	vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo(
		{},
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
		&debugCallback
	);
	vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> instanceCreateInfoChain(instanceCreateInfo, debugUtilsMessengerCreateInfo);
#else
	vk::StructureChain<vk::InstanceCreateInfo> instanceCreateInfoChain(instanceCreateInfo);
#endif

	vk::UniqueInstance instance = vk::createInstanceUnique(instanceCreateInfoChain.get<vk::InstanceCreateInfo>());

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
#endif
	vk::UniqueSurfaceKHR surface;
	{
		VkSurfaceKHR _surface;
		if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != VK_SUCCESS) {
			throw std::runtime_error("could not create window surface");
		}
		vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> _deleter(*instance);
		surface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(_surface), _deleter);
	}

#ifdef USE_VALIDATION
	vk::UniqueDebugUtilsMessengerEXT debugUtilsMessenger = instance->createDebugUtilsMessengerEXTUnique(debugUtilsMessengerCreateInfo);
#endif

	// TODO: Find device that is suitable and most appropriate.
	vk::PhysicalDevice physicalDevice = instance->enumeratePhysicalDevices().front();

	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

	std::optional<uint32_t> graphicsQueueFamily;
	for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
		if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
			graphicsQueueFamily = i;
			break;
		}
	}
	if (!graphicsQueueFamily.has_value()) {
		throw std::runtime_error("could not find suitable graphics queue family");
	}

	std::optional<uint32_t> presentQueueFamily;
	for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
		if (physicalDevice.getSurfaceSupportKHR(i, *surface)) {
			presentQueueFamily = i;
			break;
		}
	}
	if (!presentQueueFamily.has_value()) {
		throw std::runtime_error("could not find suitable present queue family");
	}

	std::set<uint32_t> uniqueQueueFamilies = { graphicsQueueFamily.value(), presentQueueFamily.value() };

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	float queuePriority = 1.0f;
	for (auto& queueFamily : uniqueQueueFamilies) {
		queueCreateInfos.push_back({ vk::DeviceQueueCreateFlags(), queueFamily, 1, &queuePriority });
	}

	vk::DeviceCreateInfo deviceCreateInfo(vk::DeviceCreateFlags(), queueCreateInfos);
	vk::UniqueDevice device = physicalDevice.createDeviceUnique(deviceCreateInfo);

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
#endif

	vk::Queue graphicsQueue = device->getQueue(graphicsQueueFamily.value(), 0);
	vk::Queue presentQueue = device->getQueue(presentQueueFamily.value(), 0);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}

int main() {
	try {
		std::cout << "Mini Renderer" << std::endl;
		run();
	}
	catch (std::exception& error) {
		std::cout << error.what() << std::endl;
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
