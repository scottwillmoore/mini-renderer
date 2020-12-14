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

uint32_t const WIDTH = 800;
uint32_t const HEIGHT = 600;

char const *APP_NAME = "Mini Renderer";
uint32_t const APP_VERSION = 1;
uint32_t const REQUIRED_VULKAN_VERSION = VK_API_VERSION_1_2;

// The order of declaration determines the order that destructors are invoked, which is important
// for safe destruction of resources. I beleive in a single compilation unit this order is
// well-defined, however it will always be better to wrap these variables in a struct or class or
// excplicitly invoke the destructors.
vk::DynamicLoader dynamicLoader;
GLFWwindow *window;
vk::UniqueInstance instance;
std::optional<vk::UniqueDebugUtilsMessengerEXT> debugMessenger;
vk::UniqueSurfaceKHR surface;
vk::PhysicalDevice physicalDevice;
uint32_t queueFamilyIndex;
vk::UniqueDevice device;
vk::Queue queue;
vk::SurfaceCapabilitiesKHR surfaceCapabilities;
vk::Format swapchainFormat;
vk::Extent2D swapchainExtent;
vk::UniqueSwapchainKHR swapchain;
std::vector<vk::UniqueImageView> swapchainImageViews;
vk::UniqueRenderPass renderPass;
vk::UniquePipelineLayout pipelineLayout;
vk::UniquePipeline pipeline;

std::vector<char> readBytes(std::string const &filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("could not open file");
    }

    return std::vector<char>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>());
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    VkDebugUtilsMessengerCallbackDataEXT const *callbackData,
    void *userData)
{
    std::cout << callbackData->pMessage << std::endl;
    return VK_FALSE;
}

void createWindow(char const *title, uint32_t width, uint32_t height)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

void destroyWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

std::vector<char const *> getWindowExtensions()
{
    uint32_t extensionCount = 0;
    const char **requiredExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    return std::vector<char const *>(requiredExtensions, requiredExtensions + extensionCount);
}

void createInstance(
    char const *appName,
    uint32_t appVersion,
    uint32_t requiredVulkanVersion,
    std::vector<char const *> requiredExtensions)
{
    uint32_t requiredMajor = VK_VERSION_MAJOR(requiredVulkanVersion);
    uint32_t requiredMinor = VK_VERSION_MINOR(requiredVulkanVersion);

    uint32_t vulkanVersion = vk::enumerateInstanceVersion();
    uint32_t major = VK_VERSION_MAJOR(vulkanVersion);
    uint32_t minor = VK_VERSION_MINOR(vulkanVersion);

    bool hasVersion = requiredMajor > major || (requiredMajor == major && requiredMinor > minor);
    if (!hasVersion) {
        throw std::runtime_error("the required vulkan version is not supported");
    }

    std::vector<char const *> enabledLayers;
    enabledLayers.push_back("VK_LAYER_KHRONOS_validation");

    auto availableLayers = vk::enumerateInstanceLayerProperties();

    for (auto const enabledLayer : enabledLayers) {
        auto hasEnabledLayer = false;
        for (auto const availableLayer : availableLayers) {
            if (strcmp(enabledLayer, availableLayer.layerName) == 0) {
                hasEnabledLayer = true;
                break;
            }
        }
        if (!hasEnabledLayer) {
            throw std::runtime_error("could not find all required instance layers");
        }
    }

    std::vector<char const *> enabledExtensions(requiredExtensions);
    enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    auto availableExtensions = vk::enumerateInstanceExtensionProperties();
    for (auto const enabledExtension : enabledExtensions) {
        auto hasEnabledExtension = false;
        for (auto const availableExtension : availableExtensions) {
            if (strcmp(enabledExtension, availableExtension.extensionName) == 0) {
                hasEnabledExtension = true;
                break;
            }
        }
        if (!hasEnabledExtension) {
            throw std::runtime_error("could not find all required instance extensions");
        }
    }

    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
        .pfnUserCallback = &debugCallback,
    };

    vk::ApplicationInfo applicationInfo{
        .pApplicationName = appName,
        .applicationVersion = appVersion,
        .pEngineName = appName,
        .engineVersion = appVersion,
        .apiVersion = requiredVulkanVersion,
    };

    vk::InstanceCreateInfo instanceCreateInfo{
        .pNext = &debugMessengerCreateInfo,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<uint32_t>(enabledLayers.size()),
        .ppEnabledLayerNames = enabledLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
        .ppEnabledExtensionNames = enabledExtensions.data(),
    };

    instance = vk::createInstanceUnique(instanceCreateInfo);

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
#endif

    debugMessenger = instance->createDebugUtilsMessengerEXTUnique(debugMessengerCreateInfo);
}

