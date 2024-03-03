#pragma once
#include "Types.hpp"
#include "ArrayProxy.hpp"
#include <glm/glm.hpp>
#include <string>
#include <functional>

struct VkImage_T;
struct VkImageView_T;

using VkImage = VkImage_T*;
using VkImageView = VkImageView_T*;

namespace vk
{
    using flags = u32;
    using ImageUsageFlags = flags;
    using AspectFlags = flags;
    using ShaderStageFlags = flags;
    using BufferUsageFlags = flags;

    namespace ImageUsage
    {
        enum : flags
        {
            eTransferSrc = 0x00000001,
            eTransferDst = 0x00000002,
            eSampled = 0x00000004,
            eStorage = 0x00000008,
            eColorAttachment = 0x00000010,
            eDepthAttachment = 0x00000020
        };
    }

    namespace Aspect
    {
        enum : flags
        {
            eColor = 1,
            eDepth = 2,
            eStencil = 4
        };
    }

    namespace ShaderStage
    {
        enum : flags
        {
            eVertex = 0x00000001,
            eGeometry = 0x00000008,
            eFragment = 0x00000010,
            eCompute = 0x00000020
        };
    }

    namespace BufferUsage
    {
        enum : flags
        {
            eTransferSrc = 0x00000001,
            eTransferDst = 0x00000002,
            eUniformBuffer = 0x00000010,
            eStorageBuffer = 0x00000020,
            eIndirectBuffer = 0x00000100
        };
    };

    enum class Format : flags
    {
        eR8_unorm = 9,
        eRGBA8_unorm = 37,
        eBGRA8_unorm = 44,
        eRG32_sfloat = 103,
        eRGB32_sfloat = 106,
        eRGBA32_sfloat = 109,
        eD32_sfloat = 126,
        eD24_unorm_S8_uint = 129
    };

    enum class ImageLayout : flags
    {
        eUndefined = 0,
        eGeneral = 1,
        eColorAttachment = 2,
        eDepthStencilAttachment = 3,
        eDepthStencilRead = 4,
        eShaderRead = 5,
        eTransferSrc = 6,
        eTransferDst = 7,
        eDepthReadStencilAttachment = 1000117000,
        eDepthAttachmentStencilRead = 1000117001,
        eDepthAttachment = 1000241000,
        eDepthRead = 1000241001,
        eStencilAttachment = 1000241002,
        eStencilRead = 1000241003,
        eRead = 1000314000,
        eAttachment = 1000314001,
        ePresent = 1000001002,
    };

    enum class PipelineBindPoint : flags
    {
        eGraphics = 0,
        eCompute = 1
    };

    enum class PipelineTopology : flags
    {
        ePoint = 0,
        eLineList = 1,
        eLineStrip = 2,
        eTriangleList = 3,
        eTriangleStrip = 4,
        eTriangleFan = 5
    };

    enum class PipelineCullMode : flags
    {
        eNone = 0,
        eFront = 0x00000001,
        eBack = 0x00000002
    };

    enum class MemoryType : flags
    {
        eHost = 0x0,
        eHostOnly = 0x1,
        eDevice = 0x2
    };

    enum class DescriptorType : flags
    {
        eSampler = 0,
        eCombinedImageSampler = 1,
        eSampledImage = 2,
        eStorageImage = 3,
        eUniformBuffer = 6,
        eStorageBuffer = 7
    };

    /*
    class Image
    {
    public:
        Image() = default;
        ~Image() = default;

        auto loadFromSwapchain(VkImage image, VkImageView imageView, Format imageFormat) -> void;
        auto allocate(glm::uvec2 size, ImageUsageFlags usage, Format format) -> void;
        auto write(void const* data, size_t dataSize) -> void;
        auto subwrite(void const* data, size_t dataSize, glm::ivec2 offset, glm::uvec2 size) -> void;
        auto loadFromFile2D(std::string_view path) -> void;

        u32 handleIndex;
        ImageUsageFlags usage;
        AspectFlags aspect;
        ImageLayout layout;
        Format format;
        glm::uvec2 size;
        u32 layerCount;
    };*/

    class Buffer
    {
    public:
        auto allocate(u32 dataSize, BufferUsageFlags usage, MemoryType memory) -> void;
        auto writeData(void const* data, size_t size = 0) -> void;

        void* mappedData{ nullptr };
        u32 handleIndex;
        u32 memoryType;
        u32 size;
    };

    struct DrawIndirectCommands
    {
        u32 vertexCount;
        u32 instanceCount;
        u32 firstVertex;
        u32 firstInstance;
    };

    struct PipelineShaderStage
    {
        ShaderStageFlags stage;
        std::string      path;
    };

    struct PipelineDescriptor
    {
        u32 binding;
        ShaderStageFlags shaderStage;
        DescriptorType descriptorType;
        Buffer* pBuffer;
        size_t offset;
        size_t size;
    };

    class Pipeline
    {
    public:
        auto writeImage(Image* pImage, u32 arrayElement, DescriptorType type) -> void;

    public:
        u32               handleIndex;
        u32               descriptor;
        u32               imagesBinding;
        PipelineBindPoint bindPoint;
    };

    class Cmd
    {
    public:
        Cmd(u32 index) : m_index{ index } {}
        ~Cmd() = default;

        auto begin()                                                    -> void;
        auto end()                                                      -> void;
        auto beginPresent()                                             -> void;
        auto endPresent()                                               -> void;
        auto beginRendering(Image const& image)                         -> void;
        auto endRendering()                                             -> void;
        auto barrier(Image& image, ImageLayout newLayout)               -> void;
        auto bindPipeline(Pipeline& pipeline)                           -> void;
        auto draw(u32 vertexCount)                                      -> void;
        auto drawIndirect(Buffer& buffer, size_t offset, u32 drawCount) -> void;
        auto drawIndirectCount(Buffer& buffer, u32 maxDraw)             -> void;

    private:
        u32 const m_index;
    };

    struct PipelineConfig
    {
        PipelineBindPoint               bindPoint;
        ArrayProxy<PipelineShaderStage> stages;
        ArrayProxy<PipelineDescriptor>  descriptors;
        PipelineTopology                topology;
        PipelineCullMode                cullMode;
        bool                            depthWrite;
        bool                            colorBlending;
    };

    auto init()                                          -> void;
    auto teardown()                                      -> void;
    auto present()                                       -> void;
    auto waitIdle()                                      -> void;
    auto rebuildCommands()                               -> void;
    auto onResize(std::function<void()> resizeCallback)  -> void;
    auto onRecord(std::function<void()> recordCallback)  -> void;
    auto getWidth()                                      -> f32;
    auto getHeight()                                     -> f32;
    auto getAspectRatio()                                -> f32;
    auto createPipeline(PipelineConfig const& config)    -> Pipeline;
    auto getCommandBufferCount()                         -> u32;
}