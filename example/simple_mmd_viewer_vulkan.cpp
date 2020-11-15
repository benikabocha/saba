
#if _WIN32
#include <Windows.h>
#include <shellapi.h>
#undef min
#undef max
#endif

//#define VULKAN_HPP_NO_SMART_HANDLE
//#define VULKAN_HPP_NO_EXCEPTIONS
//#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <map>
#include <type_traits>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define	STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb_image.h>

#include <Saba/Base/Path.h>
#include <Saba/Base/File.h>
#include <Saba/Base/UnicodeUtil.h>
#include <Saba/Base/Time.h>
#include <Saba/Model/MMD/PMDModel.h>
#include <Saba/Model/MMD/PMXModel.h>
#include <Saba/Model/MMD/VMDFile.h>
#include <Saba/Model/MMD/VMDAnimation.h>
#include <Saba/Model/MMD/VMDCameraAnimation.h>

#include "resource_vulkan/mmd.vert.spv.h"
#include "resource_vulkan/mmd.frag.spv.h"
#include "resource_vulkan/mmd_edge.vert.spv.h"
#include "resource_vulkan/mmd_edge.frag.spv.h"
#include "resource_vulkan/mmd_ground_shadow.vert.spv.h"
#include "resource_vulkan/mmd_ground_shadow.frag.spv.h"

struct AppContext;

struct Input
{
	std::string					m_modelPath;
	std::vector<std::string>	m_vmdPaths;
};

void Usage()
{
	std::cout << "app [-model <pmd|pmx file path>] [-vmd <vmd file path>]\n";
	std::cout << "e.g. app -model model1.pmx -vmd anim1_1.vmd -vmd anim1_2.vmd  -model model2.pmx\n";
}

namespace
{
	bool FindMemoryTypeIndex(
		const vk::PhysicalDeviceMemoryProperties& memProps,
		uint32_t typeBits,
		vk::MemoryPropertyFlags reqMask,
		uint32_t* typeIndex)
	{
		if (typeIndex == nullptr) { return false; }

		for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
		{
			if (((typeBits >> i) & 1) == 1)
			{
				if ((memProps.memoryTypes[i].propertyFlags & reqMask) == reqMask)
				{
					*typeIndex = i;
					return true;
				}
			}
		}
		return false;
	}

	void SetImageLayout(vk::CommandBuffer cmdBuf, vk::Image image, vk::ImageAspectFlags aspectMask,
		vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout,
		vk::ImageSubresourceRange subresourceRange
	)
	{
		auto imageMemoryBarrier = vk::ImageMemoryBarrier()
			.setOldLayout(oldImageLayout)
			.setNewLayout(newImageLayout)
			.setImage(image)
			.setSubresourceRange(subresourceRange);
		switch (oldImageLayout)
		{
		case vk::ImageLayout::eUndefined:
			imageMemoryBarrier.setSrcAccessMask(vk::AccessFlags());
			break;
		case vk::ImageLayout::eTransferDstOptimal:
			imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
			break;
		case vk::ImageLayout::ePreinitialized:
			imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eHostWrite);
			break;
		default:
			break;
		}

		switch (newImageLayout)
		{
		case vk::ImageLayout::eShaderReadOnlyOptimal:
			imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
			break;
		case vk::ImageLayout::eTransferSrcOptimal:
			imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);
			break;
		case vk::ImageLayout::eTransferDstOptimal:
			imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
			break;
		default:
			break;
		}

		vk::PipelineStageFlags srcStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
		vk::PipelineStageFlags destStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
		if (oldImageLayout == vk::ImageLayout::eUndefined &&
			newImageLayout == vk::ImageLayout::eTransferDstOptimal)
		{
			srcStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
			destStageFlags = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (oldImageLayout == vk::ImageLayout::eTransferDstOptimal &&
			newImageLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			srcStageFlags = vk::PipelineStageFlagBits::eTransfer;
			destStageFlags = vk::PipelineStageFlagBits::eFragmentShader;
		}

		cmdBuf.pipelineBarrier(
			srcStageFlags,
			destStageFlags,
			vk::DependencyFlags(),
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier
		);
	}
}

// vertex

struct Vertex
{
	glm::vec3	m_position;
	glm::vec3	m_normal;
	glm::vec2	m_uv;
};

// MMD Shader uniform buffer

struct MMDVertxShaderUB
{
	glm::mat4	m_wv;
	glm::mat4	m_wvp;
};

struct MMDFragmentShaderUB
{
	glm::vec3	m_diffuse;
	float		m_alpha;
	glm::vec3	m_ambient;
	float		m_dummy1;
	glm::vec3	m_specular;
	float		m_specularPower;
	glm::vec3	m_lightColor;
	float		m_dummy2;
	glm::vec3	m_lightDir;
	float		m_dummy3;

	glm::vec4	m_texMulFactor;
	glm::vec4	m_texAddFactor;

	glm::vec4	m_toonTexMulFactor;
	glm::vec4	m_toonTexAddFactor;

	glm::vec4	m_sphereTexMulFactor;
	glm::vec4	m_sphereTexAddFactor;

	glm::ivec4	m_textureModes;
};

// MMD Edge Shader uniform buffer

struct MMDEdgeVertexShaderUB
{
	glm::mat4	m_wv;
	glm::mat4	m_wvp;
	glm::vec2	m_screenSize;
	float		m_dummy[2];
};

struct MMDEdgeSizeVertexShaderUB
{
	float		m_edgeSize;
	float		m_dummy[3];
};

struct MMDEdgeFragmentShaderUB
{
	glm::vec4	m_edgeColor;
};

// MMD Ground Shadow Shader uniform buffer

struct MMDGroundShadowVertexShaderUB
{
	glm::mat4	m_wvp;
};

struct MMDGroundShadowFragmentShaderUB
{
	glm::vec4	m_shadowColor;
};

// Swap chain

struct SwapchainImageResource
{
	SwapchainImageResource() {}

	vk::Image		m_image;
	vk::ImageView	m_imageView;
	vk::Framebuffer	m_framebuffer;
	vk::CommandBuffer	m_cmdBuffer;

	void Clear(AppContext& appContext);
};

struct Buffer
{
	Buffer() {}

	vk::DeviceMemory	m_memory;
	vk::Buffer			m_buffer;
	vk::DeviceSize		m_memorySize = 0;

	bool Setup(
		AppContext&				appContext,
		vk::DeviceSize			memSize,
		vk::BufferUsageFlags	usage,
		vk::MemoryPropertyFlags	memProperty
	);
	void Clear(AppContext& appContext);
};

struct Texture
{
	Texture() {}

	vk::Image		m_image;
	vk::ImageView	m_imageView;
	vk::ImageLayout	m_imageLayout = vk::ImageLayout::eUndefined;
	vk::DeviceMemory	m_memory;
	vk::Format			m_format;
	uint32_t			m_mipLevel;
	bool				m_hasAlpha;

	bool Setup(AppContext& appContext, uint32_t width, uint32_t height, vk::Format format, int mipCount = 1);
	void Clear(AppContext& appContext);
};

struct StagingBuffer
{
	StagingBuffer() {}

	vk::DeviceMemory	m_memory;
	vk::Buffer			m_buffer;

	vk::DeviceSize		m_memorySize = 0;

	vk::CommandBuffer	m_copyCommand;
	vk::Fence			m_transferCompleteFence;
	vk::Semaphore		m_transferCompleteSemaphore;

	vk::Semaphore		m_waitSemaphore;

	bool Setup(AppContext& appContext, vk::DeviceSize size);
	void Clear(AppContext& appContext);
	void Wait(AppContext& appContext);
	bool CopyBuffer(AppContext& appContext, vk::Buffer destBuffer, vk::DeviceSize size);
	bool CopyImage(
		AppContext& appContext,
		vk::Image destImage,
		vk::ImageLayout imageLayout,
		uint32_t regionCount,
		vk::BufferImageCopy* regions
	);
};

struct AppContext
{
	AppContext() {}

	std::string m_resourceDir;
	std::string	m_shaderDir;
	std::string	m_mmdDir;

	glm::mat4	m_viewMat;
	glm::mat4	m_projMat;
	int	m_screenWidth = 0;
	int	m_screenHeight = 0;

	glm::vec3	m_lightColor = glm::vec3(1, 1, 1);
	glm::vec3	m_lightDir = glm::vec3(-0.5f, -1.0f, -0.5f);

	float	m_elapsed = 0.0f;
	float	m_animTime = 0.0f;
	std::unique_ptr<saba::VMDCameraAnimation>	m_vmdCameraAnim;

	vk::Instance		m_vkInst;
	vk::SurfaceKHR		m_surface;
	vk::PhysicalDevice	m_gpu;
	vk::Device			m_device;

	vk::PhysicalDeviceMemoryProperties	m_memProperties;

	uint32_t	m_graphicsQueueFamilyIndex;
	uint32_t	m_presentQueueFamilyIndex;

	vk::Format	m_colorFormat = vk::Format::eUndefined;
	vk::Format	m_depthFormat = vk::Format::eUndefined;
	vk::SampleCountFlagBits	m_msaaSampleCount = vk::SampleCountFlagBits::e4;

	// Sync Objects
	struct FrameSyncData
	{
		FrameSyncData() {}

		vk::Fence		m_fence;
		vk::Semaphore	m_presentCompleteSemaphore;
		vk::Semaphore	m_renderCompleteSemaphore;
	};
	std::vector<FrameSyncData>	m_frameSyncDatas;
	uint32_t					m_frameIndex = 0;

	// Buffer and Framebuffer
	vk::SwapchainKHR					m_swapchain;
	std::vector<SwapchainImageResource>	m_swapchainImageResouces;
	vk::Image							m_depthImage;
	vk::DeviceMemory					m_depthMem;
	vk::ImageView						m_depthImageView;
	vk::Image							m_msaaColorImage;
	vk::DeviceMemory					m_msaaColorMem;
	vk::ImageView						m_msaaColorImageView;
	vk::Image							m_msaaDepthImage;
	vk::DeviceMemory					m_msaaDepthMem;
	vk::ImageView						m_msaaDepthImageView;

	// Render Pass
	vk::RenderPass	m_renderPass;

	// Pipeline Cache
	vk::PipelineCache	m_pipelineCache;

	// MMD Draw pipeline
	enum class MMDRenderType
	{
		AlphaBlend_FrontFace,
		AlphaBlend_BothFace,
		MaxTypes
	};
	vk::DescriptorSetLayout	m_mmdDescriptorSetLayout;
	vk::PipelineLayout	m_mmdPipelineLayout;
	vk::Pipeline		m_mmdPipelines[int(MMDRenderType::MaxTypes)];
	vk::ShaderModule	m_mmdVSModule;
	vk::ShaderModule	m_mmdFSModule;

	// MMD Edge Draw pipeline
	vk::DescriptorSetLayout	m_mmdEdgeDescriptorSetLayout;
	vk::PipelineLayout	m_mmdEdgePipelineLayout;
	vk::Pipeline		m_mmdEdgePipeline;
	vk::ShaderModule	m_mmdEdgeVSModule;
	vk::ShaderModule	m_mmdEdgeFSModule;

	// MMD Ground Shadow Draw pipeline
	vk::DescriptorSetLayout	m_mmdGroundShadowDescriptorSetLayout;
	vk::PipelineLayout	m_mmdGroundShadowPipelineLayout;
	vk::Pipeline		m_mmdGroundShadowPipeline;
	vk::ShaderModule	m_mmdGroundShadowVSModule;
	vk::ShaderModule	m_mmdGroundShadowFSModule;

	const uint32_t DefaultImageCount = 3;
	uint32_t	m_imageCount;
	uint32_t	m_imageIndex = 0;

	// Command Pool
	vk::CommandPool		m_commandPool;
	vk::CommandPool		m_transferCommandPool;

	// Queue
	vk::Queue			m_graphicsQueue;

	// Staging Buffer
	std::vector<std::unique_ptr<StagingBuffer>>	m_stagingBuffers;

	// Texture
	using TextureUPtr = std::unique_ptr<Texture>;
	std::map<std::string, TextureUPtr>	m_textures;

	// default tex
	TextureUPtr	m_defaultTexture;

	// Sampler
	vk::Sampler	m_defaultSampler;

	bool Setup(vk::Instance inst, vk::SurfaceKHR surface, vk::PhysicalDevice gpu, vk::Device device);
	void Destory();

	bool Prepare();
	bool PrepareCommandPool();
	bool PrepareBuffer();
	bool PrepareSyncObjects();
	bool PrepareRenderPass();
	bool PrepareFramebuffer();
	bool PreparePipelineCache();
	bool PrepareMMDPipeline();
	bool PrepareMMDEdgePipeline();
	bool PrepareMMDGroundShadowPipeline();
	bool PrepareDefaultTexture();

	bool Resize();

	bool GetStagingBuffer(vk::DeviceSize memSize, StagingBuffer** outBuf);
	void WaitAllStagingBuffer();

	bool GetTexture(const std::string& texturePath, Texture** outTex);
};

