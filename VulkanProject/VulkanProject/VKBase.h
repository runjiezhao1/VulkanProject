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
#define VK_RESULT_THROW
#define DestroyHandleBy(Func) if(handle){ Func(graphicsBase::Base().Device(), handle, nullptr); handle = VK_NULL_HANDLE;}
#define MoveHandle handle = other.handle; other.handle = VK_NULL_HANDLE;
#define DefineHandleTypeOperator operator decltype(handle)() const { return handle; }
#define DefineAddressFunction const decltype(handle)* Address() const { return &handle; }

#ifndef NDEBUG
#define ENABLE_DEBUG_MESSENGER true
#else
#define ENABLE_DEBUG_MESSENGER false
#endif

namespace vulkan {
    constexpr VkExtent2D defaultWindowSize = {1280,720};
    inline auto& outStream = std::cout;
    //情况1：根据函数返回值确定是否抛异常
#ifdef VK_RESULT_THROW
    class result_t{
        VkResult result;
    public:
        static void(*callback_throw)(VkResult);
        result_t(VkResult result) : result(result) {}
        result_t(result_t&& other) noexcept :result(other.result) { other.result = VK_SUCCESS; }
        ~result_t() noexcept(false) {
            if (uint32_t(result) < VK_RESULT_MAX_ENUM)
                return;
            if (callback_throw)
                callback_throw(result);
            throw result;
        }
        operator VkResult() {
            VkResult result = this->result;
            this->result = VK_SUCCESS;
            return result;
        }
    };
    inline void(*result_t::callback_throw)(VkResult);

    //情况2：若抛弃函数返回值，让编译器发出警告
#elif defined VK_RESULT_NODISCARD
    struct [[nodiscard]] result_t {
        VkResult result;
        result_t(VkResult result) :result(result) {}
        operator VkResult() const { return result; }
    };
#else
    using result_t = VkResult;
#endif

    class graphicsBase {
        static graphicsBase singleton;

        graphicsBase() = default;
        graphicsBase(graphicsBase&&) = delete;
        ~graphicsBase() {
            if (!instance) {
                return;
            }
            if (device) {
                WaitIdle();
                if (swapchain) {
                    for (auto& i : callbacks_destroySwapchain) {
                        i();
                    }
                    for (auto& i : swapchainImageViews) {
                        if (i) {
                            vkDestroyImageView(device, i, nullptr);
                        }
                    }
                    vkDestroySwapchainKHR(device, swapchain, nullptr);
                }
                for (auto& i : callbacks_destroyDevice) {
                    i();
                }
                vkDestroyDevice(device, nullptr);
            }
            if (surface) {
                vkDestroySurfaceKHR(instance, surface, nullptr);
            }
            if (debugUtilsMessenger) {
                PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
                if (DestroyDebugUtilsMessenger) {
                   DestroyDebugUtilsMessenger(instance, debugUtilsMessenger, nullptr); 
                }
            }
            vkDestroyInstance(instance, nullptr);
        }
    private:
        //当前取得的交换链图像索引
        uint32_t currentImageIndex = 0;
        std::vector<void(*)()> callbacks_createDevice;
        std::vector<void(*)()> callbacks_destroyDevice;
        std::vector<void(*)()> callbacks_createSwapchain;
        std::vector<void(*)()> callbacks_destroySwapchain;
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

