#define VMA_IMPLEMENTATION
#include "Renderer.hpp"
#include <aixlog.hpp>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <deque>
#include <memory>
#include <cstring>
#include <algorithm>
#include <string>
#include <string_view>
#include <fstream>
#include "Types.hpp"
#include "Window.hpp"
#include "Thread.hpp"

static VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, char const*, char const*, void*);
static auto vkErrorString(VkResult result) -> char const*;
static auto vkCheck(VkResult result) -> void;
static auto readFile(std::string_view filepath) -> std::vector<char>;

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
        auto recreateSwapchain()    -> void;
        auto teardown()             -> void;

    private:
        using ImageVector            = std::vector<Image>;
        using CommandPoolVector      = std::vector<VkCommandPool>;
        using CommandBufferVector    = std::vector<VkCommandBuffer>;
        using PipelineVector         = std::vector<VkPipeline>;
        using PipelineLayoutVector   = std::vector<VkPipelineLayout>;
        using DescriptorLayoutVector = std::vector<VkDescriptorSetLayout>;
        using DescriptorVector       = std::vector<VkDescriptorSet>;
        using SemaphoreVector        = std::vector<VkSemaphore>;
        using FenceVector            = std::vector<VkFence>;
        using ResizeCallback         = std::function<void()>;

    public:
        VmaAllocator             allocator;
        QueueHandle              graphicsQueue;
        VkInstance               instance;
        VkDebugReportCallbackEXT debugCallback;
        VkSurfaceKHR             surface;
        VkPhysicalDevice         physicalDevice;
        VkDevice                 device;
        VkSwapchainKHR           swapchain;
        VkExtent2D               extent;
        VkFormat                 colorFormat;
        VkDescriptorPool         descriptorPool;
        DescriptorVector         descriptors;
        DescriptorLayoutVector   descriptorLayouts;
        PipelineLayoutVector     pipelineLayouts;
        PipelineVector           pipelines;
        CommandPoolVector        commandPools;
        CommandBufferVector      commandBuffers;
        SemaphoreVector          renderSemaphores;
        SemaphoreVector          presentSemaphores;
        FenceVector              fences;
        ImageVector              swapchainImages;
        Pipeline*                currentPipeline;
        ResizeCallback           onResize;
        u32                      imageIndex;
        u32                      frameIndex;
        u32                      currentCmd;
        u32                      maxImageIndex;
    };

    auto static g_context{ Context() };

    Image::~Image()
    {
        auto device{ g_context.device };

        if (device && allocation)
        {
            if (handleView) vkDestroyImageView(device, handleView, nullptr);
        } 
    }

    Buffer::~Buffer()
    {
        if (g_context.device &&allocation && handle) vmaDestroyBuffer(g_context.allocator, handle, allocation);
    }

    auto Buffer::allocate(u32 dataSize, BufferUsageFlags usage, MemoryType memory) -> void
    {
        size = dataSize;

        VkBufferCreateInfo bufferCreateInfo;
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.pNext = nullptr;
        bufferCreateInfo.flags = 0;
        bufferCreateInfo.usage = usage;
        bufferCreateInfo.size = size;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferCreateInfo.queueFamilyIndexCount = 0;
        bufferCreateInfo.pQueueFamilyIndices = nullptr;

        VmaAllocationCreateInfo allocationCreateInfo{};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        switch (memory)
        {
        case MemoryType::eHost:
            allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            allocationCreateInfo.flags  = VMA_ALLOCATION_CREATE_MAPPED_BIT;
            allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
            allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            break;
        case MemoryType::eHostOnly:
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
        case MemoryType::eDevice:
            allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        default:
            break;
        }

        auto allocationInfo{ VmaAllocationInfo() };

        vkCheck(vmaCreateBuffer(
            g_context.allocator,
            &bufferCreateInfo,
            &allocationCreateInfo,
            &handle,
            &allocation,
            &allocationInfo
        ));

        mappedData = allocationInfo.pMappedData;

        vmaGetAllocationMemoryProperties(g_context.allocator, allocation, &memoryType);
    }

    auto Buffer::writeData(void const *data) -> void
    {
        if (memoryType & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            memcpy(mappedData, data, size);
            vmaFlushAllocation(g_context.allocator, allocation, 0, size);
        }
    }

    auto Image::loadFromSwapchain(VkImage image, VkImageView imageView, Format imageFormat) -> void
    {
        this->handle = image;
        this->handleView = imageView;
        this->width = g_context.extent.width;
        this->height = g_context.extent.height;
        this->layout = ImageLayout::eUndefined;
        this->aspect = Aspect::eColor;
        this->usage = ImageUsage::eColorAttachment | ImageUsage::eTransferDst;
        this->allocation = nullptr;
        this->format = imageFormat;
        this->layerCount = 1;
    }

    Pipeline::~Pipeline()
    {
        if (g_context.pipelines[handleIndex])         vkDestroyPipeline(g_context.device, g_context.pipelines[handleIndex], nullptr);
        if (g_context.pipelineLayouts[handleIndex])   vkDestroyPipelineLayout(g_context.device, g_context.pipelineLayouts[handleIndex], nullptr);
        if (descriptor != ~0u)
        if (g_context.descriptorLayouts[descriptor])  vkDestroyDescriptorSetLayout(g_context.device, g_context.descriptorLayouts[descriptor], nullptr);
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

        if (previousWidth  != Window::GetWidth() ||
            previousHeight != Window::GetHeight())
        {
            g_context.recreateSwapchain();

            previousWidth  = Window::GetWidth();
            previousHeight = Window::GetHeight();
        }

        vkCheck(vkWaitForFences(g_context.device, 1, &g_context.fences[g_context.frameIndex], false, ~0ull));
        vkCheck(vkResetFences(g_context.device, 1, &g_context.fences[g_context.frameIndex]));

        auto result{ vkAcquireNextImageKHR(
            g_context.device,
            g_context.swapchain,
            ~0ull,
            g_context.renderSemaphores[g_context.frameIndex],
            nullptr,
            &g_context.imageIndex
        ) };

        switch (result)
        {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            g_context.recreateSwapchain();
        default:
            vkCheck(result);
            break;
        }
    }

    auto present() -> void
    {
        VkPipelineStageFlags constexpr stage{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo;
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.waitSemaphoreCount = 1;
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
            g_context.recreateSwapchain();
            break;
        default:
            vkCheck(result);
            break;
        }

        g_context.frameIndex = (g_context.frameIndex + 1) % g_context.maxImageIndex;
    }

    auto waitIdle() -> void
    {
        if (g_context.device) vkDeviceWaitIdle(g_context.device);
    }

    auto cmdBegin() -> void
    {
        vkCheck(vkResetCommandPool(g_context.device, g_context.commandPools[g_context.currentCmd], 0));

        VkCommandBufferBeginInfo beginInfo;
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pInheritanceInfo = nullptr;
        beginInfo.pNext = nullptr;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        vkCheck(vkBeginCommandBuffer(g_context.commandBuffers[g_context.currentCmd], &beginInfo));
    }

    auto cmdEnd() -> void
    {
        vkCheck(vkEndCommandBuffer(g_context.commandBuffers[g_context.currentCmd]));
    }

    auto cmdBeginPresent() -> void
    {
        cmdBarrier(g_context.swapchainImages[g_context.currentCmd], ImageLayout::eColorAttachment);
        cmdBeginRendering(g_context.swapchainImages[g_context.currentCmd]);
    }

    auto cmdEndPresent() -> void
    {
        cmdEndRendering();
        cmdBarrier(g_context.swapchainImages[g_context.currentCmd], ImageLayout::ePresent);
    }

    auto cmdBeginRendering(Image const& image) -> void
    {
        VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        colorAttachment.imageView = image.handleView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.flags = 0;
        renderingInfo.viewMask = 0;
        renderingInfo.layerCount = 1;
        renderingInfo.renderArea.extent = VkExtent2D{ image.width, image.height };
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(g_context.commandBuffers[g_context.currentCmd], &renderingInfo);

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

        vkCmdSetViewport(g_context.commandBuffers[g_context.currentCmd], 0, 1, &viewport);
        vkCmdSetScissor(g_context.commandBuffers[g_context.currentCmd], 0, 1, &scissor);
    }

    auto cmdEndRendering() -> void
    {
        vkCmdEndRendering(g_context.commandBuffers[g_context.currentCmd]);
    }

    auto cmdBarrier(Image& image, ImageLayout newLayout) -> void
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
        imageBarrier.image = image.handle;

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
        vkCmdPipelineBarrier2(g_context.commandBuffers[g_context.currentCmd], &dependency);

        image.layout = newLayout;
    }

    auto cmdBindPipeline(Pipeline& pipeline) -> void
    {
        vkCmdBindPipeline(
            g_context.commandBuffers[g_context.currentCmd],
            static_cast<VkPipelineBindPoint>(pipeline.bindPoint),
            g_context.pipelines[pipeline.handleIndex]
        );

        if (pipeline.descriptor != ~0u)
        {
            vkCmdBindDescriptorSets(
                g_context.commandBuffers[g_context.currentCmd],
                static_cast<VkPipelineBindPoint>(pipeline.bindPoint),
                g_context.pipelineLayouts[pipeline.handleIndex],
                0,
                1,
                &g_context.descriptors[pipeline.descriptor],
                0,
                 nullptr
            );
        }

        g_context.currentPipeline = &pipeline;
    }

    auto cmdDraw(u32 vertexCount) -> void
    {
        vkCmdDraw(g_context.commandBuffers[g_context.currentCmd], vertexCount, 1, 0, 0);
    }

    auto cmdNext() -> void
    {
        g_context.currentCmd = (g_context.currentCmd + 1) % g_context.maxImageIndex;
    }

    auto createPipeline(PipelineConfig const& config) -> Pipeline
    {
        auto static currentHandleIndex{ u32(0) };
        auto static currentDescriptor { u32(0) };

        auto descriptorSetLayout{ VkDescriptorSetLayout(nullptr) };
        auto descriptorSet      { VkDescriptorSet(nullptr)       };
        auto pipelineLayout     { VkPipelineLayout(nullptr)      };
        auto pipeline           { VkPipeline(nullptr)            };

        if (!config.descriptors.empty())
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings(config.descriptors.size());
            std::vector<VkWriteDescriptorSet> writes; writes.reserve(config.descriptors.size());
            std::deque<VkDescriptorBufferInfo> bufferInfos;

            for (auto i{ u32{} }; i < config.descriptors.size(); ++i)
            {
                auto const& descriptor{ config.descriptors[i] };

                bindings[i].binding = descriptor.binding;
                bindings[i].descriptorCount = descriptor.pBuffer ? 1 : 1024;
                bindings[i].descriptorType = static_cast<VkDescriptorType>(descriptor.descriptorType);
                bindings[i].pImmutableSamplers = nullptr;
                bindings[i].stageFlags = descriptor.shaderStage;

                if (!descriptor.pBuffer)
                {
                    break;
                }

                bufferInfos.push_back(VkDescriptorBufferInfo{
                    .buffer = descriptor.pBuffer->handle,
                    .offset = 0,
                    .range = ~0ULL
                });
            }

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.pNext = nullptr;
            descriptorSetLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
            descriptorSetLayoutCreateInfo.bindingCount = static_cast<u32>(bindings.size());
            descriptorSetLayoutCreateInfo.pBindings = bindings.data();
            vkCheck(vkCreateDescriptorSetLayout(g_context.device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));

            VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
            descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorSetAllocateInfo.pNext = nullptr;
            descriptorSetAllocateInfo.descriptorPool = g_context.descriptorPool;
            descriptorSetAllocateInfo.descriptorSetCount = 1;
            descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
            vkCheck(vkAllocateDescriptorSets(g_context.device, &descriptorSetAllocateInfo, &descriptorSet));

            for (auto i{ u32{} }; i < config.descriptors.size(); ++i)
            {
                auto const& descriptor{ config.descriptors[i] };

                VkWriteDescriptorSet descriptorWrite;
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.pNext = nullptr;
                descriptorWrite.pTexelBufferView = nullptr;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.dstBinding = descriptor.binding;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.descriptorType = static_cast<VkDescriptorType>(descriptor.descriptorType);
                descriptorWrite.pImageInfo = nullptr;
                descriptorWrite.pBufferInfo = &bufferInfos.back();
                descriptorWrite.dstSet = descriptorSet;
                writes.emplace_back(descriptorWrite);
            }

            if (!writes.empty())
            {
                vkUpdateDescriptorSets(g_context.device, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
            }
        }

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pNext = nullptr;
        pipelineLayoutCreateInfo.flags = 0;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.setLayoutCount = descriptorSet ? 1 : 0;
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSet ? &descriptorSetLayout : nullptr;
        vkCheck(vkCreatePipelineLayout(g_context.device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

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
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_LINE;
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
        pipelineCreateInfo.layout = pipelineLayout;
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
        g_context.pipelineLayouts.emplace_back(pipelineLayout);

        if (descriptorSet)
        {
            g_context.descriptorLayouts.emplace_back(descriptorSetLayout);
            g_context.descriptors.emplace_back(descriptorSet);
        }

        for (auto shaderModule : shaderModules)
        {
            if (shaderModule) vkDestroyShaderModule(g_context.device, shaderModule, nullptr);
        }

        return Pipeline{
            .handleIndex = currentHandleIndex++,
            .descriptor = descriptorSet ? currentDescriptor++ : ~u32(0),
            .bindPoint = config.bindPoint
        };
    }

    auto getCommandBufferCount() -> u32
    {
        return g_context.maxImageIndex;
    }

    auto onResize(std::function<void()> resizeCallback) -> void
    {
        g_context.onResize = std::move(resizeCallback);
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
            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
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
        enabledFeatures.features.fillModeNonSolid = true;

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

        this->createAllocator();
        this->createSwapchain();

        VkSemaphoreCreateInfo semaphoreCreateInfo;
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = nullptr;
        semaphoreCreateInfo.flags = 0;

        VkFenceCreateInfo fenceCreateInfo;
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fenceCreateInfo.pNext = nullptr;

        renderSemaphores.resize(maxImageIndex);
        presentSemaphores.resize(maxImageIndex);
        fences.resize(maxImageIndex);
        for (size_t i = 0; i < maxImageIndex; ++i)
        {
            vkCheck(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderSemaphores[i]));
            vkCheck(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentSemaphores[i]));
            vkCheck(vkCreateFence(device, &fenceCreateInfo, nullptr, &fences[i]));
        }

        VkCommandPoolCreateInfo commandPoolCreateInfo;
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.pNext = nullptr;
        commandPoolCreateInfo.flags = 0;
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

        VkDescriptorPoolSize poolSizes[] = {
            { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1024 },
            { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1024 },
            { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .descriptorCount = 1024 }
        };

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.pNext = nullptr;
        descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        descriptorPoolCreateInfo.maxSets = 1024;
        descriptorPoolCreateInfo.poolSizeCount = 1;
        descriptorPoolCreateInfo.pPoolSizes = poolSizes;
        vkCheck(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

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

        extent.width  = std::clamp((u32)Window::GetWidth(),  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
        extent.height = std::clamp((u32)Window::GetHeight(), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        auto oldSwapchain{ swapchain };

        colorFormat = surfaceFormat.format;

        auto minImageCount{ std::max(3u, capabilities.minImageCount) };

        if (capabilities.maxImageCount)
        {
            minImageCount = std::min(minImageCount, capabilities.maxImageCount);
        }

        VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchainCreateInfo.preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.imageColorSpace  = surfaceFormat.colorSpace;
        swapchainCreateInfo.imageFormat      = surfaceFormat.format;
        swapchainCreateInfo.minImageCount    = minImageCount;
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

        vkGetSwapchainImagesKHR(device, swapchain, &maxImageIndex, nullptr);
        std::vector<VkImage> images(maxImageIndex);
        vkGetSwapchainImagesKHR(device, swapchain, &maxImageIndex, images.data());

        swapchainImages = std::vector<Image>(maxImageIndex);

        for (u32 i = 0; i < maxImageIndex; ++i)
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

    auto Context::recreateSwapchain() -> void
    {
        while (Window::GetWidth() < 1 || Window::GetHeight() < 1)
        {
            Window::Update();
        }

        vkDeviceWaitIdle(g_context.device);
        this->createSwapchain();
        this->onResize();
    }

    auto Context::teardown() -> void
    {
        vkDeviceWaitIdle(device);

        vmaDestroyAllocator(allocator);

        if (device)
        {
            for (size_t i = 0; i < swapchainImages.size(); ++i)
            {
                if (commandPools[i])               vkDestroyCommandPool(device, commandPools[i], nullptr);
                if (swapchainImages[i].handleView) vkDestroyImageView(device, swapchainImages[i].handleView, nullptr);
                if (fences[i])                     vkDestroyFence(device, fences[i], nullptr);
                if (renderSemaphores[i])           vkDestroySemaphore(device, renderSemaphores[i], nullptr);
                if (presentSemaphores[i])          vkDestroySemaphore(device, presentSemaphores[i], nullptr);
            }

            if (descriptorPool)      vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            if (swapchain)           vkDestroySwapchainKHR(device, swapchain, nullptr);
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