void createSurface()
{
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("could not create window surface");
    }
    vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> _deleter(*instance);
    surface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(_surface), _deleter);
}

void choosePhysicalDevice()
{
    auto physicalDevices = instance->enumeratePhysicalDevices();
    physicalDevice = physicalDevices.front();
}

void chooseQueueFamily()
{
    auto queueFamilies = physicalDevice.getQueueFamilyProperties();

    for (size_t i = 0; i < queueFamilies.size(); i++) {
        bool supportsGraphics(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics);
        bool supportsTransfer(queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer);

        bool supportsPresentation = physicalDevice.getSurfaceSupportKHR(i, *surface);

        if (supportsGraphics && supportsPresentation) {
            queueFamilyIndex = i;
            return;
        }
    }

    throw std::runtime_error("could not find queue family that supports graphics and presentation");
}

void createDevice(std::vector<char const *> requiredExtensions)
{
    std::vector<char const *> enabledExtensions(requiredExtensions);

    auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    for (auto const enabledExtension : enabledExtensions) {
        auto hasEnabledExtension = false;
        for (auto const availableExtension : availableExtensions) {
            if (strcmp(enabledExtension, availableExtension.extensionName) == 0) {
                hasEnabledExtension = true;
                break;
            }
        }
        if (!hasEnabledExtension) {
            throw std::runtime_error("could not find all required device extensions");
        }
    }

    float queuePriority = 1.0;
    vk::DeviceQueueCreateInfo queueCreateInfo{
        .queueFamilyIndex = queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.push_back(queueCreateInfo);

    vk::DeviceCreateInfo deviceCreateInfo{
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
        .ppEnabledExtensionNames = enabledExtensions.data(),
    };

    device = physicalDevice.createDeviceUnique(deviceCreateInfo);

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
#endif
}

void createSwapchain()
{
    surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);

    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
    if (surfaceFormats.empty()) {
        throw std::runtime_error("could not find any surface formats");
    }

    vk::SurfaceFormatKHR surfaceFormat = surfaceFormats[0];
    for (auto &surfaceFormat : surfaceFormats) {
        if (surfaceFormat.format == vk::Format::eB8G8R8A8Srgb
            && surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            surfaceFormat = surfaceFormat;
        }
    }
    swapchainFormat = surfaceFormat.format;
    vk::ColorSpaceKHR swapchainColorSpace = surfaceFormat.colorSpace;

    swapchainExtent = surfaceCapabilities.currentExtent;
    if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        swapchainExtent = vk::Extent2D{
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
        };

        swapchainExtent.width = std::clamp(
            swapchainExtent.width,
            surfaceCapabilities.minImageExtent.width,
            surfaceCapabilities.maxImageExtent.width);

        swapchainExtent.height = std::clamp(
            swapchainExtent.height,
            surfaceCapabilities.minImageExtent.height,
            surfaceCapabilities.maxImageExtent.height);
    }

    auto surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
    if (surfacePresentModes.empty()) {
        throw std::runtime_error("could not find any surface present modes");
    }

    vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;
    for (auto &surfacePresentMode : surfacePresentModes) {
        if (surfacePresentMode == vk::PresentModeKHR::eMailbox) {
            swapchainPresentMode = surfacePresentMode;
        }
    }

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{
        .surface = *surface,
        .minImageCount = imageCount,
        .imageFormat = swapchainFormat,
        .imageColorSpace = swapchainColorSpace,
        .imageExtent = swapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .presentMode = swapchainPresentMode,
        .clipped = VK_TRUE,
    };

    swapchain = device->createSwapchainKHRUnique(swapchainCreateInfo);

    auto swapchainImages = device->getSwapchainImagesKHR(*swapchain);

    swapchainImageViews = std::vector<vk::UniqueImageView>(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++) {
        vk::ImageViewCreateInfo imageViewCreateInfo{
            .image = swapchainImages[i],
            .viewType = vk::ImageViewType::e2D,
            .format = swapchainFormat,
            .subresourceRange =
                vk::ImageSubresourceRange{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };

        swapchainImageViews[i] = device->createImageViewUnique(imageViewCreateInfo);
    }
}