bool AppContext::Setup(vk::Instance inst, vk::SurfaceKHR surface, vk::PhysicalDevice gpu, vk::Device device)
{
	m_vkInst = inst;
	m_surface = surface;
	m_gpu = gpu;
	m_device = device;

	m_resourceDir = saba::PathUtil::GetExecutablePath();
	m_resourceDir = saba::PathUtil::GetDirectoryName(m_resourceDir);
	m_resourceDir = saba::PathUtil::Combine(m_resourceDir, "resource");
	m_shaderDir = saba::PathUtil::Combine(m_resourceDir, "shader");
	m_mmdDir = saba::PathUtil::Combine(m_resourceDir, "mmd");

	// Get Memory Properties
	auto memProperties = m_gpu.getMemoryProperties();
	m_memProperties = memProperties;

	// Select queue family
	auto queueFamilies = gpu.getQueueFamilyProperties();
	if (queueFamilies.empty())
	{
		return false;
	}

	uint32_t selectGraphicsQueueFamilyIndex = UINT32_MAX;
	uint32_t selectPresentQueueFamilyIndex = UINT32_MAX;
	for (uint32_t i = 0; i < (uint32_t)queueFamilies.size(); i++)
	{
		const auto& queue = queueFamilies[i];
		if (queue.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			if (selectGraphicsQueueFamilyIndex == UINT32_MAX)
			{
				selectGraphicsQueueFamilyIndex = i;
			}
			auto supported = gpu.getSurfaceSupportKHR(i, surface);
			if (supported)
			{
				selectGraphicsQueueFamilyIndex = i;
				selectPresentQueueFamilyIndex = i;
				break;
			}
		}
	}
	if (selectGraphicsQueueFamilyIndex == UINT32_MAX) {
		std::cout << "Failed to find graphics queue family.\n";
		return false;
	}

	if (selectPresentQueueFamilyIndex == UINT32_MAX)
	{
		for (uint32_t i = 0; i < (uint32_t)queueFamilies.size(); i++)
		{
			auto supported = gpu.getSurfaceSupportKHR(i, surface);
			if (supported)
			{
				selectPresentQueueFamilyIndex = i;
				break;
			}
		}
	}
	if (selectPresentQueueFamilyIndex == UINT32_MAX)
	{
		std::cout << "Failed to find present queue family.\n";
		return false;
	}

	m_graphicsQueueFamilyIndex = selectGraphicsQueueFamilyIndex;
	m_presentQueueFamilyIndex = selectPresentQueueFamilyIndex;

	// Get Queue
	m_device.getQueue(m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);

	auto surfaceCaps = m_gpu.getSurfaceCapabilitiesKHR(m_surface);
	m_imageCount = std::max(surfaceCaps.minImageCount, uint32_t(2));
	if (surfaceCaps.maxImageCount > DefaultImageCount)
	{
		m_imageCount = DefaultImageCount;
	}

	// Select buffer format
	vk::Format selectColorFormats[] = {
		vk::Format::eB8G8R8A8Unorm,
		vk::Format::eB8G8R8A8Srgb,
	};
	m_colorFormat = vk::Format::eUndefined;
	auto surfaceFormats = m_gpu.getSurfaceFormatsKHR(m_surface);
	for (const auto& selectFmt : selectColorFormats)
	{
		for (const auto& surfaceFmt : surfaceFormats)
		{
			if (selectFmt == surfaceFmt.format)
			{
				m_colorFormat = selectFmt;
				break;
			}
		}
		if (m_colorFormat != vk::Format::eUndefined)
		{
			break;
		}
	}
	if (m_colorFormat == vk::Format::eUndefined)
	{
		std::cout << "Failed to find color formant.\n";
		return false;
	}

	m_depthFormat = vk::Format::eUndefined;
	vk::Format selectDepthFormats[] = {
		vk::Format::eD24UnormS8Uint,
		vk::Format::eD16UnormS8Uint,
		vk::Format::eD32SfloatS8Uint,
	};
	for (const auto& selectFmt : selectDepthFormats)
	{
		auto fmtProp = m_gpu.getFormatProperties(selectFmt);
		if (fmtProp.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
		{
			m_depthFormat = selectFmt;
			break;
		}
	}
	if (m_depthFormat == vk::Format::eUndefined)
	{
		std::cout << "Failed to find depth formant.\n";
		return false;
	}
	std::cout << "Select color format [" << int(m_colorFormat) << "]\n";
	std::cout << "Select depth format [" << int(m_depthFormat) << "]\n";

	if (!Prepare())
	{
		return false;
	}

	return true;
}

void AppContext::Destory()
{
	m_vmdCameraAnim.reset();

	if (!m_device)
	{
		return;
	}

	m_device.waitIdle();

	// Sync Objects
	for (auto& frameSyncData : m_frameSyncDatas)
	{
		m_device.destroySemaphore(frameSyncData.m_presentCompleteSemaphore, nullptr);
		frameSyncData.m_presentCompleteSemaphore = nullptr;

		m_device.destroySemaphore(frameSyncData.m_renderCompleteSemaphore, nullptr);
		frameSyncData.m_renderCompleteSemaphore = nullptr;

		m_device.destroyFence(frameSyncData.m_fence, nullptr);
		frameSyncData.m_fence = nullptr;
	}

	// Buffer and Framebuffer
	if (m_swapchain)
	{
		m_device.destroySwapchainKHR(m_swapchain, nullptr);
	}

	for (auto& res : m_swapchainImageResouces)
	{
		res.Clear(*this);
	}
	m_swapchainImageResouces.clear();

	// Clear depth
	m_device.destroyImage(m_depthImage, nullptr);
	m_depthImage = nullptr;

	m_device.freeMemory(m_depthMem, nullptr);
	m_depthMem = nullptr;

	m_device.destroyImageView(m_depthImageView, nullptr);
	m_depthImageView = nullptr;

	// Clear msaa color
	m_device.destroyImageView(m_msaaColorImageView, nullptr);
	m_msaaColorImageView = nullptr;

	m_device.destroyImage(m_msaaColorImage, nullptr);
	m_msaaColorImage = nullptr;

	m_device.freeMemory(m_msaaColorMem, nullptr);
	m_msaaColorMem = nullptr;

	// Clear msaa depth
	m_device.destroyImageView(m_msaaDepthImageView, nullptr);
	m_msaaDepthImageView = nullptr;

	m_device.destroyImage(m_msaaDepthImage, nullptr);
	m_msaaDepthImage = nullptr;

	m_device.freeMemory(m_msaaDepthMem, nullptr);
	m_msaaDepthMem = nullptr;

	// Render Pass
	m_device.destroyRenderPass(m_renderPass, nullptr);
	m_renderPass = nullptr;

	// MMD Draw Pipeline
	m_device.destroyDescriptorSetLayout(m_mmdDescriptorSetLayout, nullptr);
	m_mmdDescriptorSetLayout = nullptr;

	m_device.destroyPipelineLayout(m_mmdPipelineLayout, nullptr);
	m_mmdPipelineLayout = nullptr;

	for (auto& pipeline : m_mmdPipelines)
	{
		m_device.destroyPipeline(pipeline, nullptr);
		pipeline = nullptr;
	}

	m_device.destroyShaderModule(m_mmdVSModule, nullptr);
	m_mmdVSModule = nullptr;

	m_device.destroyShaderModule(m_mmdFSModule, nullptr);
	m_mmdFSModule = nullptr;

	// MMD Edge Draw Pipeline
	m_device.destroyDescriptorSetLayout(m_mmdEdgeDescriptorSetLayout, nullptr);
	m_mmdEdgeDescriptorSetLayout = nullptr;

	m_device.destroyPipelineLayout(m_mmdEdgePipelineLayout, nullptr);
	m_mmdEdgePipelineLayout = nullptr;

	m_device.destroyPipeline(m_mmdEdgePipeline, nullptr);
	m_mmdEdgePipeline = nullptr;

	m_device.destroyShaderModule(m_mmdEdgeVSModule, nullptr);
	m_mmdEdgeVSModule = nullptr;

	m_device.destroyShaderModule(m_mmdEdgeFSModule, nullptr);
	m_mmdEdgeFSModule = nullptr;

	// MMD Ground Shadow Draw Pipeline
	m_device.destroyDescriptorSetLayout(m_mmdGroundShadowDescriptorSetLayout, nullptr);
	m_mmdGroundShadowDescriptorSetLayout = nullptr;

	m_device.destroyPipelineLayout(m_mmdGroundShadowPipelineLayout, nullptr);
	m_mmdGroundShadowPipelineLayout = nullptr;

	m_device.destroyPipeline(m_mmdGroundShadowPipeline, nullptr);
	m_mmdGroundShadowPipeline = nullptr;

	m_device.destroyShaderModule(m_mmdGroundShadowVSModule, nullptr);
	m_mmdGroundShadowVSModule = nullptr;

	m_device.destroyShaderModule(m_mmdGroundShadowFSModule, nullptr);
	m_mmdGroundShadowFSModule = nullptr;

	// Pipeline Cache
	m_device.destroyPipelineCache(m_pipelineCache, nullptr);
	m_pipelineCache = nullptr;

	// Queue
	m_graphicsQueue = nullptr;

	// Staging Buffer
	for (auto& stBuf : m_stagingBuffers)
	{
		stBuf->Clear(*this);
	}
	m_stagingBuffers.clear();

	// Texture
	for (auto& it : m_textures)
	{
		it.second->Clear(*this);
	}
	m_textures.clear();

	if (m_defaultTexture)
	{
		m_defaultTexture->Clear(*this);
		m_defaultTexture.reset();
	}
	m_device.destroySampler(m_defaultSampler, nullptr);
	m_defaultSampler = nullptr;

	// Command Pool
	m_device.destroyCommandPool(m_commandPool, nullptr);
	m_commandPool = nullptr;

	m_device.destroyCommandPool(m_transferCommandPool, nullptr);
	m_transferCommandPool = nullptr;

}

bool AppContext::Prepare()
{
	if (!PrepareCommandPool()) { return false; }
	if (!PrepareBuffer()) { return false; }
	if (!PrepareSyncObjects()) { return false; }
	if (!PrepareRenderPass()) { return false; }
	if (!PrepareFramebuffer()) { return false; }
	if (!PreparePipelineCache()) { return false; }
	if (!PrepareMMDPipeline()) { return false; }
	if (!PrepareMMDEdgePipeline()) { return false; }
	if (!PrepareMMDGroundShadowPipeline()) { return false; }
	if (!PrepareDefaultTexture()) { return false; }
	return true;
}

bool AppContext::PrepareCommandPool()
{
	vk::Result ret;

	// Create command pool
	auto cmdPoolInfo = vk::CommandPoolCreateInfo()
		.setQueueFamilyIndex(m_graphicsQueueFamilyIndex)
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	ret = m_device.createCommandPool(&cmdPoolInfo, nullptr, &m_commandPool);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create Command Pool.\n";
		return false;
	}

	// Create command pool (Transfer)
	auto transferCmdPoolInfo = vk::CommandPoolCreateInfo()
		.setQueueFamilyIndex(m_graphicsQueueFamilyIndex)
		.setFlags(vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	ret = m_device.createCommandPool(&transferCmdPoolInfo, nullptr, &m_transferCommandPool);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create Transfer Command Pool.\n";
		return false;
	}

	return true;
}

bool AppContext::PrepareBuffer()
{
	for (auto& res : m_swapchainImageResouces)
	{
		res.Clear(*this);
	}
	m_swapchainImageResouces.clear();

	// Clear depth
	m_device.destroyImageView(m_depthImageView, nullptr);
	m_depthImageView = nullptr;

	m_device.destroyImage(m_depthImage, nullptr);
	m_depthImage = nullptr;

	m_device.freeMemory(m_depthMem, nullptr);
	m_depthMem = nullptr;

	// Clear msaa color
	m_device.destroyImageView(m_msaaColorImageView, nullptr);
	m_msaaColorImageView = nullptr;

	m_device.destroyImage(m_msaaColorImage, nullptr);
	m_msaaColorImage = nullptr;

	m_device.freeMemory(m_msaaColorMem, nullptr);
	m_msaaColorMem = nullptr;

	// Clear msaa depth
	m_device.destroyImageView(m_msaaDepthImageView, nullptr);
	m_msaaDepthImageView = nullptr;

	m_device.destroyImage(m_msaaDepthImage, nullptr);
	m_msaaDepthImage = nullptr;

	m_device.freeMemory(m_msaaDepthMem, nullptr);
	m_msaaDepthMem = nullptr;

	// Prepare swapchain
	auto oldSwapchain =
		m_swapchain;

	auto surfaceCaps = m_gpu.getSurfaceCapabilitiesKHR(m_surface);

	vk::PresentModeKHR selectPresentModes[] = {
		vk::PresentModeKHR::eMailbox,
		vk::PresentModeKHR::eImmediate,
		vk::PresentModeKHR::eFifo,
	};
	auto presentModes = m_gpu.getSurfacePresentModesKHR(m_surface);
	bool findPresentMode = false;
	vk::PresentModeKHR selectPresentMode;
	for (auto selectMode : selectPresentModes)
	{
		for (auto presentMode : presentModes)
		{
			if (selectMode == presentMode)
			{
				selectPresentMode = selectMode;
				findPresentMode = true;
				break;
			}
		}
		if (findPresentMode)
		{
			break;
		}
	}
	if (!findPresentMode)
	{
		std::cout << "Present mode unsupported.\n";
		return false;
	}
	std::cout << "Select present mode [" << int(selectPresentMode) << "]\n";

	auto formats = m_gpu.getSurfaceFormatsKHR(m_surface);
	auto format = vk::Format::eB8G8R8A8Unorm;
	uint32_t selectFmtIdx = UINT32_MAX;
	for (uint32_t i = 0; i < (uint32_t)formats.size(); i++)
	{
		if (formats[i].format == format)
		{
			selectFmtIdx = i;
			break;
		}
	}
	if (selectFmtIdx == UINT32_MAX)
	{
		std::cout << "Faild to find surface format.\n";
		return false;
	}
	auto colorSpace = formats[selectFmtIdx].colorSpace;

	vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	vk::CompositeAlphaFlagBitsKHR compositeAlphaFlags[] =
	{
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
		vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
		vk::CompositeAlphaFlagBitsKHR::eInherit
	};
	for (const auto& flag : compositeAlphaFlags)
	{
		if (surfaceCaps.supportedCompositeAlpha & flag)
		{
			compositeAlpha = flag;
			break;
		}
	}

	uint32_t indices[] = {
		m_graphicsQueueFamilyIndex,
		m_presentQueueFamilyIndex,
	};
	uint32_t* queueFamilyIndices = nullptr;
	uint32_t queueFamilyCount = 0;
	auto sharingMode = vk::SharingMode::eExclusive;
	if (m_graphicsQueueFamilyIndex != m_presentQueueFamilyIndex)
	{
		queueFamilyIndices = indices;
		queueFamilyCount = std::extent<decltype(indices)>::value;
		sharingMode = vk::SharingMode::eConcurrent;
	}
	auto swapChainInfo = vk::SwapchainCreateInfoKHR()
		.setSurface(m_surface)
		.setMinImageCount(m_imageCount)
		.setImageFormat(format)
		.setImageColorSpace(colorSpace)
		.setImageExtent(surfaceCaps.currentExtent)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		.setImageSharingMode(sharingMode)
		.setQueueFamilyIndexCount(queueFamilyCount)
		.setPQueueFamilyIndices(queueFamilyIndices)
		.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
		.setCompositeAlpha(compositeAlpha)
		.setPresentMode(selectPresentMode)
		.setClipped(true)
		.setOldSwapchain(oldSwapchain);
	vk::SwapchainKHR swapchain;
	vk::Result ret;
	ret = m_device.createSwapchainKHR(&swapChainInfo, nullptr, &swapchain);
	if (ret != vk::Result::eSuccess)
	{
		std::cout << "Failed to create Swap Chain.\n";
		return false;
	}
	m_screenWidth = surfaceCaps.currentExtent.width;
	m_screenHeight = surfaceCaps.currentExtent.height;

	m_swapchain = swapchain;
	if (oldSwapchain)
	{
		m_device.destroySwapchainKHR(oldSwapchain);
	}

	auto swapchainImages = m_device.getSwapchainImagesKHR(swapchain);
	m_swapchainImageResouces.resize(swapchainImages.size());
	for (size_t i = 0; i < swapchainImages.size(); i++)
	{
		m_swapchainImageResouces[i].m_image = swapchainImages[i];
		auto imageViewInfo = vk::ImageViewCreateInfo()
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(format)
			.setImage(swapchainImages[i])
			.setSubresourceRange(
				vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
			);
		vk::ImageView imageView;
		ret = m_device.createImageView(&imageViewInfo, nullptr, &imageView);
		if (vk::Result::eSuccess != ret)
		{
			std::cout << "Failed to create Image View.\n";
			return false;
		}
		m_swapchainImageResouces[i].m_imageView = imageView;

		// CommandBuffer
		auto cmdBufInfo = vk::CommandBufferAllocateInfo()
			.setCommandBufferCount(1)
			.setCommandPool(m_commandPool)
			.setLevel(vk::CommandBufferLevel::ePrimary);
		vk::CommandBuffer cmdBuf;
		ret = m_device.allocateCommandBuffers(&cmdBufInfo, &cmdBuf);
		if (vk::Result::eSuccess != ret)
		{
			std::cout << "Failed to create Command Buffer.\n";
			return false;
		}
		m_swapchainImageResouces[i].m_cmdBuffer = cmdBuf;
	}

	// Depth buffer

	// Create the depth buffer image object
	auto depthFormant = m_depthFormat;
	auto depthImageInfo = vk::ImageCreateInfo()
		.setImageType(vk::ImageType::e2D)
		.setFormat(depthFormant)
		.setExtent({ (uint32_t)m_screenWidth, (uint32_t)m_screenHeight, 1 })
		.setMipLevels(1)
		.setArrayLayers(1)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
		.setPQueueFamilyIndices(nullptr)
		.setQueueFamilyIndexCount(0)
		.setSharingMode(vk::SharingMode::eExclusive);
	vk::Image depthImage;
	ret = m_device.createImage(&depthImageInfo, nullptr, &depthImage);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create Image.\n";
		return false;
	}
	m_depthImage = depthImage;

	// Allocate the Memory for Depth Buffer
	auto depthMemoReq = m_device.getImageMemoryRequirements(depthImage);
	uint32_t depthMemIdx = 0;
	if (!FindMemoryTypeIndex(
		m_memProperties,
		depthMemoReq.memoryTypeBits,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		&depthMemIdx))
	{
		std::cout << "Failed to find memory property.\n";
		return false;
	}
	auto depthMemAllocInfo = vk::MemoryAllocateInfo()
		.setAllocationSize(depthMemoReq.size)
		.setMemoryTypeIndex(depthMemIdx);
	vk::DeviceMemory depthMem;
	ret = m_device.allocateMemory(&depthMemAllocInfo, nullptr, &depthMem);
	if (ret != vk::Result::eSuccess)
	{
		std::cout << "Failed to allocate depth memory.\n";
		return false;
	}
	m_depthMem = depthMem;

	// Bind thee Memory to the Depth Buffer
	m_device.bindImageMemory(depthImage, depthMem, 0);

	// Create the Depth Image View
	auto depthImageViewInfo = vk::ImageViewCreateInfo()
		.setImage(depthImage)
		.setFormat(m_depthFormat)
		.setViewType(vk::ImageViewType::e2D)
		.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
	vk::ImageView depthImageView;
	ret = m_device.createImageView(&depthImageViewInfo, nullptr, &depthImageView);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create Image View.\n";
		return false;
	}
	m_depthImageView = depthImageView;


	// MSAA color buffer

	// Create the msaa color buffer image object
	auto msaaColorFormant = m_colorFormat;
	auto msaaColorImageInfo = vk::ImageCreateInfo()
		.setImageType(vk::ImageType::e2D)
		.setFormat(msaaColorFormant)
		.setExtent({ (uint32_t)m_screenWidth, (uint32_t)m_screenHeight, 1 })
		.setMipLevels(1)
		.setArrayLayers(1)
		.setSamples(m_msaaSampleCount)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setUsage(vk::ImageUsageFlagBits::eColorAttachment)
		.setPQueueFamilyIndices(nullptr)
		.setQueueFamilyIndexCount(0)
		.setSharingMode(vk::SharingMode::eExclusive);
	vk::Image msaaColorImage;
	ret = m_device.createImage(&msaaColorImageInfo, nullptr, &msaaColorImage);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MSAA Color Image.\n";
		return false;
	}
	m_msaaColorImage = msaaColorImage;

	// Allocate the Memory for MSAA Color Buffer
	auto msaaColorMemoReq = m_device.getImageMemoryRequirements(msaaColorImage);
	uint32_t msaaColorMemIdx = 0;
	if (!FindMemoryTypeIndex(
		m_memProperties,
		msaaColorMemoReq.memoryTypeBits,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		&msaaColorMemIdx))
	{
		std::cout << "Failed to find MSAA Color memory property.\n";
		return false;
	}
	auto msaaColorMemAllocInfo = vk::MemoryAllocateInfo()
		.setAllocationSize(msaaColorMemoReq.size)
		.setMemoryTypeIndex(msaaColorMemIdx);
	vk::DeviceMemory msaaColorMem;
	ret = m_device.allocateMemory(&msaaColorMemAllocInfo, nullptr, &msaaColorMem);
	if (ret != vk::Result::eSuccess)
	{
		std::cout << "Failed to allocate MSAA Color memory.\n";
		return false;
	}
	m_msaaColorMem = msaaColorMem;

	// Bind thee Memory to the MSAA Color Buffer
	m_device.bindImageMemory(msaaColorImage, msaaColorMem, 0);

	// Create the msaa color Image View
	auto msaaColorImageViewInfo = vk::ImageViewCreateInfo()
		.setImage(msaaColorImage)
		.setFormat(msaaColorFormant)
		.setViewType(vk::ImageViewType::e2D)
		.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
	vk::ImageView msaaColorImageView;
	ret = m_device.createImageView(&msaaColorImageViewInfo, nullptr, &msaaColorImageView);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MSAA Color Image View.\n";
		return false;
	}
	m_msaaColorImageView = msaaColorImageView;

	// MSAA Depth buffer

	// Create the msaa Depth buffer image object
	auto msaaDepthFormant = m_depthFormat;
	auto msaaDepthImageInfo = vk::ImageCreateInfo()
		.setImageType(vk::ImageType::e2D)
		.setFormat(msaaDepthFormant)
		.setExtent({ (uint32_t)m_screenWidth, (uint32_t)m_screenHeight, 1 })
		.setMipLevels(1)
		.setArrayLayers(1)
		.setSamples(m_msaaSampleCount)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
		.setPQueueFamilyIndices(nullptr)
		.setQueueFamilyIndexCount(0)
		.setSharingMode(vk::SharingMode::eExclusive);
	vk::Image msaaDepthImage;
	ret = m_device.createImage(&msaaDepthImageInfo, nullptr, &msaaDepthImage);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MSAA Depth Image.\n";
		return false;
	}
	m_msaaDepthImage = msaaDepthImage;

	// Allocate the Memory for MSAA Depth Buffer
	auto msaaDepthMemoReq = m_device.getImageMemoryRequirements(msaaDepthImage);
	uint32_t msaaDepthMemIdx = 0;
	if (!FindMemoryTypeIndex(
		m_memProperties,
		msaaDepthMemoReq.memoryTypeBits,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		&msaaDepthMemIdx))
	{
		std::cout << "Failed to find MSAA Depth memory property.\n";
		return false;
	}
	auto msaaDepthMemAllocInfo = vk::MemoryAllocateInfo()
		.setAllocationSize(msaaDepthMemoReq.size)
		.setMemoryTypeIndex(msaaDepthMemIdx);
	vk::DeviceMemory msaaDepthMem;
	ret = m_device.allocateMemory(&msaaDepthMemAllocInfo, nullptr, &msaaDepthMem);
	if (ret != vk::Result::eSuccess)
	{
		std::cout << "Failed to allocate MSAA Depth memory.\n";
		return false;
	}
	m_msaaDepthMem = msaaDepthMem;

	// Bind thee Memory to the MSAA Depth Buffer
	m_device.bindImageMemory(msaaDepthImage, msaaDepthMem, 0);

	// Create the MSAA Depth Image View
	auto msaaDepthImageViewInfo = vk::ImageViewCreateInfo()
		.setImage(msaaDepthImage)
		.setFormat(msaaDepthFormant)
		.setViewType(vk::ImageViewType::e2D)
		.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
	vk::ImageView msaaDepthImageView;
	ret = m_device.createImageView(&msaaDepthImageViewInfo, nullptr, &msaaDepthImageView);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MSAA Depth Image View.\n";
		return false;
	}
	m_msaaDepthImageView = msaaDepthImageView;

	m_imageIndex = 0;

	return true;
}

