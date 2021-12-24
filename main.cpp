#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <unordered_set>
#include <optional>
#include <stdexcept>
#include <cstdlib>

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
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
        // currently unused since we don't need to check for any special features
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // we need to check if this device supports interfacing with this windowing system's swap chain
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        QueueFamilyIndices deviceQueueFamilies = findQueueFamilies(device);

        return deviceQueueFamilies.isSufficient() && extensionsSupported;
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

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
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