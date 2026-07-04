// #define WIN32_LEAN_AND_MEAN
// #define NOMINMAX
// #define VK_USE_PLATFORM_WIN32_KHR
//
// #include <windows.h>
// #include <vulkan/vulkan.h>
// #include <DirectXMath.h>
//
// #include <algorithm>
// #include <array>
// #include <chrono>
// #include <cmath>
// #include <cstdint>
// #include <cstring>
// #include <stdexcept>
// #include <string>
// #include <vector>
//
// using namespace DirectX;
//
// namespace
// {
// constexpr wchar_t kWindowClassName[] = L"VulkanForwardRendererWindow";
// constexpr uint32_t kInitialWidth = 1280;
// constexpr uint32_t kInitialHeight = 720;
// constexpr uint32_t kShadowMapSize = 2048;
// constexpr uint32_t kMaxFramesInFlight = 2;
// constexpr uint32_t kMaxObjects = 64;
//
// struct Vertex
// {
//     XMFLOAT3 position;
//     XMFLOAT3 normal;
// };
//
// struct alignas(16) FrameGpu
// {
//     XMFLOAT4 cameraPositionExposure;
//     XMFLOAT4 lightDirectionIntensity;
//     XMFLOAT4 lightColorAmbient;
//     XMFLOAT4 shadowTexelPadding;
// };
//
// struct alignas(16) ObjectGpu
// {
//     XMFLOAT4X4 world;
//     XMFLOAT4X4 worldViewProjection;
//     XMFLOAT4X4 lightWorldViewProjection;
//     XMFLOAT4 albedoMetallic;
//     XMFLOAT4 roughnessPadding;
// };
//
// struct Material
// {
//     XMFLOAT3 albedo;
//     float metallic = 0.0f;
//     float roughness = 0.5f;
// };
//
// struct Mesh
// {
//     VkBuffer vertexBuffer = VK_NULL_HANDLE;
//     VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
//     VkBuffer indexBuffer = VK_NULL_HANDLE;
//     VkDeviceMemory indexMemory = VK_NULL_HANDLE;
//     uint32_t indexCount = 0;
// };
//
// struct RenderItem
// {
//     Mesh* mesh = nullptr;
//     XMMATRIX world = XMMatrixIdentity();
//     Material material = {};
//     bool castsShadow = true;
// };
//
// struct QueueFamilyIndices
// {
//     uint32_t graphicsFamily = UINT32_MAX;
//     uint32_t presentFamily = UINT32_MAX;
//
//     bool IsComplete() const
//     {
//         return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX;
//     }
// };
//
// struct SwapChainSupport
// {
//     VkSurfaceCapabilitiesKHR capabilities = {};
//     std::vector<VkSurfaceFormatKHR> formats;
//     std::vector<VkPresentModeKHR> presentModes;
// };
//
// void Check(VkResult result, const char* message)
// {
//     if (result != VK_SUCCESS)
//     {
//         throw std::runtime_error(message);
//     }
// }
//
// std::vector<char> ReadFileBytes(const std::wstring& path)
// {
//     HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
//     if (file == INVALID_HANDLE_VALUE)
//     {
//         throw std::runtime_error("Failed to open shader bytecode.");
//     }
//
//     LARGE_INTEGER fileSize = {};
//     if (!GetFileSizeEx(file, &fileSize) || fileSize.QuadPart <= 0)
//     {
//         CloseHandle(file);
//         throw std::runtime_error("Invalid shader bytecode file.");
//     }
//
//     std::vector<char> bytes(static_cast<size_t>(fileSize.QuadPart));
//     DWORD bytesRead = 0;
//     const BOOL readOk = ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &bytesRead, nullptr);
//     CloseHandle(file);
//
//     if (!readOk || bytesRead != bytes.size())
//     {
//         throw std::runtime_error("Failed to read shader bytecode.");
//     }
//
//     return bytes;
// }
//
// std::wstring GetExecutableDirectory()
// {
//     std::wstring path(MAX_PATH, L'\0');
//     DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
//     while (length == path.size())
//     {
//         path.resize(path.size() * 2);
//         length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
//     }
//
//     if (length == 0)
//     {
//         throw std::runtime_error("Failed to resolve executable path.");
//     }
//
//     path.resize(length);
//     const size_t slash = path.find_last_of(L"\\/");
//     if (slash == std::wstring::npos)
//     {
//         return L".";
//     }
//
//     return path.substr(0, slash);
// }
//
// XMMATRIX MakeVulkanProjection(XMMATRIX projection)
// {
//     projection.r[1].m128_f32[1] *= -1.0f;
//     return projection;
// }
//
// class ForwardRenderer
// {
// public:
//     void Initialize(HWND hwnd, uint32_t width, uint32_t height)
//     {
//         hwnd_ = hwnd;
//         viewportWidth_ = width;
//         viewportHeight_ = height;
//         shaderDirectory_ = GetExecutableDirectory();
//
//         CreateInstance();
//         CreateSurface();
//         PickPhysicalDevice();
//         CreateDevice();
//         CreateCommandPool();
//         CreateSwapChain();
//         CreateDepthResources();
//         CreateShadowResources();
//         CreateRenderPasses();
//         CreateDescriptorSetLayout();
//         CreateForwardPipeline();
//         CreateShadowPipeline();
//         CreateGeometry();
//         CreateFrameResources();
//         CreateDescriptorPool();
//         CreateDescriptorSets();
//         CreateFramebuffers();
//         CreateSyncObjects();
//         initialized_ = true;
//     }
//
//     void Shutdown()
//     {
//         if (!initialized_)
//         {
//             return;
//         }
//
//         vkDeviceWaitIdle(device_);
//
//         CleanupSwapChain();
//         DestroyMesh(cubeMesh_);
//         DestroyMesh(planeMesh_);
//         DestroyMesh(sphereMesh_);
//
//         for (uint32_t frame = 0; frame < kMaxFramesInFlight; ++frame)
//         {
//             if (frameUniformBuffers_[frame] != VK_NULL_HANDLE)
//             {
//                 vkDestroyBuffer(device_, frameUniformBuffers_[frame], nullptr);
//             }
//             if (frameUniformMemories_[frame] != VK_NULL_HANDLE)
//             {
//                 vkFreeMemory(device_, frameUniformMemories_[frame], nullptr);
//             }
//             if (objectBuffers_[frame] != VK_NULL_HANDLE)
//             {
//                 vkDestroyBuffer(device_, objectBuffers_[frame], nullptr);
//             }
//             if (objectMemories_[frame] != VK_NULL_HANDLE)
//             {
//                 vkFreeMemory(device_, objectMemories_[frame], nullptr);
//             }
//             if (imageAvailableSemaphores_[frame] != VK_NULL_HANDLE)
//             {
//                 vkDestroySemaphore(device_, imageAvailableSemaphores_[frame], nullptr);
//             }
//             if (renderFinishedSemaphores_[frame] != VK_NULL_HANDLE)
//             {
//                 vkDestroySemaphore(device_, renderFinishedSemaphores_[frame], nullptr);
//             }
//             if (inFlightFences_[frame] != VK_NULL_HANDLE)
//             {
//                 vkDestroyFence(device_, inFlightFences_[frame], nullptr);
//             }
//         }
//
//         if (descriptorPool_ != VK_NULL_HANDLE)
//         {
//             vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
//         }
//         if (descriptorSetLayout_ != VK_NULL_HANDLE)
//         {
//             vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
//         }
//         if (shadowPipeline_ != VK_NULL_HANDLE)
//         {
//             vkDestroyPipeline(device_, shadowPipeline_, nullptr);
//         }
//         if (pipelineLayout_ != VK_NULL_HANDLE)
//         {
//             vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
//         }
//         if (shadowFramebuffer_ != VK_NULL_HANDLE)
//         {
//             vkDestroyFramebuffer(device_, shadowFramebuffer_, nullptr);
//         }
//         if (shadowRenderPass_ != VK_NULL_HANDLE)
//         {
//             vkDestroyRenderPass(device_, shadowRenderPass_, nullptr);
//         }
//         if (shadowSampler_ != VK_NULL_HANDLE)
//         {
//             vkDestroySampler(device_, shadowSampler_, nullptr);
//         }
//         if (shadowImageView_ != VK_NULL_HANDLE)
//         {
//             vkDestroyImageView(device_, shadowImageView_, nullptr);
//         }
//         if (shadowImage_ != VK_NULL_HANDLE)
//         {
//             vkDestroyImage(device_, shadowImage_, nullptr);
//         }
//         if (shadowImageMemory_ != VK_NULL_HANDLE)
//         {
//             vkFreeMemory(device_, shadowImageMemory_, nullptr);
//         }
//         if (commandPool_ != VK_NULL_HANDLE)
//         {
//             vkDestroyCommandPool(device_, commandPool_, nullptr);
//         }
//         if (device_ != VK_NULL_HANDLE)
//         {
//             vkDestroyDevice(device_, nullptr);
//         }
//         if (surface_ != VK_NULL_HANDLE)
//         {
//             vkDestroySurfaceKHR(instance_, surface_, nullptr);
//         }
//         if (instance_ != VK_NULL_HANDLE)
//         {
//             vkDestroyInstance(instance_, nullptr);
//         }
//
//         initialized_ = false;
//     }
//
//     void Resize(uint32_t width, uint32_t height)
//     {
//         viewportWidth_ = width;
//         viewportHeight_ = height;
//         framebufferResized_ = true;
//     }
//
//     void Render(float timeSeconds)
//     {
//         if (!initialized_ || viewportWidth_ == 0 || viewportHeight_ == 0)
//         {
//             return;
//         }
//
//         vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);
//
//         uint32_t imageIndex = 0;
//         VkResult acquireResult = vkAcquireNextImageKHR(
//             device_,
//             swapChain_,
//             UINT64_MAX,
//             imageAvailableSemaphores_[currentFrame_],
//             VK_NULL_HANDLE,
//             &imageIndex);
//
//         if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
//         {
//             RecreateSwapChain();
//             return;
//         }
//
//         if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
//         {
//             throw std::runtime_error("Failed to acquire swapchain image.");
//         }
//
//         const std::vector<RenderItem> scene = BuildScene(timeSeconds);
//         const SceneMatrices matrices = UpdateFrameData(scene);
//
//         vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);
//         vkResetCommandBuffer(commandBuffers_[currentFrame_], 0);
//         RecordCommandBuffer(commandBuffers_[currentFrame_], imageIndex, scene, matrices);
//
//         const VkSemaphore waitSemaphores[] = { imageAvailableSemaphores_[currentFrame_] };
//         const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
//         const VkSemaphore signalSemaphores[] = { renderFinishedSemaphores_[currentFrame_] };
//
//         VkSubmitInfo submitInfo = {};
//         submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//         submitInfo.waitSemaphoreCount = 1;
//         submitInfo.pWaitSemaphores = waitSemaphores;
//         submitInfo.pWaitDstStageMask = waitStages;
//         submitInfo.commandBufferCount = 1;
//         submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_];
//         submitInfo.signalSemaphoreCount = 1;
//         submitInfo.pSignalSemaphores = signalSemaphores;
//         Check(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]), "Failed to submit command buffer.");
//
//         VkPresentInfoKHR presentInfo = {};
//         presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
//         presentInfo.waitSemaphoreCount = 1;
//         presentInfo.pWaitSemaphores = signalSemaphores;
//         presentInfo.swapchainCount = 1;
//         presentInfo.pSwapchains = &swapChain_;
//         presentInfo.pImageIndices = &imageIndex;
//
//         const VkResult presentResult = vkQueuePresentKHR(presentQueue_, &presentInfo);
//         if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || framebufferResized_)
//         {
//             framebufferResized_ = false;
//             RecreateSwapChain();
//         }
//         else if (presentResult != VK_SUCCESS)
//         {
//             throw std::runtime_error("Failed to present swapchain image.");
//         }
//
//         currentFrame_ = (currentFrame_ + 1) % kMaxFramesInFlight;
//     }
//
// private:
//     struct SceneMatrices
//     {
//         XMMATRIX viewProjection = XMMatrixIdentity();
//         XMMATRIX lightViewProjection = XMMatrixIdentity();
//     };
//
//     void CreateInstance()
//     {
//         VkApplicationInfo appInfo = {};
//         appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
//         appInfo.pApplicationName = "CG2PBR Vulkan Forward Renderer";
//         appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
//         appInfo.pEngineName = "PFL";
//         appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
//         appInfo.apiVersion = VK_API_VERSION_1_0;
//
//         const char* extensions[] = {
//             VK_KHR_SURFACE_EXTENSION_NAME,
//             VK_KHR_WIN32_SURFACE_EXTENSION_NAME
//         };
//
//         VkInstanceCreateInfo createInfo = {};
//         createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
//         createInfo.pApplicationInfo = &appInfo;
//         createInfo.enabledExtensionCount = static_cast<uint32_t>(std::size(extensions));
//         createInfo.ppEnabledExtensionNames = extensions;
//
//         Check(vkCreateInstance(&createInfo, nullptr, &instance_), "Failed to create Vulkan instance.");
//     }
//
//     void CreateSurface()
//     {
//         VkWin32SurfaceCreateInfoKHR createInfo = {};
//         createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
//         createInfo.hinstance = GetModuleHandleW(nullptr);
//         createInfo.hwnd = hwnd_;
//         Check(vkCreateWin32SurfaceKHR(instance_, &createInfo, nullptr, &surface_), "Failed to create Win32 Vulkan surface.");
//     }
//
//     void PickPhysicalDevice()
//     {
//         uint32_t deviceCount = 0;
//         Check(vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr), "Failed to enumerate physical devices.");
//         if (deviceCount == 0)
//         {
//             throw std::runtime_error("No Vulkan physical device found.");
//         }
//
//         std::vector<VkPhysicalDevice> devices(deviceCount);
//         Check(vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data()), "Failed to enumerate physical devices.");
//
//         for (VkPhysicalDevice candidate : devices)
//         {
//             if (IsDeviceSuitable(candidate))
//             {
//                 physicalDevice_ = candidate;
//                 break;
//             }
//         }
//
//         if (physicalDevice_ == VK_NULL_HANDLE)
//         {
//             throw std::runtime_error("No suitable Vulkan physical device found.");
//         }
//
//         vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties_);
//     }
//
//     bool IsDeviceSuitable(VkPhysicalDevice candidate) const
//     {
//         const QueueFamilyIndices indices = FindQueueFamilies(candidate);
//         if (!indices.IsComplete() || !CheckDeviceExtensions(candidate))
//         {
//             return false;
//         }
//
//         const SwapChainSupport support = QuerySwapChainSupport(candidate);
//         return !support.formats.empty() && !support.presentModes.empty();
//     }
//
//     bool CheckDeviceExtensions(VkPhysicalDevice candidate) const
//     {
//         uint32_t extensionCount = 0;
//         vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, nullptr);
//         std::vector<VkExtensionProperties> extensions(extensionCount);
//         vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, extensions.data());
//
//         for (const VkExtensionProperties& extension : extensions)
//         {
//             if (std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
//             {
//                 return true;
//             }
//         }
//
//         return false;
//     }
//
//     QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice candidate) const
//     {
//         QueueFamilyIndices indices = {};
//         uint32_t queueFamilyCount = 0;
//         vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, nullptr);
//         std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
//         vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, families.data());
//
//         for (uint32_t index = 0; index < queueFamilyCount; ++index)
//         {
//             if (families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
//             {
//                 indices.graphicsFamily = index;
//             }
//
//             VkBool32 presentSupport = VK_FALSE;
//             vkGetPhysicalDeviceSurfaceSupportKHR(candidate, index, surface_, &presentSupport);
//             if (presentSupport)
//             {
//                 indices.presentFamily = index;
//             }
//
//             if (indices.IsComplete())
//             {
//                 break;
//             }
//         }
//
//         return indices;
//     }
//
//     SwapChainSupport QuerySwapChainSupport(VkPhysicalDevice candidate) const
//     {
//         SwapChainSupport support = {};
//         vkGetPhysicalDeviceSurfaceCapabilitiesKHR(candidate, surface_, &support.capabilities);
//
//         uint32_t formatCount = 0;
//         vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, surface_, &formatCount, nullptr);
//         if (formatCount != 0)
//         {
//             support.formats.resize(formatCount);
//             vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, surface_, &formatCount, support.formats.data());
//         }
//
//         uint32_t presentModeCount = 0;
//         vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, surface_, &presentModeCount, nullptr);
//         if (presentModeCount != 0)
//         {
//             support.presentModes.resize(presentModeCount);
//             vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, surface_, &presentModeCount, support.presentModes.data());
//         }
//
//         return support;
//     }
//
//     void CreateDevice()
//     {
//         queueFamilies_ = FindQueueFamilies(physicalDevice_);
//         std::vector<uint32_t> uniqueFamilies = { queueFamilies_.graphicsFamily };
//         if (queueFamilies_.presentFamily != queueFamilies_.graphicsFamily)
//         {
//             uniqueFamilies.push_back(queueFamilies_.presentFamily);
//         }
//
//         const float queuePriority = 1.0f;
//         std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
//         queueCreateInfos.reserve(uniqueFamilies.size());
//         for (uint32_t family : uniqueFamilies)
//         {
//             VkDeviceQueueCreateInfo queueCreateInfo = {};
//             queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//             queueCreateInfo.queueFamilyIndex = family;
//             queueCreateInfo.queueCount = 1;
//             queueCreateInfo.pQueuePriorities = &queuePriority;
//             queueCreateInfos.push_back(queueCreateInfo);
//         }
//
//         const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
//         VkPhysicalDeviceFeatures features = {};
//         features.samplerAnisotropy = VK_TRUE;
//
//         VkDeviceCreateInfo createInfo = {};
//         createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
//         createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
//         createInfo.pQueueCreateInfos = queueCreateInfos.data();
//         createInfo.pEnabledFeatures = &features;
//         createInfo.enabledExtensionCount = static_cast<uint32_t>(std::size(extensions));
//         createInfo.ppEnabledExtensionNames = extensions;
//
//         Check(vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_), "Failed to create Vulkan logical device.");
//         vkGetDeviceQueue(device_, queueFamilies_.graphicsFamily, 0, &graphicsQueue_);
//         vkGetDeviceQueue(device_, queueFamilies_.presentFamily, 0, &presentQueue_);
//     }
//
//     void CreateCommandPool()
//     {
//         VkCommandPoolCreateInfo createInfo = {};
//         createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//         createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
//         createInfo.queueFamilyIndex = queueFamilies_.graphicsFamily;
//         Check(vkCreateCommandPool(device_, &createInfo, nullptr, &commandPool_), "Failed to create command pool.");
//     }
//
//     void CreateSwapChain()
//     {
//         const SwapChainSupport support = QuerySwapChainSupport(physicalDevice_);
//         const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(support.formats);
//         const VkPresentModeKHR presentMode = ChoosePresentMode(support.presentModes);
//         const VkExtent2D extent = ChooseSwapExtent(support.capabilities);
//
//         uint32_t imageCount = support.capabilities.minImageCount + 1;
//         if (support.capabilities.maxImageCount > 0)
//         {
//             imageCount = std::min(imageCount, support.capabilities.maxImageCount);
//         }
//
//         VkSwapchainCreateInfoKHR createInfo = {};
//         createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
//         createInfo.surface = surface_;
//         createInfo.minImageCount = imageCount;
//         createInfo.imageFormat = surfaceFormat.format;
//         createInfo.imageColorSpace = surfaceFormat.colorSpace;
//         createInfo.imageExtent = extent;
//         createInfo.imageArrayLayers = 1;
//         createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
//
//         const uint32_t familyIndices[] = { queueFamilies_.graphicsFamily, queueFamilies_.presentFamily };
//         if (queueFamilies_.graphicsFamily != queueFamilies_.presentFamily)
//         {
//             createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
//             createInfo.queueFamilyIndexCount = 2;
//             createInfo.pQueueFamilyIndices = familyIndices;
//         }
//         else
//         {
//             createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
//         }
//
//         createInfo.preTransform = support.capabilities.currentTransform;
//         createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
//         createInfo.presentMode = presentMode;
//         createInfo.clipped = VK_TRUE;
//
//         Check(vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_), "Failed to create swapchain.");
//         vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, nullptr);
//         swapChainImages_.resize(imageCount);
//         vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, swapChainImages_.data());
//
//         swapChainFormat_ = surfaceFormat.format;
//         swapChainExtent_ = extent;
//         swapChainImageViews_.resize(swapChainImages_.size());
//         for (size_t index = 0; index < swapChainImages_.size(); ++index)
//         {
//             swapChainImageViews_[index] = CreateImageView(swapChainImages_[index], swapChainFormat_, VK_IMAGE_ASPECT_COLOR_BIT);
//         }
//     }
//
//     VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const
//     {
//         for (const VkSurfaceFormatKHR& format : formats)
//         {
//             if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
//             {
//                 return format;
//             }
//         }
//
//         return formats.front();
//     }
//
//     VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const
//     {
//         for (VkPresentModeKHR mode : presentModes)
//         {
//             if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
//             {
//                 return mode;
//             }
//         }
//
//         return VK_PRESENT_MODE_FIFO_KHR;
//     }
//
//     VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
//     {
//         if (capabilities.currentExtent.width != UINT32_MAX)
//         {
//             return capabilities.currentExtent;
//         }
//
//         VkExtent2D extent = { viewportWidth_, viewportHeight_ };
//         extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
//         extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
//         return extent;
//     }
//
//     void CreateDepthResources()
//     {
//         depthFormat_ = FindDepthFormat();
//         CreateImage(
//             swapChainExtent_.width,
//             swapChainExtent_.height,
//             depthFormat_,
//             VK_IMAGE_TILING_OPTIMAL,
//             VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
//             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//             depthImage_,
//             depthImageMemory_);
//         depthImageView_ = CreateImageView(depthImage_, depthFormat_, VK_IMAGE_ASPECT_DEPTH_BIT);
//     }
//
//     void CreateShadowResources()
//     {
//         shadowFormat_ = VK_FORMAT_D32_SFLOAT;
//         CreateImage(
//             kShadowMapSize,
//             kShadowMapSize,
//             shadowFormat_,
//             VK_IMAGE_TILING_OPTIMAL,
//             VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
//             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//             shadowImage_,
//             shadowImageMemory_);
//         shadowImageView_ = CreateImageView(shadowImage_, shadowFormat_, VK_IMAGE_ASPECT_DEPTH_BIT);
//
//         VkSamplerCreateInfo samplerInfo = {};
//         samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
//         samplerInfo.magFilter = VK_FILTER_LINEAR;
//         samplerInfo.minFilter = VK_FILTER_LINEAR;
//         samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
//         samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
//         samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
//         samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
//         samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
//         samplerInfo.compareEnable = VK_TRUE;
//         samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
//         samplerInfo.minLod = 0.0f;
//         samplerInfo.maxLod = 1.0f;
//         Check(vkCreateSampler(device_, &samplerInfo, nullptr, &shadowSampler_), "Failed to create shadow sampler.");
//     }
//
//     void CreateRenderPasses()
//     {
//         CreateForwardRenderPass();
//         CreateShadowRenderPass();
//     }
//
//     void CreateForwardRenderPass()
//     {
//         VkAttachmentDescription colorAttachment = {};
//         colorAttachment.format = swapChainFormat_;
//         colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
//         colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//         colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//         colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//         colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//         colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//         colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//
//         VkAttachmentDescription depthAttachment = {};
//         depthAttachment.format = depthFormat_;
//         depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
//         depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//         depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//         depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//         depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//         depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//         depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//         const std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
//
//         VkAttachmentReference colorReference = {};
//         colorReference.attachment = 0;
//         colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//         VkAttachmentReference depthReference = {};
//         depthReference.attachment = 1;
//         depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//         VkSubpassDescription subpass = {};
//         subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//         subpass.colorAttachmentCount = 1;
//         subpass.pColorAttachments = &colorReference;
//         subpass.pDepthStencilAttachment = &depthReference;
//
//         std::array<VkSubpassDependency, 2> dependencies = {};
//         dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
//         dependencies[0].dstSubpass = 0;
//         dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//         dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//         dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//
//         dependencies[1].srcSubpass = 0;
//         dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
//         dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//         dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
//         dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//
//         VkRenderPassCreateInfo renderPassInfo = {};
//         renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//         renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
//         renderPassInfo.pAttachments = attachments.data();
//         renderPassInfo.subpassCount = 1;
//         renderPassInfo.pSubpasses = &subpass;
//         renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
//         renderPassInfo.pDependencies = dependencies.data();
//         Check(vkCreateRenderPass(device_, &renderPassInfo, nullptr, &forwardRenderPass_), "Failed to create forward render pass.");
//     }
//
//     void CreateShadowRenderPass()
//     {
//         VkAttachmentDescription depthAttachment = {};
//         depthAttachment.format = shadowFormat_;
//         depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
//         depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//         depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//         depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//         depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//         depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//         depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
//
//         VkAttachmentReference depthReference = {};
//         depthReference.attachment = 0;
//         depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//         VkSubpassDescription subpass = {};
//         subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//         subpass.pDepthStencilAttachment = &depthReference;
//
//         std::array<VkSubpassDependency, 2> dependencies = {};
//         dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
//         dependencies[0].dstSubpass = 0;
//         dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//         dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//         dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
//         dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//
//         dependencies[1].srcSubpass = 0;
//         dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
//         dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
//         dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//         dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//         dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//
//         VkRenderPassCreateInfo renderPassInfo = {};
//         renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//         renderPassInfo.attachmentCount = 1;
//         renderPassInfo.pAttachments = &depthAttachment;
//         renderPassInfo.subpassCount = 1;
//         renderPassInfo.pSubpasses = &subpass;
//         renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
//         renderPassInfo.pDependencies = dependencies.data();
//         Check(vkCreateRenderPass(device_, &renderPassInfo, nullptr, &shadowRenderPass_), "Failed to create shadow render pass.");
//     }
//
//     void CreateDescriptorSetLayout()
//     {
//         VkDescriptorSetLayoutBinding frameBinding = {};
//         frameBinding.binding = 0;
//         frameBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//         frameBinding.descriptorCount = 1;
//         frameBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
//
//         VkDescriptorSetLayoutBinding objectBinding = {};
//         objectBinding.binding = 1;
//         objectBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//         objectBinding.descriptorCount = 1;
//         objectBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
//
//         VkDescriptorSetLayoutBinding shadowBinding = {};
//         shadowBinding.binding = 2;
//         shadowBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//         shadowBinding.descriptorCount = 1;
//         shadowBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
//
//         const std::array<VkDescriptorSetLayoutBinding, 3> bindings = { frameBinding, objectBinding, shadowBinding };
//         VkDescriptorSetLayoutCreateInfo layoutInfo = {};
//         layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//         layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
//         layoutInfo.pBindings = bindings.data();
//         Check(vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayout_), "Failed to create descriptor set layout.");
//
//         VkPushConstantRange pushConstantRange = {};
//         pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
//         pushConstantRange.offset = 0;
//         pushConstantRange.size = sizeof(uint32_t);
//
//         VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
//         pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//         pipelineLayoutInfo.setLayoutCount = 1;
//         pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
//         pipelineLayoutInfo.pushConstantRangeCount = 1;
//         pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
//         Check(vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_), "Failed to create pipeline layout.");
//     }
//
//     void CreateForwardPipeline()
//     {
//         VkShaderModule vertexShader = CreateShaderModule(shaderDirectory_ + L"\\forward.vert.spv");
//         VkShaderModule fragmentShader = CreateShaderModule(shaderDirectory_ + L"\\forward.frag.spv");
//
//         VkPipelineShaderStageCreateInfo vertexStage = {};
//         vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//         vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
//         vertexStage.module = vertexShader;
//         vertexStage.pName = "main";
//
//         VkPipelineShaderStageCreateInfo fragmentStage = {};
//         fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//         fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//         fragmentStage.module = fragmentShader;
//         fragmentStage.pName = "main";
//
//         const VkPipelineShaderStageCreateInfo stages[] = { vertexStage, fragmentStage };
//         forwardPipeline_ = CreateGraphicsPipeline(stages, 2, forwardRenderPass_, false, true);
//
//         vkDestroyShaderModule(device_, fragmentShader, nullptr);
//         vkDestroyShaderModule(device_, vertexShader, nullptr);
//     }
//
//     void CreateShadowPipeline()
//     {
//         VkShaderModule vertexShader = CreateShaderModule(shaderDirectory_ + L"\\shadow.vert.spv");
//
//         VkPipelineShaderStageCreateInfo vertexStage = {};
//         vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//         vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
//         vertexStage.module = vertexShader;
//         vertexStage.pName = "main";
//
//         shadowPipeline_ = CreateGraphicsPipeline(&vertexStage, 1, shadowRenderPass_, true, false);
//         vkDestroyShaderModule(device_, vertexShader, nullptr);
//     }
//
//     VkPipeline CreateGraphicsPipeline(
//         const VkPipelineShaderStageCreateInfo* shaderStages,
//         uint32_t shaderStageCount,
//         VkRenderPass renderPass,
//         bool depthBias,
//         bool hasColorAttachment)
//     {
//         VkVertexInputBindingDescription bindingDescription = {};
//         bindingDescription.binding = 0;
//         bindingDescription.stride = sizeof(Vertex);
//         bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//
//         std::array<VkVertexInputAttributeDescription, 2> attributes = {};
//         attributes[0].binding = 0;
//         attributes[0].location = 0;
//         attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
//         attributes[0].offset = offsetof(Vertex, position);
//         attributes[1].binding = 0;
//         attributes[1].location = 1;
//         attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
//         attributes[1].offset = offsetof(Vertex, normal);
//
//         VkPipelineVertexInputStateCreateInfo vertexInput = {};
//         vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//         vertexInput.vertexBindingDescriptionCount = 1;
//         vertexInput.pVertexBindingDescriptions = &bindingDescription;
//         vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
//         vertexInput.pVertexAttributeDescriptions = attributes.data();
//
//         VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
//         inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//         inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//
//         VkViewport viewport = {};
//         viewport.width = 1.0f;
//         viewport.height = 1.0f;
//         viewport.maxDepth = 1.0f;
//
//         VkRect2D scissor = {};
//         scissor.extent = { 1, 1 };
//
//         VkPipelineViewportStateCreateInfo viewportState = {};
//         viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//         viewportState.viewportCount = 1;
//         viewportState.pViewports = &viewport;
//         viewportState.scissorCount = 1;
//         viewportState.pScissors = &scissor;
//
//         VkPipelineRasterizationStateCreateInfo rasterizer = {};
//         rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//         rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//         rasterizer.cullMode = VK_CULL_MODE_NONE;
//         rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
//         rasterizer.lineWidth = 1.0f;
//         rasterizer.depthBiasEnable = depthBias ? VK_TRUE : VK_FALSE;
//         rasterizer.depthBiasConstantFactor = 1.25f;
//         rasterizer.depthBiasSlopeFactor = 1.75f;
//
//         VkPipelineMultisampleStateCreateInfo multisampling = {};
//         multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//         multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//
//         VkPipelineDepthStencilStateCreateInfo depthStencil = {};
//         depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//         depthStencil.depthTestEnable = VK_TRUE;
//         depthStencil.depthWriteEnable = VK_TRUE;
//         depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
//         depthStencil.depthBoundsTestEnable = VK_FALSE;
//         depthStencil.stencilTestEnable = VK_FALSE;
//
//         VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
//         colorBlendAttachment.colorWriteMask =
//             VK_COLOR_COMPONENT_R_BIT |
//             VK_COLOR_COMPONENT_G_BIT |
//             VK_COLOR_COMPONENT_B_BIT |
//             VK_COLOR_COMPONENT_A_BIT;
//
//         VkPipelineColorBlendStateCreateInfo colorBlending = {};
//         colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//         colorBlending.attachmentCount = hasColorAttachment ? 1u : 0u;
//         colorBlending.pAttachments = hasColorAttachment ? &colorBlendAttachment : nullptr;
//
//         const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
//         VkPipelineDynamicStateCreateInfo dynamicState = {};
//         dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//         dynamicState.dynamicStateCount = static_cast<uint32_t>(std::size(dynamicStates));
//         dynamicState.pDynamicStates = dynamicStates;
//
//         VkGraphicsPipelineCreateInfo pipelineInfo = {};
//         pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//         pipelineInfo.stageCount = shaderStageCount;
//         pipelineInfo.pStages = shaderStages;
//         pipelineInfo.pVertexInputState = &vertexInput;
//         pipelineInfo.pInputAssemblyState = &inputAssembly;
//         pipelineInfo.pViewportState = &viewportState;
//         pipelineInfo.pRasterizationState = &rasterizer;
//         pipelineInfo.pMultisampleState = &multisampling;
//         pipelineInfo.pDepthStencilState = &depthStencil;
//         pipelineInfo.pColorBlendState = &colorBlending;
//         pipelineInfo.pDynamicState = &dynamicState;
//         pipelineInfo.layout = pipelineLayout_;
//         pipelineInfo.renderPass = renderPass;
//         pipelineInfo.subpass = 0;
//
//         VkPipeline pipeline = VK_NULL_HANDLE;
//         Check(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), "Failed to create graphics pipeline.");
//         return pipeline;
//     }
//
//     VkShaderModule CreateShaderModule(const std::wstring& path)
//     {
//         const std::vector<char> code = ReadFileBytes(path);
//         VkShaderModuleCreateInfo createInfo = {};
//         createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//         createInfo.codeSize = code.size();
//         createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
//
//         VkShaderModule shaderModule = VK_NULL_HANDLE;
//         Check(vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule), "Failed to create shader module.");
//         return shaderModule;
//     }
//
//     void CreateGeometry()
//     {
//         CreateCubeMesh(cubeMesh_);
//         CreatePlaneMesh(planeMesh_);
//         CreateSphereMesh(sphereMesh_, 32, 16);
//     }
//
//     void CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, Mesh& mesh)
//     {
//         CreateDeviceLocalBuffer(
//             vertices.data(),
//             vertices.size() * sizeof(Vertex),
//             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
//             mesh.vertexBuffer,
//             mesh.vertexMemory);
//         CreateDeviceLocalBuffer(
//             indices.data(),
//             indices.size() * sizeof(uint16_t),
//             VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
//             mesh.indexBuffer,
//             mesh.indexMemory);
//         mesh.indexCount = static_cast<uint32_t>(indices.size());
//     }
//
//     void CreateCubeMesh(Mesh& mesh)
//     {
//         const std::vector<Vertex> vertices = {
//             { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
//             { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
//             { XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
//             { XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
//             { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
//             { XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
//             { XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
//             { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
//             { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
//             { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
//             { XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
//             { XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
//             { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
//             { XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
//             { XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
//             { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
//             { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
//             { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
//             { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
//             { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
//             { XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
//             { XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
//             { XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
//             { XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }
//         };
//
//         const std::vector<uint16_t> indices = {
//              0,  1,  2,  0,  2,  3,
//              4,  5,  6,  4,  6,  7,
//              8,  9, 10,  8, 10, 11,
//             12, 13, 14, 12, 14, 15,
//             16, 17, 18, 16, 18, 19,
//             20, 21, 22, 20, 22, 23
//         };
//
//         CreateMesh(vertices, indices, mesh);
//     }
//
//     void CreatePlaneMesh(Mesh& mesh)
//     {
//         constexpr float size = 8.0f;
//         const std::vector<Vertex> vertices = {
//             { XMFLOAT3(-size, 0.0f, -size), XMFLOAT3(0.0f, 1.0f, 0.0f) },
//             { XMFLOAT3(-size, 0.0f,  size), XMFLOAT3(0.0f, 1.0f, 0.0f) },
//             { XMFLOAT3( size, 0.0f,  size), XMFLOAT3(0.0f, 1.0f, 0.0f) },
//             { XMFLOAT3( size, 0.0f, -size), XMFLOAT3(0.0f, 1.0f, 0.0f) }
//         };
//         const std::vector<uint16_t> indices = { 0, 1, 2, 0, 2, 3 };
//         CreateMesh(vertices, indices, mesh);
//     }
//
//     void CreateSphereMesh(Mesh& mesh, uint32_t slices, uint32_t stacks)
//     {
//         std::vector<Vertex> vertices;
//         std::vector<uint16_t> indices;
//         vertices.reserve((slices + 1) * (stacks + 1));
//
//         for (uint32_t stack = 0; stack <= stacks; ++stack)
//         {
//             const float v = static_cast<float>(stack) / static_cast<float>(stacks);
//             const float phi = v * XM_PI;
//             for (uint32_t slice = 0; slice <= slices; ++slice)
//             {
//                 const float u = static_cast<float>(slice) / static_cast<float>(slices);
//                 const float theta = u * XM_2PI;
//                 const float x = std::sinf(phi) * std::cosf(theta);
//                 const float y = std::cosf(phi);
//                 const float z = std::sinf(phi) * std::sinf(theta);
//                 vertices.push_back({ XMFLOAT3(x, y, z), XMFLOAT3(x, y, z) });
//             }
//         }
//
//         for (uint32_t stack = 0; stack < stacks; ++stack)
//         {
//             for (uint32_t slice = 0; slice < slices; ++slice)
//             {
//                 const uint16_t a = static_cast<uint16_t>(stack * (slices + 1) + slice);
//                 const uint16_t b = static_cast<uint16_t>(a + slices + 1);
//                 indices.push_back(a);
//                 indices.push_back(b);
//                 indices.push_back(static_cast<uint16_t>(a + 1));
//                 indices.push_back(static_cast<uint16_t>(a + 1));
//                 indices.push_back(b);
//                 indices.push_back(static_cast<uint16_t>(b + 1));
//             }
//         }
//
//         CreateMesh(vertices, indices, mesh);
//     }
//
//     void CreateFrameResources()
//     {
//         for (uint32_t frame = 0; frame < kMaxFramesInFlight; ++frame)
//         {
//             CreateBuffer(
//                 sizeof(FrameGpu),
//                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                 frameUniformBuffers_[frame],
//                 frameUniformMemories_[frame]);
//             vkMapMemory(device_, frameUniformMemories_[frame], 0, sizeof(FrameGpu), 0, &frameUniformMapped_[frame]);
//
//             CreateBuffer(
//                 sizeof(ObjectGpu) * kMaxObjects,
//                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
//                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                 objectBuffers_[frame],
//                 objectMemories_[frame]);
//             vkMapMemory(device_, objectMemories_[frame], 0, sizeof(ObjectGpu) * kMaxObjects, 0, &objectMapped_[frame]);
//         }
//
//         VkCommandBufferAllocateInfo allocateInfo = {};
//         allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//         allocateInfo.commandPool = commandPool_;
//         allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//         allocateInfo.commandBufferCount = kMaxFramesInFlight;
//         Check(vkAllocateCommandBuffers(device_, &allocateInfo, commandBuffers_.data()), "Failed to allocate command buffers.");
//     }
//
//     void CreateDescriptorPool()
//     {
//         const std::array<VkDescriptorPoolSize, 3> poolSizes = {
//             VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kMaxFramesInFlight },
//             VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kMaxFramesInFlight },
//             VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxFramesInFlight }
//         };
//
//         VkDescriptorPoolCreateInfo poolInfo = {};
//         poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//         poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
//         poolInfo.pPoolSizes = poolSizes.data();
//         poolInfo.maxSets = kMaxFramesInFlight;
//         Check(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_), "Failed to create descriptor pool.");
//     }
//
//     void CreateDescriptorSets()
//     {
//         std::array<VkDescriptorSetLayout, kMaxFramesInFlight> layouts = {};
//         layouts.fill(descriptorSetLayout_);
//
//         VkDescriptorSetAllocateInfo allocateInfo = {};
//         allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//         allocateInfo.descriptorPool = descriptorPool_;
//         allocateInfo.descriptorSetCount = kMaxFramesInFlight;
//         allocateInfo.pSetLayouts = layouts.data();
//         Check(vkAllocateDescriptorSets(device_, &allocateInfo, descriptorSets_.data()), "Failed to allocate descriptor sets.");
//
//         for (uint32_t frame = 0; frame < kMaxFramesInFlight; ++frame)
//         {
//             VkDescriptorBufferInfo frameBufferInfo = {};
//             frameBufferInfo.buffer = frameUniformBuffers_[frame];
//             frameBufferInfo.offset = 0;
//             frameBufferInfo.range = sizeof(FrameGpu);
//
//             VkDescriptorBufferInfo objectBufferInfo = {};
//             objectBufferInfo.buffer = objectBuffers_[frame];
//             objectBufferInfo.offset = 0;
//             objectBufferInfo.range = sizeof(ObjectGpu) * kMaxObjects;
//
//             VkDescriptorImageInfo shadowImageInfo = {};
//             shadowImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
//             shadowImageInfo.imageView = shadowImageView_;
//             shadowImageInfo.sampler = shadowSampler_;
//
//             std::array<VkWriteDescriptorSet, 3> writes = {};
//             writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//             writes[0].dstSet = descriptorSets_[frame];
//             writes[0].dstBinding = 0;
//             writes[0].descriptorCount = 1;
//             writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//             writes[0].pBufferInfo = &frameBufferInfo;
//
//             writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//             writes[1].dstSet = descriptorSets_[frame];
//             writes[1].dstBinding = 1;
//             writes[1].descriptorCount = 1;
//             writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//             writes[1].pBufferInfo = &objectBufferInfo;
//
//             writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//             writes[2].dstSet = descriptorSets_[frame];
//             writes[2].dstBinding = 2;
//             writes[2].descriptorCount = 1;
//             writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//             writes[2].pImageInfo = &shadowImageInfo;
//
//             vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
//         }
//     }
//
//     void CreateFramebuffers()
//     {
//         swapChainFramebuffers_.resize(swapChainImageViews_.size());
//         for (size_t index = 0; index < swapChainImageViews_.size(); ++index)
//         {
//             const std::array<VkImageView, 2> attachments = { swapChainImageViews_[index], depthImageView_ };
//             VkFramebufferCreateInfo framebufferInfo = {};
//             framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//             framebufferInfo.renderPass = forwardRenderPass_;
//             framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
//             framebufferInfo.pAttachments = attachments.data();
//             framebufferInfo.width = swapChainExtent_.width;
//             framebufferInfo.height = swapChainExtent_.height;
//             framebufferInfo.layers = 1;
//             Check(vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapChainFramebuffers_[index]), "Failed to create swapchain framebuffer.");
//         }
//
//         VkFramebufferCreateInfo shadowFramebufferInfo = {};
//         shadowFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//         shadowFramebufferInfo.renderPass = shadowRenderPass_;
//         shadowFramebufferInfo.attachmentCount = 1;
//         shadowFramebufferInfo.pAttachments = &shadowImageView_;
//         shadowFramebufferInfo.width = kShadowMapSize;
//         shadowFramebufferInfo.height = kShadowMapSize;
//         shadowFramebufferInfo.layers = 1;
//         Check(vkCreateFramebuffer(device_, &shadowFramebufferInfo, nullptr, &shadowFramebuffer_), "Failed to create shadow framebuffer.");
//     }
//
//     void CreateSyncObjects()
//     {
//         VkSemaphoreCreateInfo semaphoreInfo = {};
//         semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
//
//         VkFenceCreateInfo fenceInfo = {};
//         fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
//         fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
//
//         for (uint32_t frame = 0; frame < kMaxFramesInFlight; ++frame)
//         {
//             Check(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[frame]), "Failed to create image semaphore.");
//             Check(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[frame]), "Failed to create render semaphore.");
//             Check(vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[frame]), "Failed to create frame fence.");
//         }
//     }
//
//     SceneMatrices UpdateFrameData(const std::vector<RenderItem>& scene)
//     {
//         const XMVECTOR cameraPosition = XMVectorSet(0.0f, 2.7f, -7.0f, 1.0f);
//         const XMVECTOR cameraTarget = XMVectorSet(0.0f, 0.2f, 0.0f, 1.0f);
//         const XMMATRIX view = XMMatrixLookAtLH(cameraPosition, cameraTarget, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
//         const float aspect = static_cast<float>(std::max<uint32_t>(swapChainExtent_.width, 1)) /
//             static_cast<float>(std::max<uint32_t>(swapChainExtent_.height, 1));
//         const XMMATRIX projection = MakeVulkanProjection(XMMatrixPerspectiveFovLH(XMConvertToRadians(55.0f), aspect, 0.1f, 100.0f));
//
//         const XMVECTOR lightDirection = XMVector3Normalize(XMVectorSet(-0.42f, -0.82f, 0.38f, 0.0f));
//         const XMVECTOR lightTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
//         const XMVECTOR lightPosition = XMVectorSubtract(lightTarget, XMVectorScale(lightDirection, 11.0f));
//         const XMMATRIX lightView = XMMatrixLookAtLH(lightPosition, lightTarget, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
//         const XMMATRIX lightProjection = MakeVulkanProjection(XMMatrixOrthographicLH(15.0f, 15.0f, 0.1f, 30.0f));
//
//         XMFLOAT3 cameraPositionFloat = {};
//         XMFLOAT3 lightDirectionFloat = {};
//         XMStoreFloat3(&cameraPositionFloat, cameraPosition);
//         XMStoreFloat3(&lightDirectionFloat, lightDirection);
//
//         FrameGpu frameData = {};
//         frameData.cameraPositionExposure = XMFLOAT4(cameraPositionFloat.x, cameraPositionFloat.y, cameraPositionFloat.z, 1.05f);
//         frameData.lightDirectionIntensity = XMFLOAT4(lightDirectionFloat.x, lightDirectionFloat.y, lightDirectionFloat.z, 7.5f);
//         frameData.lightColorAmbient = XMFLOAT4(1.0f, 0.95f, 0.86f, 0.045f);
//         frameData.shadowTexelPadding = XMFLOAT4(1.0f / kShadowMapSize, 1.0f / kShadowMapSize, 0.0f, 0.0f);
//         std::memcpy(frameUniformMapped_[currentFrame_], &frameData, sizeof(frameData));
//
//         const SceneMatrices matrices = { view * projection, lightView * lightProjection };
//         ObjectGpu* objectData = static_cast<ObjectGpu*>(objectMapped_[currentFrame_]);
//         for (size_t index = 0; index < scene.size(); ++index)
//         {
//             XMStoreFloat4x4(&objectData[index].world, scene[index].world);
//             XMStoreFloat4x4(&objectData[index].worldViewProjection, scene[index].world * matrices.viewProjection);
//             XMStoreFloat4x4(&objectData[index].lightWorldViewProjection, scene[index].world * matrices.lightViewProjection);
//             objectData[index].albedoMetallic = XMFLOAT4(
//                 scene[index].material.albedo.x,
//                 scene[index].material.albedo.y,
//                 scene[index].material.albedo.z,
//                 scene[index].material.metallic);
//             objectData[index].roughnessPadding = XMFLOAT4(scene[index].material.roughness, 0.0f, 0.0f, 0.0f);
//         }
//
//         return matrices;
//     }
//
//     std::vector<RenderItem> BuildScene(float timeSeconds)
//     {
//         std::vector<RenderItem> scene;
//         scene.reserve(6);
//
//         scene.push_back({ &planeMesh_, XMMatrixTranslation(0.0f, -1.25f, 0.0f), { XMFLOAT3(0.58f, 0.61f, 0.56f), 0.0f, 0.68f }, false });
//         scene.push_back({
//             &cubeMesh_,
//             XMMatrixScaling(0.85f, 0.85f, 0.85f) *
//                 XMMatrixRotationRollPitchYaw(timeSeconds * 0.7f, timeSeconds * 1.1f, 0.0f) *
//                 XMMatrixTranslation(0.0f, -0.25f, 0.0f),
//             { XMFLOAT3(0.86f, 0.17f, 0.13f), 0.0f, 0.36f },
//             true
//         });
//         scene.push_back({
//             &sphereMesh_,
//             XMMatrixScaling(0.55f, 0.55f, 0.55f) *
//                 XMMatrixTranslation(std::cosf(timeSeconds * 0.9f) * 2.4f, -0.55f + std::sinf(timeSeconds * 1.7f) * 0.25f, std::sinf(timeSeconds * 0.9f) * 2.4f),
//             { XMFLOAT3(0.95f, 0.67f, 0.18f), 1.0f, 0.18f },
//             true
//         });
//         scene.push_back({
//             &sphereMesh_,
//             XMMatrixScaling(0.45f, 0.45f, 0.45f) *
//                 XMMatrixTranslation(std::cosf(timeSeconds * 1.35f + 1.8f) * 3.1f, -0.65f, std::sinf(timeSeconds * 1.35f + 1.8f) * 1.6f),
//             { XMFLOAT3(0.18f, 0.49f, 0.92f), 0.0f, 0.12f },
//             true
//         });
//         scene.push_back({
//             &cubeMesh_,
//             XMMatrixScaling(0.42f, 0.42f, 0.42f) *
//                 XMMatrixRotationRollPitchYaw(timeSeconds * 1.9f, -timeSeconds * 0.8f, timeSeconds * 0.55f) *
//                 XMMatrixTranslation(-2.2f, -0.55f + std::sinf(timeSeconds * 2.2f) * 0.45f, -1.7f),
//             { XMFLOAT3(0.36f, 0.82f, 0.48f), 0.0f, 0.78f },
//             true
//         });
//         scene.push_back({
//             &cubeMesh_,
//             XMMatrixScaling(0.35f, 0.9f, 0.35f) *
//                 XMMatrixRotationY(timeSeconds * 0.5f) *
//                 XMMatrixTranslation(2.6f, -0.35f, -1.4f),
//             { XMFLOAT3(0.62f, 0.42f, 0.92f), 0.65f, 0.28f },
//             true
//         });
//
//         return scene;
//     }
//
//     void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, const std::vector<RenderItem>& scene, const SceneMatrices&)
//     {
//         VkCommandBufferBeginInfo beginInfo = {};
//         beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//         Check(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin command buffer.");
//
//         VkClearValue shadowClear = {};
//         shadowClear.depthStencil.depth = 1.0f;
//
//         VkRenderPassBeginInfo shadowPassInfo = {};
//         shadowPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//         shadowPassInfo.renderPass = shadowRenderPass_;
//         shadowPassInfo.framebuffer = shadowFramebuffer_;
//         shadowPassInfo.renderArea.extent = { kShadowMapSize, kShadowMapSize };
//         shadowPassInfo.clearValueCount = 1;
//         shadowPassInfo.pClearValues = &shadowClear;
//         vkCmdBeginRenderPass(commandBuffer, &shadowPassInfo, VK_SUBPASS_CONTENTS_INLINE);
//
//         VkViewport shadowViewport = {};
//         shadowViewport.width = static_cast<float>(kShadowMapSize);
//         shadowViewport.height = static_cast<float>(kShadowMapSize);
//         shadowViewport.maxDepth = 1.0f;
//         VkRect2D shadowScissor = {};
//         shadowScissor.extent = { kShadowMapSize, kShadowMapSize };
//         vkCmdSetViewport(commandBuffer, 0, 1, &shadowViewport);
//         vkCmdSetScissor(commandBuffer, 0, 1, &shadowScissor);
//         vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline_);
//         vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1, &descriptorSets_[currentFrame_], 0, nullptr);
//
//         for (uint32_t index = 0; index < scene.size(); ++index)
//         {
//             if (!scene[index].castsShadow)
//             {
//                 continue;
//             }
//
//             DrawItem(commandBuffer, scene[index], index);
//         }
//
//         vkCmdEndRenderPass(commandBuffer);
//
//         std::array<VkClearValue, 2> clearValues = {};
//         clearValues[0].color = { 0.04f, 0.055f, 0.075f, 1.0f };
//         clearValues[1].depthStencil = { 1.0f, 0 };
//
//         VkRenderPassBeginInfo forwardPassInfo = {};
//         forwardPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//         forwardPassInfo.renderPass = forwardRenderPass_;
//         forwardPassInfo.framebuffer = swapChainFramebuffers_[imageIndex];
//         forwardPassInfo.renderArea.extent = swapChainExtent_;
//         forwardPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
//         forwardPassInfo.pClearValues = clearValues.data();
//         vkCmdBeginRenderPass(commandBuffer, &forwardPassInfo, VK_SUBPASS_CONTENTS_INLINE);
//
//         VkViewport viewport = {};
//         viewport.width = static_cast<float>(swapChainExtent_.width);
//         viewport.height = static_cast<float>(swapChainExtent_.height);
//         viewport.maxDepth = 1.0f;
//         VkRect2D scissor = {};
//         scissor.extent = swapChainExtent_;
//         vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
//         vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
//         vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, forwardPipeline_);
//         vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1, &descriptorSets_[currentFrame_], 0, nullptr);
//
//         for (uint32_t index = 0; index < scene.size(); ++index)
//         {
//             DrawItem(commandBuffer, scene[index], index);
//         }
//
//         vkCmdEndRenderPass(commandBuffer);
//         Check(vkEndCommandBuffer(commandBuffer), "Failed to record command buffer.");
//     }
//
//     void DrawItem(VkCommandBuffer commandBuffer, const RenderItem& item, uint32_t objectIndex)
//     {
//         const VkBuffer vertexBuffers[] = { item.mesh->vertexBuffer };
//         const VkDeviceSize offsets[] = { 0 };
//         vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
//         vkCmdBindIndexBuffer(commandBuffer, item.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
//         vkCmdPushConstants(commandBuffer, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &objectIndex);
//         vkCmdDrawIndexed(commandBuffer, item.mesh->indexCount, 1, 0, 0, 0);
//     }
//
//     void RecreateSwapChain()
//     {
//         if (viewportWidth_ == 0 || viewportHeight_ == 0)
//         {
//             return;
//         }
//
//         vkDeviceWaitIdle(device_);
//         CleanupSwapChain();
//         CreateSwapChain();
//         CreateDepthResources();
//         CreateForwardRenderPass();
//         CreateForwardPipeline();
//         CreateFramebuffers();
//     }
//
//     void CleanupSwapChain()
//     {
//         for (VkFramebuffer framebuffer : swapChainFramebuffers_)
//         {
//             vkDestroyFramebuffer(device_, framebuffer, nullptr);
//         }
//         swapChainFramebuffers_.clear();
//
//         if (forwardPipeline_ != VK_NULL_HANDLE)
//         {
//             vkDestroyPipeline(device_, forwardPipeline_, nullptr);
//             forwardPipeline_ = VK_NULL_HANDLE;
//         }
//         if (forwardRenderPass_ != VK_NULL_HANDLE)
//         {
//             vkDestroyRenderPass(device_, forwardRenderPass_, nullptr);
//             forwardRenderPass_ = VK_NULL_HANDLE;
//         }
//         if (depthImageView_ != VK_NULL_HANDLE)
//         {
//             vkDestroyImageView(device_, depthImageView_, nullptr);
//             depthImageView_ = VK_NULL_HANDLE;
//         }
//         if (depthImage_ != VK_NULL_HANDLE)
//         {
//             vkDestroyImage(device_, depthImage_, nullptr);
//             depthImage_ = VK_NULL_HANDLE;
//         }
//         if (depthImageMemory_ != VK_NULL_HANDLE)
//         {
//             vkFreeMemory(device_, depthImageMemory_, nullptr);
//             depthImageMemory_ = VK_NULL_HANDLE;
//         }
//
//         for (VkImageView imageView : swapChainImageViews_)
//         {
//             vkDestroyImageView(device_, imageView, nullptr);
//         }
//         swapChainImageViews_.clear();
//         swapChainImages_.clear();
//
//         if (swapChain_ != VK_NULL_HANDLE)
//         {
//             vkDestroySwapchainKHR(device_, swapChain_, nullptr);
//             swapChain_ = VK_NULL_HANDLE;
//         }
//     }
//
//     VkFormat FindDepthFormat() const
//     {
//         const std::array<VkFormat, 3> candidates = {
//             VK_FORMAT_D32_SFLOAT,
//             VK_FORMAT_D32_SFLOAT_S8_UINT,
//             VK_FORMAT_D24_UNORM_S8_UINT
//         };
//
//         for (VkFormat format : candidates)
//         {
//             VkFormatProperties properties = {};
//             vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &properties);
//             if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
//             {
//                 return format;
//             }
//         }
//
//         throw std::runtime_error("Failed to find a supported depth format.");
//     }
//
//     VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
//     {
//         VkImageViewCreateInfo createInfo = {};
//         createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//         createInfo.image = image;
//         createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//         createInfo.format = format;
//         createInfo.subresourceRange.aspectMask = aspectFlags;
//         createInfo.subresourceRange.baseMipLevel = 0;
//         createInfo.subresourceRange.levelCount = 1;
//         createInfo.subresourceRange.baseArrayLayer = 0;
//         createInfo.subresourceRange.layerCount = 1;
//
//         VkImageView imageView = VK_NULL_HANDLE;
//         Check(vkCreateImageView(device_, &createInfo, nullptr, &imageView), "Failed to create image view.");
//         return imageView;
//     }
//
//     void CreateImage(
//         uint32_t width,
//         uint32_t height,
//         VkFormat format,
//         VkImageTiling tiling,
//         VkImageUsageFlags usage,
//         VkMemoryPropertyFlags properties,
//         VkImage& image,
//         VkDeviceMemory& imageMemory)
//     {
//         VkImageCreateInfo imageInfo = {};
//         imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//         imageInfo.imageType = VK_IMAGE_TYPE_2D;
//         imageInfo.extent.width = width;
//         imageInfo.extent.height = height;
//         imageInfo.extent.depth = 1;
//         imageInfo.mipLevels = 1;
//         imageInfo.arrayLayers = 1;
//         imageInfo.format = format;
//         imageInfo.tiling = tiling;
//         imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//         imageInfo.usage = usage;
//         imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
//         imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//         Check(vkCreateImage(device_, &imageInfo, nullptr, &image), "Failed to create image.");
//
//         VkMemoryRequirements memoryRequirements = {};
//         vkGetImageMemoryRequirements(device_, image, &memoryRequirements);
//
//         VkMemoryAllocateInfo allocateInfo = {};
//         allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//         allocateInfo.allocationSize = memoryRequirements.size;
//         allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties);
//         Check(vkAllocateMemory(device_, &allocateInfo, nullptr, &imageMemory), "Failed to allocate image memory.");
//         Check(vkBindImageMemory(device_, image, imageMemory, 0), "Failed to bind image memory.");
//     }
//
//     void CreateBuffer(
//         VkDeviceSize size,
//         VkBufferUsageFlags usage,
//         VkMemoryPropertyFlags properties,
//         VkBuffer& buffer,
//         VkDeviceMemory& bufferMemory)
//     {
//         VkBufferCreateInfo bufferInfo = {};
//         bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//         bufferInfo.size = size;
//         bufferInfo.usage = usage;
//         bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//         Check(vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer), "Failed to create buffer.");
//
//         VkMemoryRequirements memoryRequirements = {};
//         vkGetBufferMemoryRequirements(device_, buffer, &memoryRequirements);
//
//         VkMemoryAllocateInfo allocateInfo = {};
//         allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//         allocateInfo.allocationSize = memoryRequirements.size;
//         allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties);
//         Check(vkAllocateMemory(device_, &allocateInfo, nullptr, &bufferMemory), "Failed to allocate buffer memory.");
//         Check(vkBindBufferMemory(device_, buffer, bufferMemory, 0), "Failed to bind buffer memory.");
//     }
//
//     void CreateDeviceLocalBuffer(const void* data, size_t size, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& memory)
//     {
//         VkBuffer stagingBuffer = VK_NULL_HANDLE;
//         VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
//         CreateBuffer(
//             size,
//             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//             stagingBuffer,
//             stagingMemory);
//
//         void* mapped = nullptr;
//         vkMapMemory(device_, stagingMemory, 0, size, 0, &mapped);
//         std::memcpy(mapped, data, size);
//         vkUnmapMemory(device_, stagingMemory);
//
//         CreateBuffer(
//             size,
//             VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
//             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//             buffer,
//             memory);
//         CopyBuffer(stagingBuffer, buffer, size);
//
//         vkDestroyBuffer(device_, stagingBuffer, nullptr);
//         vkFreeMemory(device_, stagingMemory, nullptr);
//     }
//
//     void CopyBuffer(VkBuffer source, VkBuffer destination, VkDeviceSize size)
//     {
//         VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
//         VkBufferCopy copyRegion = {};
//         copyRegion.size = size;
//         vkCmdCopyBuffer(commandBuffer, source, destination, 1, &copyRegion);
//         EndSingleTimeCommands(commandBuffer);
//     }
//
//     VkCommandBuffer BeginSingleTimeCommands()
//     {
//         VkCommandBufferAllocateInfo allocateInfo = {};
//         allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//         allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//         allocateInfo.commandPool = commandPool_;
//         allocateInfo.commandBufferCount = 1;
//
//         VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
//         vkAllocateCommandBuffers(device_, &allocateInfo, &commandBuffer);
//
//         VkCommandBufferBeginInfo beginInfo = {};
//         beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//         beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//         vkBeginCommandBuffer(commandBuffer, &beginInfo);
//         return commandBuffer;
//     }
//
//     void EndSingleTimeCommands(VkCommandBuffer commandBuffer)
//     {
//         vkEndCommandBuffer(commandBuffer);
//
//         VkSubmitInfo submitInfo = {};
//         submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//         submitInfo.commandBufferCount = 1;
//         submitInfo.pCommandBuffers = &commandBuffer;
//         vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
//         vkQueueWaitIdle(graphicsQueue_);
//         vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
//     }
//
//     uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
//     {
//         for (uint32_t index = 0; index < memoryProperties_.memoryTypeCount; ++index)
//         {
//             if ((typeFilter & (1u << index)) &&
//                 (memoryProperties_.memoryTypes[index].propertyFlags & properties) == properties)
//             {
//                 return index;
//             }
//         }
//
//         throw std::runtime_error("Failed to find suitable memory type.");
//     }
//
//     void DestroyMesh(Mesh& mesh)
//     {
//         if (mesh.vertexBuffer != VK_NULL_HANDLE)
//         {
//             vkDestroyBuffer(device_, mesh.vertexBuffer, nullptr);
//         }
//         if (mesh.vertexMemory != VK_NULL_HANDLE)
//         {
//             vkFreeMemory(device_, mesh.vertexMemory, nullptr);
//         }
//         if (mesh.indexBuffer != VK_NULL_HANDLE)
//         {
//             vkDestroyBuffer(device_, mesh.indexBuffer, nullptr);
//         }
//         if (mesh.indexMemory != VK_NULL_HANDLE)
//         {
//             vkFreeMemory(device_, mesh.indexMemory, nullptr);
//         }
//         mesh = {};
//     }
//
//     bool initialized_ = false;
//     bool framebufferResized_ = false;
//     HWND hwnd_ = nullptr;
//     uint32_t viewportWidth_ = 1;
//     uint32_t viewportHeight_ = 1;
//     uint32_t currentFrame_ = 0;
//     std::wstring shaderDirectory_;
//
//     VkInstance instance_ = VK_NULL_HANDLE;
//     VkSurfaceKHR surface_ = VK_NULL_HANDLE;
//     VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
//     VkDevice device_ = VK_NULL_HANDLE;
//     VkQueue graphicsQueue_ = VK_NULL_HANDLE;
//     VkQueue presentQueue_ = VK_NULL_HANDLE;
//     QueueFamilyIndices queueFamilies_ = {};
//     VkPhysicalDeviceMemoryProperties memoryProperties_ = {};
//
//     VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;
//     std::vector<VkImage> swapChainImages_;
//     std::vector<VkImageView> swapChainImageViews_;
//     std::vector<VkFramebuffer> swapChainFramebuffers_;
//     VkFormat swapChainFormat_ = VK_FORMAT_UNDEFINED;
//     VkExtent2D swapChainExtent_ = {};
//
//     VkImage depthImage_ = VK_NULL_HANDLE;
//     VkDeviceMemory depthImageMemory_ = VK_NULL_HANDLE;
//     VkImageView depthImageView_ = VK_NULL_HANDLE;
//     VkFormat depthFormat_ = VK_FORMAT_UNDEFINED;
//
//     VkImage shadowImage_ = VK_NULL_HANDLE;
//     VkDeviceMemory shadowImageMemory_ = VK_NULL_HANDLE;
//     VkImageView shadowImageView_ = VK_NULL_HANDLE;
//     VkSampler shadowSampler_ = VK_NULL_HANDLE;
//     VkFormat shadowFormat_ = VK_FORMAT_D32_SFLOAT;
//     VkFramebuffer shadowFramebuffer_ = VK_NULL_HANDLE;
//
//     VkRenderPass forwardRenderPass_ = VK_NULL_HANDLE;
//     VkRenderPass shadowRenderPass_ = VK_NULL_HANDLE;
//     VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
//     VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
//     VkPipeline forwardPipeline_ = VK_NULL_HANDLE;
//     VkPipeline shadowPipeline_ = VK_NULL_HANDLE;
//     VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
//     std::array<VkDescriptorSet, kMaxFramesInFlight> descriptorSets_ = {};
//
//     VkCommandPool commandPool_ = VK_NULL_HANDLE;
//     std::array<VkCommandBuffer, kMaxFramesInFlight> commandBuffers_ = {};
//     std::array<VkSemaphore, kMaxFramesInFlight> imageAvailableSemaphores_ = {};
//     std::array<VkSemaphore, kMaxFramesInFlight> renderFinishedSemaphores_ = {};
//     std::array<VkFence, kMaxFramesInFlight> inFlightFences_ = {};
//
//     std::array<VkBuffer, kMaxFramesInFlight> frameUniformBuffers_ = {};
//     std::array<VkDeviceMemory, kMaxFramesInFlight> frameUniformMemories_ = {};
//     std::array<void*, kMaxFramesInFlight> frameUniformMapped_ = {};
//     std::array<VkBuffer, kMaxFramesInFlight> objectBuffers_ = {};
//     std::array<VkDeviceMemory, kMaxFramesInFlight> objectMemories_ = {};
//     std::array<void*, kMaxFramesInFlight> objectMapped_ = {};
//
//     Mesh cubeMesh_;
//     Mesh planeMesh_;
//     Mesh sphereMesh_;
// };
//
// ForwardRenderer gRenderer;
//
// LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
// {
//     switch (message)
//     {
//     case WM_SIZE:
//         gRenderer.Resize(LOWORD(lParam), HIWORD(lParam));
//         return 0;
//
//     case WM_DESTROY:
//         PostQuitMessage(0);
//         return 0;
//
//     default:
//         return DefWindowProc(hwnd, message, wParam, lParam);
//     }
// }
//
// int RunApplication(HINSTANCE instance, int commandShow)
// {
//     try
//     {
//         WNDCLASSEXW windowClass = {};
//         windowClass.cbSize = sizeof(windowClass);
//         windowClass.style = CS_HREDRAW | CS_VREDRAW;
//         windowClass.lpfnWndProc = WindowProc;
//         windowClass.hInstance = instance;
//         windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
//         windowClass.lpszClassName = kWindowClassName;
//         RegisterClassExW(&windowClass);
//
//         RECT windowRect = { 0, 0, static_cast<LONG>(kInitialWidth), static_cast<LONG>(kInitialHeight) };
//         AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
//
//         HWND hwnd = CreateWindowExW(
//             0,
//             kWindowClassName,
//             L"Vulkan Forward Renderer - PBR Shadow Scene",
//             WS_OVERLAPPEDWINDOW,
//             CW_USEDEFAULT,
//             CW_USEDEFAULT,
//             windowRect.right - windowRect.left,
//             windowRect.bottom - windowRect.top,
//             nullptr,
//             nullptr,
//             instance,
//             nullptr);
//
//         if (!hwnd)
//         {
//             return 1;
//         }
//
//         gRenderer.Initialize(hwnd, kInitialWidth, kInitialHeight);
//         ShowWindow(hwnd, commandShow);
//
//         const auto startTime = std::chrono::steady_clock::now();
//         MSG message = {};
//         while (message.message != WM_QUIT)
//         {
//             if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
//             {
//                 TranslateMessage(&message);
//                 DispatchMessageW(&message);
//                 continue;
//             }
//
//             const auto now = std::chrono::steady_clock::now();
//             const float seconds = std::chrono::duration<float>(now - startTime).count();
//             gRenderer.Render(seconds);
//         }
//
//         gRenderer.Shutdown();
//         return static_cast<int>(message.wParam);
//     }
//     catch (const std::exception& exception)
//     {
//         gRenderer.Shutdown();
//         MessageBoxA(nullptr, exception.what(), "Vulkan renderer failed", MB_ICONERROR | MB_OK);
//         return 1;
//     }
// }
// }
//
// int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int commandShow)
// {
//     return RunApplication(instance, commandShow);
// }
//
// int main()
// {
//     return RunApplication(GetModuleHandleW(nullptr), SW_SHOWDEFAULT);
// }
//