bool AppContext::PrepareSyncObjects()
{
	vk::Result ret;
	m_frameSyncDatas.resize(m_imageCount);
	for (auto& frameSyncData : m_frameSyncDatas)
	{
		auto semaphoreInfo = vk::SemaphoreCreateInfo();
		ret = m_device.createSemaphore(&semaphoreInfo, nullptr, &frameSyncData.m_presentCompleteSemaphore);
		if (vk::Result::eSuccess != ret)
		{
			std::cout << "Failed to create Present Complete Semaphore.\n";
			return false;
		}

		ret = m_device.createSemaphore(&semaphoreInfo, nullptr, &frameSyncData.m_renderCompleteSemaphore);
		if (vk::Result::eSuccess != ret)
		{
			std::cout << "Failed to create Render Complete Semaphore.\n";
			return false;
		}

		auto fenceInfo = vk::FenceCreateInfo()
			.setFlags(vk::FenceCreateFlagBits::eSignaled);
		ret = m_device.createFence(&fenceInfo, nullptr, &frameSyncData.m_fence);
		if (vk::Result::eSuccess != ret)
		{
			std::cout << "Failed to create Wait Fence.\n";
			return false;
		}
	}

	return true;
}

bool AppContext::PrepareRenderPass()
{
	vk::Result ret;
	// Create the Render pass
	auto msaaColorAttachment = vk::AttachmentDescription()
		.setFormat(m_colorFormat)
		.setSamples(m_msaaSampleCount)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
	auto colorAttachment = vk::AttachmentDescription()
		.setFormat(m_colorFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
	auto msaaDepthAttachment = vk::AttachmentDescription()
		.setFormat(m_depthFormat)
		.setSamples(m_msaaSampleCount)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setStencilLoadOp(vk::AttachmentLoadOp::eClear)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	auto depthAttachment = vk::AttachmentDescription()
		.setFormat(m_depthFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setStencilLoadOp(vk::AttachmentLoadOp::eClear)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	auto colorRef = vk::AttachmentReference()
		.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	auto depthRef = vk::AttachmentReference()
		.setAttachment(2)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	auto resolveRef = vk::AttachmentReference()
		.setAttachment(1)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	auto subpass = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setInputAttachmentCount(0)
		.setPInputAttachments(nullptr)
		.setColorAttachmentCount(1)
		.setPColorAttachments(&colorRef)
		.setPResolveAttachments(&resolveRef)
		.setPDepthStencilAttachment(&depthRef)
		.setPreserveAttachmentCount(0)
		.setPPreserveAttachments(nullptr);
	vk::SubpassDependency dependencies[2];
	dependencies[0]
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
		.setDependencyFlags(vk::DependencyFlagBits::eByRegion);
	dependencies[1]
		.setSrcSubpass(0)
		.setDstSubpass(VK_SUBPASS_EXTERNAL)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
		.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
		.setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
		.setDependencyFlags(vk::DependencyFlagBits::eByRegion);
	vk::AttachmentDescription attachments[] = {
		msaaColorAttachment,
		colorAttachment,
		msaaDepthAttachment,
		depthAttachment
	};
	auto renderPassInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount(std::extent<decltype(attachments)>::value)
		.setPAttachments(attachments)
		.setSubpassCount(1)
		.setPSubpasses(&subpass)
		.setDependencyCount(std::extent<decltype(dependencies)>::value)
		.setPDependencies(dependencies);

	vk::RenderPass renderPass;
	ret = m_device.createRenderPass(&renderPassInfo, nullptr, &renderPass);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create Render Pass.\n";
		return false;
	}
	m_renderPass = renderPass;
	return true;
}

bool AppContext::PrepareFramebuffer()
{
	vk::Result ret;

	vk::ImageView attachments[4];

	attachments[0] = m_msaaColorImageView;
	// attachments[1] = swapchain image
	attachments[2] = m_msaaDepthImageView;
	attachments[3] = m_depthImageView;

	auto framebufferInfo = vk::FramebufferCreateInfo()
		.setRenderPass(m_renderPass)
		.setAttachmentCount(std::extent<decltype(attachments)>::value)
		.setPAttachments(attachments)
		.setWidth((uint32_t)m_screenWidth)
		.setHeight((uint32_t)m_screenHeight)
		.setLayers(1);
	for (size_t i = 0; i < m_swapchainImageResouces.size(); i++)
	{
		attachments[1] = m_swapchainImageResouces[i].m_imageView;
		vk::Framebuffer framebuffer;
		ret = m_device.createFramebuffer(&framebufferInfo, nullptr, &framebuffer);
		if (vk::Result::eSuccess != ret)
		{
			std::cout << "Failed to create Framebuffer.\n";
			return false;
		}
		m_swapchainImageResouces[i].m_framebuffer = framebuffer;
	}

	return true;
}

bool AppContext::PreparePipelineCache()
{
	vk::Result ret;

	auto pipelineCacheInfo = vk::PipelineCacheCreateInfo();
	ret = m_device.createPipelineCache(&pipelineCacheInfo, nullptr, &m_pipelineCache);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create Pipeline Cache.\n";
		return false;
	}
	return true;
}

bool AppContext::PrepareMMDPipeline()
{
	vk::Result ret;

	// VS Binding
	auto vsUnifomDescSetBinding = vk::DescriptorSetLayoutBinding()
		.setBinding(0)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);

	// FS Binding
	auto fsUniformDescSetBinding = vk::DescriptorSetLayoutBinding()
		.setBinding(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	auto fsTexDescSetBinding = vk::DescriptorSetLayoutBinding()
		.setBinding(2)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	auto fsToonTexDescSetBinding = vk::DescriptorSetLayoutBinding()
		.setBinding(3)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	auto fsSphereTexDescSetBinding = vk::DescriptorSetLayoutBinding()
		.setBinding(4)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutBinding bindings[] = {
		vsUnifomDescSetBinding,
		fsUniformDescSetBinding,
		fsTexDescSetBinding,
		fsToonTexDescSetBinding,
		fsSphereTexDescSetBinding,
	};

	// Create Descriptor Set Layout
	auto descSetLayoutInfo = vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount(std::extent<decltype(bindings)>::value)
		.setPBindings(bindings);
	ret = m_device.createDescriptorSetLayout(&descSetLayoutInfo, nullptr, &m_mmdDescriptorSetLayout);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Descriptor Set Layout.\n";
		return false;
	}

	// Create Pipeline Layout
	auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
		.setSetLayoutCount(1)
		.setPSetLayouts(&m_mmdDescriptorSetLayout);
	ret = m_device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &m_mmdPipelineLayout);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Pipeline Layout.\n";
		return false;
	}

	// Create Vertex Shader Module
	auto vsInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(sizeof(mmd_vert_spv_data))
		.setPCode(reinterpret_cast<const uint32_t*>(mmd_vert_spv_data));
	ret = m_device.createShaderModule(&vsInfo, nullptr, &m_mmdVSModule);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Vertex Shader Module.\n";
		return false;
	}

	// Create Fragment Shader Module
	auto fsInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(sizeof(mmd_frag_spv_data))
		.setPCode(reinterpret_cast<const uint32_t*>(mmd_frag_spv_data));
	ret = m_device.createShaderModule(&fsInfo, nullptr, &m_mmdFSModule);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Fragment Shader Module.\n";
		return false;
	}

	// Pipeline

	auto pipelineInfo = vk::GraphicsPipelineCreateInfo()
		.setLayout(m_mmdPipelineLayout)
		.setRenderPass(m_renderPass);

	// Input Assembly
	auto inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(vk::PrimitiveTopology::eTriangleList);
	pipelineInfo.setPInputAssemblyState(&inputAssemblyInfo);

	// Rasterization State
	auto rasterizationInfo = vk::PipelineRasterizationStateCreateInfo()
		.setPolygonMode(vk::PolygonMode::eFill)
		.setCullMode(vk::CullModeFlagBits::eNone)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setDepthBiasEnable(false)
		.setLineWidth(1.0f);
	pipelineInfo.setPRasterizationState(&rasterizationInfo);

	// Color blend state
	auto colorBlendAttachmentInfo = vk::PipelineColorBlendAttachmentState()
		.setColorWriteMask(vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA)
		.setBlendEnable(false);
	auto colorBlendStateInfo = vk::PipelineColorBlendStateCreateInfo()
		.setAttachmentCount(1)
		.setPAttachments(&colorBlendAttachmentInfo);
	pipelineInfo.setPColorBlendState(&colorBlendStateInfo);

	// Viewport State
	auto viewportInfo = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1)
		.setScissorCount(1);
	pipelineInfo.setPViewportState(&viewportInfo);

	// Dynamic State
	vk::DynamicState dynamicStates[2] = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};
	auto dynamicInfo = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStateCount(std::extent<decltype(dynamicStates)>::value)
		.setPDynamicStates(dynamicStates);
	pipelineInfo.setPDynamicState(&dynamicInfo);

	// Depth and Stencil State
	auto depthAndStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
		.setDepthBoundsTestEnable(false)
		.setBack(vk::StencilOpState()
			.setFailOp(vk::StencilOp::eKeep)
			.setPassOp(vk::StencilOp::eKeep)
			.setCompareOp(vk::CompareOp::eAlways))
		.setStencilTestEnable(false);
	depthAndStencilInfo.front = depthAndStencilInfo.back;
	pipelineInfo.setPDepthStencilState(&depthAndStencilInfo);

	// Multisample
	auto multisampleInfo = vk::PipelineMultisampleStateCreateInfo()
		.setRasterizationSamples(m_msaaSampleCount)
		.setSampleShadingEnable(true)
		.setMinSampleShading(0.25f);
	pipelineInfo.setPMultisampleState(&multisampleInfo);

	// Vertex input binding
	auto vertexInputBindingDesc = vk::VertexInputBindingDescription()
		.setBinding(0)
		.setStride(sizeof(Vertex))
		.setInputRate(vk::VertexInputRate::eVertex);
	auto posAttr = vk::VertexInputAttributeDescription()
		.setBinding(0)
		.setLocation(0)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(Vertex, m_position));
	auto normalAttr = vk::VertexInputAttributeDescription()
		.setBinding(0)
		.setLocation(1)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(Vertex, m_normal));
	auto uvAttr = vk::VertexInputAttributeDescription()
		.setBinding(0)
		.setLocation(2)
		.setFormat(vk::Format::eR32G32Sfloat)
		.setOffset(offsetof(Vertex, m_uv));
	vk::VertexInputAttributeDescription vertexInputAttrs[3] = {
		posAttr,
		normalAttr,
		uvAttr,
	};

	auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptionCount(1)
		.setPVertexBindingDescriptions(&vertexInputBindingDesc)
		.setVertexAttributeDescriptionCount(std::extent<decltype(vertexInputAttrs)>::value)
		.setPVertexAttributeDescriptions(vertexInputAttrs);
	pipelineInfo.setPVertexInputState(&vertexInputInfo);

	// Shader
	auto vsStageInfo = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(m_mmdVSModule)
		.setPName("main");
	auto fsStageInfo = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(m_mmdFSModule)
		.setPName("main");
	vk::PipelineShaderStageCreateInfo shaderStages[2] = {
		vsStageInfo,
		fsStageInfo,
	};
	pipelineInfo
		.setStageCount(std::extent<decltype(shaderStages)>::value)
		.setPStages(shaderStages);

	// Set alpha blend mode
	colorBlendAttachmentInfo
		.setBlendEnable(true)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
		.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
		.setAlphaBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
		.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);

	// Set front face mode
	rasterizationInfo.
		setCullMode(vk::CullModeFlagBits::eBack);

	ret = m_device.createGraphicsPipelines(
		m_pipelineCache,
		1, &pipelineInfo, nullptr,
		&m_mmdPipelines[int(MMDRenderType::AlphaBlend_FrontFace)]);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Pipeline.\n";
		return false;
	}

	// Set both face mode
	rasterizationInfo.
		setCullMode(vk::CullModeFlagBits::eNone);

	ret = m_device.createGraphicsPipelines(
		m_pipelineCache,
		1, &pipelineInfo, nullptr,
		&m_mmdPipelines[int(MMDRenderType::AlphaBlend_BothFace)]);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Pipeline.\n";
		return false;
	}

	return true;
}


