#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "main.hpp"

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const char *APP_NAME = "Mini Renderer";
const uint32_t APP_VERSION = 1;

const char *ENGINE_NAME = "Mini Renderer";
const uint32_t ENGINE_VERSION = 1;

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    VkDebugUtilsMessengerCallbackDataEXT const *callbackData,
    void *userData)
{
    std::stringstream message;

    message << "messageSeverity = "
            << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity))
            << std::endl;

    message << "messageTypes    = "
            << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes))
            << std::endl;

    message << "\tmessageIdName   = " << callbackData->pMessageIdName << std::endl;
    message << "\tmessageIdNumber = " << callbackData->messageIdNumber << std::endl;
    message << "\tmessage         = " << callbackData->pMessage << std::endl;

    std::cout << message.str() << std::endl;

    return VK_FALSE;
}

std::vector<char>
readBinaryFile(const std::string &name)
{
    std::ifstream file(name, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("could not open file");
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

bool
hasLayers(std::vector<vk::LayerProperties> layerProperties, std::vector<char const *> layerNames)
{
    return std::all_of(
        layerNames.begin(),
        layerNames.end(),
        [&layerProperties](char const *layerName) {
            return std::find_if(
                       layerProperties.begin(),
                       layerProperties.end(),
                       [&layerName](vk::LayerProperties const &layerProperty) {
                           return strcmp(layerProperty.layerName, layerName) == 0;
                       })
                != layerProperties.end();
        });
}

bool
hasExtensions(
    std::vector<vk::ExtensionProperties> extensionProperties,
    std::vector<char const *> extensionNames)
{
    return std::all_of(
        extensionNames.begin(),
        extensionNames.end(),
        [&extensionProperties](char const *extensionName) {
            return std::find_if(
                       extensionProperties.begin(),
                       extensionProperties.end(),
                       [&extensionName](vk::ExtensionProperties const &extensionProperty) {
                           return strcmp(extensionProperty.extensionName, extensionName) == 0;
                       })
                != extensionProperties.end();
        });
}

vk::DebugUtilsMessengerCreateInfoEXT
createDebugMessengerCreateInfo()
{
    return vk::DebugUtilsMessengerCreateInfoEXT()
        .setMessageSeverity(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        .setMessageType(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
        .setPfnUserCallback(&debugCallback);
}

vk::UniqueInstance
createInstance(std::optional<vk::DebugUtilsMessengerCreateInfoEXT> debugMessengerCreateInfo)
{
    std::vector<char const *> layerNames;
    if (debugMessengerCreateInfo.has_value()) {
        layerNames.push_back("VK_LAYER_KHRONOS_validation");
    }
    if (!hasLayers(vk::enumerateInstanceLayerProperties(), layerNames)) {
        throw std::runtime_error("could not find all required instance layers");
    }

    uint32_t glfwRequiredExtensionsCount = 0;
    const char **glfwRequiredExtensions =
        glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionsCount);
    std::vector<char const *> extensionNames(
        glfwRequiredExtensions,
        glfwRequiredExtensions + glfwRequiredExtensionsCount);
    if (debugMessengerCreateInfo.has_value()) {
        extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    if (!hasExtensions(vk::enumerateInstanceExtensionProperties(), extensionNames)) {
        throw std::runtime_error("could not find all required instance extensions");
    }

    vk::ApplicationInfo applicationInfo{
        .pApplicationName = APP_NAME,
        .applicationVersion = APP_VERSION,
        .pEngineName = ENGINE_NAME,
        .engineVersion = ENGINE_VERSION,
        .apiVersion = VK_API_VERSION_1_2,
    };

    vk::InstanceCreateInfo instanceCreateInfo{
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<uint32_t>(layerNames.size()),
        .ppEnabledLayerNames = layerNames.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data(),
    };

    if (debugMessengerCreateInfo.has_value()) {
        instanceCreateInfo.pNext = &debugMessengerCreateInfo.value();
    }

    auto instance = vk::createInstanceUnique(instanceCreateInfo);

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
#endif

    return instance;
}

vk::UniqueDebugUtilsMessengerEXT
createDebugMessenger(
    vk::Instance instance,
    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo)
{
    return instance.createDebugUtilsMessengerEXTUnique(debugMessengerCreateInfo);
}

vk::UniqueSurfaceKHR
createSurface(GLFWwindow *window, vk::Instance instance)
{
    vk::UniqueSurfaceKHR surface;
    {
        VkSurfaceKHR _surface;
        if (glfwCreateWindowSurface(instance, window, nullptr, &_surface) != VK_SUCCESS) {
            throw std::runtime_error("could not create window surface");
        }
        vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> _deleter(instance);
        surface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(_surface), _deleter);
    }
    return surface;
}

// TODO: Improve physical device selection.
// The first physical device is selected, however there may be multiple
// Vulkan-enabled physical devices.
vk::PhysicalDevice
choosePhysicalDevice(vk::Instance instance)
{
    return instance.enumeratePhysicalDevices().front();
}

// NOTE: Improve queue selection.
// The graphics and present queues could be different, however they are
// currently assumed identical. In addition, more queues may be desired for
// transfer and compute operations.
uint32_t
choosePrimaryQueueFamily(vk::SurfaceKHR surface, vk::PhysicalDevice physicalDevice)
{
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        physicalDevice.getQueueFamilyProperties();

    for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics
            && physicalDevice.getSurfaceSupportKHR(i, surface)) {
            return i;
        }
    }

    throw std::runtime_error(
        "could not find suitable queue family that supports graphics and presentation");
}

vk::UniqueDevice
createDevice(
    vk::Instance instance,
    vk::SurfaceKHR surface,
    vk::PhysicalDevice physicalDevice,
    uint32_t primaryQueueFamily)
{
    std::vector<char const *> extensionNames;
    extensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    if (!hasExtensions(physicalDevice.enumerateDeviceExtensionProperties(), extensionNames)) {
        throw std::runtime_error("could not find all required device extensions");
    }

    float primaryQueuePriority = 1.0;
    vk::DeviceQueueCreateInfo primaryQueueCreateInfo{
        .queueFamilyIndex = primaryQueueFamily,
        .queueCount = 1,
        .pQueuePriorities = &primaryQueuePriority};

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.push_back(primaryQueueCreateInfo);

    vk::DeviceCreateInfo deviceCreateInfo{
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data(),
    };

    auto device = physicalDevice.createDeviceUnique(deviceCreateInfo);

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
#endif

    return device;
}

/**
vk::SurfaceFormatKHR
chooseSurfaceFormat(vk::SurfaceKHR surface, vk::PhysicalDevice physicalDevice)
{
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
    if (surfaceFormats.empty()) {
        throw std::runtime_error("could not find any surface formats");
    }

    vk::SurfaceFormatKHR chosenFormat;
    for (auto &surfaceFormat : surfaceFormats) {
        if (surfaceFormat.format == vk::Format::eB8G8R8A8Srgb
            && surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            chosenFormat = surfaceFormat;
        }
    }

    return chosenFormat;
}

vk::PresentModeKHR
choosePresentMode(vk::SurfaceKHR surface, vk::PhysicalDevice physicalDevice)
{
    std::vector<vk::PresentModeKHR> surfacePresentModes =
        physicalDevice.getSurfacePresentModesKHR(surface);
    if (surfacePresentModes.empty()) {
        throw std::runtime_error("could not find any surface present modes");
    }

    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
    for (auto &surfacePresentMode : surfacePresentModes) {
        if (surfacePresentMode == vk::PresentModeKHR::eMailbox) {
            presentMode = surfacePresentMode;
        }
    }

    return presentMode;
}

vk::SurfaceCapabilitiesKHR
getSurfaceCapabilities(vk::SurfaceKHR surface, vk::PhysicalDevice physicalDevice)
{
    return physicalDevice.getSurfaceCapabilitiesKHR(surface);
}

vk::Extent2D
chooseExtent(GLFWwindow *window, vk::SurfaceCapabilitiesKHR surfaceCapabilities)
{
    auto extent = surfaceCapabilities.currentExtent;

    if (surfaceCapabilities.currentExtent.width == UINT32_MAX) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        vk::Extent2D windowExtent(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        windowExtent.width = std::clamp(
            windowExtent.width,
            surfaceCapabilities.minImageExtent.width,
            surfaceCapabilities.maxImageExtent.width);
        windowExtent.height = std::clamp(
            windowExtent.height,
            surfaceCapabilities.minImageExtent.height,
            surfaceCapabilities.maxImageExtent.height);

        extent = windowExtent;
    }

    return extent;
}

// NOTE: Changes may be required if the graphics and present queue families are
// different.
// vk::UniqueSwapchainKHR createSwapchain(vk::SurfaceKHR surface,
// vk::SurfaceCapabilitiesKHR surfaceCapabilities, vk::SurfaceFormatKHR
// surfaceFormat, vk::PresentModeKHR presentMode, vk::Extent2D extent) {
vk::UniqueSwapchainKHR
createSwapchain(
    vk::SurfaceKHR surface,
    vk::Format format,
    vk::ColorSpaceKHR colorSpace,
    vk::SurfaceFormatKHR surfaceFormat,
    vk::PresentModeKHR presentMode,
    vk::Extent2D extent)
{
    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    auto swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
                                   .setSurface(surface)
                                   .setMinImageCount(imageCount)
                                   .setImageFormat(format)
                                   .setImageColorSpace(colorSpace)
                                   .setImageExtent(extent)
                                   .setImageArrayLayers(1)
                                   .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                                   .setImageSharingMode(vk::SharingMode::eExclusive)
                                   .setPreTransform(surfaceCapabilities.currentTransform)
                                   .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                                   .setPresentMode(swapchainPresentMode)
                                   .setClipped(VK_TRUE);

    return device.createSwapchainKHRUnique(swapchainCreateInfo);
}
*/

void
run()
{
    // createWindow
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(WIDTH, HEIGHT, APP_NAME, nullptr, nullptr);

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
    vk::DynamicLoader dynamicLoader;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
        dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#endif

    // createInstance
    auto useValidation = true;
    std::optional<vk::DebugUtilsMessengerCreateInfoEXT> debugMessengerCreateInfo;
    if (useValidation) {
        debugMessengerCreateInfo = createDebugMessengerCreateInfo();
    }

    auto instance = createInstance(debugMessengerCreateInfo);

    std::optional<vk::UniqueDebugUtilsMessengerEXT> debugMessenger;
    if (useValidation) {
        debugMessenger = createDebugMessenger(*instance, debugMessengerCreateInfo.value());
    }

    // createSurface
    auto surface = createSurface(window, *instance);

    // createDevice
    auto physicalDevice = choosePhysicalDevice(*instance);
    auto primaryQueueFamily = choosePrimaryQueueFamily(*surface, physicalDevice);
    auto device = createDevice(*instance, *surface, physicalDevice, primaryQueueFamily);
    auto primaryQueue = device->getQueue(primaryQueueFamily, 0);

    // createSwapchain
    /*
    auto surfaceCapabilities = getSurfaceCapabilities(*surface, physicalDevice);
    auto surfaceFormat = chooseSurfaceFormat(*surface, physicalDevice);
    auto presentMode = choosePresentMode(*surface, physicalDevice);
    auto extent = chooseExtent(window, surfaceCapabilities);

    auto swapchain = createSwapchain(window, *surface, physicalDevice, *device);

    std::vector<vk::Image> swapchainImages = device->getSwapchainImagesKHR(*swapchain);

    std::vector<vk::UniqueImageView> swapchainImageViews(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++) {
        vk::ImageViewCreateInfo imageViewCreateInfo(
            vk::ImageViewCreateFlags(),
            swapchainImages[i],
            vk::ImageViewType::e2D,
            swapchainFormat.format,
            {},
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        swapchainImageViews[i] = device->createImageViewUnique(imageViewCreateInfo);
    }

    auto vertexShaderBuffer = readBinaryFile("../resources/shader.vert.spv");
    vk::ShaderModuleCreateInfo vertexShaderCreateInfo(
        vk::ShaderModuleCreateFlags(),
        vertexShaderBuffer.size(),
        reinterpret_cast<uint32_t *>(vertexShaderBuffer.data()));
    vk::UniqueShaderModule vertexShader = device->createShaderModuleUnique(vertexShaderCreateInfo);

    auto fragmentShaderBuffer = readBinaryFile("../resources/shader.frag.spv");
    vk::ShaderModuleCreateInfo fragmentShaderCreateInfo(
        vk::ShaderModuleCreateFlags(),
        fragmentShaderBuffer.size(),
        reinterpret_cast<uint32_t *>(fragmentShaderBuffer.data()));
    vk::UniqueShaderModule fragmentShader =
        device->createShaderModuleUnique(fragmentShaderCreateInfo);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    shaderStages.push_back(vk::PipelineShaderStageCreateInfo(
        vk::PipelineShaderStageCreateFlags(),
        vk::ShaderStageFlagBits::eVertex,
        *vertexShader,
        "main"));
    shaderStages.push_back(vk::PipelineShaderStageCreateInfo(
        vk::PipelineShaderStageCreateFlags(),
        vk::ShaderStageFlagBits::eFragment,
        *fragmentShader,
        "main"));

    // NOTE: Decided to use the setter functions to see if it improves
    // readability.

    auto vertexInputCreateInfo = vk::PipelineVertexInputStateCreateInfo();

    auto inputAssemblyCreateInfo = vk::PipelineInputAssemblyStateCreateInfo()
                                       .setTopology(vk::PrimitiveTopology::eTriangleList)
                                       .setPrimitiveRestartEnable(false);

    auto viewport = vk::Viewport()
                        .setX(0.0)
                        .setY(0.0)
                        .setWidth(swapchainExtent.width)
                        .setHeight(swapchainExtent.height)
                        .setMinDepth(0.0)
                        .setMaxDepth(1.0);

    auto scissor = vk::Rect2D().setOffset({0, 0}).setExtent(swapchainExtent);

    auto viewportStageCreateInfo =
        vk::PipelineViewportStateCreateInfo().setViewports(viewport).setScissors(scissor);

    auto rasterizerCreateInfo = vk::PipelineRasterizationStateCreateInfo()
                                    .setDepthClampEnable(false)
                                    .setPolygonMode(vk::PolygonMode::eFill)
                                    .setLineWidth(1.0)
                                    .setCullMode(vk::CullModeFlagBits::eBack)
                                    .setFrontFace(vk::FrontFace::eClockwise)
                                    .setDepthBiasClamp(false);

    auto multisampleCreateInfo = vk::PipelineMultisampleStateCreateInfo()
                                     .setSampleShadingEnable(false)
                                     .setRasterizationSamples(vk::SampleCountFlagBits::e1)
                                     .setMinSampleShading(1.0);

    auto colorBlendAttachment =
        vk::PipelineColorBlendAttachmentState()
            .setColorWriteMask(
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
            .setBlendEnable(false)
            .setSrcColorBlendFactor(vk::BlendFactor::eOne)
            .setDstColorBlendFactor(vk::BlendFactor::eZero)
            .setColorBlendOp(vk::BlendOp::eAdd)
            .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
            .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
            .setAlphaBlendOp(vk::BlendOp::eAdd);

    auto colorBlendCreateInfo = vk::PipelineColorBlendStateCreateInfo()
                                    .setLogicOpEnable(false)
                                    .setLogicOp(vk::LogicOp::eCopy)
                                    .setAttachments(colorBlendAttachment)
                                    .setBlendConstants({0.0, 0.0, 0.0, 0.0});

    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eLineWidth};
    auto dynamicStateCreateInfo =
        vk::PipelineDynamicStateCreateInfo().setDynamicStates(dynamicStates);

    auto pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo();
    auto pipelinelayout = device->createPipelineLayoutUnique(pipelineLayoutCreateInfo);
    */

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    // destroyWindow
    glfwDestroyWindow(window);
    glfwTerminate();
}

int
main()
{
    try {
        std::cout << "Mini Renderer" << std::endl;
        run();
    } catch (std::exception &error) {
        std::cout << error.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
