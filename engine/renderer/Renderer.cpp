#define VMA_IMPLEMENTATION
#include "Renderer.hpp"
#include <aixlog.hpp>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <array>
#include <vector>
#include <memory>
#include <cstring>
#include <algorithm>
#include <string>
#include <string_view>
#include <fstream>
#include "Types.hpp"
#include "Window.hpp"

static VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, char const*, char const*, void*);
static auto vkErrorString(VkResult result) -> char const*;
static auto vkCheck(VkResult result) -> void;
static auto readFile(std::string_view filepath) -> std::vector<char>;

auto constexpr static g_framesInFlight = u32(2);

namespace vk
{
    struct QueueHandle
    {
        VkQueue queue;
        u32 index, family;
    };

    class Context
    {
    public:
        auto createInstance()       -> void;
        auto createPhysicalDevice() -> void;
        auto createDevice()         -> void;
        auto createAllocator()      -> void;
        auto createSwapchain()      -> void;
        auto teardown()             -> void;

    public:
        using ImageVector         = std::vector<Image>;
        using CommandPoolVector   = std::vector<VkCommandPool>;
        using CommandBufferVector = std::vector<VkCommandBuffer>;
        using PipelineVector      = std::vector<VkPipeline>;
        using SemaphoreArray      = std::array<VkSemaphore, g_framesInFlight>;
        using FenceArray          = std::array<VkFence, g_framesInFlight>;

    public:
        VmaAllocator             allocator;
        VkInstance               instance;
        VkDebugReportCallbackEXT debugCallback;
        VkSurfaceKHR             surface;
        VkPhysicalDevice         physicalDevice;
        VkDevice                 device;
        VkSwapchainKHR           swapchain;
        VkExtent2D               extent;
        VkFormat                 colorFormat;
        VkPipelineLayout         pipelineLayout;
        QueueHandle              graphicsQueue;
        PipelineVector           pipelines;
        CommandPoolVector        commandPools;
        CommandBufferVector      commandBuffers;
        SemaphoreArray           presentSemaphores;
        SemaphoreArray           renderSemaphores;
        FenceArray               fences;
        ImageVector              swapchainImages;
        Pipeline*                currentPipeline;
        u32                      imageIndex;
        u32                      frameIndex;
    };

    auto static g_context = Context();

    Image::~Image()
    {
        auto device{ g_context.device };

        if (device && allocation)
        {
            if (handleView) vkDestroyImageView(device, handleView, nullptr);
        } 
    }

    auto Image::loadFromSwapchain(VkImage image, VkImageView imageView, Format format) -> void
    {
        this->handle = image;
        this->handleView = imageView;
        this->width = g_context.extent.width;
        this->height = g_context.extent.height;
        this->layout = ImageLayout::eUndefined;
        this->aspect = Aspect::eColor;
        this->usage = ImageUsage::eColorAttachment | ImageUsage::eTransferDst;
        this->allocation = nullptr;
        this->format = format;
        this->layerCount = 1;
    }

    auto init() -> void
    {
        std::memset(&g_context, 0, sizeof(Context));
        g_context.createInstance();
        g_context.createPhysicalDevice();
        g_context.createDevice();
    }

    auto teardown() -> void
    {
        g_context.teardown();
    }

    auto acquire() -> void
    {
        auto static previousWidth { g_context.extent.width  };
        auto static previousHeight{ g_context.extent.height };

        vkCheck(vkWaitForFences(g_context.device, 1, &g_context.fences[g_context.frameIndex], true, ~0ull));

        if (previousWidth  != Window::GetWidth() ||
            previousHeight != Window::GetHeight())
        {
            vkDeviceWaitIdle(g_context.device);
            previousWidth  = Window::GetWidth();
            previousHeight = Window::GetHeight();

            g_context.createSwapchain();
        }

        auto result{ vkAcquireNextImageKHR(g_context.device, g_context.swapchain, ~0ull, g_context.renderSemaphores[g_context.frameIndex], nullptr, &g_context.imageIndex) };

        switch (result)
        {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            vkDeviceWaitIdle(g_context.device);
            g_context.createSwapchain();
        default:
            vkCheck(result);
            break;
        }
    }

