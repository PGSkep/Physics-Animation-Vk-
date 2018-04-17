#ifndef RENDERER_H
#define RENDERER_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VkU.h"

#include <vector>
#include <array>

#include "Loader.h"

#include "Physics.h"

#include "Scene.h"

class Renderer
{
	Timer timer;
	// Basic
	VkInstance instance = VK_NULL_HANDLE;
	VkDebugReportCallbackEXT debugReportCallback = VK_NULL_HANDLE;
	std::vector<VkU::PhysicalDevice> physicalDevices;
	VkU::Window window;
	VkU::Surface surface;
	VkU::Device device;
	VkCommandPool commandPool;
	VkCommandBuffer setupCommandBuffer;
	VkFence setupFence;
	VkRenderPass renderPass;
	VkU::Image depthImage;
	VkU::Swapchain swapchain;
	uint32_t swapchainImageIndex;
	VkSemaphore semaphoreImageAvailable;
	VkSemaphore semaphoreRenderDone;

	// Storage
	VkU::Camera camera;
	std::vector<glm::mat4> modelMatrices;
	struct skeleton
	{
		glm::mat4 bones[64];
	};
	std::vector<skeleton> skeletons;

	std::vector<VkU::ShaderModule>	shaderModules;
	std::vector<VkU::Image>			textures;

	std::vector<VkU::UniformBuffer>	uniformBuffers;
	std::vector<VkU::VertexBuffer>	vertexBuffers;
	std::vector<VkU::IndexBuffer>	indexBuffers;

	// Derived
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	std::vector<VkPipeline> pipelines;
	std::vector<VkVertexInputBindingDescription>			vertexInputBindingDescriptions;
	std::vector<std::vector<VkVertexInputAttributeDescription>>	vertexInputAttributeDescriptions;
	std::vector<VkViewport>									viewports;
	std::vector<VkRect2D>									scissors;
	std::vector<VkPipelineColorBlendAttachmentState>		pipelineColorBlendAttachmentState;
	std::vector<std::vector<VkPipelineShaderStageCreateInfo>>	pipelineShaderStagesCreateInfos;
	std::vector<VkPipelineVertexInputStateCreateInfo>			pipelineVertexInputStateCreateInfos;
	std::vector<VkPipelineInputAssemblyStateCreateInfo>			pipelineInputAssemblyStateCreateInfos;
	std::vector<VkPipelineTessellationStateCreateInfo>			pipelineTessellationStateCreateInfos;
	std::vector<VkPipelineViewportStateCreateInfo>				pipelineViewportStateCreateInfos;
	std::vector<VkPipelineRasterizationStateCreateInfo>			pipelineRasterizationStateCreateInfos;
	std::vector<VkPipelineMultisampleStateCreateInfo>			pipelineMultisampleStateCreateInfos;
	std::vector<VkPipelineDepthStencilStateCreateInfo>			pipelineDepthStencilStateCreateInfos;
	std::vector<VkPipelineColorBlendStateCreateInfo>			pipelineColorBlendStateCreateInfos;
	std::vector<VkPipelineDynamicStateCreateInfo>				pipelineDynamicStateCreateInfos;
	std::vector<VkGraphicsPipelineCreateInfo> graphicsPipelineCreateInfos;

	VkDescriptorPool	descriptorPool;
	VkDescriptorSet		descriptorSet;
	VkSampler			sampler;

	std::vector<VkCommandBuffer> renderCommandBuffers;
	std::vector<VkFence> renderFences;

	size_t singleTimeTransformCount;

public:
	static Renderer* gRenderer;
	bool resizing = false;

	struct RendererInitProperties
	{
		const char*	appName;
		uint32_t	appVersion;
		const char*	engineName;
		uint32_t	engineVersion;

		bool		enableDebug;
		bool		enableSwapchain;

		PFN_vkDebugReportCallbackEXT debugReportCallback;
		VkDebugReportFlagsEXT debugFlags;

		std::vector<VkFormat>* preferedDepthFormats;

		uint32_t windowWidth;
		uint32_t windowHeight;
		const char* windowTitle;
		const char* windowName;

		std::vector<VkU::Queue> queues;

		bool useDepthBuffer;
		uint32_t targetSwapchainImageCount;
	};
	struct DataPackProperties
	{
		struct ShaderModuleName
		{
			const char* filename;
			VkShaderStageFlagBits stage;
			const char* entryPointName;
		};

		std::vector<ShaderModuleName> shaderModulesNames;

		std::vector<const char*> textureNames;

		std::vector<const char*> modelNames;

		size_t maxModelMatrixCount; // largest model matrix index you can reference -1
		size_t maxAnimationCount; // largest animation index you can reference -1
	};
	struct PipelineProperties
	{
		size_t shaderStateIndex;
		size_t vertexInputStateIndex;
		size_t inputAssemblyStateIndex;
		size_t viewportStateIndex;
		size_t rasterizationStateIndex;
		size_t multisampleStateIndex;
		size_t depthStencilStateIndex;
		size_t colorBlendStateIndex;
	};
	struct SetupProperties
	{
		std::vector<std::vector<size_t>> shaders;

		std::vector<PipelineProperties> pipelines;
	};

	void Init(RendererInitProperties _rendererInitProperties);

	void LoadScene(DataPackProperties _dataPackProperties);
	void Setup(SetupProperties _setupProperties);

	void UpdateRenderList(Scene& _scene);
	void UpdateUniforms(Scene& _scene);
	void HandleWindow();
	void PrepareToDraw();
	void Draw(Scene& _scene);
	void Render();

	void ShutDown();

	void Update(Scene& _scene);
};

#endif