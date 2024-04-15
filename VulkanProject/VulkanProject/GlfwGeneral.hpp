#include "VKBase.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")

GLFWwindow* pWindow;

GLFWmonitor* pMonitor;

const char* windowTitle = "Vulkan Project";

using namespace vulkan;

bool InitializeWindow(VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true) {
    if (!glfwInit()) {
        std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n");
        return false;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, isResizable);
    
    pMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
    pWindow = fullScreen ?
        glfwCreateWindow(pMode->width, pMode->height, windowTitle, pMonitor, nullptr) :
        glfwCreateWindow(size.width, size.height, windowTitle, nullptr, nullptr);
    if (!pWindow) {
        std::cout << std::format("[ InitializeWindow ]\nFailed to create a glfw window!\n");
        glfwTerminate();
        return false;
    }

    //Process the physical device part
#ifdef _WIN32
    std::cout << "Using win32" << std::endl;
    graphicsBase::Base().AddInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
    graphicsBase::Base().AddInstanceExtension("VK_KHR_win32_surface");
    graphicsBase::Base().AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#else // _WIN32
    std::cout << "Using win64" << std::endl;
    uint32_t extensionCount = 0;
    const char** extensionNames;
    extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);
    if (!extensionNames) {
        std::cout << std::format("[ InitializeWindow ]\nVulkan is not available on this machine!\n");
        glfwTerminate();
        return false;
    }
    for (size_t i = 0; i < extensionCount; i++) {
        vulkan::graphicsBase::Base().AddInstanceExtension(extensionNames[i]);
    }
#endif
    //在创建window surface前创建Vulkan实例
    graphicsBase::Base().UseLatestApiVersion();
    if (graphicsBase::Base().CreateInstance()) {
        return false;
    }    

    //创建window surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (VkResult result = glfwCreateWindowSurface(graphicsBase::Base().Instance(), pWindow, nullptr, &surface)) {
        std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to create a window surface!\nError code: {}\n", int32_t(result));
        glfwTerminate();
        return false;
    }

    graphicsBase::Base().Surface(surface);
    //获取物理设备，并使用列表中的第一个物理设备，这里不考虑以下任意函数失败后更换物理设备的情况
    if (graphicsBase::Base().GetPhysicalDevices() || graphicsBase::Base().DeterminePhysicalDevice(0,true,false) || graphicsBase::Base().CreateDevice()) {
        return false;
    }

    if (graphicsBase::Base().CreateSwapchain(limitFrameRate)) {
        return false;
    }
    return true;
}

void TerminateWindow() {
    glfwTerminate();
}

void TitleFps() {
    static double time0 = glfwGetTime();
    static double time1;
    static double dt;
    static int dframe = -1;
    static std::stringstream info;
    time1 = glfwGetTime();
    dframe++;
    if ((dt = time1 - time0) >= 1) {
        info.precision(1);
        info << windowTitle << "    " << std::fixed << dframe / dt << " FPS";
        glfwSetWindowTitle(pWindow, info.str().c_str());
        info.str("");
        time0 = time1;
        dframe = 0;
    }
}

void MakeWindowFullScreen() {
    const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
    glfwSetWindowMonitor(pWindow, pMonitor, 0, 0, pMode->width, pMode->height, pMode->refreshRate);
}

void MakeWindowWindowed(VkOffset2D position, VkExtent2D size) {
    const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
    glfwSetWindowMonitor(pWindow, nullptr, position.x, position.y, size.width, size.height, pMode->refreshRate);
}