    auto present() -> void
    {
        vkCheck(vkResetFences(g_context.device, 1, &g_context.fences[g_context.frameIndex]));

        VkPipelineStageFlags constexpr stage{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo;
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.commandBufferCount = 1;
        submitInfo.pWaitDstStageMask = &stage;
        submitInfo.pWaitSemaphores = &g_context.renderSemaphores[g_context.frameIndex];
        submitInfo.pSignalSemaphores = &g_context.presentSemaphores[g_context.frameIndex];
        submitInfo.pCommandBuffers = &g_context.commandBuffers[g_context.imageIndex];
        vkCheck(vkQueueSubmit(g_context.graphicsQueue.queue, 1, &submitInfo, g_context.fences[g_context.frameIndex]));

        VkPresentInfoKHR presentInfo;
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.pImageIndices = &g_context.imageIndex;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &g_context.swapchain;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &g_context.presentSemaphores[g_context.frameIndex];
        presentInfo.pResults = nullptr;
        auto result{ vkQueuePresentKHR(g_context.graphicsQueue.queue, &presentInfo) };

        switch (result)
        {
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
            vkDeviceWaitIdle(g_context.device);
            g_context.createSwapchain();
            break;
        default:
            vkCheck(result);
            break;
        }

        g_context.frameIndex = (g_context.frameIndex + 1) % g_framesInFlight;
    }

    auto cmdBegin() -> void
    {
        vkResetCommandPool(g_context.device, g_context.commandPools[g_context.imageIndex], 0);

        VkCommandBufferBeginInfo beginInfo;
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pInheritanceInfo = nullptr;
        beginInfo.pNext = nullptr;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkCheck(vkBeginCommandBuffer(g_context.commandBuffers[g_context.imageIndex], &beginInfo));
    }

    auto cmdEnd() -> void
    {
        vkCheck(vkEndCommandBuffer(g_context.commandBuffers[g_context.imageIndex]));
    }

    auto cmdBeginPresent() -> void
    {
        cmdBarrier(g_context.swapchainImages[g_context.imageIndex], ImageLayout::eColorAttachment);
        cmdBeginRendering(g_context.swapchainImages[g_context.imageIndex]);
    }

    auto cmdEndPresent() -> void
    {
        cmdEndRendering();
        cmdBarrier(g_context.swapchainImages[g_context.imageIndex], ImageLayout::ePresent);
    }

    auto cmdBeginRendering(Image const& image) -> void
    {
        VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        colorAttachment.imageView = image.handleView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = { 0, 0, 0, 0 };

        VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.flags = 0;
        renderingInfo.viewMask = 0;
        renderingInfo.layerCount = 1;
        renderingInfo.renderArea.extent = VkExtent2D{ image.width, image.height };
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(g_context.commandBuffers[g_context.imageIndex], &renderingInfo);

        VkViewport viewport;
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;
        viewport.x = 0.f;
        viewport.y = static_cast<f32>(image.height);
        viewport.width  =  static_cast<f32>(image.width);
        viewport.height = -static_cast<f32>(image.height);

        VkRect2D scissor;
        scissor.offset = { };
        scissor.extent = { image.width, image.height };

        vkCmdSetViewport(g_context.commandBuffers[g_context.imageIndex], 0, 1, &viewport);
        vkCmdSetScissor(g_context.commandBuffers[g_context.imageIndex], 0, 1, &scissor);
    }

    auto cmdEndRendering() -> void
    {
        vkCmdEndRendering(g_context.commandBuffers[g_context.imageIndex]);
    }

    auto cmdBarrier(Image &image, ImageLayout newLayout) -> void
    {
        VkImageMemoryBarrier2 imageBarrier;
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarrier.pNext = nullptr;
        imageBarrier.oldLayout = static_cast<VkImageLayout>(image.layout);
        imageBarrier.newLayout = static_cast<VkImageLayout>(newLayout);
        imageBarrier.subresourceRange.aspectMask = image.aspect;
        imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        imageBarrier.subresourceRange.baseMipLevel = 0;
        imageBarrier.subresourceRange.baseArrayLayer = 0;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = g_context.swapchainImages[g_context.imageIndex].handle;

        switch (image.layout)
        {
        case ImageLayout::eUndefined:
            imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            break;
        case ImageLayout::eColorAttachment:
            switch (newLayout)
            {
            case ImageLayout::ePresent:
                imageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                imageBarrier.dstAccessMask = VK_ACCESS_2_NONE;
                imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            default:
                break;
            }
            break;
        case ImageLayout::ePresent:
            switch (newLayout)
            {
            case ImageLayout::eColorAttachment:
                imageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
                imageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }

        VkDependencyInfo dependency;
        dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.pNext = nullptr;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependency.bufferMemoryBarrierCount = 0;
        dependency.memoryBarrierCount = 0;
        dependency.imageMemoryBarrierCount = 1;
        dependency.pImageMemoryBarriers = &imageBarrier;
        vkCmdPipelineBarrier2(g_context.commandBuffers[g_context.imageIndex], &dependency);

        image.layout = newLayout;
    }

    auto cmdBindPipeline(Pipeline& pipeline) -> void
    {
        vkCmdBindPipeline(
            g_context.commandBuffers[g_context.imageIndex],
            static_cast<VkPipelineBindPoint>(pipeline.bindPoint),
            g_context.pipelines[pipeline.handleIndex]
        );

        g_context.currentPipeline = &pipeline;
    }

    auto cmdDraw(u32 vertexCount) -> void
    {
        vkCmdDraw(g_context.commandBuffers[g_context.imageIndex], vertexCount, 1, 0, 0);
    }

    auto createPipeline(PipelineConfig const& config) -> Pipeline
    {
        auto static currentHandleIndex{ u32(0) };
        auto pipeline{ VkPipeline(nullptr) };

        std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos(config.stages.size());
        std::vector<VkShaderModule> shaderModules(config.stages.size());

        for (u32 i = 0; i < config.stages.size(); ++i)
        {
            auto stage{ config.stages.begin() + i };
            auto code { readFile(stage->filepath) };

            if (code.empty())
            {
                throw std::runtime_error(std::string("Failed to load shader file: ") + stage->filepath);
            }

            VkShaderModuleCreateInfo shaderModuleCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
            shaderModuleCreateInfo.codeSize = code.size();
            shaderModuleCreateInfo.pCode = reinterpret_cast<u32 const*>(code.data());

            vkCheck(vkCreateShaderModule(g_context.device, &shaderModuleCreateInfo, nullptr, &shaderModules[i]));

            shaderStageCreateInfos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageCreateInfos[i].pNext = nullptr;
            shaderStageCreateInfos[i].flags = 0;
            shaderStageCreateInfos[i].stage = static_cast<VkShaderStageFlagBits>(stage->stage);
            shaderStageCreateInfos[i].module = shaderModules[i];
            shaderStageCreateInfos[i].pName = "main";
            shaderStageCreateInfos[i].pSpecializationInfo = nullptr;
        }

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.scissorCount  = 1;

        std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicStateCreateInfo.dynamicStateCount = static_cast<u32>(dynamicStates.size());
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCreateInfo.lineWidth = 1.0f;
        rasterizationStateCreateInfo.depthClampEnable = false;

        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState blendAttachmentState{};
        blendAttachmentState.blendEnable = false;
        blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.colorWriteMask  = VK_COLOR_COMPONENT_R_BIT;
        blendAttachmentState.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
        blendAttachmentState.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
        blendAttachmentState.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &blendAttachmentState;

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencilStateCreateInfo.depthTestEnable = false;
        depthStencilStateCreateInfo.depthWriteEnable = false;
        depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilStateCreateInfo.depthBoundsTestEnable = false;
        depthStencilStateCreateInfo.stencilTestEnable = false;

        VkPipelineRenderingCreateInfo renderingCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        renderingCreateInfo.colorAttachmentCount = 1;
        renderingCreateInfo.pColorAttachmentFormats = &g_context.colorFormat;
        renderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
        renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineCreateInfo.pNext = (void*)&renderingCreateInfo;
        pipelineCreateInfo.stageCount = static_cast<u32>(shaderStageCreateInfos.size());
        pipelineCreateInfo.pStages = shaderStageCreateInfos.data();
        pipelineCreateInfo.layout = g_context.pipelineLayout;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        vkCheck(vkCreateGraphicsPipelines(g_context.device, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline));

        g_context.pipelines.emplace_back(pipeline);

        for (auto shaderModule : shaderModules)
        {
            if (shaderModule) vkDestroyShaderModule(g_context.device, shaderModule, nullptr);
        }

        return Pipeline{
            .handleIndex = currentHandleIndex++,
            .bindPoint = config.bindPoint
        };
    }

    auto Context::createInstance() -> void
    {
        vkCheck(volkInitialize());

        u32 extensionCount;
        auto extensionNames{ glfwGetRequiredInstanceExtensions(&extensionCount) };

        std::vector<char const*> extensions(extensionNames, extensionNames + extensionCount);
        std::vector<char const*> layers;

        #ifndef NDEBUG
        constexpr auto enableValidation{ true };
        #else
        constexpr auto enableValidation{ false };
        #endif

        if constexpr (enableValidation)
        {
            layers.emplace_back("VK_LAYER_KHRONOS_validation");
            extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        VkValidationFeatureEnableEXT enabledValidationFeatures[] =
        {
            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
            VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT // Annoying thing, will be disabled later
        };

        VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
        validationFeatures.enabledValidationFeatureCount = 1;
        validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;

        VkApplicationInfo applicationInfo;
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pNext = nullptr;
        applicationInfo.apiVersion = VK_API_VERSION_1_3;
        applicationInfo.pEngineName = "Light Frame";
        applicationInfo.pApplicationName = "Light Frame Application";
        applicationInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        applicationInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);

        VkInstanceCreateInfo instanceCreateInfo;
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pNext = nullptr;
        instanceCreateInfo.flags = 0;
        instanceCreateInfo.pApplicationInfo = &applicationInfo;
        instanceCreateInfo.enabledLayerCount = static_cast<u32>(layers.size());
        instanceCreateInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
        instanceCreateInfo.ppEnabledLayerNames = layers.data();

        if constexpr (enableValidation)
        {
            instanceCreateInfo.pNext = &validationFeatures;
        }

        vkCheck(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
        volkLoadInstanceOnly(instance);

        if constexpr (enableValidation)
        {
            VkDebugReportCallbackCreateInfoEXT reportCallbackInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
            reportCallbackInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
            reportCallbackInfo.pfnCallback = debugReportCallback;

            vkCheck(vkCreateDebugReportCallbackEXT(instance, &reportCallbackInfo, nullptr, &debugCallback));
            LOG(INFO, "Vulkan Context") << "Enabled validation layers\n";
        }

        vkCheck(glfwCreateWindowSurface(instance, (GLFWwindow*)Window::GetHandle(), nullptr, &surface));

        LOG(INFO, "Vulkan Context") << "Created instance\n";
    }

    auto Context::createPhysicalDevice() -> void
    {
        u32 deviceCount;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

        if (physicalDevices.empty())
        {
            throw std::runtime_error("No available graphics cards");
        }

        for (auto const gpu : physicalDevices)
        {
            VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
            VkPhysicalDeviceProperties2 properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            VkPhysicalDeviceFeatures2 features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

            features.pNext = &meshShaderFeatures;

            vkGetPhysicalDeviceProperties2(gpu, &properties);
            vkGetPhysicalDeviceFeatures2(gpu, &features);

            if (properties.properties.apiVersion < VK_API_VERSION_1_3) continue;
            if (features.features.geometryShader == false)             continue;

            physicalDevice = gpu;

            if (meshShaderFeatures.meshShader) break;
        }

        LOG(INFO, "Vulkan Context") << "Created physical device\n";
    }

    auto Context::createDevice() -> void
    {
        u32 propertyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propertyCount, nullptr);
        std::vector<VkQueueFamilyProperties> properties(propertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propertyCount, properties.data());

        for (u32 i = 0; i < propertyCount; ++i)
        {
            if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
                properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT &&
                glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, i))
            {
                graphicsQueue.family = i;
                graphicsQueue.index = 0;
                break;
            }
        }

        const f32 queuePriority = 1.f;

        VkDeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.pNext = nullptr;
        queueCreateInfo.flags = 0;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.queueFamilyIndex = graphicsQueue.family;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceVulkan11Features vulkan11Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        VkPhysicalDeviceVulkan12Features vulkan12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        vulkan12Features.pNext = &vulkan11Features;

        VkPhysicalDeviceVulkan13Features vulkan13Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
        vulkan13Features.pNext = &vulkan12Features;
        vulkan13Features.synchronization2 = true;
        vulkan13Features.dynamicRendering = true;

        VkPhysicalDeviceFeatures2 enabledFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        enabledFeatures.pNext = &vulkan13Features;

        auto const* extension{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        VkDeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = &enabledFeatures;
        deviceCreateInfo.flags = 0;
        deviceCreateInfo.enabledExtensionCount = 1;
        deviceCreateInfo.ppEnabledExtensionNames = &extension;
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.pEnabledFeatures = nullptr;

        vkCheck(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
        volkLoadDevice(device);

        vkGetDeviceQueue(device, graphicsQueue.family, graphicsQueue.index, &graphicsQueue.queue);

        VkSemaphoreCreateInfo semaphoreCreateInfo;
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = nullptr;
        semaphoreCreateInfo.flags = 0;

        VkFenceCreateInfo fenceCreateInfo;
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fenceCreateInfo.pNext = nullptr;

        for (size_t i = 0; i < g_framesInFlight; ++i)
        {
            vkCheck(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentSemaphores[i]));
            vkCheck(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderSemaphores[i]));
            vkCheck(vkCreateFence(device, &fenceCreateInfo, nullptr, &fences[i]));
        }

        this->createAllocator();
        this->createSwapchain();

        VkCommandPoolCreateInfo commandPoolCreateInfo;
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.pNext = nullptr;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        commandPoolCreateInfo.queueFamilyIndex = graphicsQueue.family;

        commandPools.resize(swapchainImages.size());
        commandBuffers.resize(swapchainImages.size());
        for (size_t i = 0; i < swapchainImages.size(); ++i)
        {
            vkCheck(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPools[i]));

            VkCommandBufferAllocateInfo commandBufferAllocateInfo;
            commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocateInfo.pNext = nullptr;
            commandBufferAllocateInfo.commandPool = commandPools[i];
            commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocateInfo.commandBufferCount = 1;
            vkCheck(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffers[i]));
        }

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pNext = nullptr;
        pipelineLayoutCreateInfo.flags = 0;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.setLayoutCount = 0;
        vkCheck(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

        LOG(INFO, "Vulkan Context") << "Created device\n";
    }

    auto Context::createAllocator() -> void
    {
        auto functions{ VmaVulkanFunctions{} };
        functions.vkGetInstanceProcAddr                   = vkGetInstanceProcAddr;
        functions.vkGetDeviceProcAddr                     = vkGetDeviceProcAddr;
        functions.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
        functions.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
        functions.vkAllocateMemory                        = vkAllocateMemory;
        functions.vkFreeMemory                            = vkFreeMemory;
        functions.vkMapMemory                             = vkMapMemory;
        functions.vkUnmapMemory                           = vkUnmapMemory;
        functions.vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges;
        functions.vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges;
        functions.vkBindBufferMemory                      = vkBindBufferMemory;
        functions.vkBindImageMemory                       = vkBindImageMemory;
        functions.vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements;
        functions.vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements;
        functions.vkCreateBuffer                          = vkCreateBuffer;
        functions.vkDestroyBuffer                         = vkDestroyBuffer;
        functions.vkCreateImage                           = vkCreateImage;
        functions.vkDestroyImage                          = vkDestroyImage;
        functions.vkCmdCopyBuffer                         = vkCmdCopyBuffer;
        functions.vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2;
        functions.vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2;
        functions.vkBindBufferMemory2KHR                  = vkBindBufferMemory2;
        functions.vkBindImageMemory2KHR                   = vkBindImageMemory2;
        functions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
        functions.vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements;
        functions.vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements;

        auto allocatorCreateInfo{ VmaAllocatorCreateInfo{} };
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorCreateInfo.physicalDevice = physicalDevice;
        allocatorCreateInfo.device = device;
        allocatorCreateInfo.instance = instance;
        allocatorCreateInfo.pVulkanFunctions = &functions;

        vkCheck(vmaCreateAllocator(&allocatorCreateInfo, &allocator));
        LOG(INFO, "Vulkan Context") << "Created allocator\n";
    }

    auto Context::createSwapchain() -> void
    {
        auto getSurfaceFormat = [&]
        {
            u32 count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
            std::vector<VkSurfaceFormatKHR> availableFormats(count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, availableFormats.data());

            if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
            {
                return VkSurfaceFormatKHR{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
            }

            for (const auto& currentFormat : availableFormats)
            {
                if (currentFormat.format == VK_FORMAT_R8G8B8A8_UNORM && currentFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    return currentFormat;
                }
            }

            for (const auto& currentFormat : availableFormats)
            {
                if (currentFormat.format == VK_FORMAT_B8G8R8A8_UNORM && currentFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    return currentFormat;
                }
            }

            return availableFormats[0];
        };

        auto isPresentModeSupported = [&](VkPresentModeKHR presentMode) -> bool
        {
            u32 modeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount, nullptr);
            std::vector<VkPresentModeKHR> presentModes(modeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount, presentModes.data());

            return std::ranges::any_of(
                presentModes,
                [&](auto const& t_p){ return t_p == static_cast<VkPresentModeKHR>(presentMode); }
            );
        };

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

        auto surfaceFormat{ getSurfaceFormat() };
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

        if (isPresentModeSupported(VK_PRESENT_MODE_IMMEDIATE_KHR))
        {
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }

        if (isPresentModeSupported(VK_PRESENT_MODE_MAILBOX_KHR))
        {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }

        extent.width = std::clamp((u32)Window::GetWidth(), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp((u32)Window::GetHeight(), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        auto oldSwapchain{ swapchain };

        colorFormat = surfaceFormat.format;

        VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchainCreateInfo.minImageCount    = std::max(3u, capabilities.minImageCount);
        swapchainCreateInfo.preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.imageColorSpace  = surfaceFormat.colorSpace;
        swapchainCreateInfo.imageFormat      = surfaceFormat.format;
        swapchainCreateInfo.presentMode      = presentMode;
        swapchainCreateInfo.surface          = surface;
        swapchainCreateInfo.imageExtent      = extent;
        swapchainCreateInfo.oldSwapchain     = oldSwapchain;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.clipped          = 1;

        vkCheck(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain));

        if (oldSwapchain) vkDestroySwapchainKHR(device, oldSwapchain, nullptr);

        for (size_t i = 0; i < swapchainImages.size(); ++i)
        {
            if (swapchainImages[i].handleView) vkDestroyImageView(device, swapchainImages[i].handleView, nullptr);
        }

        u32 imageCount;
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        std::vector<VkImage> images(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());

        swapchainImages = std::vector<Image>(imageCount);

        for (u32 i = 0; i < imageCount; ++i)
        {
            VkImageViewCreateInfo imageViewCreateInfo;
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.pNext = nullptr;
            imageViewCreateInfo.flags = 0;
            imageViewCreateInfo.image = images[i];
            imageViewCreateInfo.format = static_cast<VkFormat>(surfaceFormat.format);
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
            imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
            imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
            imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
            imageViewCreateInfo.subresourceRange = VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            };

            VkImageView imageView;
            vkCheck(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView));
            
            swapchainImages[i].loadFromSwapchain(images[i], imageView, Format(surfaceFormat.format));
        }

        LOG(INFO, "Vulkan Context") << "Created swapchain\n";
    }

    auto Context::teardown() -> void
    {
        vkDeviceWaitIdle(device);

        vmaDestroyAllocator(allocator);

        if (device)
        {
            for (auto pipeline : pipelines)
            {
                if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
            }

            for (size_t i = 0; i < swapchainImages.size(); ++i)
            {
                if (commandPools[i])               vkDestroyCommandPool(device, commandPools[i], nullptr);
                if (swapchainImages[i].handleView) vkDestroyImageView(device, swapchainImages[i].handleView, nullptr);
            }

            for (u32 i = 0; i < g_framesInFlight; ++i)
            {
                if (fences[i])            vkDestroyFence(device, fences[i], nullptr);
                if (renderSemaphores[i])  vkDestroySemaphore(device, renderSemaphores[i], nullptr);
                if (presentSemaphores[i]) vkDestroySemaphore(device, presentSemaphores[i], nullptr);
            }

            if (pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            if (swapchain)      vkDestroySwapchainKHR(device, swapchain, nullptr);
            vkDestroyDevice(device, nullptr);
        }

        if (instance)
        {
            if (surface)       vkDestroySurfaceKHR(instance, surface, nullptr);
            if (debugCallback) vkDestroyDebugReportCallbackEXT(instance, debugCallback, nullptr);

            vkDestroyInstance(instance, nullptr);
        }

        device = nullptr;
        instance = nullptr;
        
        LOG(INFO, "Vulkan Context") << "Destroyed context\n";
    }
}

static auto vkErrorString(VkResult result) -> char const*
{
    switch((i32)result)
    {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_EVENT_SET:
        return "VK_EVENT_SET";
    case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
        return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
        return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
    case VK_ERROR_INVALID_SHADER_NV:
        return "VK_ERROR_INVALID_SHADER_NV";
    default:
        return "VK_<Unknown>";
    }
}

static auto vkCheck(VkResult result) -> void
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(std::string("Vulkan error: ") + vkErrorString(result));
    }
}

static VkBool32 VKAPI_CALL debugReportCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t object,
    size_t location,
    int32_t messageCode,
    char const* pLayerPrefix,
    char const* pMessage,
    void* pUserData)
{
    LOG(DEBUG, "Validation Layer") << pMessage << '\n';
    
    return VK_FALSE;
}

static auto readFile(std::string_view filepath) -> std::vector<char>
{
    std::ifstream file(filepath.data(), std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return {};
    }

    auto fileSize{ (std::streamsize)file.tellg() };
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}