bool AppContext::PrepareMMDEdgePipeline()
{
	vk::Result ret;

	// VS Binding
	auto vsModelUnifomDescSetBinding = vk::DescriptorSetLayoutBinding()
		.setBinding(0)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);

	auto vsMatUnifomDescSetBinding = vk::DescriptorSetLayoutBinding()
		.setBinding(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);

	// FS Binding
	auto fsMatUniformDescSetBinding = vk::DescriptorSetLayoutBinding()
		.setBinding(2)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutBinding bindings[] = {
		vsModelUnifomDescSetBinding,
		vsMatUnifomDescSetBinding,
		fsMatUniformDescSetBinding,
	};

	// Create Descriptor Set Layout
	auto descSetLayoutInfo = vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount(std::extent<decltype(bindings)>::value)
		.setPBindings(bindings);
	ret = m_device.createDescriptorSetLayout(&descSetLayoutInfo, nullptr, &m_mmdEdgeDescriptorSetLayout);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Edge Descriptor Set Layout.\n";
		return false;
	}

	// Create Pipeline Layout
	auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
		.setSetLayoutCount(1)
		.setPSetLayouts(&m_mmdEdgeDescriptorSetLayout);
	ret = m_device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &m_mmdEdgePipelineLayout);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Edge Pipeline Layout.\n";
		return false;
	}

	// Create Vertex Shader Module
	auto vsInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(sizeof(mmd_edge_vert_spv_data))
		.setPCode(reinterpret_cast<const uint32_t*>(mmd_edge_vert_spv_data));
	ret = m_device.createShaderModule(&vsInfo, nullptr, &m_mmdEdgeVSModule);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Edge Vertex Shader Module.\n";
		return false;
	}

	// Create Fragment Shader Module
	auto fsInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(sizeof(mmd_edge_frag_spv_data))
		.setPCode(reinterpret_cast<const uint32_t*>(mmd_edge_frag_spv_data));
	ret = m_device.createShaderModule(&fsInfo, nullptr, &m_mmdEdgeFSModule);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Edge Fragment Shader Module.\n";
		return false;
	}

	// Pipeline
	auto pipelineInfo = vk::GraphicsPipelineCreateInfo()
		.setLayout(m_mmdEdgePipelineLayout)
		.setRenderPass(m_renderPass);

	// Input Assembly
	auto inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(vk::PrimitiveTopology::eTriangleList);
	pipelineInfo.setPInputAssemblyState(&inputAssemblyInfo);

	// Rasterization State
	auto rasterizationInfo = vk::PipelineRasterizationStateCreateInfo()
		.setPolygonMode(vk::PolygonMode::eFill)
		.setCullMode(vk::CullModeFlagBits::eNone)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setDepthBiasEnable(false)
		.setLineWidth(1.0f);
	pipelineInfo.setPRasterizationState(&rasterizationInfo);

	// Color blend state
	auto colorBlendAttachmentInfo = vk::PipelineColorBlendAttachmentState()
		.setColorWriteMask(vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA)
		.setBlendEnable(false);
	auto colorBlendStateInfo = vk::PipelineColorBlendStateCreateInfo()
		.setAttachmentCount(1)
		.setPAttachments(&colorBlendAttachmentInfo);
	pipelineInfo.setPColorBlendState(&colorBlendStateInfo);

	// Viewport State
	auto viewportInfo = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1)
		.setScissorCount(1);
	pipelineInfo.setPViewportState(&viewportInfo);

	// Dynamic State
	vk::DynamicState dynamicStates[2] = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};
	auto dynamicInfo = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStateCount(std::extent<decltype(dynamicStates)>::value)
		.setPDynamicStates(dynamicStates);
	pipelineInfo.setPDynamicState(&dynamicInfo);

	// Depth and Stencil State
	auto depthAndStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
		.setDepthBoundsTestEnable(false)
		.setBack(vk::StencilOpState()
			.setFailOp(vk::StencilOp::eKeep)
			.setPassOp(vk::StencilOp::eKeep)
			.setCompareOp(vk::CompareOp::eAlways))
		.setStencilTestEnable(false);
	depthAndStencilInfo.front = depthAndStencilInfo.back;
	pipelineInfo.setPDepthStencilState(&depthAndStencilInfo);

	// Multisample
	auto multisampleInfo = vk::PipelineMultisampleStateCreateInfo()
		.setRasterizationSamples(m_msaaSampleCount)
		.setSampleShadingEnable(true)
		.setMinSampleShading(0.25f);

	pipelineInfo.setPMultisampleState(&multisampleInfo);

	// Vertex input binding
	auto vertexInputBindingDesc = vk::VertexInputBindingDescription()
		.setBinding(0)
		.setStride(sizeof(Vertex))
		.setInputRate(vk::VertexInputRate::eVertex);
	auto posAttr = vk::VertexInputAttributeDescription()
		.setBinding(0)
		.setLocation(0)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(Vertex, m_position));
	auto normalAttr = vk::VertexInputAttributeDescription()
		.setBinding(0)
		.setLocation(1)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(Vertex, m_normal));
	vk::VertexInputAttributeDescription vertexInputAttrs[] = {
		posAttr,
		normalAttr,
	};

	auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptionCount(1)
		.setPVertexBindingDescriptions(&vertexInputBindingDesc)
		.setVertexAttributeDescriptionCount(std::extent<decltype(vertexInputAttrs)>::value)
		.setPVertexAttributeDescriptions(vertexInputAttrs);
	pipelineInfo.setPVertexInputState(&vertexInputInfo);

	// Shader
	auto vsStageInfo = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(m_mmdEdgeVSModule)
		.setPName("main");
	auto fsStageInfo = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(m_mmdEdgeFSModule)
		.setPName("main");
	vk::PipelineShaderStageCreateInfo shaderStages[2] = {
		vsStageInfo,
		fsStageInfo,
	};
	pipelineInfo
		.setStageCount(std::extent<decltype(shaderStages)>::value)
		.setPStages(shaderStages);

	// Set alpha blend mode
	colorBlendAttachmentInfo
		.setBlendEnable(true)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
		.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
		.setAlphaBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
		.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);

	// Set front face mode
	rasterizationInfo.
		setCullMode(vk::CullModeFlagBits::eFront);

	ret = m_device.createGraphicsPipelines(
		m_pipelineCache,
		1, &pipelineInfo, nullptr,
		&m_mmdEdgePipeline);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Edge Pipeline.\n";
		return false;
	}

	return true;
}

bool AppContext::PrepareMMDGroundShadowPipeline()
{
	vk::Result ret;

	// VS Binding
	auto vsModelUnifomDescSetBinding = vk::DescriptorSetLayoutBinding()
		.setBinding(0)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);

	// FS Binding
	auto fsMatUniformDescSetBinding = vk::DescriptorSetLayoutBinding()
		.setBinding(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutBinding bindings[] = {
		vsModelUnifomDescSetBinding,
		fsMatUniformDescSetBinding,
	};

	// Create Descriptor Set Layout
	auto descSetLayoutInfo = vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount(std::extent<decltype(bindings)>::value)
		.setPBindings(bindings);
	ret = m_device.createDescriptorSetLayout(&descSetLayoutInfo, nullptr, &m_mmdGroundShadowDescriptorSetLayout);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Ground Shadow Descriptor Set Layout.\n";
		return false;
	}

	// Create Pipeline Layout
	auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
		.setSetLayoutCount(1)
		.setPSetLayouts(&m_mmdGroundShadowDescriptorSetLayout);
	ret = m_device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &m_mmdGroundShadowPipelineLayout);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Ground Shadow Pipeline Layout.\n";
		return false;
	}

	// Create Vertex Shader Module
	auto vsInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(sizeof(mmd_ground_shadow_vert_spv_data))
		.setPCode(reinterpret_cast<const uint32_t*>(mmd_ground_shadow_vert_spv_data));
	ret = m_device.createShaderModule(&vsInfo, nullptr, &m_mmdGroundShadowVSModule);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Ground Shadow Vertex Shader Module.\n";
		return false;
	}

	// Create Fragment Shader Module
	auto fsInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(sizeof(mmd_ground_shadow_frag_spv_data))
		.setPCode(reinterpret_cast<const uint32_t*>(mmd_ground_shadow_frag_spv_data));
	ret = m_device.createShaderModule(&fsInfo, nullptr, &m_mmdGroundShadowFSModule);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD GroundShadow Fragment Shader Module.\n";
		return false;
	}

	// Pipeline
	auto pipelineInfo = vk::GraphicsPipelineCreateInfo()
		.setLayout(m_mmdGroundShadowPipelineLayout)
		.setRenderPass(m_renderPass);

	// Input Assembly
	auto inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(vk::PrimitiveTopology::eTriangleList);
	pipelineInfo.setPInputAssemblyState(&inputAssemblyInfo);

	// Rasterization State
	auto rasterizationInfo = vk::PipelineRasterizationStateCreateInfo()
		.setPolygonMode(vk::PolygonMode::eFill)
		.setCullMode(vk::CullModeFlagBits::eNone)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setDepthBiasEnable(false)
		.setLineWidth(1.0f);
	pipelineInfo.setPRasterizationState(&rasterizationInfo);

	// Color blend state
	auto colorBlendAttachmentInfo = vk::PipelineColorBlendAttachmentState()
		.setColorWriteMask(vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA)
		.setBlendEnable(false);
	auto colorBlendStateInfo = vk::PipelineColorBlendStateCreateInfo()
		.setAttachmentCount(1)
		.setPAttachments(&colorBlendAttachmentInfo);
	pipelineInfo.setPColorBlendState(&colorBlendStateInfo);

	// Viewport State
	auto viewportInfo = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1)
		.setScissorCount(1);
	pipelineInfo.setPViewportState(&viewportInfo);

	// Dynamic State
	vk::DynamicState dynamicStates[] = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
		vk::DynamicState::eDepthBias,
		vk::DynamicState::eStencilReference,
		vk::DynamicState::eStencilCompareMask,
		vk::DynamicState::eStencilWriteMask,
	};
	auto dynamicInfo = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStateCount(std::extent<decltype(dynamicStates)>::value)
		.setPDynamicStates(dynamicStates);
	pipelineInfo.setPDynamicState(&dynamicInfo);

	// Depth and Stencil State
	auto depthAndStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
		.setDepthBoundsTestEnable(false)
		.setFront(vk::StencilOpState()
			.setCompareOp(vk::CompareOp::eNotEqual)
			.setFailOp(vk::StencilOp::eKeep)
			.setDepthFailOp(vk::StencilOp::eKeep)
			.setPassOp(vk::StencilOp::eReplace)
		)
		.setBack(vk::StencilOpState()
			.setCompareOp(vk::CompareOp::eNotEqual)
			.setFailOp(vk::StencilOp::eKeep)
			.setDepthFailOp(vk::StencilOp::eKeep)
			.setPassOp(vk::StencilOp::eReplace))
		.setStencilTestEnable(true);
	depthAndStencilInfo.front = depthAndStencilInfo.back;
	pipelineInfo.setPDepthStencilState(&depthAndStencilInfo);

	// Multisample
	auto multisampleInfo = vk::PipelineMultisampleStateCreateInfo()
		.setRasterizationSamples(m_msaaSampleCount)
		.setSampleShadingEnable(true)
		.setMinSampleShading(0.25f);
	pipelineInfo.setPMultisampleState(&multisampleInfo);

	// Vertex input binding
	auto vertexInputBindingDesc = vk::VertexInputBindingDescription()
		.setBinding(0)
		.setStride(sizeof(Vertex))
		.setInputRate(vk::VertexInputRate::eVertex);
	auto posAttr = vk::VertexInputAttributeDescription()
		.setBinding(0)
		.setLocation(0)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(Vertex, m_position));
	vk::VertexInputAttributeDescription vertexInputAttrs[] = {
		posAttr,
	};

	auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptionCount(1)
		.setPVertexBindingDescriptions(&vertexInputBindingDesc)
		.setVertexAttributeDescriptionCount(std::extent<decltype(vertexInputAttrs)>::value)
		.setPVertexAttributeDescriptions(vertexInputAttrs);
	pipelineInfo.setPVertexInputState(&vertexInputInfo);

	// Shader
	auto vsStageInfo = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(m_mmdGroundShadowVSModule)
		.setPName("main");
	auto fsStageInfo = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(m_mmdGroundShadowFSModule)
		.setPName("main");
	vk::PipelineShaderStageCreateInfo shaderStages[2] = {
		vsStageInfo,
		fsStageInfo,
	};
	pipelineInfo
		.setStageCount(std::extent<decltype(shaderStages)>::value)
		.setPStages(shaderStages);

	// Set alpha blend mode
	colorBlendAttachmentInfo
		.setBlendEnable(true)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
		.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
		.setAlphaBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
		.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);

	// Set front face mode
	rasterizationInfo.
		setCullMode(vk::CullModeFlagBits::eNone);

	ret = m_device.createGraphicsPipelines(
		m_pipelineCache,
		1, &pipelineInfo, nullptr,
		&m_mmdGroundShadowPipeline);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create MMD Ground Shadow Pipeline.\n";
		return false;
	}

	return true;
}

bool AppContext::PrepareDefaultTexture()
{
	m_defaultTexture = std::make_unique<Texture>();
	if (!m_defaultTexture->Setup(*this, 1, 1, vk::Format::eR8G8B8A8Unorm))
	{
		return false;
	}

	StagingBuffer* imgStBuf;
	uint32_t x = 1;
	uint32_t y = 1;
	uint32_t memSize = x * y * 4;
	if (!GetStagingBuffer(memSize, &imgStBuf))
	{
		return false;
	}
	void* imgPtr;
	m_device.mapMemory(imgStBuf->m_memory, 0, memSize, vk::MemoryMapFlags(), &imgPtr);
	uint8_t* pixels = (uint8_t*)imgPtr;
	pixels[0] = 0;
	pixels[0] = 0;
	pixels[0] = 0;
	pixels[0] = 255;
	m_device.unmapMemory(imgStBuf->m_memory);

	auto bufferImageCopy = vk::BufferImageCopy()
		.setImageSubresource(vk::ImageSubresourceLayers()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setMipLevel(0)
			.setBaseArrayLayer(0)
			.setLayerCount(1))
		.setImageExtent(vk::Extent3D(x, y, 1))
		.setBufferOffset(0);
	if (!imgStBuf->CopyImage(
		*this,
		m_defaultTexture->m_image,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		1, &bufferImageCopy))
	{
		std::cout << "Failed to copy image.\n";
		return false;
	}
	m_defaultTexture->m_imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	m_defaultTexture->m_hasAlpha = true;
	return true;
}

bool AppContext::Resize()
{
	m_device.waitIdle();
	if (!PrepareBuffer()) { return false; }
	if (!PrepareFramebuffer()) { return false; }

	return true;
}

bool AppContext::GetStagingBuffer(vk::DeviceSize memSize, StagingBuffer** outBuf)
{
	if (outBuf == nullptr)
	{
		return false;
	}

	vk::Result ret;
	for (auto& stBuf : m_stagingBuffers)
	{
		ret = m_device.getFenceStatus(stBuf->m_transferCompleteFence);
		if (vk::Result::eSuccess == ret && memSize < stBuf->m_memorySize)
		{
			m_device.resetFences(1, &stBuf->m_transferCompleteFence);
			*outBuf = stBuf.get();
			return true;
		}
	}

	for (auto& stBuf : m_stagingBuffers)
	{
		ret = m_device.getFenceStatus(stBuf->m_transferCompleteFence);
		if (vk::Result::eSuccess == ret)
		{
			if (!stBuf->Setup(*this, memSize))
			{
				std::cout << "Failed to setup Staging Buffer.\n";
				return false;
			}
			m_device.resetFences(1, &stBuf->m_transferCompleteFence);
			*outBuf = stBuf.get();
			return true;
		}
	}

	auto newStagingBuffer = std::make_unique<StagingBuffer>();
	if (!newStagingBuffer->Setup(*this, memSize))
	{
		std::cout << "Failed to setup Staging Buffer.\n";
		newStagingBuffer->Clear(*this);
		return false;
	}
	m_device.resetFences(1, &newStagingBuffer->m_transferCompleteFence);
	*outBuf = newStagingBuffer.get();
	m_stagingBuffers.emplace_back(std::move(newStagingBuffer));
	return true;
}

void AppContext::WaitAllStagingBuffer()
{
	for (auto& stBuf : m_stagingBuffers)
	{
		m_device.waitForFences(1, &stBuf->m_transferCompleteFence, true, UINT64_MAX);
	}
}

bool AppContext::GetTexture(const std::string & texturePath, Texture** outTex)
{
	auto it = m_textures.find(texturePath);
	if (it == m_textures.end())
	{
		saba::File file;
		if (!file.Open(texturePath))
		{
			std::cout << "Failed to open file.[" << texturePath << "]\n";
			return false;
		}
		int x, y, comp;
		if (stbi_info_from_file(file.GetFilePointer(), &x, &y, &comp) == 0)
		{
			std::cout << "Failed to read file.\n";
			return false;
		}

		int reqComp = 0;
		bool hasAlpha = false;
		if (comp != 4)
		{
			hasAlpha = false;
		}
		else
		{
			hasAlpha = true;
		}
		uint8_t* image = stbi_load_from_file(file.GetFilePointer(), &x, &y, &comp, STBI_rgb_alpha);

		vk::Format format = vk::Format::eR8G8B8A8Unorm;
		TextureUPtr tex = std::make_unique<Texture>();
		if (!tex->Setup(*this, x, y, format))
		{
			stbi_image_free(image);
			return false;
		}

		uint32_t memSize = x * y * 4;
		StagingBuffer* imgStBuf;
		if (!GetStagingBuffer(memSize, &imgStBuf))
		{
			stbi_image_free(image);
			return false;
		}
		void* imgPtr;
		m_device.mapMemory(imgStBuf->m_memory, 0, memSize, vk::MemoryMapFlags(), &imgPtr);
		memcpy(imgPtr, image, memSize);
		m_device.unmapMemory(imgStBuf->m_memory);
		stbi_image_free(image);

		auto bufferImageCopy = vk::BufferImageCopy()
			.setImageSubresource(vk::ImageSubresourceLayers()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setMipLevel(0)
				.setBaseArrayLayer(0)
				.setLayerCount(1))
			.setImageExtent(vk::Extent3D(x, y, 1))
			.setBufferOffset(0);
		if (!imgStBuf->CopyImage(
			*this,
			tex->m_image,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			1, &bufferImageCopy))
		{
			std::cout << "Failed to copy image.\n";
			return false;
		}
		tex->m_imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		tex->m_hasAlpha = hasAlpha;

		*outTex = tex.get();
		m_textures[texturePath] = std::move(tex);
	}
	else
	{
		*outTex = (*it).second.get();
	}
	return true;
}

void SwapchainImageResource::Clear(AppContext& appContext)
{
	auto device = appContext.m_device;
	auto commandPool = appContext.m_commandPool;

	device.destroyImageView(m_imageView, nullptr);
	m_imageView = nullptr;

	device.destroyFramebuffer(m_framebuffer, nullptr);
	m_framebuffer = nullptr;

	device.freeCommandBuffers(commandPool, 1, &m_cmdBuffer);
	m_cmdBuffer = nullptr;
}