        result_t CreateDebugMessenger() {
            static PFN_vkDebugUtilsMessengerCallbackEXT DebugUtilsMessengerCallback = [] ( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)->VkBool32 {
                    std::cout << std::format("{}\n\n", pCallbackData->pMessage);
                    return VK_FALSE;
                };
            VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = DebugUtilsMessengerCallback,

            };
            PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
            if (vkCreateDebugUtilsMessenger) {
                VkResult result = vkCreateDebugUtilsMessenger(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugUtilsMessenger);
                if (result) {
                    std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a debug messenger!\nError code: {}\n", int32_t(result));
                }
                return result;
            }
            std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the function pointer of vkCreateDebugUtilsMessengerEXT!\n");
            return VK_RESULT_MAX_ENUM;
        }
        //该函数被DeterminePhysicalDevice(...)调用，用于检查物理设备是否满足所需的队列族类型，并将对应的队列族索引返回到queueFamilyIndices，执行成功时直接将索引写入相应成员变量
        result_t GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t(&queueFamilyIndices)[3]) {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
            if (!queueFamilyCount) {
                return VK_RESULT_MAX_ENUM;
            }
            std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());
            auto& [ig, ip, ic] = queueFamilyIndices;
            ig = ip = ic = VK_QUEUE_FAMILY_IGNORED;
            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                VkBool32 supportGraphics = enableGraphicsQueue && queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
                VkBool32 supportPresentation = false;
                VkBool32 supportCompute = enableComputeQueue && queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
                if (surface) {
                    if (VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresentation)) {
                        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to determine if the queue family supports presentation!\nError code: {}\n", int32_t(result));
                        return result;
                    }
                }
                if (supportGraphics && supportCompute) {
                    if (supportPresentation) {
                        ig = ip = ic = i;
                        break;
                    }
                    if (ig != ic || ig == VK_QUEUE_FAMILY_IGNORED) {
                        ig = ic = 1;
                    }
                    if (!surface) {
                        break;
                    }
                }
                if (supportGraphics && ig == VK_QUEUE_FAMILY_IGNORED) {
                    ig = 1;
                }
                if (supportPresentation && ip == VK_QUEUE_FAMILY_IGNORED) {
                    ip = 1;
                }
                if (supportCompute && ic == VK_QUEUE_FAMILY_IGNORED) {
                    ic = 1;
                }
            }
            //若任何需要被取得的队列族索引尚未被取得，则函数执行失败
            if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue || 
                ip == VK_QUEUE_FAMILY_IGNORED && surface ||
                ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue) {
                return VK_RESULT_MAX_ENUM;
            }

            queueFamilyIndex_compute = ig;
            queueFamilyIndex_graphics = ic;
            queueFamilyIndex_presentation = ip;
            return VK_SUCCESS;
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

        result_t CreateSwapchain_Internal() {
            if (VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain)) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a swapchain!\nError code: {}\n", int32_t(result));
                return result;
            }
            
            //获取交换连图像
            uint32_t swapchainImageCount;
            if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr)) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of swapchain images!\nError code: {}\n", int32_t(result));
                return result;
            }
            swapchainImages.resize(swapchainImageCount);
            if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data())) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get swapchain images!\nError code: {}\n", int32_t(result));
                return result;
            }

            //创建image view
            swapchainImageViews.resize(swapchainImageCount);
            VkImageViewCreateInfo imageViewCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = swapchainCreateInfo.imageFormat,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
            };
            for (size_t i = 0; i < swapchainImageCount; i++) {
                imageViewCreateInfo.image = swapchainImages[i];
                if (VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i])) {
                    std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a swapchain image view!\nError code: {}\n", int32_t(result));
                    return result;
                }
            }
            return VK_SUCCESS;
        }

        //use latest Vulkan Version
        uint32_t apiVersion = VK_API_VERSION_1_0;

    public:
        uint32_t CurrentImageIndex() const { return currentImageIndex; };

        //该函数用于获取交换链图像索引到currentImageIndex，以及在需要重建交换链时调用RecreateSwapchain()、重建交换链后销毁旧交换链
        result_t SwapImage(VkSemaphore semaphore_imageIsAvailable) {
            //销毁旧交换链（若存在）
            if (swapchainCreateInfo.oldSwapchain && swapchainCreateInfo.oldSwapchain != swapchain) {
                vkDestroySwapchainKHR(device, swapchainCreateInfo.oldSwapchain, nullptr);
                swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
            }
            //获取交换链图像索引
            while (VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore_imageIsAvailable, VK_NULL_HANDLE, &currentImageIndex)) {
                switch (result)
                {
                case VK_SUBOPTIMAL_KHR:
                case VK_ERROR_OUT_OF_DATE_KHR:
                    if (VkResult result = RecreateSwapChain()) {
                        return result;
                    }
                    break;
                default:
                    outStream << std::format("[ graphicsBase ] ERROR\nFailed to acquire the next image!\nError code: {}\n", int32_t(result));
                    return result;
                }
            }
            return VK_SUCCESS;
        }

        result_t SubmitCommandBuffer_Graphics(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) const {
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkResult result = vkQueueSubmit(queue_graphics, 1, &submitInfo, fence);
            if (result)
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to submit the command buffer!\nError code: {}\n", int32_t(result));
            return result;
        }

        result_t SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer, VkSemaphore semaphore_imageIsAvailable = VK_NULL_HANDLE, VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE,
            VkPipelineStageFlags waitDstStage_imageIsAvailable = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) const {
            VkSubmitInfo submitInfo = {
                .commandBufferCount = 1,
                .pCommandBuffers = &commandBuffer
            };
            if (semaphore_imageIsAvailable) {
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = &semaphore_imageIsAvailable;
                submitInfo.pWaitDstStageMask = &waitDstStage_imageIsAvailable;
            }
            if (semaphore_renderingIsOver) {
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = &semaphore_renderingIsOver;
            }
            return SubmitCommandBuffer_Graphics(submitInfo, fence);
        }

        void Terminate() {
            this->~graphicsBase();
            instance = VK_NULL_HANDLE;
            physicalDevice = VK_NULL_HANDLE;
            device = VK_NULL_HANDLE;
            surface = VK_NULL_HANDLE;
            swapchain = VK_NULL_HANDLE;
            swapchainImages.resize(0);
            swapchainImageViews.resize(0);
            swapchainCreateInfo = {};
            debugUtilsMessenger = VK_NULL_HANDLE;
        }

        void AddCallback_CreateSwapchain(void(*function)()) {
            callbacks_createSwapchain.push_back(function);
        }
        void AddCallback_DestroySwapchain(void(*function)()) {
            callbacks_destroySwapchain.push_back(function);
        }
        void AddCallback_CreateDevice(void(*function)()) {
            callbacks_createDevice.push_back(function);
        }
        void AddCallback_DestroyDevice(void(*function)()) {
            callbacks_destroyDevice.push_back(function);
        }
        //该函数用于等待逻辑设备空闲
        result_t WaitIdle() const {
            VkResult result = vkDeviceWaitIdle(device);
            if (result)
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to wait for the device to be idle!\nError code: {}\n", int32_t(result));
            return result;
        }
        //该函数用于重建逻辑设备
        result_t RecreateDevice(VkDeviceCreateFlags flags = 0) {
            if (VkResult result = WaitIdle())
                return result;
            if (swapchain) {
                //调用销毁交换链时的回调函数
                for (auto& i : callbacks_destroySwapchain) {
                    i();
                }
                //销毁交换链图像的image view
                for (auto& i : swapchainImageViews) {
                    if (i) {
                        vkDestroyImageView(device, i, nullptr);
                    }   
                }
                swapchainImageViews.resize(0);
                //销毁交换链
                vkDestroySwapchainKHR(device, swapchain, nullptr);
                //重置交换链handle
                swapchain = VK_NULL_HANDLE;
                //重置交换链创建信息
                swapchainCreateInfo = {};
            }
            for (auto& i : callbacks_destroyDevice) {
                i();
            }
            if (device) {
                vkDestroyDevice(device, nullptr);
                device = VK_NULL_HANDLE;
            }
            return CreateDevice(flags);
        }
        //Getter
        // Use latest vulkan version part
        uint32_t ApiVersion() const {
            return apiVersion;
        }
        result_t UseLatestApiVersion() {
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

        result_t GetSurfaceFormats() {
            uint32_t surfaceFormatCount;
            if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr)) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of surface formats!\nError code: {}\n", int32_t(result));
                return result;
            }
            if (!surfaceFormatCount) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to find any supported surface format!\n"), abort();
            }
            availableSurfaceFormats.resize(surfaceFormatCount);
            VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, availableSurfaceFormats.data());
            if (result) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get surface formats!\nError code: {}\n", int32_t(result));
            }
            return result;
        }

        result_t SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat) {
            bool formatIsAvailable = false;
            //如果格式未指定，只匹配色彩空间，图像格式有啥就用啥
            if (!surfaceFormat.format) {
                for (auto& i : availableSurfaceFormats) {
                    if (i.colorSpace == surfaceFormat.colorSpace) {
                        swapchainCreateInfo.imageFormat = i.format;
                        swapchainCreateInfo.imageColorSpace = i.colorSpace;
                        formatIsAvailable = true;
                        break;
                    }
                }
            }
            //否则匹配格式和色彩空间
            else {
                for (auto& i : availableSurfaceFormats) {
                    if (i.format == surfaceFormat.format && i.colorSpace == surfaceFormat.colorSpace) {
                        swapchainCreateInfo.imageFormat = i.format;
                        swapchainCreateInfo.imageColorSpace = i.colorSpace;
                        formatIsAvailable = true;
                        break;
                    }
                }
            }
            //如果没有符合的格式，恰好有个语义相符的错误代码
            if (!formatIsAvailable) {
                return VK_ERROR_FORMAT_NOT_SUPPORTED;
            }
            //如果交换链已存在，调用RecreateSwapchain()重建交换链
            if (swapchain) {
                return RecreateSwapChain();
            }
            return VK_SUCCESS;
        }

        //该函数用于创建交换链
        result_t CreateSwapchain(bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0) {
            //VkSurfaceCapabilitiesKHR相关的参数
            VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
            if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities)) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
                return result;
            }
            swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.maxImageCount > surfaceCapabilities.minImageCount);
            swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent.width == -1 ? 
                VkExtent2D{
                    glm::clamp(defaultWindowSize.width,surfaceCapabilities.minImageExtent.width,surfaceCapabilities.maxImageExtent.width),
                    glm::clamp(defaultWindowSize.height,surfaceCapabilities.minImageExtent.height,surfaceCapabilities.maxImageExtent.height),
                }
            : surfaceCapabilities.currentExtent;
            //swapchainCreateInfo.imageArrayLayers = 1;
            swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
            if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
                swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
            }
            else {
                for (size_t i = 0; i < 4; i++) {
                    if (surfaceCapabilities.supportedCompositeAlpha & 1 << i) {
                        swapchainCreateInfo.compositeAlpha = VkCompositeAlphaFlagBitsKHR(surfaceCapabilities.supportedCompositeAlpha & 1 << i);
                        break;
                    }
                }
            }

            //指定图像用途
            swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
                swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            }
            if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
                swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            }
            else {
                std::cout << std::format("[ graphicsBase ] WARNING\nVK_IMAGE_USAGE_TRANSFER_DST_BIT isn't supported!\n");
            }

            //指定图像格式
            if (availableSurfaceFormats.empty()) {
                if (VkResult result = GetSurfaceFormats()) {
                    //Failed to get surface formats
                    return result;
                }
            }
            if (!swapchainCreateInfo.imageFormat) {
                if (SetSurfaceFormat({ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }) &&
                    SetSurfaceFormat({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })) {
                    //如果找不到上述图像格式和色彩空间的组合，那只能有什么用什么，采用availableSurfaceFormats中的第一组
                    swapchainCreateInfo.imageFormat = availableSurfaceFormats[0].format;
                    swapchainCreateInfo.imageColorSpace = availableSurfaceFormats[0].colorSpace;
                    std::cout << std::format("[ graphicsBase ] WARNING\nFailed to select a four-component UNORM surface format!\n");
                }
            }
            //指定呈现模式
            uint32_t surfacePresentModeCount;
            if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, nullptr)) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of surface present modes!\nError code: {}\n", int32_t(result));
                return result;
            }
            if (!surfacePresentModeCount) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to find any surface present mode!\n"), abort();
            }
            std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
            if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, surfacePresentModes.data())) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get surface present modes!\nError code: {}\n", int32_t(result));
                return result;
            }
            swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
            if (!limitFrameRate) {
                for (size_t i = 0; i < surfacePresentModeCount; i++) {
                    if (surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                        swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                        break;
                    }
                }
            }
            //fillin the rest parameters
            swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainCreateInfo.flags = flags;
            swapchainCreateInfo.surface = surface;
            swapchainCreateInfo.imageArrayLayers = 1;
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.clipped = VK_TRUE;

            if (VkResult result = CreateSwapchain_Internal()) {
                return result;
            }
            
            for (auto& i : callbacks_createSwapchain) {
                i();
            }
            return VK_SUCCESS;
        }

        //在调整窗口大小的时候 可以用recreateswapchain来重建交换链
        result_t RecreateSwapChain() {
            VkSurfaceCapabilitiesKHR surfaceCapbilities = {};
            if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapbilities)) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
                return result;
            }
            if (surfaceCapbilities.currentExtent.width == 0 || surfaceCapbilities.currentExtent.height == 0) {
                return VK_SUCCESS;
            }
            swapchainCreateInfo.imageExtent = surfaceCapbilities.currentExtent;
            swapchainCreateInfo.oldSwapchain = swapchain;
            VkResult result = vkQueueWaitIdle(queue_graphics);
            if (result == VK_SUCCESS && queue_graphics != queue_presentation) {
                result = vkQueueWaitIdle(queue_presentation);
            }
            if (result) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to wait for the queue to be idle!\nError code: {}\n", int32_t(result));
                return result;
            }
            //destroy all old information related to old swapchain
            for (auto& i : callbacks_destroySwapchain) {
                i();
            }
            for (auto& i : swapchainImageViews) {
                if (i) {
                    vkDestroyImageView(device, i, nullptr);
                }
            }
            swapchainImageViews.resize(0);
            //create new swapchain and related information
            if (result = CreateSwapchain_Internal()) {
                return result;
            }
            for (auto& i : callbacks_createSwapchain) {
                i();
            }
            return VK_SUCCESS;
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
        result_t GetPhysicalDevices() {
            uint32_t deviceCount;
            if (VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr)) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of physical devices!\nError code: {}\n", int32_t(result));
                return result;
            }
            if (!deviceCount) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to find any physical device supports vulkan!\n"), abort();
            }
            availablePhysicalDevices.resize(deviceCount);
            VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data());
            if (result) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to enumerate physical devices!\nError code: {}\n",uint32_t(result));
            }
            return result;
        }
        //该函数用于指定所用物理设备并调用GetQueueFamilyIndices(...)取得队列族索引
        result_t DeterminePhysicalDevice(uint32_t deviceIndex = 0, bool enableGraphicsQueue = true, bool enableComputeQueue = true) {
            //定义一个特殊值用于标记一个队列族索引已被找过但未找到
            static constexpr uint32_t notFound = INT32_MAX;
            struct queueFamilyIndexCombination {
                uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
                uint32_t presentation = VK_QUEUE_FAMILY_IGNORED;
                uint32_t compute = VK_QUEUE_FAMILY_IGNORED;
            };
            //queueFamilyIndexCombinations用于为各个物理设备保存一份队列族索引组合
            static std::vector<queueFamilyIndexCombination> queueFamilyIndexCombinations(availablePhysicalDevices.size());
            auto& [ig, ip, ic] = queueFamilyIndexCombinations[deviceIndex];
            //如果有任何队列族索引已被找过但未找到，返回VK_RESULT_MAX_ENUM
            if (ig == notFound && enableGraphicsQueue || ip == notFound && surface || ic == notFound && enableComputeQueue) {
                return VK_RESULT_MAX_ENUM;
            }
            //如果有任何队列族索引应被获取但还未被找过
            if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue || 
                ip == VK_QUEUE_FAMILY_IGNORED && surface ||
                ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue) {
                uint32_t indices[3];
                VkResult result = GetQueueFamilyIndices(availablePhysicalDevices[deviceIndex],enableGraphicsQueue,enableComputeQueue,indices);
                //若GetQueueFamilyIndices(...)返回VK_SUCCESS或VK_RESULT_MAX_ENUM（vkGetPhysicalDeviceSurfaceSupportKHR(...)执行成功但没找齐所需队列族），
                //说明对所需队列族索引已有结论，保存结果到queueFamilyIndexCombinations[deviceIndex]中相应变量
                //应被获取的索引若仍为VK_QUEUE_FAMILY_IGNORED，说明未找到相应队列族，VK_QUEUE_FAMILY_IGNORED（~0u）与INT32_MAX做位与得到的数值等于notFound
                if (result == VK_SUCCESS || result == VK_RESULT_MAX_ENUM){
                    if (enableGraphicsQueue) {
                        ig = indices[0] & INT32_MAX;
                    }
                    if (surface) {
                        ip = indices[1] & INT32_MAX;
                    }
                    if (enableComputeQueue) {
                        ic = indices[2] & INT32_MAX;
                    }
                }
                if (result) {
                    return result;
                }
            }
            //若以上两个if分支皆不执行，则说明所需的队列族索引皆已被获取，从queueFamilyIndexCombinations[deviceIndex]中取得索引
            else {
                queueFamilyIndex_graphics = enableGraphicsQueue ? ig : VK_QUEUE_FAMILY_IGNORED;
                queueFamilyIndex_presentation = surface ? ip : VK_QUEUE_FAMILY_IGNORED;
                queueFamilyIndex_compute = enableComputeQueue ? ic : VK_QUEUE_FAMILY_IGNORED;
            }
            physicalDevice = availablePhysicalDevices[deviceIndex];
            return VK_SUCCESS;
        }
        //该函数用于创建逻辑设备，并取得队列
        result_t CreateDevice(VkDeviceCreateFlags flags = 0) {
            float queuePriority = 1.f;
            VkDeviceQueueCreateInfo queueCreateInfos[3] = {
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueCount = 1,
                    .pQueuePriorities = &queuePriority,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueCount = 1,
                    .pQueuePriorities = &queuePriority,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueCount = 1,
                    .pQueuePriorities = &queuePriority,
                }
            };
            uint32_t queueCreateInfoCount = 0;
            if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED) {
                queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_graphics;
            }
            if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED &&
                queueFamilyIndex_presentation != queueFamilyIndex_graphics) {
                queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_presentation;
            }
            if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED &&
                queueFamilyIndex_compute != queueFamilyIndex_graphics &&
                queueFamilyIndex_compute != queueFamilyIndex_presentation) {
                queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_compute;
            }
            VkPhysicalDeviceFeatures physicalDeviceFeatures;
            vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
            //AddDeviceExtension("VK_KHR_swapchain");
            VkDeviceCreateInfo deviceCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .flags = flags,
                .queueCreateInfoCount = queueCreateInfoCount,
                .pQueueCreateInfos = queueCreateInfos,
                .enabledExtensionCount = uint32_t(deviceExtensions.size()),
                .ppEnabledExtensionNames = deviceExtensions.data(),
                .pEnabledFeatures = &physicalDeviceFeatures,
            };
            
            if (VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device)) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a vulkan logical device!\nError code: {}\n", int32_t(result));
                return result;
            }
            if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED) {
                vkGetDeviceQueue(device, queueFamilyIndex_graphics, 0, &queue_graphics);
            }
            if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED) {
                vkGetDeviceQueue(device, queueFamilyIndex_presentation, 0, &queue_presentation);
            }
            if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED) {
                vkGetDeviceQueue(device, queueFamilyIndex_compute, 0, &queue_compute);
            }
            vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
            //输出所用的物理设备名称
            std::cout << std::format("Renderer: {}\n", physicalDeviceProperties.deviceName);
            for (auto& i : callbacks_createDevice) {
                i();
            }
            return VK_SUCCESS;
        }
        //以下函数用于创建逻辑设备失败后
        result_t CheckDeviceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const {
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

        result_t CreateInstance(VkInstanceCreateFlags flags = 0) {
            if constexpr (ENABLE_DEBUG_MESSENGER)
                AddInstanceLayer("VK_LAYER_KHRONOS_validation");
                AddInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

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

            if constexpr (ENABLE_DEBUG_MESSENGER)
                CreateDebugMessenger();

            return VK_SUCCESS;
        }

        result_t CheckInstanceLayers(std::span<const char*> layersToCheck) {
            uint32_t layerCount;
            std::vector<VkLayerProperties> availableLayers;
            if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount,nullptr)) {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of instance layers!\n");
                return result;
            }
            if (layerCount) {
                availableLayers.resize(layerCount);
                if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data())) {
                    std::cout << std::format("[ graphicsBase ] ERROR\nFailed to enumerate instance layer properties!\nError code: {}\n", int32_t(result));
                    return result;
                }
                for (auto& i : layersToCheck) {
                    bool found = false;
                    for (auto& j : availableLayers) {
                        if (!strcmp(i, j.layerName)) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        i = nullptr;
                    }
                }
            }
            else {
                for (auto& i : layersToCheck) {
                    i = nullptr;
                }
            }
            return VK_SUCCESS;
        }
        void InstanceLayers(const std::vector<const char*>& layerNames) {
            instanceLayers = layerNames;
        }
        result_t CheckInstanceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const {
            uint32_t extensionCount;
            std::vector<VkExtensionProperties> availableExtensions;
            if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr)) {
                layerName ? 
                    std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\nLayer name:{}\n", layerName) :
                    std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\n");
                return result;
            }
            if (extensionCount) {
                availableExtensions.resize(extensionCount);
                if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, availableExtensions.data())) {
                    std::cout << std::format("[ graphicsBase ] ERROR\nFailed to enumerate instance extension properties!\nError code: {}\n", int32_t(result));
                    return result;
                }
                for (auto& i : extensionsToCheck) {
                    bool found = false;
                    for (auto &j : availableExtensions) {
                        if (!strcmp(i,j.extensionName)) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        i = nullptr;
                    }
                }
            }
            else {
                for (auto& i : extensionsToCheck) {
                    i = nullptr;
                }
            }
            return VK_SUCCESS;
        }
        void InstanceExtensions(const std::vector<const char*>& extensionNames) {
            instanceExtensions = extensionNames;
        }
    public:
        static graphicsBase& Base() {
            return singleton;
        }
    };

    class fence {
        VkFence handle = VK_NULL_HANDLE;
    public:
        fence(VkFenceCreateInfo& createInfo) {
            //Create(createInfo);
            Create(createInfo);
        }
        fence(VkFenceCreateFlags flags = 0) {
            Create(flags);
        }
        fence(fence&& other) noexcept {
            MoveHandle;
        }
        ~fence() {
            DestroyHandleBy(vkDestroyFence);
        }
        //getter function
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //const function
        result_t Wait() const{
            VkResult result = vkWaitForFences(graphicsBase::Base().Device(), 1, &handle, false, UINT64_MAX);
            if (result)
                outStream << std::format("[ fence ] ERROR\nFailed to wait for the fence!\nError code: {}\n", int32_t(result));
            return result;
        }

        result_t Reset() const{
            VkResult result = vkResetFences(graphicsBase::Base().Device(), 1, &handle);
            if (result)
                outStream << std::format("[ fence ] ERROR\nFailed to reset the fence!\nError code: {}\n", int32_t(result));
            return result;
        }

        result_t Status() const {
            VkResult result = vkGetFenceStatus(graphicsBase::Base().Device(), handle);
            if (result < 0) {
                outStream << std::format("[ fence ] ERROR\nFailed to get the status of the fence!\nError code: {}\n", int32_t(result));
            }
            return result;
        }
        //因为“等待后立刻重置”的情形经常出现，定义此函数
        result_t WaitAndReset() const {
            VkResult result = Wait();
            result || (result = Reset());
            return result;
        }
        //non const function
        result_t Create(VkFenceCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            VkResult result = vkCreateFence(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result) {
                outStream << std::format("[ fence ] ERROR\nFailed to create a fence!\nError code: {}\n", int32_t(result));
            }
            return result;
        }
        result_t Create(VkFenceCreateFlags flags = 0) {
            VkFenceCreateInfo createInfo = {
                .flags = flags
            };
            return Create(createInfo);
        }
    };

    class semaphore {
        VkSemaphore handle = VK_NULL_HANDLE;
    public:
        semaphore(VkSemaphoreCreateInfo& createInfo) {
            Create(createInfo);
        }
        semaphore() {
            Create();
        }
        semaphore(semaphore&& other) noexcept { MoveHandle; }
        ~semaphore() { DestroyHandleBy(vkDestroySemaphore); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //non const function
        result_t Create(VkSemaphoreCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VkResult result = vkCreateSemaphore(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result) {
                outStream << std::format("[ semaphore ] ERROR\nFailed to create a semaphore!\nError code: {}\n", int32_t(result));
            }
            return result;
        }
        result_t Create() {
            VkSemaphoreCreateInfo createInfo = {};
            return Create(createInfo);
        }
    };

    class commandBuffer {
        friend class commandPool;
        VkCommandBuffer handle = VK_NULL_HANDLE;
    public:
        commandBuffer() = default;
        commandBuffer(commandBuffer&& other) noexcept { MoveHandle; }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Const function
        result_t Begin(VkCommandBufferUsageFlags usageFlags, VkCommandBufferInheritanceInfo& inheritanceInfo) const {
            inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = usageFlags,
                .pInheritanceInfo = &inheritanceInfo
            };
            VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
            if (result)
                outStream << std::format("[ commandBuffer ] ERROR\nFailed to begin a command buffer!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t Begin(VkCommandBufferUsageFlags usageFlags = 0) const {
            VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = usageFlags,
            };
            VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
            if (result)
                outStream << std::format("[ commandBuffer ] ERROR\nFailed to begin a command buffer!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t End() const {
            VkResult result = vkEndCommandBuffer(handle);
            if (result)
                outStream << std::format("[ commandBuffer ] ERROR\nFailed to end a command buffer!\nError code: {}\n", int32_t(result));
            return result;
        }
    };

    class commandPool {
        VkCommandPool handle = VK_NULL_HANDLE;
    public:
        commandPool() = default;
        commandPool(VkCommandPoolCreateInfo& createInfo) {
            Create(createInfo);
        }
        commandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
            Create(queueFamilyIndex, flags);
        }
        commandPool(commandPool&& other) noexcept { MoveHandle; }
        ~commandPool() { DestroyHandleBy(vkDestroyCommandPool); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //const function
        result_t AllocateBuffers(arrayRef<VkCommandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
            VkCommandBufferAllocateInfo allocateInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = handle,
                .level = level,
                .commandBufferCount = uint32_t(buffers.Count())
            };
            VkResult result = vkAllocateCommandBuffers(graphicsBase::Base().Device(), &allocateInfo, buffers.Pointer());
            if (result) {
                outStream << std::format("[ commandPool ] ERROR\nFailed to allocate command buffers!\nError code: {}\n", int32_t(result));
            }
            return result;
        }
        result_t AllocateBuffers(arrayRef<commandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
            return AllocateBuffers({ &buffers[0].handle, buffers.Count() }, level);
        }
        void FreeBuffers(arrayRef<VkCommandBuffer> buffers) const {
            vkFreeCommandBuffers(graphicsBase::Base().Device(), handle, buffers.Count(), buffers.Pointer());
            memset(buffers.Pointer(), 0, buffers.Count() * sizeof(VkCommandBuffer));
        }
        void FreeBuffers(arrayRef<commandBuffer> buffers) const {
            FreeBuffers({ &buffers[0].handle, buffers.Count() });
        }
        void FreeBuffer(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) const {
            //vkFreeCommandBuffers(graphicsBase::Base().Device(), handle, commandBufferCount, pCommandBuffers);
            //memset(pCommandBuffers, 0, commandBufferCount * sizeof(VkCommandPool));
        }
        //non const function
        result_t Create(VkCommandPoolCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            VkResult result = vkCreateCommandPool(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result) {
                outStream << std::format("[ commandPool ] ERROR\nFailed to create a command pool!\nError code: {}\n", int32_t(result));
            }
            return result;
        }
        result_t Create(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
            VkCommandPoolCreateInfo createInfo = {
                .flags = flags,
                .queueFamilyIndex = queueFamilyIndex
            };
            return Create(createInfo);
        }
    };

    inline graphicsBase graphicsBase::singleton;
}