void createRenderPass()
{
    vk::AttachmentDescription colorAttachment{
        .format = swapchainFormat,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR,
    };

    vk::AttachmentReference colorAttachmentReference{
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
    };

    vk::SubpassDescription subpass{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentReference,
    };

    vk::RenderPassCreateInfo renderPassCreateInfo{
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    renderPass = device->createRenderPassUnique(renderPassCreateInfo);
}

void createGraphicsPipeline()
{
    auto vertexShaderBytes = readBytes("../resources/shader.vert.spv");
    vk::ShaderModuleCreateInfo vertexShaderCreateInfo{
        .codeSize = vertexShaderBytes.size(),
        .pCode = reinterpret_cast<uint32_t const *>(vertexShaderBytes.data()),
    };
    auto vertexShader = device->createShaderModuleUnique(vertexShaderCreateInfo);
    vk::PipelineShaderStageCreateInfo vertexShaderStageCreateInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .pName = "main",
    };
    // The current Visual Studio C++20 compiler doesn't allow module as an identifier.
    vertexShaderStageCreateInfo.setModule(*vertexShader);

    auto fragmentShaderBytes = readBytes("../resources/shader.frag.spv");
    vk::ShaderModuleCreateInfo fragmentShaderCreateInfo{
        .codeSize = fragmentShaderBytes.size(),
        .pCode = reinterpret_cast<uint32_t const *>(fragmentShaderBytes.data()),
    };
    auto fragmentShader = device->createShaderModuleUnique(fragmentShaderCreateInfo);
    vk::PipelineShaderStageCreateInfo fragmentShaderStageCreateInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .pName = "main",
    };
    // The current Visual Studio C++20 compiler doesn't allow module as an identifier.
    fragmentShaderStageCreateInfo.setModule(*fragmentShader);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{
        vertexShaderStageCreateInfo,
        fragmentShaderStageCreateInfo,
    };

    vk::PipelineVertexInputStateCreateInfo vertexInputState{
        //.vertexBindingDescriptionCount = 0,
        //.pVertexBindingDescriptions = nullptr,
        //.vertexAttributeDescriptionCount = 0,
        //.pVertexAttributeDescriptions = nullptr,
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = VK_FALSE,
    };

    vk::Viewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(swapchainExtent.width),
        .height = static_cast<float>(swapchainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vk::Rect2D scissor{
        .offset = {0, 0},
        .extent = swapchainExtent,
    };

    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizationState{
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = VK_FALSE,
        //.depthBiasConstantFactor = 0.0f,
        //.depthBiasClamp = 0.0f,
        //.depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo multisampleState{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = VK_FALSE,
        //.minSampleShading = 1.0f,
        //.pSampleMask = nullptr,
        //.alphaToCoverageEnable = VK_FALSE,
        //.alphaToOneEnable = VK_FALSE,
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = VK_FALSE,
        //.srcColorBlendFactor = vk::BlendFactor::eOne,
        //.dstColorBlendFactor = vk::BlendFactor::eZero,
        //.colorBlendOp = vk::BlendOp::eAdd,
        //.srcAlphaBlendFactor = vk::BlendFactor::eOne,
        //.dstAlphaBlendFactor = vk::BlendFactor::eZero,
        //.alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendState{
        .logicOpEnable = VK_FALSE,
        //.logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        //.blendConstants = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f},
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        //.setLayoutCount = 0,
        //.pSetLayouts = nullptr,
        //.pushConstantRangeCount = 0,
        //.pPushConstantRanges = nullptr,
    };

    pipelineLayout = device->createPipelineLayoutUnique(pipelineLayoutCreateInfo);

    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo{
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        //.pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlendState,
        //.pDynamicState = nullptr,
        .layout = *pipelineLayout,
        .renderPass = *renderPass,
        .subpass = 0,
        //.basePipelineHandle = nullptr,
        //.basePipelineIndex = -1,
    };

    pipeline = device->createGraphicsPipelineUnique(nullptr, graphicsPipelineCreateInfo);
}

void run()
{
    createWindow(APP_NAME, WIDTH, HEIGHT);

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
        dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#endif

    std::vector<char const *> instanceExtensions;
    for (auto const windowExtension : getWindowExtensions()) {
        instanceExtensions.push_back(windowExtension);
    }

    std::vector<char const *> deviceExtensions;
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    createInstance(APP_NAME, APP_VERSION, REQUIRED_VULKAN_VERSION, instanceExtensions);
    createSurface();
    choosePhysicalDevice();
    chooseQueueFamily();
    createDevice(deviceExtensions);
    createSwapchain();
    createRenderPass();
    createGraphicsPipeline();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    destroyWindow();
}

/*
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

int main()
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
