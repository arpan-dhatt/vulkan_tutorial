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

    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
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
        createGraphicsPipeline();
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
        // create views so we can render to the image in our pipeline
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

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
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