bool Buffer::Setup(
	AppContext&				appContext,
	vk::DeviceSize			memSize,
	vk::BufferUsageFlags	usage,
	vk::MemoryPropertyFlags	memProperty
)
{
	vk::Result ret;
	auto device = appContext.m_device;

	Clear(appContext);

	// Create Buffer
	auto bufferInfo = vk::BufferCreateInfo()
		.setSize(memSize)
		.setUsage(usage);
	ret = device.createBuffer(&bufferInfo, nullptr, &m_buffer);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create Buffer.\n";
		return false;
	}

	// Allocate memory
	auto bufMemReq = device.getBufferMemoryRequirements(m_buffer);
	uint32_t bufMemTypeIndex;
	if (!FindMemoryTypeIndex(
		appContext.m_memProperties,
		bufMemReq.memoryTypeBits,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		&bufMemTypeIndex)
		)
	{
		std::cout << "Failed to found Memory Type Index.\n";
		return false;
	}
	auto bufMemAllocInfo = vk::MemoryAllocateInfo()
		.setAllocationSize(bufMemReq.size)
		.setMemoryTypeIndex(bufMemTypeIndex);
	ret = device.allocateMemory(&bufMemAllocInfo, nullptr, &m_memory);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to allocate Memory.\n";
		return false;
	}

	// Bind
	device.bindBufferMemory(m_buffer, m_memory, 0);
	m_memorySize = memSize;

	return true;
}

void Buffer::Clear(AppContext& appContext)
{
	auto device = appContext.m_device;
	if (m_buffer)
	{
		device.destroyBuffer(m_buffer, nullptr);
		m_buffer = nullptr;
	}
	if (m_memory)
	{
		device.freeMemory(m_memory, nullptr);
		m_memory = nullptr;
	}
	m_memorySize = 0;
}

bool Texture::Setup(AppContext& appContext, uint32_t width, uint32_t height, vk::Format format, int mipCount)
{
	vk::Result ret;

	auto device = appContext.m_device;

	auto imageInfo = vk::ImageCreateInfo()
		.setImageType(vk::ImageType::e2D)
		.setFormat(format)
		.setMipLevels(1)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setExtent(vk::Extent3D(width, height, 1))
		.setArrayLayers(1)
		.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
	ret = device.createImage(&imageInfo, nullptr, &m_image);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create Image.\n";
		return false;
	}
	auto memReq = device.getImageMemoryRequirements(m_image);

	uint32_t memTypeIndex;
	if (!FindMemoryTypeIndex(
		appContext.m_memProperties,
		memReq.memoryTypeBits,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		&memTypeIndex))
	{
		std::cout << "Failed to find Memory Type Index.\n";
		return false;
	}

	auto memAllocInfo = vk::MemoryAllocateInfo()
		.setAllocationSize(memReq.size)
		.setMemoryTypeIndex(memTypeIndex);
	ret = device.allocateMemory(&memAllocInfo, nullptr, &m_memory);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to allocate memory.\n";
		return false;
	}

	device.bindImageMemory(m_image, m_memory, 0);

	auto imageViewInfo = vk::ImageViewCreateInfo()
		.setFormat(format)
		.setViewType(vk::ImageViewType::e2D)
		.setComponents(vk::ComponentMapping(
			vk::ComponentSwizzle::eR,
			vk::ComponentSwizzle::eG,
			vk::ComponentSwizzle::eB,
			vk::ComponentSwizzle::eA))
		.setSubresourceRange(vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)
			.setBaseArrayLayer(0)
			.setLayerCount(1)
			.setLevelCount(mipCount))
		.setImage(m_image);
	ret = device.createImageView(&imageViewInfo, nullptr, &m_imageView);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create Image View.\n";
		return false;
	}

	m_format = format;
	m_mipLevel = mipCount;
	
	return true;
}

void Texture::Clear(AppContext& appContext)
{
	auto device = appContext.m_device;

	device.destroyImage(m_image, nullptr);
	m_image = nullptr;

	device.destroyImageView(m_imageView, nullptr);
	m_imageView = nullptr;

	m_imageLayout = vk::ImageLayout::eUndefined;

	device.freeMemory(m_memory, nullptr);
	m_memory = nullptr;
}

bool StagingBuffer::Setup(AppContext& appContext, vk::DeviceSize size)
{
	vk::Result ret;
	auto device = appContext.m_device;

	if (size <= m_memorySize)
	{
		return true;
	}

	Clear(appContext);

	// Create Buffer
	auto bufInfo = vk::BufferCreateInfo()
		.setSize(size)
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
	ret = device.createBuffer(&bufInfo, nullptr, &m_buffer);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create Staging Buffer.\n";
		return false;
	}

	// Allocate Mmoery
	auto bufMemReq = device.getBufferMemoryRequirements(m_buffer);
	uint32_t bufMemTypeIndex;
	if (!FindMemoryTypeIndex(
		appContext.m_memProperties,
		bufMemReq.memoryTypeBits,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		&bufMemTypeIndex)
		)
	{
		std::cout << "Failed to found Staging Buffer Memory Type Index.\n";
		return false;
	}
	auto memAllocInfo = vk::MemoryAllocateInfo()
		.setAllocationSize(bufMemReq.size)
		.setMemoryTypeIndex(bufMemTypeIndex);
	ret = device.allocateMemory(&memAllocInfo, nullptr, &m_memory);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to allocate Staging Buffer Memory.\n";
		return false;
	}

	device.bindBufferMemory(m_buffer, m_memory, 0);

	m_memorySize = uint32_t(bufMemReq.size);

	// Allocate Command Buffer
	auto cmdPool = appContext.m_transferCommandPool;
	auto cmdBufAllocInfo = vk::CommandBufferAllocateInfo()
		.setCommandPool(cmdPool)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount(1);
	ret = device.allocateCommandBuffers(&cmdBufAllocInfo, &m_copyCommand);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to allocate Staging Buffer Copy Command Buffer.\n";
		return false;
	}

	// Create Fence
	auto fenceInfo = vk::FenceCreateInfo()
		.setFlags(vk::FenceCreateFlagBits::eSignaled);
	ret = device.createFence(&fenceInfo, nullptr, &m_transferCompleteFence);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to  create Staging Buffer Transfer Complete Fence.\n";
		return false;
	}

	// Create Semaphore
	auto semaphoreInfo = vk::SemaphoreCreateInfo();
	ret = device.createSemaphore(&semaphoreInfo, nullptr, &m_transferCompleteSemaphore);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to  create Staging Buffer Transer Complete Semaphore.\n";
		return false;
	}

	return true;
}

void StagingBuffer::Wait(AppContext& appContext)
{
	vk::Result ret;
	auto device = appContext.m_device;
	if (m_transferCompleteFence)
	{
		ret = device.waitForFences(1, &m_transferCompleteFence, true, -1);
	}
}

void StagingBuffer::Clear(AppContext& appContext)
{
	auto device = appContext.m_device;

	Wait(appContext);

	device.destroyFence(m_transferCompleteFence, nullptr);
	m_transferCompleteFence = nullptr;

	device.destroySemaphore(m_transferCompleteSemaphore, nullptr);
	m_transferCompleteSemaphore = nullptr;
	m_waitSemaphore = nullptr;

	auto cmdPool = appContext.m_transferCommandPool;
	device.freeCommandBuffers(cmdPool, 1, &m_copyCommand);
	m_copyCommand = nullptr;

	device.freeMemory(m_memory, nullptr);
	m_memory = nullptr;

	device.destroyBuffer(m_buffer, nullptr);
	m_buffer = nullptr;
}

bool StagingBuffer::CopyBuffer(AppContext& appContext, vk::Buffer buffer, vk::DeviceSize size)
{
	vk::Result ret;

	auto device = appContext.m_device;

	auto cmdBufInfo = vk::CommandBufferBeginInfo()
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	ret = m_copyCommand.begin(&cmdBufInfo);

	auto copyRegion = vk::BufferCopy()
		.setSize(size);
	m_copyCommand.copyBuffer(m_buffer, buffer, 1, &copyRegion);

	m_copyCommand.end();

	// Submit
	auto submitInfo = vk::SubmitInfo()
		.setCommandBufferCount(1)
		.setPCommandBuffers(&m_copyCommand)
		.setSignalSemaphoreCount(1)
		.setPSignalSemaphores(&m_transferCompleteSemaphore);
	vk::PipelineStageFlags waitDstStage = vk::PipelineStageFlagBits::eTransfer;
	if (m_waitSemaphore)
	{
		submitInfo
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&m_waitSemaphore)
			.setPWaitDstStageMask(&waitDstStage);
	}

	auto queue = appContext.m_graphicsQueue;
	ret = queue.submit(1, &submitInfo, m_transferCompleteFence);
	m_waitSemaphore = m_transferCompleteSemaphore;
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to submit Copy Command Buffer.\n";
		return false;
	}

	return true;
}

bool StagingBuffer::CopyImage(
	AppContext & appContext,
	vk::Image destImage,
	vk::ImageLayout imageLayout,
	uint32_t regionCount,
	vk::BufferImageCopy* regions
)
{
	vk::Result ret;

	auto device = appContext.m_device;

	auto cmdBufInfo = vk::CommandBufferBeginInfo()
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	ret = m_copyCommand.begin(&cmdBufInfo);


	auto subresourceRange = vk::ImageSubresourceRange()
		.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setBaseMipLevel(0)
		.setLevelCount(regionCount)
		.setLayerCount(1);

	SetImageLayout(
		m_copyCommand,
		destImage,
		vk::ImageAspectFlagBits::eColor,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal,
		subresourceRange
	);

	m_copyCommand.copyBufferToImage(
		m_buffer,
		destImage,
		vk::ImageLayout::eTransferDstOptimal,
		regionCount,
		regions
	);

	SetImageLayout(
		m_copyCommand,
		destImage,
		vk::ImageAspectFlagBits::eColor,
		vk::ImageLayout::eTransferDstOptimal,
		imageLayout,
		subresourceRange
	);

	m_copyCommand.end();

	// Submit
	auto submitInfo = vk::SubmitInfo()
		.setCommandBufferCount(1)
		.setPCommandBuffers(&m_copyCommand)
		.setSignalSemaphoreCount(1)
		.setPSignalSemaphores(&m_transferCompleteSemaphore);
	vk::PipelineStageFlags waitDstStage = vk::PipelineStageFlagBits::eTransfer;
	if (m_waitSemaphore)
	{
		submitInfo
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&m_waitSemaphore)
			.setPWaitDstStageMask(&waitDstStage);
	}

	auto queue = appContext.m_graphicsQueue;
	ret = queue.submit(1, &submitInfo, m_transferCompleteFence);
	m_waitSemaphore = m_transferCompleteSemaphore;
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to submit Copy Command Buffer.\n";
		return false;
	}
	return true;
}


struct Model
{
	std::shared_ptr<saba::MMDModel>		m_mmdModel;
	std::unique_ptr<saba::VMDAnimation>	m_vmdAnim;

	Buffer	m_indexBuffer;
	vk::IndexType	m_indexType;

	bool Setup(AppContext& appContext);
	bool SetupVertexBuffer(AppContext& appContext);
	bool SetupDescriptorPool(AppContext& appContext);
	bool SetupDescriptorSet(AppContext& appContext);
	bool SetupCommandBuffer(AppContext& appContext);
	void Destroy(AppContext& appContext);

	void UpdateAnimation(const AppContext& appContext);
	void Update(AppContext& appContext);
	void Draw(AppContext& appContext);

	vk::CommandBuffer GetCommandBuffer(uint32_t imageIndex) const;

	struct Material
	{
		Texture*	m_mmdTex;
		vk::Sampler	m_mmdTexSampler;

		Texture*	m_mmdToonTex;
		vk::Sampler	m_mmdToonTexSampler;

		Texture*	m_mmdSphereTex;
		vk::Sampler	m_mmdSphereTexSampler;
	};

	struct ModelResource
	{
		Buffer	m_vertexBuffer;

		Buffer	m_uniformBuffer;

		// MMD Shader
		uint32_t	m_mmdVSUBOffset;

		// MMD Edge Shader
		uint32_t	m_mmdEdgeVSUBOffset;

		// MMD Ground Shader
		uint32_t	m_mmdGroundShadowVSUBOffset;
	};

	struct MaterialResource
	{
		vk::DescriptorSet	m_mmdDescSet;
		vk::DescriptorSet	m_mmdEdgeDescSet;
		vk::DescriptorSet	m_mmdGroundShadowDescSet;

		// MMD Shader
		uint32_t			m_mmdFSUBOffset;

		// MMD Edge Shader
		uint32_t			m_mmdEdgeSizeVSUBOffset;
		uint32_t			m_mmdEdgeFSUBOffset;

		// MMD Ground Shadow Shader
		uint32_t			m_mmdGroundShadowFSUBOffset;
	};

	struct Resource
	{
		ModelResource					m_modelResource;
		std::vector<MaterialResource>	m_materialResources;
	};

	std::vector<Material>	m_materials;
	Resource				m_resource;
	vk::DescriptorPool		m_descPool;

	std::vector<vk::CommandBuffer>	m_cmdBuffers;
};

/*
	Model
*/
bool Model::Setup(AppContext& appContext)
{
	vk::Result ret;
	auto device = appContext.m_device;

	auto swapImageCount = uint32_t(appContext.m_swapchainImageResouces.size());

	size_t matCount = m_mmdModel->GetMaterialCount();
	m_materials.resize(matCount);
	for (size_t i = 0; i < matCount; i++)
	{
		const auto materials = m_mmdModel->GetMaterials();
		const auto& mmdMat = materials[i];
		auto& mat = m_materials[i];

		// Tex
		if (!mmdMat.m_texture.empty())
		{
			if (!appContext.GetTexture(mmdMat.m_texture, &mat.m_mmdTex))
			{
				std::cout << "Failed to get Texture.\n";
				return false;
			}
		}
		else
		{
			mat.m_mmdTex = appContext.m_defaultTexture.get();
		}

		auto samplerInfo = vk::SamplerCreateInfo()
			.setMagFilter(vk::Filter::eLinear)
			.setMinFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat)
			.setMipLodBias(0.0f)
			.setCompareOp(vk::CompareOp::eNever)
			.setMinLod(0.0f)
			.setMaxLod(float(mat.m_mmdTex->m_mipLevel))
			.setMaxAnisotropy(1.0f)
			.setAnisotropyEnable(false);
		ret = device.createSampler(&samplerInfo, nullptr, &mat.m_mmdTexSampler);
		if (vk::Result::eSuccess != ret)
		{
			std::cout << "Failed to create Sampler.\n";
			return false;
		}

		// Toon Tex
		if (!mmdMat.m_toonTexture.empty())
		{
			if (!appContext.GetTexture(mmdMat.m_toonTexture, &mat.m_mmdToonTex))
			{
				std::cout << "Failed to get Toon Texture.\n";
				return false;
			}
		}
		else
		{
			mat.m_mmdToonTex = appContext.m_defaultTexture.get();
		}

		auto toonSamplerInfo = vk::SamplerCreateInfo()
			.setMagFilter(vk::Filter::eLinear)
			.setMinFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
			.setMipLodBias(0.0f)
			.setCompareOp(vk::CompareOp::eNever)
			.setMinLod(0.0f)
			.setMaxLod(float(mat.m_mmdToonTex->m_mipLevel))
			.setMaxAnisotropy(1.0f)
			.setAnisotropyEnable(false);
		ret = device.createSampler(&toonSamplerInfo, nullptr, &mat.m_mmdToonTexSampler);
		if (vk::Result::eSuccess != ret)
		{
			std::cout << "Failed to create Sampler.\n";
			return false;
		}

		// Tex
		if (!mmdMat.m_spTexture.empty())
		{
			if (!appContext.GetTexture(mmdMat.m_spTexture, &mat.m_mmdSphereTex))
			{
				std::cout << "Failed to get Sphere Texture.\n";
				return false;
			}
		}
		else
		{
			mat.m_mmdSphereTex = appContext.m_defaultTexture.get();
		}

		auto sphereSamplerInfo = vk::SamplerCreateInfo()
			.setMagFilter(vk::Filter::eLinear)
			.setMinFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat)
			.setMipLodBias(0.0f)
			.setCompareOp(vk::CompareOp::eNever)
			.setMinLod(0.0f)
			.setMaxLod(float(mat.m_mmdSphereTex->m_mipLevel))
			.setMaxAnisotropy(1.0f)
			.setAnisotropyEnable(false);
		ret = device.createSampler(&sphereSamplerInfo, nullptr, &mat.m_mmdSphereTexSampler);
		if (vk::Result::eSuccess != ret)
		{
			std::cout << "Failed to create Sampler.\n";
			return false;
		}
	}

	if (!SetupVertexBuffer(appContext)) { return false; }
	if (!SetupDescriptorPool(appContext)) { return false; }
	if (!SetupDescriptorSet(appContext)) { return false; }
	if (!SetupCommandBuffer(appContext)) { return false; }

	return true;
}

