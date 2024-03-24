#include "Pipeline.hpp"
#include "Device.hpp"
#include "Buffer.hpp"
#include <volk.h>
#include <array>
#include <vector>
#include <deque>
#include <stdexcept>
#include <string_view>
#include <fstream>

static auto readFile(std::string_view filepath) -> std::vector<char>;

vk::Pipeline::Pipeline()
    : m{}
{}

vk::Pipeline::Pipeline(Device& device, Config const& config)
    : m{
        .device = &device,
        .point = config.point
    }
{
    if (!config.descriptors.empty())
    {
        auto bindings{ std::vector<VkDescriptorSetLayoutBinding>(config.descriptors.size()) };
        auto bindingFlags{ std::vector<VkDescriptorBindingFlags>(config.descriptors.size()) };
        auto writes{ std::vector<VkWriteDescriptorSet>{} }; writes.reserve(config.descriptors.size());
        auto bufferInfos{ std::deque<VkDescriptorBufferInfo>{} };

        for (auto i{ config.descriptors.size() }; i--; )
        {
            auto const& descriptor{ config.descriptors[i] };

            bindings[i] = VkDescriptorSetLayoutBinding{
                .binding = descriptor.binding,
                .descriptorType = static_cast<VkDescriptorType>(descriptor.type),
                .descriptorCount = descriptor.pBuffer ? 1u : 1024u,
                .stageFlags = descriptor.stage
            };

            bindingFlags[i] = descriptor.pBuffer ? 0u : VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
        }

        auto const setLayoutBindingFlags{ VkDescriptorSetLayoutBindingFlagsCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = static_cast<u32>(bindingFlags.size()),
            .pBindingFlags = bindingFlags.data()
        }};

        auto const descriptorSetLayoutCreateInfo{ VkDescriptorSetLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &setLayoutBindingFlags,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = static_cast<u32>(bindings.size()),
            .pBindings = bindings.data()
        }};

        if (vkCreateDescriptorSetLayout(*m.device, &descriptorSetLayoutCreateInfo, nullptr, &m.setLayout))
        {
            throw std::runtime_error("Failed to create VkDescriptorSetLayout");
        }

        auto const descriptorSetAllocateInfo{ VkDescriptorSetAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = *m.device,
            .descriptorSetCount = 1,
            .pSetLayouts = &m.setLayout
        }};

        if (vkAllocateDescriptorSets(*m.device, &descriptorSetAllocateInfo, &m.set))
        {
            throw std::runtime_error("Failed to allocate VkDescriptorSet");
        }

        for (auto i{ u32{} }; i < config.descriptors.size(); ++i)
        {
            auto const& descriptor{ config.descriptors[i] };

            if (!descriptor.pBuffer)
            {
                m.imagesBinding = descriptor.binding;
                continue;
            }

            bufferInfos.emplace_back(VkDescriptorBufferInfo{
                .buffer = *descriptor.pBuffer,
                .offset = descriptor.offset,
                .range = descriptor.size ? descriptor.size : ~0ull
            });

            writes.emplace_back(VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m.set,
                .dstBinding = descriptor.binding,
                .descriptorCount = 1,
                .descriptorType = static_cast<VkDescriptorType>(descriptor.type),
                .pBufferInfo = &bufferInfos.back()
            });
        }

        if (!writes.empty())
        {
            vkUpdateDescriptorSets(*m.device, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
        }
    }
    {
        auto const layoutCreateInfo { VkPipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = m.setLayout ? 1u : 0u,
            .pSetLayouts = m.setLayout ? &m.setLayout : nullptr
        }};

        if (vkCreatePipelineLayout(*m.device, &layoutCreateInfo, nullptr, &m.layout))
        {
            throw std::runtime_error("Failed to create VkPipelineLayout");
        }
    }
    {
        auto shaderStageCreateInfos{ std::vector<VkPipelineShaderStageCreateInfo>{config.stages.size()} };
        auto shaderModules{ std::vector<VkShaderModule>{config.stages.size()} };

        for (auto i{ config.stages.size() }; i--; )
        {
            auto stage{ config.stages.begin() + i };
            auto code { readFile(stage->path)     };

            if (code.empty())
            {
                throw std::runtime_error(std::string("Failed to load shader file: ") + stage->path);
            }

            auto const moduleCreateInfo{ VkShaderModuleCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = code.size(),
                .pCode = reinterpret_cast<u32 const*>(code.data())
            }};

            if (vkCreateShaderModule(*m.device, &moduleCreateInfo, nullptr, &shaderModules[i]))
            {
                throw std::runtime_error("Failed to create VkShaderModule");
            }

            shaderStageCreateInfos[i] = VkPipelineShaderStageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = static_cast<VkShaderStageFlagBits>(stage->stage),
                .module = shaderModules[i],
                .pName = "main"
            };
        }

        auto constexpr dynamicStates{ std::array{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR} };

        auto const dynamicStateCreateInfo{ VkPipelineDynamicStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<u32>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        }};

        auto const viewportStateCreateInfo{ VkPipelineViewportStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount  = 1
        }};

        auto const inputAssemblyStateCreateInfo{ VkPipelineInputAssemblyStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = static_cast<VkPrimitiveTopology>(config.topology)
        }};

        auto const rasterizationStateCreateInfo{ VkPipelineRasterizationStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .cullMode = static_cast<VkCullModeFlags>(config.cullMode),
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .lineWidth = 1.0f,
        }};

        auto const multisampleStateCreateInfo{ VkPipelineMultisampleStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        }};

        auto const blendAttachmentState{ VkPipelineColorBlendAttachmentState{
            .blendEnable = config.colorBlending,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        }};

        auto const colorBlendStateCreateInfo{ VkPipelineColorBlendStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &blendAttachmentState
        }};

        auto const depthStencilStateCreateInfo{ VkPipelineDepthStencilStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = config.depthWrite,
            .depthWriteEnable = config.depthWrite,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .stencilTestEnable = false
        }};

        auto const vertexInputStateCreateInfo{ VkPipelineVertexInputStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
        }};

        auto const colorFormat{ static_cast<VkFormat>(m.device->getSurfaceFormat()) };

        auto const renderingCreateInfo{ VkPipelineRenderingCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &colorFormat,
            .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
            .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
        }};

        auto const pipelineCreateInfo{ VkGraphicsPipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = (void*)&renderingCreateInfo,
            .stageCount = static_cast<u32>(shaderStageCreateInfos.size()),
            .pStages = shaderStageCreateInfos.data(),
            .pVertexInputState = &vertexInputStateCreateInfo,
            .pInputAssemblyState = &inputAssemblyStateCreateInfo,
            .pViewportState = &viewportStateCreateInfo,
            .pRasterizationState = &rasterizationStateCreateInfo,
            .pMultisampleState = &multisampleStateCreateInfo,
            .pDepthStencilState = &depthStencilStateCreateInfo,
            .pColorBlendState = &colorBlendStateCreateInfo,
            .pDynamicState = &dynamicStateCreateInfo,
            .layout = m.layout
        }};

        if (vkCreateGraphicsPipelines(*m.device, nullptr, 1, &pipelineCreateInfo, nullptr, &m.pipeline))
        {
            throw std::runtime_error("Failed to create VkPipeline");
        }

        for (auto shaderModule : shaderModules)
        {
            if (shaderModule)
            {
                vkDestroyShaderModule(*m.device, shaderModule, nullptr);
            }
        }
    }
}