// 最核心差距：Vulkan 是显式 API，DX11 是隐式高层 API。
// •
// 按注释里的 Vulkan 实现意图对比，Vulkan 约 1897 行，DX11 文件 CG2PBR/CG2PBR.cpp:82 的实际实现约 797 行，差距主要来自 API 责任边界。
// •
// DX11 一次 D3D11CreateDeviceAndSwapChain 就拿到 device、context、swapchain：CG2PBR/CG2PBR.cpp:154。
// •
// Vulkan 要手动拆成 instance、surface、物理设备选择、queue family、逻辑设备、swapchain、render pass、pipeline、descriptor、framebuffer、同步对象：CG2PBR/vulkan.cpp:172。
// •
// DX11 的资源创建也更短，比如 vertex/index buffer 直接 CreateBuffer：CG2PBR/CG2PBR.cpp:511；Vulkan 要创建 buffer、查 memory type、分配 memory、bind，上传还要 staging buffer：CG2PBR/vulkan.cpp:1613。
// •
// DX11 的绘制是 immediate context 直接改状态并 draw：CG2PBR/CG2PBR.cpp:768；Vulkan 要录 command buffer、提交 queue、管理 semaphore/fence：CG2PBR/vulkan.cpp:306 和 CG2PBR/vulkan.cpp:1389。
// 一句话：DX11 替你隐藏了同步、内存、资源状态、队列提交和部分 pipeline 管理；Vulkan 把这些全部显式交给你，所以代码量自然暴涨。