bool Model::SetupVertexBuffer(AppContext& appContext)
{
	vk::Result ret;
	auto device = appContext.m_device;
	//auto swapImageCount = uint32_t(m_resources.size());

	// Vertex Buffer
	//for (uint32_t i = 0; i < swapImageCount; i++)
	{
		//auto& res = m_resources[i];
		auto& res = m_resource;
		auto& modelRes = res.m_modelResource;
		auto& vb = modelRes.m_vertexBuffer;

		// Create buffer
		auto vbMemSize = uint32_t(sizeof(Vertex) * m_mmdModel->GetVertexCount());
		if (!vb.Setup(
			appContext,
			vbMemSize,
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal
		))
		{
			std::cout << "Failed to create Vertex Buffer.\n";
			return false;
		}
	}

	// Index Buffer
	{
		// Create buffer
		auto ibMemSize = uint32_t(m_mmdModel->GetIndexElementSize() * m_mmdModel->GetIndexCount());
		if (!m_indexBuffer.Setup(
			appContext,
			ibMemSize,
			vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal
		))
		{
			std::cout << "Failed to create Index Buffer.\n";
			return false;
		}

		StagingBuffer* indicesStagingBuffer;
		if (!appContext.GetStagingBuffer(ibMemSize, &indicesStagingBuffer))
		{
			std::cout << "Failed to get Staging Buffer.\n";
			return false;
		}

		void* mapMem;
		ret = device.mapMemory(indicesStagingBuffer->m_memory, 0, ibMemSize, vk::MemoryMapFlagBits(0), &mapMem);
		if (vk::Result::eSuccess != ret)
		{
			std::cout << "Failed to map memory.\n";
			return false;
		}
		std::memcpy(mapMem, m_mmdModel->GetIndices(), ibMemSize);
		device.unmapMemory(indicesStagingBuffer->m_memory);

		if (!indicesStagingBuffer->CopyBuffer(appContext, m_indexBuffer.m_buffer, ibMemSize))
		{
			std::cout << "Failed to copy buffer.\n";
			return false;
		}
		indicesStagingBuffer->Wait(appContext);

		if (m_mmdModel->GetIndexElementSize() == 1)
		{
			std::cout << "Vulkan is not supported uint8_t index.\n";
			return false;
		}
		else if (m_mmdModel->GetIndexElementSize() == 2)
		{
			m_indexType = vk::IndexType::eUint16;
		}
		else if (m_mmdModel->GetIndexElementSize() == 4)
		{
			m_indexType = vk::IndexType::eUint32;
		}
		else
		{
			std::cout << "Unknown index size.[" << m_mmdModel->GetIndexElementSize() << "]\n";
			return false;
		}
	}

	return true;
}

bool Model::SetupDescriptorPool(AppContext & appContext)
{
	vk::Result ret;

	auto device = appContext.m_device;

	// Descriptor Pool
	uint32_t matCount = uint32_t(m_mmdModel->GetMaterialCount());

	/*
	Uniform Count
	MMD Sahder
	- MMDVertxShaderUB
	- MMDFragmentShaderUB
	MMD Edge Shader
	- MMDEdgeVertexShaderUB
	- MMDEdgeSizeVertexShaderUB
	- MMDEdgeFragmentShaderUB
	MMD Ground Shadow Shader
	- MMDGroundShadowVertexShaderUB
	- MMDGroundShadowFragmentShaderUB
	*/
	uint32_t ubCount = 7;
	ubCount *= matCount;
	auto ubPoolSize = vk::DescriptorPoolSize()
		.setType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(ubCount);

	/*
	Image Count
	MMD Shader
	- Texture
	- Toon Texture
	- Sphere Texture
	*/
	uint32_t imgPoolCount = 3;
	imgPoolCount *= matCount;
	auto imgPoolSize = vk::DescriptorPoolSize()
		.setType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(imgPoolCount);

	vk::DescriptorPoolSize poolSizes[] = {
		ubPoolSize,
		imgPoolSize
	};

	/*
	Descriptor Set Count
	- MMD
	- MMD Edge
	- MMD Ground Shadow
	*/
	uint32_t descSetCount = 3;
	descSetCount *= matCount;
	auto descPoolInfo = vk::DescriptorPoolCreateInfo()
		.setMaxSets(descSetCount)
		.setPoolSizeCount(std::extent<decltype(poolSizes)>::value)
		.setPPoolSizes(poolSizes);
	ret = device.createDescriptorPool(&descPoolInfo, nullptr, &m_descPool);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create Descriptor Pool.\n";
		return false;
	}

	return true;
}

bool Model::SetupDescriptorSet(AppContext& appContext)
{
	vk::Result ret;

	auto device = appContext.m_device;

	uint32_t swapImageCount = uint32_t(appContext.m_swapchainImageResouces.size());

	auto mmdDescSetAllocInfo = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(m_descPool)
		.setDescriptorSetCount(1)
		.setPSetLayouts(&appContext.m_mmdDescriptorSetLayout);

	auto mmdEdgeDescSetAllocInfo = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(m_descPool)
		.setDescriptorSetCount(1)
		.setPSetLayouts(&appContext.m_mmdEdgeDescriptorSetLayout);

	auto mmdGroundShadowDescSetAllocInfo = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(m_descPool)
		.setDescriptorSetCount(1)
		.setPSetLayouts(&appContext.m_mmdGroundShadowDescriptorSetLayout);

	auto gpu = appContext.m_gpu;
	auto gpuProp = gpu.getProperties();
	uint32_t ubAlign = uint32_t(gpuProp.limits.minUniformBufferOffsetAlignment);
	//for (uint32_t imgIdx = 0; imgIdx < swapImageCount; imgIdx++)
	{
		uint32_t ubOffset = 0;

		//auto& res = m_resources[imgIdx];
		auto& res = m_resource;
		auto& modelRes = res.m_modelResource;

		// MMDVertxShaderUB
		modelRes.m_mmdVSUBOffset = ubOffset;
		ubOffset += sizeof(MMDVertxShaderUB);
		ubOffset = (ubOffset + ubAlign) - ((ubOffset + ubAlign) % ubAlign);

		// MMDEdgeVertexShaderUB
		modelRes.m_mmdEdgeVSUBOffset = ubOffset;
		ubOffset += sizeof(MMDEdgeVertexShaderUB);
		ubOffset = (ubOffset + ubAlign) - ((ubOffset + ubAlign) % ubAlign);

		// MMDGroundShadowVertexShaderUB
		modelRes.m_mmdGroundShadowVSUBOffset = ubOffset;
		ubOffset += sizeof(MMDGroundShadowVertexShaderUB);
		ubOffset = (ubOffset + ubAlign) - ((ubOffset + ubAlign) % ubAlign);

		size_t matCount = m_mmdModel->GetMaterialCount();
		res.m_materialResources.resize(matCount);
		for (size_t matIdx = 0; matIdx < matCount; matIdx++)
		{
			auto& matRes = res.m_materialResources[matIdx];

			// MMD Descriptor Set
			ret = device.allocateDescriptorSets(&mmdDescSetAllocInfo, &matRes.m_mmdDescSet);
			if (vk::Result::eSuccess != ret)
			{
				std::cout << "Failed to allocate MMD Descriptor Set.\n";
				return false;
			}

			// MMD Edge Descriptor Set
			ret = device.allocateDescriptorSets(&mmdEdgeDescSetAllocInfo, &matRes.m_mmdEdgeDescSet);
			if (vk::Result::eSuccess != ret)
			{
				std::cout << "Failed to allocate MMD Edge Descriptor Set.\n";
				return false;
			}

			// MMD Ground Shadow Descriptor Set
			ret = device.allocateDescriptorSets(&mmdGroundShadowDescSetAllocInfo, &matRes.m_mmdGroundShadowDescSet);
			if (vk::Result::eSuccess != ret)
			{
				std::cout << "Failed to allocate MMD Ground Shadow Descriptor Set.\n";
				return false;
			}

			// MMDFragmentShaderUB
			matRes.m_mmdFSUBOffset = ubOffset;
			ubOffset += sizeof(MMDFragmentShaderUB);
			ubOffset = (ubOffset + ubAlign) - ((ubOffset + ubAlign) % ubAlign);

			// MMDEdgeSizeVertexShaderUB
			matRes.m_mmdEdgeSizeVSUBOffset = ubOffset;
			ubOffset += sizeof(MMDEdgeSizeVertexShaderUB);
			ubOffset = (ubOffset + ubAlign) - ((ubOffset + ubAlign) % ubAlign);

			// MMDEdgeFragmentShaderUB
			matRes.m_mmdEdgeFSUBOffset = ubOffset;
			ubOffset += sizeof(MMDEdgeFragmentShaderUB);
			ubOffset = (ubOffset + ubAlign) - ((ubOffset + ubAlign) % ubAlign);

			// MMDEdgeFragmentShaderUB
			matRes.m_mmdGroundShadowFSUBOffset = ubOffset;
			ubOffset += sizeof(MMDGroundShadowFragmentShaderUB);
			ubOffset = (ubOffset + ubAlign) - ((ubOffset + ubAlign) % ubAlign);
		}

		// Create Uniform Buffer
		auto& ub = modelRes.m_uniformBuffer;
		auto ubMemSize = ubOffset;
		if (!ub.Setup(
			appContext,
			ubMemSize,
			vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal
		))
		{
			std::cout << "Failed to create Uniform Buffer.\n";
			return false;
		}
	}

	// MMD Descriptor Set

	// MMDVertxShaderUB
	auto mmdVSBufferInfo = vk::DescriptorBufferInfo()
		.setOffset(0)
		.setRange(sizeof(MMDVertxShaderUB));
	auto mmdVSWriteDescSet = vk::WriteDescriptorSet()
		.setDstBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setPBufferInfo(&mmdVSBufferInfo);

	// MMDFragmentShaderUB
	auto mmdFSBufferInfo = vk::DescriptorBufferInfo()
		.setOffset(0)
		.setRange(sizeof(MMDFragmentShaderUB));
	auto mmdFSWriteDescSet = vk::WriteDescriptorSet()
		.setDstBinding(1)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setPBufferInfo(&mmdFSBufferInfo);

	// MMD Texture
	auto mmdFSTexSamplerInfo = vk::DescriptorImageInfo();
	auto mmdFSTexSamplerWriteDescSet = vk::WriteDescriptorSet()
		.setDstBinding(2)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setPImageInfo(&mmdFSTexSamplerInfo);

	// MMD Toon Texture
	auto mmdFSToonTexSamplerInfo = vk::DescriptorImageInfo();
	auto mmdFSToonTexSamplerWriteDescSet = vk::WriteDescriptorSet()
		.setDstBinding(3)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setPImageInfo(&mmdFSToonTexSamplerInfo);

	// MMD Sphere Texture
	auto mmdFSSphereTexSamplerInfo = vk::DescriptorImageInfo();
	auto mmdFSSphereTexSamplerWriteDescSet = vk::WriteDescriptorSet()
		.setDstBinding(4)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setPImageInfo(&mmdFSSphereTexSamplerInfo);

	vk::WriteDescriptorSet mmdWriteDescSets[] = {
		mmdVSWriteDescSet,
		mmdFSWriteDescSet,
		mmdFSTexSamplerWriteDescSet,
		mmdFSToonTexSamplerWriteDescSet,
		mmdFSSphereTexSamplerWriteDescSet,
	};

	// MMD Edge Descriptor Set

	// MMDEdgeVertxShaderUB
	auto mmdEdgeVSBufferInfo = vk::DescriptorBufferInfo()
		.setOffset(0)
		.setRange(sizeof(MMDEdgeVertexShaderUB));
	auto mmdEdgeVSWriteDescSet = vk::WriteDescriptorSet()
		.setDstBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setPBufferInfo(&mmdEdgeVSBufferInfo);

	// MMDEdgeSizeVertexShaderUB
	auto mmdEdgeSizeVSBufferInfo = vk::DescriptorBufferInfo()
		.setOffset(0)
		.setRange(sizeof(MMDEdgeSizeVertexShaderUB));
	auto mmdEdgeSizeVSWriteDescSet = vk::WriteDescriptorSet()
		.setDstBinding(1)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setPBufferInfo(&mmdEdgeSizeVSBufferInfo);

	// MMDEdgeFragmentShaderUB
	auto mmdEdgeFSBufferInfo = vk::DescriptorBufferInfo()
		.setOffset(0)
		.setRange(sizeof(MMDEdgeFragmentShaderUB));
	auto mmdEdgeFSWriteDescSet = vk::WriteDescriptorSet()
		.setDstBinding(2)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setPBufferInfo(&mmdEdgeFSBufferInfo);

	vk::WriteDescriptorSet mmdEdgeWriteDescSets[] = {
		mmdEdgeVSWriteDescSet,
		mmdEdgeSizeVSWriteDescSet,
		mmdEdgeFSWriteDescSet,
	};

	// MMD GroundShadow Descriptor Set

	// MMDGroundShadowVertxShaderUB
	auto mmdGroundShadowVSBufferInfo = vk::DescriptorBufferInfo()
		.setOffset(0)
		.setRange(sizeof(MMDGroundShadowVertexShaderUB));
	auto mmdGroundShadowVSWriteDescSet = vk::WriteDescriptorSet()
		.setDstBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setPBufferInfo(&mmdGroundShadowVSBufferInfo);

	// MMDGroundShadowFragmentShaderUB
	auto mmdGroundShadowFSBufferInfo = vk::DescriptorBufferInfo()
		.setOffset(0)
		.setRange(sizeof(MMDGroundShadowFragmentShaderUB));
	auto mmdGroundShadowFSWriteDescSet = vk::WriteDescriptorSet()
		.setDstBinding(1)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setPBufferInfo(&mmdGroundShadowFSBufferInfo);

	vk::WriteDescriptorSet mmdGroundShadowWriteDescSets[] = {
		mmdGroundShadowVSWriteDescSet,
		mmdGroundShadowFSWriteDescSet,
	};

	//for (uint32_t imgIdx = 0; imgIdx < swapImageCount; imgIdx++)
	{
		//auto& res = m_resources[imgIdx];
		auto& res = m_resource;
		auto& modelRes = res.m_modelResource;

		// MMDVertxShaderUB
		mmdVSBufferInfo.setBuffer(modelRes.m_uniformBuffer.m_buffer);
		mmdVSBufferInfo.setOffset(modelRes.m_mmdVSUBOffset);

		// MMDEdgeVertexShaderUB
		mmdEdgeVSBufferInfo.setBuffer(modelRes.m_uniformBuffer.m_buffer);
		mmdEdgeVSBufferInfo.setOffset(modelRes.m_mmdEdgeVSUBOffset);

		// MMDGroundShadowVertexShaderUB
		mmdGroundShadowVSBufferInfo.setBuffer(modelRes.m_uniformBuffer.m_buffer);
		mmdGroundShadowVSBufferInfo.setOffset(modelRes.m_mmdGroundShadowVSUBOffset);

		size_t matCount = m_mmdModel->GetMaterialCount();
		res.m_materialResources.resize(matCount);
		for (size_t matIdx = 0; matIdx < matCount; matIdx++)
		{
			auto& matRes = res.m_materialResources[matIdx];
			const auto& mat = m_materials[matIdx];

			// MMDFragmentShaderUB
			mmdFSBufferInfo.setBuffer(modelRes.m_uniformBuffer.m_buffer);
			mmdFSBufferInfo.setOffset(matRes.m_mmdFSUBOffset);

			// Tex
			mmdFSTexSamplerInfo.setImageView(mat.m_mmdTex->m_imageView);
			mmdFSTexSamplerInfo.setImageLayout(mat.m_mmdTex->m_imageLayout);
			mmdFSTexSamplerInfo.setSampler(mat.m_mmdTexSampler);

			// Toon Tex
			mmdFSToonTexSamplerInfo.setImageView(mat.m_mmdToonTex->m_imageView);
			mmdFSToonTexSamplerInfo.setImageLayout(mat.m_mmdToonTex->m_imageLayout);
			mmdFSToonTexSamplerInfo.setSampler(mat.m_mmdToonTexSampler);

			// Sphere Tex
			mmdFSSphereTexSamplerInfo.setImageView(mat.m_mmdSphereTex->m_imageView);
			mmdFSSphereTexSamplerInfo.setImageLayout(mat.m_mmdSphereTex->m_imageLayout);
			mmdFSSphereTexSamplerInfo.setSampler(mat.m_mmdSphereTexSampler);

			// Write MMD descriptor set
			uint32_t writesCount = std::extent<decltype(mmdWriteDescSets)>::value;
			for (uint32_t i = 0; i < writesCount; i++)
			{
				mmdWriteDescSets[i].setDstSet(matRes.m_mmdDescSet);
			}
			device.updateDescriptorSets(writesCount, mmdWriteDescSets, 0, nullptr);

			// MMDEdgeSizeVertexShaderUB
			mmdEdgeSizeVSBufferInfo.setBuffer(modelRes.m_uniformBuffer.m_buffer);
			mmdEdgeSizeVSBufferInfo.setOffset(matRes.m_mmdEdgeSizeVSUBOffset);

			// MMDEdgeFragmentShaderUB
			mmdEdgeFSBufferInfo.setBuffer(modelRes.m_uniformBuffer.m_buffer);
			mmdEdgeFSBufferInfo.setOffset(matRes.m_mmdEdgeFSUBOffset);

			// Write MMD Edge descriptor set
			writesCount = std::extent<decltype(mmdEdgeWriteDescSets)>::value;
			for (uint32_t i = 0; i < writesCount; i++)
			{
				mmdEdgeWriteDescSets[i].setDstSet(matRes.m_mmdEdgeDescSet);
			}
			device.updateDescriptorSets(writesCount, mmdEdgeWriteDescSets, 0, nullptr);

			// MMDGroundShadowFragmentShaderUB
			mmdGroundShadowFSBufferInfo.setBuffer(modelRes.m_uniformBuffer.m_buffer);
			mmdGroundShadowFSBufferInfo.setOffset(matRes.m_mmdGroundShadowFSUBOffset);

			// Write MMD Ground Shadow descriptor set
			writesCount = std::extent<decltype(mmdGroundShadowWriteDescSets)>::value;
			for (uint32_t i = 0; i < writesCount; i++)
			{
				mmdGroundShadowWriteDescSets[i].setDstSet(matRes.m_mmdGroundShadowDescSet);
			}
			device.updateDescriptorSets(writesCount, mmdGroundShadowWriteDescSets, 0, nullptr);
		}
	}

	return true;
}

