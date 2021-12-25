#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
    // just adding a standard diagnostics layer
    const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };
    // we need certain physical device extensions
    const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    // we only want to enable these on debug builds
    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif

    void initWindow() {
        // don't forget this! :)
        glfwInit();

        // set GLFW to not create an OpenGL context and to make window non-resizable
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createSwapChainImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSemaphores();
    }

    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        // must set struct type
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        // user-specified stuff
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        // Vulkan API version
        appInfo.apiVersion = VK_API_VERSION_1_1;

        VkInstanceCreateInfo createInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // must create instance with required extensions to interface with this GLFW window
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        printf("GLFW Required Vulkan Instance Extensions:\n");
        for (int i = 0; i < glfwExtensionCount; i++) {
            printf(" - %s\n", glfwExtensions[i]);
        }
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        // checking for available Vk extensions
        uint32_t vkExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr);
        std::vector<VkExtensionProperties> vkExtensions(vkExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, vkExtensions.data());
        printf("Available Vulkan Instance Extensions:\n");
        for (const auto& extension : vkExtensions) {
            printf(" - %s\n", extension.extensionName);
        }

        // set validation layers if wanted
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = validationLayers.size();
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        VkResult res;
        if ((res = vkCreateInstance(&createInfo, nullptr, &instance)) != VK_SUCCESS) {
            printf("Failed to create VkInstance (VkResult: %d)\n", res);
            throw std::runtime_error("failed to create VkInstance!");
        }
    }

    bool checkValidationLayerSupport() {
        // using the same pattern as checking available vulkan instance extensions
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // checking to make sure all wanted validation layers are available
        for (const auto& needed : validationLayers) {
            bool foundLayer = false;
            for (const auto& available : availableLayers) {
                if (strcmp(needed, available.layerName) == 0) {
                    foundLayer = true;
                    break;
                }
            }
            if (!foundLayer) {
                return false;
            }
        }

        return true;
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("Could not find devices supporting Vulkan!");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        printf("Available Physical Devices:\n");
        // pick first suitable device
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            printf(" - %s", deviceProperties.deviceName);
            switch (deviceProperties.deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    printf(" (Other)");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    printf(" (Integrated GPU)");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    printf(" (Discrete GPU)");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    printf(" (Virtual GPU)");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    printf(" (CPU)");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:
                    printf(" (Max?)");
                    break;
            }
            if (isDeviceSuitable(device) && physicalDevice == VK_NULL_HANDLE) {
                physicalDevice = device;
                printf(" <=");
            }
            printf("\n");
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("found supported devices but none are suitable for application!");
        }
    }

    bool isDeviceSuitable(const VkPhysicalDevice& device) {
        // currently, unused since we don't need to check for any special features
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // we need to check if this device supports interfacing with this windowing system's swap chain
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        // need to check if this device can communicate with the swapchain appropriately
        bool swapChainAdequate = false;
        if (extensionsSupported) {
            // we only want to query the swap chain capabilities if the device actually supports a swap chain at all
            SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupportDetails.formats.empty()
                    && !swapChainSupportDetails.presentModes.empty();
        }

        QueueFamilyIndices deviceQueueFamilies = findQueueFamilies(device);

        return deviceQueueFamilies.isSufficient() && extensionsSupported && swapChainAdequate;
    }

    bool checkDeviceExtensionSupport(const VkPhysicalDevice& device) {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::unordered_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isSufficient() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        // checking if all needed queue families exist for this device
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            // we need to make sure a presentation queue exists for this device (ex. mining GPUs might not be able to)
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isSufficient()) {
                break;
            }
            i++;
        }

        return indices;
    }

    void createLogicalDevice() {
        // first we need to know what command queues we want
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        // this is to create both queues we want (present Queue and graphics Queue)
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::unordered_set<uint32_t> uniqueQueueFamilies = {
                indices.graphicsFamily.value(), indices.presentFamily.value()
        };

        // we don't really care about priority for now, but if you want present > compute, then you could do that
        float queuePriority[] = { 1.0 };
        for (uint32_t queueIndex : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueIndex;
            queueCreateInfo.queueCount = 1;
            // sets the priority of each of the queueCount queues to be created (we have just one per family)
            queueCreateInfo.pQueuePriorities = queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // for now, we are leaving this VK_FALSE since we don't need any special features
        VkPhysicalDeviceFeatures deviceFeatures{};
        // same pattern as instance creation
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        // make sure we have needed extensions (for now just swapchain)
        createInfo.enabledExtensionCount = deviceExtensions.size();
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        // newer vulkan impls ignore this and use instance layers but older ones might want this
        if (enableValidationLayers){
            createInfo.enabledLayerCount = validationLayers.size();
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        VkResult res = VK_SUCCESS;
        if ((res = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device)) != VK_SUCCESS) {
            printf("Failed to create VkDevice (VkResult: %d)\n", res);
            throw std::runtime_error("failed to create logical device");
        }

        // gets a handle to the actual queues where we can submit commands (we have 1 queue only so we can just use ix 0)
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void createSurface() {
        VkResult res;
        if ((res = glfwCreateWindowSurface(instance, window, nullptr, &surface)) != VK_SUCCESS) {
            printf("Failed to create Window Surface (VkResult: %d)\n", res);
            throw std::runtime_error("failed to create GLFW window surface");
        }
    }

    SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device) {
        SwapChainSupportDetails details;
        // these details include stuff like max/min numbers of images in the swap chain
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        // we need to know what kind of pixel formats this surface supports
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        // presentation modes are how images are swapped to and from the actual screen
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }
        return details;
    }

    VkSurfaceFormatKHR chooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            // just choosing this since it's simple, and we don't need anything fancy
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        // just choose this in case what we want is unavailable
        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availableFormats) {
        // https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain#page_Choosing-the-right-settings-for-the-swap-chain
        // this mode is guaranteed to be available so let's just use this
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilitiesKhr) {
        if (capabilitiesKhr.currentExtent.width != UINT32_MAX) {
            // everything is set, so we don't need to change anything
            return capabilitiesKhr.currentExtent;
        } else {
            // need to set stuff manually now
            int width, height;
            // must do it this way to get pixels and not screen coordinates (HiDPI screens!)
            glfwGetFramebufferSize(window, &width, &height);
            VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
            };
            actualExtent.width = std::clamp(actualExtent.width, capabilitiesKhr.minImageExtent.width, capabilitiesKhr.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilitiesKhr.minImageExtent.height, capabilitiesKhr.maxImageExtent.height);

            return actualExtent;
        }
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapChainSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapChainPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapChainExtent(swapChainSupport.capabilities);

        // we want one more than the minimum, so we are never waiting on the driver when rendering/presenting
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        // always 1 unless we are doing some stereoscopic VR-type stuff
        createInfo.imageArrayLayers = 1;
        // since we are rendering DIRECTLY images of the swap chain need to basically be the output of the frag shader
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        // we would use VK_IMAGE_USAGE_TRANSFER_DST_BIT instead if we wanted to do stuff like post-processing

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        // we need to specify how
        if (indices.graphicsFamily != indices.presentFamily) {
            // concurrent since we need to share between the two queues
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            // most common case so no other queue needs access to the images being rendered to the screen
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        // we don't need to rotate or flip the image so just do default
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        // we don't want to blend with other windows (just draw everything on top and make it opaque)
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        // don't render stuff on this image if it's behind another window
        createInfo.clipped = VK_TRUE;

        // necessary when recreating swapchains on stuff like window resize
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VkResult res;
        if ((res = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain)) != VK_SUCCESS) {
            printf("Failed to create swapchain (VkResult: %d)\n", res);
            throw std::runtime_error("failed to create swapchain");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        // save these for rendering
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createSwapChainImageViews() {
        // create views, so we can render to the image in our pipeline
        for (const auto& image : swapChainImages) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = image;
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            // we can change stuff to only render into the R channel, but we will stick with defaults
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            // no mip-mapping since we're rendering this to framebuffer anyway
            createInfo.subresourceRange.baseMipLevel = 0;
            // only one level and layer since this isn't a stereo app
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;
            VkResult res;
            if ((res = vkCreateImageView(device, &createInfo, nullptr, &imageView)) != VK_SUCCESS) {
                printf("failed to create an image view (VkResult: %d)\n", res);
                throw std::runtime_error("failed to create an image view");
            }
            swapChainImageViews.push_back(imageView);
        }
    }

    void createRenderPass() {
        // this is the output attachment that gets shown
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        // no MSAA so we just do one sample
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        // clears framebuffer when it's loaded for use in the render pass
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // we want to store stuff into the framebuffer
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // currently, don't care about these
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // make sure the image is in presentable form
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // now create a reference to this attachment that supports how we write to it from out frag shader
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // we need to reference a color attachment of this render pass
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        // finally, we define the render pass that has the color attachments and subpasses (one of each for now)
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }

    }

    void createGraphicsPipeline() {
        auto vertexShaderCode = readFile("shaders/shader.vert.spv");
        auto fragmentShaderCode = readFile("shaders/shader.frag.spv");

        VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
        VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
        vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        // the stage we are defining is the vertex stage
        vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageCreateInfo.module = vertexShaderModule;
        // this is the entry point of this shader (means we can use the same shader module for multiple stages with
        // different entry points)
        vertShaderStageCreateInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
        fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageCreateInfo.module = fragmentShaderModule;
        fragShaderStageCreateInfo.pName = "main";

        // an array of shader stages we can use later
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo};

        // vertex input pipeline
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        // settings these to 0 since we hardcoded the vertices
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

        // describing how geometry should be assembled. we are doing triangle list since that's common but for stuff
        // like an n-body simulation we could do points
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // we are drawing to the entire framebuffer so that's why we set it to the whole width/height
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // we want to draw to the entire framebuffer, so we use its extents (if we wanted to have some UI at the bottom
        // we could scissor those out and save efficiency
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        // things with depth outside [0.0, 1.0] are discarded rather than clamped
        rasterizer.depthClampEnable = VK_FALSE;
        // if we set this to true stuff won't rasterizer and go to the framebuffer (stopping pipeline early)
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        // thickness of lines for line mode
        rasterizer.lineWidth = 1.0f;
        // you recognize these!
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // will revisit this, but for now we won't have any anti-aliasing
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // no depth and stencil testing for now

        // we just want to overwrite any color there from a previous fragment
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        // listing stuff we can change dynamically rather than reconstructing the entire pipeline
        VkDynamicState dynamicStates[] = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_LINE_WIDTH
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        VkComputePipelineCreateInfo something{};
        something.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        // counting programmable stages we are using
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        // filling in all the other necessary data
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.layout = pipelineLayout;
        // we define this pipeline to be the first of one subpass of the entire render pass
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        // we destroy these at the end of the function since they're not what's executed, just what is used to make
        // the machine code that actually will be executed
        vkDestroyShaderModule(device, vertexShaderModule, nullptr);
        vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule shaderModule;
        VkResult res;
        if ((res = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule)) != VK_SUCCESS) {
            printf("Failed to create shader module (VkResult: %d)\n", res);
            throw std::runtime_error("Failed to create shader module");
        }
        return shaderModule;
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }
        size_t fileSize = file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    void createFramebuffers() {
        // we need one framebuffer from each image/imageview in our swapchain
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                    swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            // making sure it's compatible with the color attachment of our render pass
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            // one layer since we're not doing VR/AR fanciness
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        // creating command pool we can use to create command buffers for the graphics queue
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void createCommandBuffers() {
        // we need multiple command buffers b/c we need one for each image in the swapchain
        commandBuffers.resize(swapChainFramebuffers.size());

        // telling the command pool to create N buffers and how to do so
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        // now we must record what each command buffer does
        for (size_t i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer");
            }

            // this command buffer shall contain a render pass
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            // specify which of the framebuffers we want to actually write to
            renderPassInfo.framebuffer = swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapChainExtent;

            // clear the framebuffer before rendering to it
            VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

            vkCmdEndRenderPass(commandBuffers[i]);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

    void createSemaphores() {
        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
    }

    void drawFrame() {
        uint32_t imageIndex;
        vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(presentQueue, &presentInfo);
    }

    void cleanup() {
        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        vkDestroyCommandPool(device, commandPool, nullptr);
        for (const auto& framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        for (const auto& imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        std::cout << "Starting Application" << std::endl;
        app.run();
        std::cout << "Closed Application" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}