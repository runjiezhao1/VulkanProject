#pragma once
#include "EasyVKStart.h"

//Initialization process
/*
* Create Vulkan instance
* Create debug messenger
* Create window surface
* Choose physical device(GPU) and logic device and obtain the VK queue which provides an interface to the execution engines of a device
* Create swapchain(交换链)
*/

namespace vulkan {
    class graphicsBase {
        static graphicsBase singleton;

        graphicsBase() = default;
        graphicsBase(graphicsBase&&) = delete;
        ~graphicsBase() {
        }
    private:
        //应用程序必须显式地告诉操作系统和显卡驱动，说明其需要使用Vulkan的功能
        VkInstance instance;
        //Debug messenger用来输出验证层所捕捉到的debug信息。若没有这东西，Vulkan编程可谓寸步难行。
        VkDebugUtilsMessengerEXT debugUtilsMessenger;
        //Vulkan是平台无关的API，你必须向其提供一个window surface（VkSurfaceKHR），以和平台特定的窗口对接。
        VkSurfaceKHR surface;
        std::vector<const char*> instanceLayers;
        std::vector<const char*> instanceExtensions;
        //创建物理设备（非严格意义上的物理）
        VkPhysicalDevice physicalDevice;
        VkPhysicalDeviceProperties physicalDeviceProperties;
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        std::vector<VkPhysicalDevice> availablePhysicalDevices;

        
        //创建逻辑设备的步骤依序为：
        //1.获取物理设备列表
        //2.检查物理设备是否满足所需的队列族类型，从中选择能满足要求的设备并顺便取得队列族索引
        //3.确定所需的设备级别扩展，不检查是否可用
        //4.用vkCreateDevice(...)创建逻辑设备，取得队列
        //5.取得物理设备属性、物理设备内存属性，以备之后使用
        VkDevice device;
        //VK_QUEUE_FAMILY_IGNORED = 0表示索引值
        //根据情况，一共需要三种类型的队列：图形、呈现、计算
        //如果你的程序没有图形界面（比如，仅仅用于对图像做某种处理的控制台程序），那么呈现队列非必须。
        //如果你不需要GPU计算或间接渲染（将CPU上的一些计算扔到GPU上然后再从计算结果做渲染），那么计算队列非必须。
        //如果你只打算搞计算（GPU是高度并行的计算设备）而不搞渲染，那么图形队列非必须。
        uint32_t queueFamilyIndex_graphics = VK_QUEUE_FAMILY_IGNORED;
        uint32_t queueFamilyIndex_presentation = VK_QUEUE_FAMILY_IGNORED;
        uint32_t queueFamilyIndex_compute = VK_QUEUE_FAMILY_IGNORED;
        VkQueue queue_graphics;
        VkQueue queue_presentation;
        VkQueue queue_compute;

        std::vector<const char*> deviceExtensions;

        static void AddLayerOrExtension(std::vector<const char*>& container, const char* name) {
            for (auto& i : container)
                if (!strcmp(name, i))
                    return; 
            container.push_back(name);
        }

        VkResult CreateDebugMessenger() {
            /*待Ch1-3填充*/
        }

        VkResult GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t(&queueFamilyIndices)[3]) {
            /*待Ch1-3填充*/
        }

        //创建交换链的步骤依序为：
        //1.填写一大堆信息
        //2.创建交换链并取得交换链图像，为交换链图像创建image view
        std::vector <VkSurfaceFormatKHR> availableSurfaceFormats;
        VkSwapchainKHR swapchain;
        std::vector <VkImage> swapchainImages;
        std::vector <VkImageView> swapchainImageViews;
        //保存交换链的创建信息以便重建交换链
        VkSwapchainCreateInfoKHR swapchainCreateInfo = {};

        VkResult CreateSwapchain_Internal() {
            /*waiting for being filled*/
        }

        //use latest Vulkan Version
        uint32_t apiVersion = VK_API_VERSION_1_0;

    public:
        //Getter
        // Use latest vulkan version part
        uint32_t ApiVersion() const {
            return apiVersion;
        }
        VkResult UseLatestApiVersion() {
            if (vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion")) {
                return vkEnumerateInstanceVersion(&apiVersion);
            }
            return VK_SUCCESS;
        }
        // End of using latest vulkan version part
        
        //Swap Chain Part
        const VkFormat& AvailableSurfaceFormat(uint32_t index) const {
            return availableSurfaceFormats[index].format;
        }

        const VkColorSpaceKHR& AvailableSurfaceColorSpace(uint32_t index) const {
            return availableSurfaceFormats[index].colorSpace;
        }

        uint32_t AvailableSurfaceFormatCount() const {
            return uint32_t(availableSurfaceFormats.size());
        }

        VkSwapchainKHR Swapchain() const {
            return swapchain;
        }
        
        VkImage SwapchainImage(uint32_t index) const {
            return swapchainImages[index];
        }

        VkImageView SwapchainImageView(uint32_t index) const {
            return swapchainImageViews[index];
        }

        uint32_t SwapchainImageCount() const {
            return uint32_t(swapchainImages.size());
        }

        const VkSwapchainCreateInfoKHR& SwapchainCreateInfo() const {
            return swapchainCreateInfo;
        }