bool Model::SetupCommandBuffer(AppContext& appContext)
{
	vk::Result ret;

	auto device = appContext.m_device;

	auto imgCount = appContext.m_imageCount;
	std::vector<vk::CommandBuffer> cmdBuffers(appContext.m_imageCount);

	auto cmdBufInfo = vk::CommandBufferAllocateInfo()
		.setCommandBufferCount(imgCount)
		.setCommandPool(appContext.m_commandPool)
		.setLevel(vk::CommandBufferLevel::eSecondary);
	ret = device.allocateCommandBuffers(&cmdBufInfo, cmdBuffers.data());
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to allocate Models Command Buffer.\n";
		return false;
	}
	m_cmdBuffers = std::move(cmdBuffers);

	return true;
}

void Model::Destroy(AppContext & appContext)
{
	auto device = appContext.m_device;

	m_indexBuffer.Clear(appContext);

	for (auto& mat : m_materials)
	{
		device.destroySampler(mat.m_mmdTexSampler, nullptr);
		device.destroySampler(mat.m_mmdToonTexSampler, nullptr);
		device.destroySampler(mat.m_mmdSphereTexSampler, nullptr);
	}
	m_materials.clear();

	{
		auto& modelRes = m_resource.m_modelResource;
		modelRes.m_vertexBuffer.Clear(appContext);
		modelRes.m_uniformBuffer.Clear(appContext);
	}
	device.freeCommandBuffers(appContext.m_commandPool, uint32_t(m_cmdBuffers.size()), m_cmdBuffers.data());
	m_cmdBuffers.clear();

	device.destroyDescriptorPool(m_descPool, nullptr);

	m_mmdModel.reset();
	m_vmdAnim.reset();
}

void Model::UpdateAnimation(const AppContext& appContext)
{
	m_mmdModel->BeginAnimation();
	m_mmdModel->UpdateAllAnimation(m_vmdAnim.get(), appContext.m_animTime * 30.0f, appContext.m_elapsed);
	m_mmdModel->EndAnimation();
}

void Model::Update(AppContext& appContext)
{
	vk::Result ret;

	auto& res = m_resource;

	auto device = appContext.m_device;

	size_t vtxCount = m_mmdModel->GetVertexCount();
	m_mmdModel->Update();
	const glm::vec3* position = m_mmdModel->GetUpdatePositions();
	const glm::vec3* normal = m_mmdModel->GetUpdateNormals();
	const glm::vec2* uv = m_mmdModel->GetUpdateUVs();

	// Update vertices

	auto memSize = vk::DeviceSize(sizeof(Vertex) * vtxCount);
	StagingBuffer* vbStBuf;
	if (!appContext.GetStagingBuffer(memSize, &vbStBuf))
	{
		std::cout << "Failed to get Staging Buffer.\n";
		return;
	}

	void* vbStMem;
	ret = device.mapMemory(vbStBuf->m_memory, 0, memSize, vk::MemoryMapFlags(), &vbStMem);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to map memory.\n";
		return;
	}
	auto v = static_cast<Vertex*>(vbStMem);
	for (size_t i = 0; i < vtxCount; i++)
	{
		v->m_position = *position;
		v->m_normal = *normal;
		v->m_uv = *uv;
		v++;
		position++;
		normal++;
		uv++;
	}
	device.unmapMemory(vbStBuf->m_memory);

	if (!vbStBuf->CopyBuffer(appContext, res.m_modelResource.m_vertexBuffer.m_buffer, memSize))
	{
		std::cout << "Failed to copy buffer.\n";
		return;
	}

	// Update uniform buffer
	auto ubMemSize = res.m_modelResource.m_uniformBuffer.m_memorySize;
	StagingBuffer* ubStBuf;
	if (!appContext.GetStagingBuffer(ubMemSize, &ubStBuf))
	{
		std::cout << "Failed to get Staging Buffer.\n";
		return;
	}
	uint8_t* ubPtr;
	ret = device.mapMemory(ubStBuf->m_memory, 0, ubMemSize, vk::MemoryMapFlags(), (void**)&ubPtr);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to map memory.\n";
		return;
	}

	// Write Model uniform buffer
	auto& modelRes = res.m_modelResource;
	{
		const auto world = glm::mat4(1.0f);
		const auto& view = appContext.m_viewMat;
		const auto& proj = appContext.m_projMat;
		glm::mat4 vkMat = glm::mat4(
			1.0f,  0.0f, 0.0f, 0.0f,
			0.0f, -1.0f, 0.0f, 0.0f,
			0.0f,  0.0f, 0.5f, 0.0f,
			0.0f,  0.0f, 0.5f, 1.0f
		);
		auto wv = view;
		auto wvp = vkMat * proj * view;
		auto mmdVSUB = reinterpret_cast<MMDVertxShaderUB*>(ubPtr + res.m_modelResource.m_mmdVSUBOffset);
		mmdVSUB->m_wv = wv;
		mmdVSUB->m_wvp = wvp;

		auto mmdEdgeVSUB = reinterpret_cast<MMDEdgeVertexShaderUB*>(ubPtr + res.m_modelResource.m_mmdEdgeVSUBOffset);
		mmdEdgeVSUB->m_wv = wv;
		mmdEdgeVSUB->m_wvp = wvp;
		mmdEdgeVSUB->m_screenSize = glm::vec2(float(appContext.m_screenWidth), float(appContext.m_screenHeight));

		auto mmdGraoundShadowVSUB = reinterpret_cast<MMDGroundShadowVertexShaderUB*>(ubPtr + res.m_modelResource.m_mmdGroundShadowVSUBOffset);

		auto plane = glm::vec4(0, 1, 0, 0);
		auto light = -appContext.m_lightDir;
		auto shadow = glm::mat4(1);

		shadow[0][0] = plane.y * light.y + plane.z * light.z;
		shadow[0][1] = -plane.x * light.y;
		shadow[0][2] = -plane.x * light.z;
		shadow[0][3] = 0;

		shadow[1][0] = -plane.y * light.x;
		shadow[1][1] = plane.x * light.x + plane.z * light.z;
		shadow[1][2] = -plane.y * light.z;
		shadow[1][3] = 0;

		shadow[2][0] = -plane.z * light.x;
		shadow[2][1] = -plane.z * light.y;
		shadow[2][2] = plane.x * light.x + plane.y * light.y;
		shadow[2][3] = 0;

		shadow[3][0] = -plane.w * light.x;
		shadow[3][1] = -plane.w * light.y;
		shadow[3][2] = -plane.w * light.z;
		shadow[3][3] = plane.x * light.x + plane.y * light.y + plane.z * light.z;

		auto wsvp = vkMat * proj * view * shadow * world;
		mmdGraoundShadowVSUB->m_wvp = wsvp;
	}

	// Write Material uniform buffer;
	{
		auto matCount = m_mmdModel->GetMaterialCount();
		for (size_t i = 0; i < matCount; i++)
		{
			const auto& mmdMat = m_mmdModel->GetMaterials()[i];
			auto& matRes = res.m_materialResources[i];
			auto mmdFSUB = reinterpret_cast<MMDFragmentShaderUB*>(ubPtr + matRes.m_mmdFSUBOffset);
			const auto& mat = m_materials[i];

			mmdFSUB->m_alpha = mmdMat.m_alpha;
			mmdFSUB->m_diffuse = mmdMat.m_diffuse;
			mmdFSUB->m_ambient = mmdMat.m_ambient;
			mmdFSUB->m_specular = mmdMat.m_specular;
			mmdFSUB->m_specularPower = mmdMat.m_specularPower;
			// Tex
			if (!mmdMat.m_texture.empty())
			{
				if (!mat.m_mmdTex->m_hasAlpha)
				{
					mmdFSUB->m_textureModes.x = 1;
				}
				else
				{
					mmdFSUB->m_textureModes.x = 2;
				}
				mmdFSUB->m_texMulFactor = mmdMat.m_textureMulFactor;
				mmdFSUB->m_texAddFactor = mmdMat.m_textureAddFactor;
			}
			else
			{
				mmdFSUB->m_textureModes.x = 0;
			}
			// Toon Tex
			if (!mmdMat.m_toonTexture.empty())
			{
				mmdFSUB->m_textureModes.y = 1;
				mmdFSUB->m_toonTexMulFactor = mmdMat.m_toonTextureMulFactor;
				mmdFSUB->m_toonTexAddFactor = mmdMat.m_toonTextureAddFactor;
			}
			else
			{
				mmdFSUB->m_textureModes.y = 0;
				mmdFSUB->m_toonTexMulFactor = mmdMat.m_toonTextureMulFactor;
				mmdFSUB->m_toonTexAddFactor = mmdMat.m_toonTextureAddFactor;
			}
			// Sphere Tex
			if (!mmdMat.m_spTexture.empty())
			{
				if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Mul)
				{
					mmdFSUB->m_textureModes.z = 1;
				}
				else if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Add)
				{
					mmdFSUB->m_textureModes.z = 2;
				}
				mmdFSUB->m_sphereTexMulFactor = mmdMat.m_spTextureMulFactor;
				mmdFSUB->m_sphereTexAddFactor = mmdMat.m_spTextureAddFactor;
			}
			else
			{
				mmdFSUB->m_textureModes.z = 0;
				mmdFSUB->m_sphereTexMulFactor = mmdMat.m_spTextureMulFactor;
				mmdFSUB->m_sphereTexAddFactor = mmdMat.m_spTextureAddFactor;
			}

			mmdFSUB->m_lightColor = appContext.m_lightColor;
			glm::vec3 lightDir = appContext.m_lightDir;
			glm::mat3 viewMat = glm::mat3(appContext.m_viewMat);
			lightDir = viewMat * lightDir;
			mmdFSUB->m_lightDir = lightDir;

			auto mmdEdgeSizeVSUB = reinterpret_cast<MMDEdgeSizeVertexShaderUB*>(ubPtr + matRes.m_mmdEdgeSizeVSUBOffset);
			mmdEdgeSizeVSUB->m_edgeSize = mmdMat.m_edgeSize;

			auto mmdEdgeFSUB = reinterpret_cast<MMDEdgeFragmentShaderUB*>(ubPtr + matRes.m_mmdEdgeFSUBOffset);
			mmdEdgeFSUB->m_edgeColor = mmdMat.m_edgeColor;

			auto mmdGraoundShadowFSUB = reinterpret_cast<MMDGroundShadowFragmentShaderUB*>(ubPtr + matRes.m_mmdGroundShadowFSUBOffset);
			mmdGraoundShadowFSUB->m_shadowColor = glm::vec4(0.4f, 0.2f, 0.2f, 0.7f);
		}
	}
	device.unmapMemory(ubStBuf->m_memory);

	ubStBuf->CopyBuffer(appContext, modelRes.m_uniformBuffer.m_buffer, ubMemSize);
}

void Model::Draw(AppContext& appContext)
{
	auto inheritanceInfo = vk::CommandBufferInheritanceInfo()
		.setRenderPass(appContext.m_renderPass)
		.setSubpass(0);
	auto beginInfo = vk::CommandBufferBeginInfo()
		.setFlags(vk::CommandBufferUsageFlagBits::eRenderPassContinue)
		.setPInheritanceInfo(&inheritanceInfo);

	auto& res = m_resource;
	auto& modelRes = res.m_modelResource;
	auto& cmdBuf = m_cmdBuffers[appContext.m_imageIndex];

	auto width = appContext.m_screenWidth;
	auto height = appContext.m_screenHeight;

	cmdBuf.begin(beginInfo);

	auto viewport = vk::Viewport()
		.setWidth(float(width))
		.setHeight(float(height))
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);
	cmdBuf.setViewport(0, 1, &viewport);

	auto scissor = vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(width, height));
	cmdBuf.setScissor(0, 1, &scissor);

	vk::DeviceSize offsets[1] = { 0 };
	cmdBuf.bindVertexBuffers(0, 1, &modelRes.m_vertexBuffer.m_buffer, offsets);
	cmdBuf.bindIndexBuffer(m_indexBuffer.m_buffer, 0, m_indexType);

	// MMD
	vk::Pipeline* currentMMDPipeline = nullptr;
	size_t subMeshCount = m_mmdModel->GetSubMeshCount();
	for (size_t i = 0; i < subMeshCount; i++)
	{
		const auto& subMesh = m_mmdModel->GetSubMeshes()[i];
		const auto& matID = subMesh.m_materialID;

		const auto& mmdMat = m_mmdModel->GetMaterials()[matID];
		auto& matRes = res.m_materialResources[matID];

		if (mmdMat.m_alpha == 0.0f)
		{
			continue;
		}

		cmdBuf.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			appContext.m_mmdPipelineLayout,
			0, 1, &matRes.m_mmdDescSet,
			0, nullptr);

		vk::Pipeline* mmdPipeline = nullptr;
		if (mmdMat.m_bothFace)
		{
			mmdPipeline = &appContext.m_mmdPipelines[
				int(AppContext::MMDRenderType::AlphaBlend_BothFace)
			];
		}
		else
		{
			mmdPipeline = &appContext.m_mmdPipelines[
				int(AppContext::MMDRenderType::AlphaBlend_FrontFace)
			];
		}
		if (currentMMDPipeline != mmdPipeline)
		{
			cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *mmdPipeline);
			currentMMDPipeline = mmdPipeline;
		}
		cmdBuf.drawIndexed(subMesh.m_vertexCount, 1, subMesh.m_beginIndex, 0, 1);
	}

	// MMD Edge
	cmdBuf.bindPipeline(
		vk::PipelineBindPoint::eGraphics,
		appContext.m_mmdEdgePipeline);
	for (size_t i = 0; i < subMeshCount; i++)
	{
		const auto& subMesh = m_mmdModel->GetSubMeshes()[i];
		const auto& matID = subMesh.m_materialID;

		const auto& mmdMat = m_mmdModel->GetMaterials()[matID];
		auto& matRes = res.m_materialResources[matID];

		if (!mmdMat.m_edgeFlag)
		{
			continue;
		}
		if (mmdMat.m_alpha == 0.0f)
		{
			continue;
		}

		cmdBuf.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			appContext.m_mmdEdgePipelineLayout,
			0, 1, &matRes.m_mmdEdgeDescSet,
			0, nullptr);

		cmdBuf.drawIndexed(subMesh.m_vertexCount, 1, subMesh.m_beginIndex, 0, 1);
	}

	// MMD GroundShadow
	cmdBuf.bindPipeline(
		vk::PipelineBindPoint::eGraphics,
		appContext.m_mmdGroundShadowPipeline);
	cmdBuf.setDepthBias(-1.0f, 0.0f, -1.0f);
	cmdBuf.setStencilReference(vk::StencilFaceFlagBits::eVkStencilFrontAndBack, 0x01);
	cmdBuf.setStencilCompareMask(vk::StencilFaceFlagBits::eVkStencilFrontAndBack, 0x01);
	cmdBuf.setStencilWriteMask(vk::StencilFaceFlagBits::eVkStencilFrontAndBack, 0xff);
	for (size_t i = 0; i < subMeshCount; i++)
	{
		const auto& subMesh = m_mmdModel->GetSubMeshes()[i];
		const auto& matID = subMesh.m_materialID;

		const auto& mmdMat = m_mmdModel->GetMaterials()[matID];
		auto& matRes = res.m_materialResources[matID];

		if (!mmdMat.m_groundShadow)
		{
			continue;
		}
		if (mmdMat.m_alpha == 0.0f)
		{
			continue;
		}

		cmdBuf.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			appContext.m_mmdGroundShadowPipelineLayout,
			0, 1, &matRes.m_mmdGroundShadowDescSet,
			0, nullptr);

		cmdBuf.drawIndexed(subMesh.m_vertexCount, 1, subMesh.m_beginIndex, 0, 1);
	}
	cmdBuf.end();
}

vk::CommandBuffer Model::GetCommandBuffer(uint32_t imageIndex) const
{
	return m_cmdBuffers[imageIndex];
}


class App
{
public:
	App(GLFWwindow* window);
	bool Run(const std::vector<std::string>& args);
	void Exit();
	void Resize(uint16_t w, uint16_t h);
	uint16_t GetWidth() const { return m_width; }
	uint16_t GetHeight() const { return m_height; }

private:
	void MainLoop();

private:
	GLFWwindow*			m_window = nullptr;
	std::thread			m_mainThread;
	std::atomic<bool>	m_runable;
	std::atomic<uint32_t>	m_newSize;
	uint16_t			m_width = 1280;
	uint16_t			m_height = 800;

	vk::Instance		m_vkInst;
	vk::SurfaceKHR		m_surface;
	vk::PhysicalDevice	m_gpu;
	vk::Device			m_device;

	AppContext	m_appContext;
	std::vector<Model>	m_models;
};

App::App(GLFWwindow* window)
{
	m_window = window;
	int fbWidth, fbHeight;
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	m_width = uint16_t(fbWidth);
	m_height = uint16_t(fbHeight);
	m_newSize = 0;
}