vk::Pipeline::~Pipeline()
{
    if (m.device)
    {
        if (m.setLayout)
        {
            vkDestroyDescriptorSetLayout(*m.device, m.setLayout, nullptr);
        }

        if (m.pipeline && m.layout)
        {
            vkDestroyPipeline(*m.device, m.pipeline, nullptr);
            vkDestroyPipelineLayout(*m.device, m.layout, nullptr);
        }
    }

    m = {};
}

vk::Pipeline::Pipeline(Pipeline&& other)
    : m{ other.m }
{
    other.m = {};
}

auto vk::Pipeline::operator=(Pipeline&& other) -> Pipeline&
{
    m = other.m;
    other.m = {};

    return *this;
}

auto vk::Pipeline::writeImage(Image& image, u32 element, DescriptorType type) -> void
{
    auto const imageInfo{ VkDescriptorImageInfo{
        .sampler = *m.device,
        .imageView = image,
        .imageLayout = static_cast<VkImageLayout>(image.getLayout()),
    }};

    auto const write{ VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m.set,
        .dstBinding = m.imagesBinding,
        .dstArrayElement = element,
        .descriptorCount = 1,
        .descriptorType = static_cast<VkDescriptorType>(type),
        .pImageInfo = &imageInfo
    }};

    vkUpdateDescriptorSets(*m.device, 1, &write, 0, nullptr);

    image.setLayout(vk::ImageLayout::eUndefined);
}

static auto readFile(std::string_view filepath) -> std::vector<char>
{
    auto file{ std::ifstream{filepath.data(), std::ios::ate | std::ios::binary} };

    if (!file.is_open())
    {
        return {};
    }

    auto fileSize{ static_cast<std::streamsize>(file.tellg()) };
    auto buffer{ std::vector<char>(fileSize) };

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}