        VkResult GetSurfaceFormats() {
            /*waiting for being filled*/
        }

        VkResult SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat) {
            /*waiting for being filled*/
        }

        //该函数用于创建交换链
        VkResult CreateSwapchain(bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0) {
            /*waiting for being filled*/
        }
        //在调整窗口大小的时候 可以用recreateswapchain来重建交换链
        VkResult RecreateSwapChain() {
            /*waiting for being filled*/
        }
        //End of Swap Chain

        //Physical Device Part
        VkPhysicalDevice PhysicalDevice() const{
            return physicalDevice;
        }

        const VkPhysicalDeviceProperties& PhysicalDeviceProperties() const{
            return physicalDeviceProperties;
        }

        const VkPhysicalDeviceMemoryProperties& PhysicalDeviceMemoryProperties() const {
            return physicalDeviceMemoryProperties;
        }

        VkPhysicalDevice AvailablePhysicalDevice(uint32_t index) const{
            return availablePhysicalDevices[index];
        }

        uint32_t AvailablePhysicalDeviceCount() const {
            return uint32_t(availablePhysicalDevices.size());
        }

        VkDevice Device() const {
            return device;
        }

        uint32_t QueueFamilyIndex_graphics() const {
            return queueFamilyIndex_graphics;
        }

        uint32_t QueueFamilyIndex_presentation() const {
            return queueFamilyIndex_presentation;
        }

        uint32_t QueueFamilyIndex_compute() const {
            return queueFamilyIndex_compute;
        }

        VkQueue Queue_graphics() const {
            return queue_graphics;
        }

        VkQueue Queue_presentation() const {
            return queue_presentation;
        }

        VkQueue Queue_compute() const {
            return queue_compute;
        }

        const std::vector<const char*>& DeviceExtensions() const {
            return deviceExtensions;
        }

        //该函数用于创建逻辑设备前
        void AddDeviceExtension(const char* extensionName) {
            AddLayerOrExtension(deviceExtensions, extensionName);
        }
        //该函数用于获取物理设备
        VkResult GetPhysicalDevices() {
            /*waiting for being filled*/
        }
        //该函数用于指定所用物理设备并调用GetQueueFamilyIndices(...)取得队列族索引
        VkResult DeterminePhysicalDevice(bool enableGraphicsQueue, uint32_t deviceIndex = 0, bool enableComputeQueue = true) {
            /*waiting for being filled*/
        }
        //该函数用于创建逻辑设备，并取得队列
        VkResult CreateDevice(VkDeviceCreateFlags flags = 0) {
            /*waiting for being filled*/
        }
        //以下函数用于创建逻辑设备失败后
        VkResult CheckDeviceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const {
            /*waiting for being filled*/
        }

        void DeviceExtensions(const std::vector<const char*>& extensionNames) {
            deviceExtensions = extensionNames;
        }
        // End of physical device part

        VkSurfaceKHR Surface() const {
            return surface;
        }

        VkInstance Instance() const {
            return instance;
        }
        const std::vector<const char*>& InstanceLayers() const {
            return instanceLayers;
        }
        const std::vector<const char*>& InstanceExtensions() const {
            return instanceExtensions;
        }

        void AddInstanceLayer(const char* layerName) {
            AddLayerOrExtension(instanceLayers, layerName);
        }
        void AddInstanceExtension(const char* extensionName) {
            AddLayerOrExtension(instanceExtensions, extensionName);
        }

        void Surface(VkSurfaceKHR surface) {
            if (!this->surface)
                this->surface = surface;
        }

        VkResult CreateInstance(VkInstanceCreateFlags flags = 0) {
#ifndef NDEBUG
            AddInstanceLayer("VK_LAYER_KHRONOS_validation");
            AddInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // !NDEBUG
            VkApplicationInfo applicationInfo = {
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .apiVersion = apiVersion
            };
            VkInstanceCreateInfo instanceCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .flags = flags,
                .pApplicationInfo = &applicationInfo,
                .enabledLayerCount = uint32_t(instanceLayers.size()),
                .ppEnabledLayerNames = instanceLayers.data(),
                .enabledExtensionCount = uint32_t(instanceExtensions.size()),
                .ppEnabledExtensionNames = instanceExtensions.data()
            };
            if (VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance)) {
                //create failed
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a vulkan instance!\nError code: {}\n", int32_t(result));
                return result;
            }
            std::cout << std::format("Vulkan API Version: {}.{}.{}\n", VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion));
#ifndef NDEBUG
            CreateDebugMessenger();
#endif // !NDEBUG
            return VK_SUCCESS;
        }

        VkResult CheckInstanceLayers(std::span<const char*> layersToCheck) {
            /*待Ch1-3填充*/
        }
        void InstanceLayers(const std::vector<const char*>& layerNames) {
            instanceLayers = layerNames;
        }
        VkResult CheckInstanceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const {
            /*待Ch1-3填充*/
        }
        void InstanceExtensions(const std::vector<const char*>& extensionNames) {
            instanceExtensions = extensionNames;
        }
    public:
        static graphicsBase& Base() {
            return singleton;
        }
    };
    inline graphicsBase graphicsBase::singleton;
}