bool App::Run(const std::vector<std::string>& args)
{
	Input currentInput;
	std::vector<Input> inputModels;
	bool enableValidation = false;
	for (auto argIt = args.begin(); argIt != args.end(); ++argIt)
	{
		const auto& arg = (*argIt);
		if (arg == "-model")
		{
			if (!currentInput.m_modelPath.empty())
			{
				inputModels.emplace_back(currentInput);
			}

			++argIt;
			if (argIt == args.end())
			{
				Usage();
				return false;
			}
			currentInput = Input();
			currentInput.m_modelPath = (*argIt);
		}
		else if (arg == "-vmd")
		{
			if (currentInput.m_modelPath.empty())
			{
				Usage();
				return false;
			}

			++argIt;
			if (argIt == args.end())
			{
				Usage();
				return false;
			}
			currentInput.m_vmdPaths.push_back((*argIt));
		}
		else if (arg == "-validation")
		{
			enableValidation = true;
		}
	}
	if (!currentInput.m_modelPath.empty())
	{
		inputModels.emplace_back(currentInput);
	}

	// Vulkan Instance
	std::vector<const char*> extensions;
	{
		uint32_t extensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
		for (uint32_t i = 0; i < extensionCount; i++)
		{
			extensions.push_back(glfwExtensions[i]);
		}
		if (enableValidation)
		{
			//extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}
	}

	std::vector<const char*> layers;
	if (enableValidation)
	{
		auto availableLayerProperties = vk::enumerateInstanceLayerProperties();
		const char* validationLayerName = "VK_LAYER_LUNARG_standard_validation";
		bool layerFound = false;
		for (const auto layerProperties : availableLayerProperties)
		{
			if (strcmp(validationLayerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (layerFound)
		{
			layers.push_back(validationLayerName);
			std::cout << "Enable validation layer.\n";
		}
		else
		{
			std::cout << "Failed to found validation layer.\n";
		}
	}

	auto instInfo = vk::InstanceCreateInfo()
		.setEnabledExtensionCount(uint32_t(extensions.size()))
		.setPpEnabledExtensionNames(extensions.data())
		.setEnabledLayerCount(uint32_t(layers.size()))
		.setPpEnabledLayerNames(layers.data());


	vk::Result ret;
	ret = vk::createInstance(&instInfo, nullptr, &m_vkInst);
	if (ret != vk::Result::eSuccess)
	{
		std::cout << "Failed to craete vulkan instance.\n";
		return false;
	}

	{
		VkSurfaceKHR surface;
		VkResult err = glfwCreateWindowSurface(m_vkInst, m_window, nullptr, &surface);
		if (err)
		{
			std::cout << "Failed to create surface.[" << err << "\n";
		}
		m_surface = surface;
	}

	// Select physical device
	auto physicalDevices = m_vkInst.enumeratePhysicalDevices();
	if (physicalDevices.empty())
	{
		std::cout << "Failed to find vulkan physical device.\n";
		return false;
	}

	m_gpu = physicalDevices[0];
	auto queueFamilies = m_gpu.getQueueFamilyProperties();
	if (queueFamilies.empty())
	{
		return false;
	}

	// Select queue family
	uint32_t selectGraphicsQueueFamilyIndex = UINT32_MAX;
	uint32_t selectPresentQueueFamilyIndex = UINT32_MAX;
	for (uint32_t i = 0; i < (uint32_t)queueFamilies.size(); i++)
	{
		const auto& queue = queueFamilies[i];
		if (queue.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			if (selectGraphicsQueueFamilyIndex == UINT32_MAX)
			{
				selectGraphicsQueueFamilyIndex = i;
			}
			auto supported = m_gpu.getSurfaceSupportKHR(i, m_surface);
			if (supported)
			{
				selectGraphicsQueueFamilyIndex = i;
				selectPresentQueueFamilyIndex = i;
				break;
			}
		}
	}
	if (selectGraphicsQueueFamilyIndex == UINT32_MAX) {
		std::cout << "Failed to find graphics queue family.\n";
		return false;
	}
	if (selectPresentQueueFamilyIndex == UINT32_MAX)
	{
		for (uint32_t i = 0; i < (uint32_t)queueFamilies.size(); i++)
		{
			auto supported = m_gpu.getSurfaceSupportKHR(i, m_surface);
			if (supported)
			{
				selectPresentQueueFamilyIndex = i;
				break;
			}
		}
	}
	if (selectPresentQueueFamilyIndex == UINT32_MAX)
	{
		std::cout << "Failed to find present queue family.\n";
		return false;
	}

	// Create device

	std::vector<const char*> deviceExtensions;
	{
		auto availableExtProperties = m_gpu.enumerateDeviceExtensionProperties();
		const char* checkExtensions[] = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		};
		for (const auto& checkExt : checkExtensions)
		{
			bool foundExt = false;
			for (const auto& extProperties : availableExtProperties)
			{
				if (strcmp(checkExt, extProperties.extensionName) == 0)
				{
					foundExt = true;
					break;
				}
			}
			if (foundExt)
			{
				deviceExtensions.push_back(checkExt);
			}
			else
			{
				std::cout << "Failed to found extension.[" << checkExt << "]\n";
				return false;
			}
		}
	}

	vk::PhysicalDeviceFeatures features;
	features.setSampleRateShading(true);

	float priority = 1.0;
	auto queueInfo = vk::DeviceQueueCreateInfo()
		.setQueueFamilyIndex(selectGraphicsQueueFamilyIndex)
		.setQueueCount(1)
		.setPQueuePriorities(&priority);

	auto deviceInfo = vk::DeviceCreateInfo()
		.setQueueCreateInfoCount(1)
		.setPQueueCreateInfos(&queueInfo)
		.setEnabledExtensionCount(uint32_t(deviceExtensions.size()))
		.setPpEnabledExtensionNames(deviceExtensions.data())
		.setPEnabledFeatures(&features);
	ret = m_gpu.createDevice(&deviceInfo, nullptr, &m_device);
	if (vk::Result::eSuccess != ret)
	{
		std::cout << "Failed to create Device.\n";
		return false;
	}

	m_appContext.m_screenWidth = m_width;
	m_appContext.m_screenHeight = m_height;
	if (!m_appContext.Setup(m_vkInst, m_surface, m_gpu, m_device))
	{
		std::cout << "Failed to setup.\n";
		return false;
	}

	for (const auto& input : inputModels)
	{
		// Load MMD model
		Model model;
		auto ext = saba::PathUtil::GetExt(input.m_modelPath);
		if (ext == "pmd")
		{
			auto pmdModel = std::make_unique<saba::PMDModel>();
			if (!pmdModel->Load(input.m_modelPath, m_appContext.m_mmdDir))
			{
				std::cout << "Failed to load pmd file.\n";
				return false;
			}
			model.m_mmdModel = std::move(pmdModel);
		}
		else if (ext == "pmx")
		{
			auto pmxModel = std::make_unique<saba::PMXModel>();
			if (!pmxModel->Load(input.m_modelPath, m_appContext.m_mmdDir))
			{
				std::cout << "Failed to load pmx file.\n";
				return false;
			}
			model.m_mmdModel = std::move(pmxModel);
		}
		else
		{
			std::cout << "Unknown file type. [" << ext << "]\n";
			return false;
		}
		model.m_mmdModel->InitializeAnimation();

		// Load VMD animation
		auto vmdAnim = std::make_unique<saba::VMDAnimation>();
		if (!vmdAnim->Create(model.m_mmdModel))
		{
			std::cout << "Failed to create VMDAnimation.\n";
			return false;
		}
		for (const auto& vmdPath : input.m_vmdPaths)
		{
			saba::VMDFile vmdFile;
			if (!saba::ReadVMDFile(&vmdFile, vmdPath.c_str()))
			{
				std::cout << "Failed to read VMD file.\n";
				return false;
			}
			if (!vmdAnim->Add(vmdFile))
			{
				std::cout << "Failed to add VMDAnimation.\n";
				return false;
			}
			if (!vmdFile.m_cameras.empty())
			{
				auto vmdCamAnim = std::make_unique<saba::VMDCameraAnimation>();
				if (!vmdCamAnim->Create(vmdFile))
				{
					std::cout << "Failed to create VMDCameraAnimation.\n";
				}
				m_appContext.m_vmdCameraAnim = std::move(vmdCamAnim);
			}
		}
		vmdAnim->SyncPhysics(0.0f);

		model.m_vmdAnim = std::move(vmdAnim);

		model.Setup(m_appContext);

		m_models.emplace_back(std::move(model));
	}

	m_runable = true;
	m_mainThread = std::thread([this]() { this->MainLoop(); });

	return true;
}

void App::Exit()
{
	if (!m_device)
	{
		return;
	}

	if (m_runable)
	{
		m_runable = false;
		m_mainThread.join();
	}

	m_device.waitIdle();

	for (auto& model : m_models)
	{
		model.Destroy(m_appContext);
	}
	m_models.clear();

	m_appContext.Destory();

	m_device.destroy();
	m_device = nullptr;

}

void App::MainLoop()
{
	double fpsTime = saba::GetTime();
	int fpsFrame = 0;
	double saveTime = saba::GetTime();
	int frame = 0;
	std::vector<vk::Semaphore> waitSemaphores;
	std::vector<vk::PipelineStageFlags> waitStageMasks;
	while (m_runable)
	{
		frame++;
		uint32_t newSize = m_newSize.exchange(0);
		if (newSize != 0)
		{
			int newWidth = int(newSize >> 16);
			int newHeight = int(newSize & 0xFFFF);
			if (m_width != newWidth || m_height != newHeight)
			{
				m_width = newWidth;
				m_height = newHeight;
				if (!m_appContext.Resize())
				{
					return;
				}
			}
		}

		vk::Result ret;
		auto& frameSyncData = m_appContext.m_frameSyncDatas[m_appContext.m_frameIndex];
		auto waitFence = frameSyncData.m_fence;
		auto renderCompleteSemaphore = frameSyncData.m_renderCompleteSemaphore;
		auto presentCompleteSemaphore = frameSyncData.m_presentCompleteSemaphore;

		m_device.waitForFences(1, &waitFence, true, UINT64_MAX);
		m_device.resetFences(1, &waitFence);

		do
		{
			ret = m_device.acquireNextImageKHR(
				m_appContext.m_swapchain,
				UINT64_MAX,
				presentCompleteSemaphore,
				vk::Fence(),
				&m_appContext.m_imageIndex
			);
			if (vk::Result::eErrorOutOfDateKHR == ret)
			{
				if (!m_appContext.Resize())
				{
					return;
				}
			}
			else if (vk::Result::eSuccess != ret)
			{
				std::cout << "acquireNextImageKHR error.[" << uint32_t(ret) << "]\n";
				return;
			}
		} while (vk::Result::eSuccess != ret);

		//FPS
		{
			fpsFrame++;
			double time = saba::GetTime();
			double deltaTime = time - fpsTime;
			if (deltaTime > 1.0)
			{
				double fps = double(fpsFrame) / deltaTime;
				std::cout << fps << " fps\n";
				fpsFrame = 0;
				fpsTime = time;
			}
		}

		double time = saba::GetTime();
		double elapsed = time - saveTime;
		if (elapsed > 1.0 / 30.0)
		{
			elapsed = 1.0 / 30.0;
		}
		saveTime = time;
		m_appContext.m_elapsed = float(elapsed);
		m_appContext.m_animTime += float(elapsed);

		// Setup Camera
		int screenWidth = m_appContext.m_screenWidth;
		int screenHeight = m_appContext.m_screenHeight;
		if (m_appContext.m_vmdCameraAnim)
		{
			m_appContext.m_vmdCameraAnim->Evaluate(m_appContext.m_animTime * 30.0f);
			const auto mmdCam = m_appContext.m_vmdCameraAnim->GetCamera();
			saba::MMDLookAtCamera lookAtCam(mmdCam);
			m_appContext.m_viewMat = glm::lookAt(lookAtCam.m_eye, lookAtCam.m_center, lookAtCam.m_up);
			m_appContext.m_projMat = glm::perspectiveFovRH(mmdCam.m_fov, float(screenWidth), float(screenHeight), 1.0f, 10000.0f);
		}
		else
		{
			m_appContext.m_viewMat = glm::lookAt(glm::vec3(0, 10, 50), glm::vec3(0, 10, 0), glm::vec3(0, 1, 0));
			m_appContext.m_projMat = glm::perspectiveFovRH(glm::radians(30.0f), float(screenWidth), float(screenHeight), 1.0f, 10000.0f);
		}


		auto imgIndex = m_appContext.m_imageIndex;
		auto& res = m_appContext.m_swapchainImageResouces[imgIndex];
		auto& cmdBuf = res.m_cmdBuffer;

		// Update / Draw Models
		for (auto& model : m_models)
		{
			// Update Animation
			model.UpdateAnimation(m_appContext);

			// Update Vertices
			model.Update(m_appContext);

			// Draw
			model.Draw(m_appContext);
		}

		auto beginInfo = vk::CommandBufferBeginInfo();
		cmdBuf.begin(beginInfo);

		auto clearColor = vk::ClearColorValue(std::array<float, 4>({ 1.0f, 0.8f, 0.75f, 1.0f }));
		auto clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
		vk::ClearValue clearValues[] = {
			clearColor,
			clearColor,
			clearDepth,
			clearDepth,
		};
		auto renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(m_appContext.m_renderPass)
			.setFramebuffer(res.m_framebuffer)
			.setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(screenWidth, screenHeight)))
			.setClearValueCount(std::extent<decltype(clearValues)>::value)
			.setPClearValues(clearValues);
		cmdBuf.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eSecondaryCommandBuffers);

		for (auto& model : m_models)
		{
			auto modelCmdBuf = model.GetCommandBuffer(imgIndex);
			cmdBuf.executeCommands(1, &modelCmdBuf);
		}

		cmdBuf.endRenderPass();
		cmdBuf.end();

		//m_appContext.WaitAllStagingBuffer();

		waitSemaphores.push_back(presentCompleteSemaphore);
		waitStageMasks.push_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		for (const auto& stBuf : m_appContext.m_stagingBuffers)
		{
			if (stBuf->m_waitSemaphore)
			{
				waitSemaphores.push_back(stBuf->m_waitSemaphore);
				waitStageMasks.push_back(vk::PipelineStageFlagBits::eTransfer);
				stBuf->m_waitSemaphore = nullptr;
			}
		}
		vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		auto submitInfo = vk::SubmitInfo()
			.setPWaitDstStageMask(waitStageMasks.data())
			.setWaitSemaphoreCount(uint32_t(waitSemaphores.size()))
			.setPWaitSemaphores(waitSemaphores.data())
			.setSignalSemaphoreCount(1)
			.setPSignalSemaphores(&renderCompleteSemaphore)
			.setCommandBufferCount(1)
			.setPCommandBuffers(&cmdBuf);
		ret = m_appContext.m_graphicsQueue.submit(1, &submitInfo, waitFence);
		waitSemaphores.clear();
		waitStageMasks.clear();

		auto presentInfo = vk::PresentInfoKHR()
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&renderCompleteSemaphore)
			.setSwapchainCount(1)
			.setPSwapchains(&m_appContext.m_swapchain)
			.setPImageIndices(&m_appContext.m_imageIndex);
		ret = m_appContext.m_graphicsQueue.presentKHR(&presentInfo);
		if (vk::Result::eSuccess != ret &&
			vk::Result::eErrorOutOfDateKHR != ret)
		{
			std::cout << "presetKHR error.[" << uint32_t(ret) << "]\n";
			return;
		}

		m_appContext.m_frameIndex++;
		m_appContext.m_frameIndex = m_appContext.m_frameIndex % m_appContext.m_imageCount;
	}
}

void App::Resize(uint16_t w, uint16_t h)
{
	uint32_t size = (uint32_t(w) << 16) | h;
	m_newSize = size;
}

void OnWindowSize(GLFWwindow* window, int w, int h)
{
	auto app = (App*)glfwGetWindowUserPointer(window);

	int fbWidth, fbHeight;
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	if (fbWidth != app->GetWidth() || fbHeight != app->GetHeight())
	{
		app->Resize(fbWidth, fbHeight);
	}
}

bool SampleMain(std::vector<std::string>& args)
{
	// Initialize glfw
	if (!glfwInit())
	{
		std::cout << "Failed to initialize glfw.\n";
		return false;
	}

	if (!glfwVulkanSupported())
	{
		std::cout << "Does not support Vulkan.\n";
		return false;
	}

	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 800, "", nullptr, nullptr);

	auto app = std::make_unique<App>(window);
	glfwSetWindowSizeCallback(window, OnWindowSize);
	glfwSetWindowUserPointer(window, app.get());

	if (!app->Run(args))
	{
		return false;
	}

	while (!glfwWindowShouldClose(window))
	{
		glfwWaitEvents();
	}
	app->Exit();

	app.reset();

	glfwTerminate();

	return true;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		Usage();
		return 1;
	}

	std::vector<std::string> args(argc);
#if _WIN32
	{
		WCHAR* cmdline = GetCommandLineW();
		int wArgc;
		WCHAR** wArgs = CommandLineToArgvW(cmdline, &wArgc);
		for (int i = 0; i < argc; i++)
		{
			args[i] = saba::ToUtf8String(wArgs[i]);
		}
	}
#else // _WIN32
	for (int i = 0; i < argc; i++)
	{
		args[i] = argv[i];
	}
#endif

	if (!SampleMain(args))
	{
		std::cout << "Failed to run.\n";
		return 1;
	}

	return 0;
}
