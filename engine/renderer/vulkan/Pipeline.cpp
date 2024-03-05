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
    : m_device{ nullptr }
    , m_layout{ nullptr }
    , m_pipeline{ nullptr }
    , m_setLayout{ nullptr }
    , m_set{ nullptr }
{}

vk::Pipeline::Pipeline(Device& device, Config const& config)
    : m_device{ &device }
    , m_layout{ nullptr }
    , m_pipeline{ nullptr }
    , m_setLayout{ nullptr }
    , m_set{ nullptr }
    , m_point{ config.point }
{
    if (!config.descriptors.empty())
    {
        auto bindings{ std::vector<VkDescriptorSetLayoutBinding>{config.descriptors.size()} };
        auto bindingFlags{ std::vector<VkDescriptorBindingFlags>{config.descriptors.size()} };
        auto writes{ std::vector<VkWriteDescriptorSet>{config.descriptors.size()} };
        auto bufferInfos{ std::deque<VkDescriptorBufferInfo>{} };

        writes.reserve(config.descriptors.size());

        for (auto i{ config.descriptors.size() }; i--; )
        {
            auto const& descriptor{ config.descriptors[i] };

            bindings[i] = VkDescriptorSetLayoutBinding{
                .binding = 0,
                .descriptorType = static_cast<VkDescriptorType>(descriptor.type),
                .descriptorCount = descriptor.pBuffer ? 1u : 1024u,
                .stageFlags = descriptor.stage
            };

            bindingFlags[i] = descriptor.pBuffer ? 0u : VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
        }

        // TODO
    }
    {
        auto const layoutCreateInfo { VkPipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
            // TODO
        }};

        if (vkCreatePipelineLayout(*m_device, &layoutCreateInfo, nullptr, &m_layout))
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

            if (vkCreateShaderModule(*m_device, &moduleCreateInfo, nullptr, &shaderModules[i]))
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

        auto const mulisampleStateCreateInfo{ VkPipelineMultisampleStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        }};

        auto const blendAttachmentState{ VkPipelineColorBlendAttachmentState{
            .blendEnable = config.colorBlending,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                              VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT
        }};

        auto const colorBlendStateCreateInfo{ VkPipelineColorBlendStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &blendAttachmentState
        }};

        auto const depthStencilStateCreateInfo{ VkPipelineDepthStencilStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = false,
            .depthWriteEnable = false,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .stencilTestEnable = false
        }};

        auto const vertexInputStateCreateInfo{ VkPipelineVertexInputStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
        }};

        auto const colorFormat{ static_cast<VkFormat>(m_device->getSurfaceFormat()) };

        auto const renderingCreateInfo{ VkPipelineRenderingCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &colorFormat,
            .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
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
            .pMultisampleState = &mulisampleStateCreateInfo,
            .pDepthStencilState = &depthStencilStateCreateInfo,
            .pColorBlendState = &colorBlendStateCreateInfo,
            .pDynamicState = &dynamicStateCreateInfo,
            .layout = m_layout
        }};

        if (vkCreateGraphicsPipelines(*m_device, nullptr, 1, &pipelineCreateInfo, nullptr, &m_pipeline))
        {
            throw std::runtime_error("Failed to create VkPipeline");
        }

        for (auto shaderModule : shaderModules)
        {
            if (shaderModule)
            {
                vkDestroyShaderModule(*m_device, shaderModule, nullptr);
            }
        }
    }
}

vk::Pipeline::~Pipeline()
{
    if (m_device)
    {
        if (m_setLayout)
        {
            vkDestroyDescriptorSetLayout(*m_device, m_setLayout, nullptr);

            m_setLayout = nullptr;
            m_set       = nullptr;
        }

        if (m_pipeline && m_layout)
        {
            vkDestroyPipeline(*m_device, m_pipeline, nullptr);
            vkDestroyPipelineLayout(*m_device, m_layout, nullptr);

            m_pipeline = nullptr;
            m_layout   = nullptr;
        }
    }
}

vk::Pipeline::Pipeline(Pipeline&& other)
    : m_device{ other.m_device }
    , m_pipeline{ other.m_pipeline }
    , m_layout{ other.m_layout}
    , m_setLayout{ other.m_setLayout }
    , m_set{ other.m_set }
    , m_point{ other.m_point }
{
    other.m_device    = nullptr;
    other.m_layout    = nullptr;
    other.m_pipeline  = nullptr;
    other.m_setLayout = nullptr;
    other.m_set       = nullptr;
}

auto vk::Pipeline::operator=(Pipeline&& other) -> Pipeline&
{
    m_device    = other.m_device;
    m_pipeline  = other.m_pipeline;
    m_layout    = other.m_layout;
    m_setLayout = other.m_setLayout;
    m_set       = other.m_set;
    m_point     = other.m_point;

    other.m_device    = nullptr;
    other.m_layout    = nullptr;
    other.m_pipeline  = nullptr;
    other.m_setLayout = nullptr;
    other.m_set       = nullptr;

    return *this;
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