#include "Renderer.h"

#include <glm/gtx/quaternion.hpp>

#define GRAPHICS_PRESENT_QUEUE_INDEX 0

#define CAMERA_UNIFORM_INDEX 0
#define CAMERA_UNIFORM_BINDING 0
#define MODEL_UNIFORM_INDEX 1
#define MODEL_UNIFORM_BINDING 1
#define SKELETON_UNIFORM_INDEX 2
#define SKELETON_UNIFORM_BINDING 2
#define TEXTURE_INDEX 0
#define TEXTURE_BINDING 3

#define MODEL_POSITION_X 0.0f
#define MODEL_POSITION_Y 0.0f
#define MODEL_POSITION_Z 1750.0f

#define CAMERA_SPEED 55.1f
#define CAMERA_OFFSET_FROM_CENTER 0.0f
#define CAMERA_VIEW_HEIGHT 0.0f
#define CAMERA_POSITION_X -400.0f
#define CAMERA_POSITION_Y -7000.0f
#define CAMERA_POSITION_Z +1000.0f

Renderer* Renderer::gRenderer = nullptr;
static LRESULT CALLBACK WndProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
{
	switch (_uMsg)
	{
	case WM_CLOSE:
		DestroyWindow(_hWnd);
		break;
	case WM_PAINT:
		ValidateRect(_hWnd, NULL);
		break;
	case WM_SIZE:
		if (Renderer::gRenderer != nullptr && _wParam != SIZE_MINIMIZED)
		{
			if (Renderer::gRenderer->resizing == true || _wParam == SIZE_MAXIMIZED || _wParam == SIZE_RESTORED)
			{
				RECT rect;

				int width = 800;
				int height = 600;
				if (GetClientRect(_hWnd, &rect))
				{
					width = rect.right - rect.left;
					height = rect.bottom - rect.top;
				}

				//Renderer::gRenderer->ReCreateSwapchain(width, height);
			}
		}
		break;
	case WM_ENTERSIZEMOVE:
		Renderer::gRenderer->resizing = true;
		break;
	case WM_EXITSIZEMOVE:
		Renderer::gRenderer->resizing = false;
		break;
	}

	return (DefWindowProc(_hWnd, _uMsg, _wParam, _lParam));
}

void Renderer::Init(RendererInitProperties _rendererInitProperties)
{
	// Timer
	{
		timer.SetResolution(Timer::RESOLUTION::RESOLUTION_NANOSECONDS);
		timer.Play();
	}

	// Logger
	{
#ifdef LOG_RENDERER
		VkU::logger.Start("RendererLog.txt");
		VkU::debugReportCallbackLogger.Start("VulkanDebugReportCallback.txt");
		VkU::cleanupLogger.Start("RendererCleanup.txt");

		VkU::logger << "Init:\n";
		VkU::timer.SetResolution(Timer::RESOLUTION::RESOLUTION_NANOSECONDS);
		VkU::timer.Play();
#endif
	}

	/// Instance creation
	{
		VkApplicationInfo applicationInfo = VkU::GetVkApplicationInfo(_rendererInitProperties.appName, _rendererInitProperties.appVersion, _rendererInitProperties.engineName, _rendererInitProperties.engineVersion);

		std::vector<const char*> instanceLayerNames;
		if (_rendererInitProperties.enableDebug)
			instanceLayerNames.push_back("VK_LAYER_LUNARG_standard_validation");
		//if (_rendererInitProperties.enableSwapchain)
		//	instanceLayerNames.push_back("VK_LAYER_LUNARG_swapchain");

		std::vector<const char*> instanceExtensionNames;
		if (_rendererInitProperties.enableDebug)
			instanceExtensionNames.push_back("VK_EXT_debug_report");
		if (_rendererInitProperties.enableSwapchain)
		{
			instanceExtensionNames.push_back("VK_KHR_surface");
			instanceExtensionNames.push_back("VK_KHR_win32_surface");
		}

		VkInstanceCreateInfo instanceCreateInfo = VkU::GetVkInstanceCreateInfo(&applicationInfo, (uint32_t)instanceLayerNames.size(), instanceLayerNames.data(), (uint32_t)instanceExtensionNames.size(), instanceExtensionNames.data());

		VK_CHECK_RESULT("	", vkCreateInstance(&instanceCreateInfo, nullptr, &instance), " - ", instance, " - vkCreateInstance");
	}

	/// Debug creation
	{
		if (_rendererInitProperties.enableDebug == true)
		{
			if (_rendererInitProperties.debugReportCallback == nullptr)
				_rendererInitProperties.debugReportCallback = VkU::DebugReportCallback;

			VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = VkU::GetVkDebugReportCallbackCreateInfoEXT(_rendererInitProperties.debugFlags, _rendererInitProperties.debugReportCallback);

			PFN_vkCreateDebugReportCallbackEXT FP_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
			VK_CHECK_RESULT("	", FP_vkCreateDebugReportCallbackEXT(instance, &debugReportCallbackCreateInfo, nullptr, &debugReportCallback), " - ", debugReportCallback, " - vkCreateDebugReportCallbackEXT");
		}
		else
			debugReportCallback = VK_NULL_HANDLE;
	}

	// PhysicalDevice creation
	physicalDevices = VkU::GetPhysicalDevices(instance, _rendererInitProperties.preferedDepthFormats);

	/// Window creation
	if (_rendererInitProperties.enableSwapchain == true)
	{
		window = VkU::GetWindow(_rendererInitProperties.windowWidth, _rendererInitProperties.windowHeight, _rendererInitProperties.windowTitle, _rendererInitProperties.windowName, WndProc);
	}

	/// Surface creation
	{
		VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = VkU::GetVkWin32SurfaceCreateInfoKHR(window.hInstance, window.hWnd);

		VK_CHECK_RESULT("	", vkCreateWin32SurfaceKHR(instance, &win32SurfaceCreateInfo, nullptr, &surface.handle), " - ", surface.handle, " - vkCreateWin32SurfaceKHR");
	}

	// PhysicalDevice & Queue picking
	{
		device.physicalDeviceIndex = -1;

		for (size_t i = 0; i != physicalDevices.size(); ++i)
		{
			bool compatible = false;
			device.queues = VkU::PickDeviceQueuesIndices(_rendererInitProperties.queues, physicalDevices[i], { surface }, &compatible);

			if (compatible == true)
			{
				device.physicalDeviceIndex = (uint32_t)i;
				break;
			}
		}

		if (device.physicalDeviceIndex == -1)
			return;
	}

	/// Device Creation
	{
		std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfo(device.queues.size());
		for (size_t i = 0; i != deviceQueueCreateInfo.size(); ++i)
			deviceQueueCreateInfo[i] = VkU::GetVkDeviceQueueCreateInfo(device.queues[i].queueFamilyIndex, device.queues[i].count, &device.queues[i].priority);

		std::vector<const char*> deviceLayerNames;
		if (debugReportCallback != VK_NULL_HANDLE)
			deviceLayerNames.push_back("VK_LAYER_LUNARG_standard_validation");

		std::vector<const char*> deviceExtensionNames;
		if (surface.handle != VK_NULL_HANDLE)
			deviceExtensionNames.push_back("VK_KHR_swapchain");

		VkPhysicalDeviceFeatures features = {};

		VkDeviceCreateInfo deviceCreateInfo = VkU::GetVkDeviceCreateInfo((uint32_t)deviceQueueCreateInfo.size(), deviceQueueCreateInfo.data(), (uint32_t)deviceLayerNames.size(), deviceLayerNames.data(), (uint32_t)deviceExtensionNames.size(), deviceExtensionNames.data(), &features);

		VK_CHECK_RESULT("	", vkCreateDevice(physicalDevices[device.physicalDeviceIndex].handle, &deviceCreateInfo, nullptr, &device.handle), " - ", device.handle, " - vkCreateDevice");

	}

	// Queue Allocation
	{
		for (size_t i = 0; i != device.queues.size(); ++i)
		{
			for (uint32_t j = 0; j != device.queues[i].count; ++j)
			{
				device.queues[i].handles.push_back(VK_NULL_HANDLE);
				vkGetDeviceQueue(device.handle, device.queues[i].queueFamilyIndex, device.queues[i].queueIndex + j, &device.queues[i].handles[j]);
			}
		}
	}

	// Finding Surface properties
	{
		surface.colorFormat = VkU::GetVkSurfaceFormatKHR(physicalDevices[device.physicalDeviceIndex].handle, surface, nullptr);// = VkA::GetSurfaceFormat(_instance.physicalDevices[device.physicalDeviceIndex], device.surfaces[i], nullptr);

		std::vector<VkCompositeAlphaFlagBitsKHR> preferedCompositeAlphas = { VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR };
		surface.compositeAlpha = VkU::GetVkCompositeAlphaFlagBitsKHR(VkU::GetVkSurfaceCapabilitiesKHR(physicalDevices[device.physicalDeviceIndex].handle, surface.handle), &preferedCompositeAlphas);

		std::vector<VkPresentModeKHR> preferedPresentModes = { VK_PRESENT_MODE_MAILBOX_KHR };
		surface.presentMode = VkU::GetVkPresentModeKHR(physicalDevices[device.physicalDeviceIndex].handle, surface.handle, &preferedPresentModes);
	}

	/// CommandPool creation
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo = VkU::GetVkCommandPoolCreateInfo(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].queueFamilyIndex);

		VK_CHECK_RESULT("	", vkCreateCommandPool(device.handle, &commandPoolCreateInfo, nullptr, &commandPool), " - ", commandPool, " - vkCreateCommandPool");
	}

	// Setup command buffer
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = VkU::GetVkCommandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

		VK_CHECK_RESULT("	", vkAllocateCommandBuffers(device.handle, &commandBufferAllocateInfo, &setupCommandBuffer), " - ", setupCommandBuffer, " - vkAllocateCommandBuffers");
	}

	// Setup fence
	{
		VkFenceCreateInfo fenceCreateInfo;
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.pNext = nullptr;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VK_CHECK_RESULT("	", vkCreateFence(device.handle, &fenceCreateInfo, nullptr, &setupFence), " - ", setupFence, " - vkCreateFence");
	}

	/// RenderPass creation
	{
		VkAttachmentDescription colorAttachmentDescription;
		colorAttachmentDescription.flags = 0;
		colorAttachmentDescription.format = surface.colorFormat.format;
		colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachmentDescription;
		depthAttachmentDescription.flags = 0;
		depthAttachmentDescription.format = physicalDevices[device.physicalDeviceIndex].depthFormat;
		depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentReference;
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentReference;
		depthAttachmentReference.attachment = 1;
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription renderSubpassDescription;
		renderSubpassDescription.flags = VK_RESERVED_FOR_FUTURE_USE;
		renderSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		renderSubpassDescription.inputAttachmentCount = 0;
		renderSubpassDescription.pInputAttachments = nullptr;
		renderSubpassDescription.colorAttachmentCount = 1;
		renderSubpassDescription.pColorAttachments = &colorAttachmentReference;
		renderSubpassDescription.pResolveAttachments = nullptr;
		if (_rendererInitProperties.useDepthBuffer == true)
			renderSubpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
		else
			renderSubpassDescription.pDepthStencilAttachment = nullptr;
		renderSubpassDescription.preserveAttachmentCount = 0;
		renderSubpassDescription.pPreserveAttachments = nullptr;

		VkSubpassDependency renderSubpassDependency;
		renderSubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		renderSubpassDependency.dstSubpass = 0;
		renderSubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		renderSubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		renderSubpassDependency.srcAccessMask = 0;
		renderSubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		renderSubpassDependency.dependencyFlags = 0;

		std::vector<VkAttachmentDescription> attachments = { colorAttachmentDescription };
		if (_rendererInitProperties.useDepthBuffer == true)
			attachments.push_back(depthAttachmentDescription);

		VkSubpassDescription subpasses[] = { renderSubpassDescription };
		VkSubpassDependency subpassDependencies[] = { renderSubpassDependency };

		VkRenderPassCreateInfo renderPassCreateInfo;
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.pNext = nullptr;
		renderPassCreateInfo.flags = VK_RESERVED_FOR_FUTURE_USE;
		renderPassCreateInfo.attachmentCount = (uint32_t)attachments.size();
		renderPassCreateInfo.pAttachments = attachments.data();
		renderPassCreateInfo.subpassCount = sizeof(subpasses) / sizeof(VkSubpassDescription);
		renderPassCreateInfo.pSubpasses = subpasses;
		renderPassCreateInfo.dependencyCount = sizeof(subpassDependencies) / sizeof(VkSubpassDependency);
		renderPassCreateInfo.pDependencies = subpassDependencies;

		VK_CHECK_RESULT("	", vkCreateRenderPass(device.handle, &renderPassCreateInfo, nullptr, &renderPass), " - ", renderPass, " - vkCreateRenderPass");
	}

	/// Swapchain creation
	{
		swapchain.extent.width = _rendererInitProperties.windowWidth;
		swapchain.extent.height = _rendererInitProperties.windowHeight;

		VkSwapchainCreateInfoKHR swapchainCreateInfoKHR = VkU::GetVkSwapchainCreateInfoKHR(physicalDevices[device.physicalDeviceIndex].handle, surface, &_rendererInitProperties.targetSwapchainImageCount, &swapchain.extent);

		VK_CHECK_RESULT("	", vkCreateSwapchainKHR(device.handle, &swapchainCreateInfoKHR, nullptr, &swapchain.handle), " - ", swapchain.handle, " - vkCreateSwapchainKHR");

		// image
		uint32_t propertyCount = 0;
		VK_CHECK_RESULT("	", vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &propertyCount, nullptr), " - ", 0, " - vkGetSwapchainImagesKHR");
		swapchain.images.resize(propertyCount);
		VK_CHECK_RESULT("	", vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &propertyCount, swapchain.images.data()), " - ", 0, " - vkGetSwapchainImagesKHR");

		// view
		VkImageViewCreateInfo imageViewCreateInfo = VkU::GetVkImageViewCreateInfo(surface.colorFormat.format, VK_NULL_HANDLE, VK_IMAGE_ASPECT_COLOR_BIT);

		swapchain.views.resize(swapchain.images.size());
		for (size_t i = 0; i != swapchain.views.size(); ++i)
		{
			imageViewCreateInfo.image = swapchain.images[i];
			VK_CHECK_RESULT("	", vkCreateImageView(device.handle, &imageViewCreateInfo, nullptr, &swapchain.views[i]), " - ", swapchain.views[i], " - vkCreateImageView");
		}

		// Depth Image
		{
			depthImage.handle = VK_NULL_HANDLE;
			depthImage.memory = VK_NULL_HANDLE;
			depthImage.view = VK_NULL_HANDLE;

			if (_rendererInitProperties.useDepthBuffer)
			{
				// create
				{
					VkImageCreateInfo imageCreateInfo = VkU::GetVkImageCreateInfo(physicalDevices[device.physicalDeviceIndex].depthFormat, { swapchain.extent.width, swapchain.extent.height, 1 }, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL);
					VK_CHECK_RESULT("	", vkCreateImage(device.handle, &imageCreateInfo, nullptr, &depthImage.handle), " - ", depthImage.handle, " - vkCreateImage");

					VkMemoryRequirements memoryRequirements;
					vkGetImageMemoryRequirements(device.handle, depthImage.handle, &memoryRequirements);

					VkMemoryAllocateInfo memoryAllocateInfo = VkU::GetVkMemoryAllocateInfo(memoryRequirements, physicalDevices[device.physicalDeviceIndex], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
					VK_CHECK_RESULT("	", vkAllocateMemory(device.handle, &memoryAllocateInfo, nullptr, &depthImage.memory), " - ", depthImage.memory, " - vkAllocateMemory");

					VK_CHECK_RESULT("	", vkBindImageMemory(device.handle, depthImage.handle, depthImage.memory, 0), " - ", 0, " - vkBindImageMemory");

					VkImageViewCreateInfo imageViewCreateInfo = VkU::GetVkImageViewCreateInfo(physicalDevices[device.physicalDeviceIndex].depthFormat, depthImage.handle, VK_IMAGE_ASPECT_DEPTH_BIT);
					VK_CHECK_RESULT("	", vkCreateImageView(device.handle, &imageViewCreateInfo, nullptr, &depthImage.view), " - ", depthImage.view, " - vkCreateImageView");
				}

				// transfer layout
				{
					VkCommandBufferBeginInfo commandBufferBeginInfo = VkU::GetVkCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
					VK_CHECK_RESULT("	", vkWaitForFences(device.handle, 1, &setupFence, VK_TRUE, -1), " - ", 0, " - vkWaitForFences");
					VK_CHECK_RESULT("	", vkResetFences(device.handle, 1, &setupFence), " - ", 0, " - vkResetFences");
					VK_CHECK_RESULT("	", vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo), " - ", 0, " - vkBeginCommandBuffer");

					VkImageMemoryBarrier imageMemoryBarrier = VkU::GetVkImageMemoryBarrier(0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, depthImage.handle);
					vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

					VK_CHECK_RESULT("	", vkEndCommandBuffer(setupCommandBuffer), " - ", 0, " - vkEndCommandBuffer");

					VkSubmitInfo submitInfo = VkU::GetVkSubmitInfo(0, nullptr, nullptr, 1, &setupCommandBuffer, 0, nullptr);

					VK_CHECK_RESULT("	", vkQueueSubmit(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0], 1, &submitInfo, setupFence), " - ", 0, " - vkQueueSubmit");
				}
			}
		}

		// framebuffer
		{
			VkFramebufferCreateInfo framebufferCreateInfo = VkU::GetVkFramebufferCreateInfo(renderPass, 0, nullptr, swapchain.extent.width, swapchain.extent.height);

			swapchain.framebuffers.resize(swapchain.images.size());
			for (size_t i = 0; i != swapchain.framebuffers.size(); ++i)
			{
				std::vector<VkImageView> attachments;
				attachments.push_back(swapchain.views[i]);
				if (_rendererInitProperties.useDepthBuffer)
					attachments.push_back(depthImage.view);

				framebufferCreateInfo.attachmentCount = (uint32_t)attachments.size();
				framebufferCreateInfo.pAttachments = attachments.data();

				VK_CHECK_RESULT("	", vkCreateFramebuffer(device.handle, &framebufferCreateInfo, nullptr, &swapchain.framebuffers[i]), " - ", swapchain.framebuffers[i], " - vkCreateFramebuffer");
			}
		}
	}

	// Semaphore creation
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo;
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = nullptr;
		semaphoreCreateInfo.flags = VK_RESERVED_FOR_FUTURE_USE;

		VK_CHECK_RESULT("	", vkCreateSemaphore(device.handle, &semaphoreCreateInfo, nullptr, &semaphoreImageAvailable), " - ", semaphoreImageAvailable, " - vkCreateSemaphore");
		VK_CHECK_RESULT("	", vkCreateSemaphore(device.handle, &semaphoreCreateInfo, nullptr, &semaphoreRenderDone), " - ", semaphoreRenderDone, " - vkCreateSemaphore");
	}
}
void Renderer::LoadScene(DataPackProperties _dataPackProperties)
{
	// Load ShaderModules
	{
		shaderModules.resize(_dataPackProperties.shaderModulesNames.size());
		for (size_t i = 0; i != _dataPackProperties.shaderModulesNames.size(); ++i)
		{
			size_t fileSize = 0;
			char* buffer = nullptr;

			Loader::LoadShader(_dataPackProperties.shaderModulesNames[i].filename, fileSize, &buffer);

			VkShaderModuleCreateInfo shaderModuleCreateInfo = VkU::GetVkShaderModuleCreateInfo(fileSize, (uint32_t*)buffer);

			VK_CHECK_RESULT("	", vkCreateShaderModule(device.handle, &shaderModuleCreateInfo, nullptr, &shaderModules[i].handle), " - ", shaderModules[i].handle, " - vkCreateShaderModule");

			delete[] buffer;

			shaderModules[i].stage = _dataPackProperties.shaderModulesNames[i].stage;
			shaderModules[i].entryPointName = _dataPackProperties.shaderModulesNames[i].entryPointName;
		}
	}

	// Load Textures
	{
		textures.resize(_dataPackProperties.textureNames.size());
		for (size_t i = 0; i != _dataPackProperties.textureNames.size(); ++i)
		{
			// loading
			uint16_t width, height;
			uint32_t size;
			uint8_t* data;
			VkFormat imageFormat;
			{
				Loader::IMAGE_TYPE type;
				Loader::LoadTGA(_dataPackProperties.textureNames[i], width, height, size, type, data);

				if (type == Loader::IMAGE_TYPE::BGR8)
					imageFormat = VK_FORMAT_B8G8R8_UNORM;
				else if (type == Loader::IMAGE_TYPE::BGRA8)
					imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
			}

			// buffer/memory/view
			{
				VkImageCreateInfo imageCreateInfo = VkU::GetVkImageCreateInfo(imageFormat, { width, height, 1 }, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL);
				VK_CHECK_RESULT("	", vkCreateImage(device.handle, &imageCreateInfo, nullptr, &textures[i].handle), " - ", textures[i].handle, " - vkCreateImage");

				VkMemoryRequirements memoryRequirements;
				vkGetImageMemoryRequirements(device.handle, textures[i].handle, &memoryRequirements);

				VkMemoryAllocateInfo memoryAllocateInfo = VkU::GetVkMemoryAllocateInfo(memoryRequirements, physicalDevices[device.physicalDeviceIndex], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				VK_CHECK_RESULT("	", vkAllocateMemory(device.handle, &memoryAllocateInfo, nullptr, &textures[i].memory), " - ", textures[i].memory, " - vkAllocateMemory");

				VK_CHECK_RESULT("	", vkBindImageMemory(device.handle, textures[i].handle, textures[i].memory, 0), " - ", 0, " - vkBindImageMemory");

				VkImageViewCreateInfo imageViewCreateInfo = VkU::GetVkImageViewCreateInfo(imageFormat, textures[i].handle, VK_IMAGE_ASPECT_COLOR_BIT);
				VK_CHECK_RESULT("	", vkCreateImageView(device.handle, &imageViewCreateInfo, nullptr, &textures[i].view), " - ", textures[i].view, " - vkCreateImageView");
			}

			// staging
			VkU::Image stagingImage;
			{
				VkImageCreateInfo imageCreateInfo = VkU::GetVkImageCreateInfo(imageFormat, { width, height, 1 }, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_TILING_LINEAR);
				VK_CHECK_RESULT("	", vkCreateImage(device.handle, &imageCreateInfo, nullptr, &stagingImage.handle), " - ", stagingImage.handle, " - vkCreateImage");

				VkMemoryRequirements memoryRequirements;
				vkGetImageMemoryRequirements(device.handle, stagingImage.handle, &memoryRequirements);

				VkMemoryAllocateInfo memoryAllocateInfo = VkU::GetVkMemoryAllocateInfo(memoryRequirements, physicalDevices[device.physicalDeviceIndex], VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
				VK_CHECK_RESULT("	", vkAllocateMemory(device.handle, &memoryAllocateInfo, nullptr, &stagingImage.memory), " - ", stagingImage.memory, " - vkAllocateMemory");

				VK_CHECK_RESULT("	", vkBindImageMemory(device.handle, stagingImage.handle, stagingImage.memory, 0), " - ", 0, " - vkBindImageMemory");

				VkImageSubresource stagingImageSubresource;
				stagingImageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				stagingImageSubresource.mipLevel = 0;
				stagingImageSubresource.arrayLayer = 0;

				VkSubresourceLayout stagingSubresourceLayout;
				vkGetImageSubresourceLayout(device.handle, stagingImage.handle, &stagingImageSubresource, &stagingSubresourceLayout);

				void* vkData;
				VK_CHECK_RESULT("	", vkMapMemory(device.handle, stagingImage.memory, 0, size, 0, &vkData), " - ", vkData, " - vkMapMemory");

				if (stagingSubresourceLayout.rowPitch == width * 4)
				{
					memcpy(vkData, data, size);
				}
				else
				{
					uint8_t* _data8b = (uint8_t*)data;
					uint8_t* data8b = (uint8_t*)vkData;

					for (uint32_t y = 0; y < height; y++)
					{
						memcpy(&data8b[y * stagingSubresourceLayout.rowPitch], &_data8b[y * width * 4], width * 4);
					}
				}

				vkUnmapMemory(device.handle, stagingImage.memory);
			}

			// transfer
			{
				VkCommandBufferBeginInfo commandBufferBeginInfo = VkU::GetVkCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
				VK_CHECK_RESULT("	", vkWaitForFences(device.handle, 1, &setupFence, VK_TRUE, -1), " - ", 0, " - vkWaitForFences");
				VK_CHECK_RESULT("	", vkResetFences(device.handle, 1, &setupFence), " - ", 0, " - vkResetFences");
				VK_CHECK_RESULT("	", vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo), " - ", 0, " - vkBeginCommandBuffer");

				// transfer texture to destination
				VkImageMemoryBarrier imageMemoryBarrier = VkU::GetVkImageMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, textures[i].handle);
				vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

				// transfer staging to source
				VkImageMemoryBarrier stagingImageMemoryBarrier = VkU::GetVkImageMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, stagingImage.handle);
				vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &stagingImageMemoryBarrier);

				// copy data
				VkImageSubresourceLayers imageSubresourceLayers;
				imageSubresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageSubresourceLayers.mipLevel = 0;
				imageSubresourceLayers.baseArrayLayer = 0;
				imageSubresourceLayers.layerCount = 1;

				VkImageCopy imageCopy;
				imageCopy.srcSubresource = imageSubresourceLayers;
				imageCopy.srcOffset = { 0, 0, 0 };
				imageCopy.dstSubresource = imageSubresourceLayers;
				imageCopy.dstOffset = { 0, 0, 0 };
				imageCopy.extent.width = width;
				imageCopy.extent.height = height;
				imageCopy.extent.depth = 1;

				vkCmdCopyImage(setupCommandBuffer, stagingImage.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, textures[i].handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

				// transfer texture to shader read layout
				VkImageMemoryBarrier finalMemoryBarrier = VkU::GetVkImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, textures[i].handle);
				vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &finalMemoryBarrier);

				VK_CHECK_RESULT("	", vkEndCommandBuffer(setupCommandBuffer), " - ", 0, " - vkEndCommandBuffer");

				VkSubmitInfo submitInfo = VkU::GetVkSubmitInfo(0, nullptr, nullptr, 1, &setupCommandBuffer, 0, nullptr);

				VK_CHECK_RESULT("	", vkQueueSubmit(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0], 1, &submitInfo, VK_NULL_HANDLE), " - ", 0, " - vkQueueSubmit");
				VK_CHECK_RESULT("	", vkQueueWaitIdle(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0]), " - ", 0, " - vkQueueWaitIdle");

				vkDestroyImage(device.handle, stagingImage.handle, nullptr);
				vkFreeMemory(device.handle, stagingImage.memory, nullptr);
			}

			delete[] data;
		}
	}

	// Load Models
	{
		for (size_t i = 0; i != _dataPackProperties.modelNames.size(); ++i)
		{
			Loader::Mesh mesh;
			Loader::LoadModelASSIMP(&mesh, nullptr, _dataPackProperties.modelNames[i], aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);

			// vertexBuffer
			{
				// buffer
				VkU::VertexBuffer vertexBuffer;
				{
					VkBufferCreateInfo bufferCreateInfo = VkU::GetVkBufferCreateInfo(mesh.vertexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
					VK_CHECK_RESULT("	", vkCreateBuffer(device.handle, &bufferCreateInfo, nullptr, &vertexBuffer.handle), " - ", vertexBuffer.handle, " - vkCreateBuffer");

					VkMemoryRequirements memoryRequirements;
					vkGetBufferMemoryRequirements(device.handle, vertexBuffer.handle, &memoryRequirements);

					VkMemoryAllocateInfo memoryAllocateInfo = VkU::GetVkMemoryAllocateInfo(memoryRequirements, physicalDevices[device.physicalDeviceIndex], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
					VK_CHECK_RESULT("	", vkAllocateMemory(device.handle, &memoryAllocateInfo, nullptr, &vertexBuffer.memory), " - ", vertexBuffer.memory, " - vkAllocateMemory");

					VK_CHECK_RESULT("	", vkBindBufferMemory(device.handle, vertexBuffer.handle, vertexBuffer.memory, 0), " - ", 0, " - vkBindBufferMemory");
				}

				// staging
				VkU::VertexBuffer stagingBuffer;
				{
					VkBufferCreateInfo stagingBufferCreateInfo = VkU::GetVkBufferCreateInfo(mesh.vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
					VK_CHECK_RESULT("	", vkCreateBuffer(device.handle, &stagingBufferCreateInfo, nullptr, &stagingBuffer.handle), " - ", stagingBuffer.handle, " - vkCreateBuffer");

					VkMemoryRequirements stagingMemoryRequirements;
					vkGetBufferMemoryRequirements(device.handle, stagingBuffer.handle, &stagingMemoryRequirements);

					VkMemoryAllocateInfo stagingMemoryAllocateInfo = VkU::GetVkMemoryAllocateInfo(stagingMemoryRequirements, physicalDevices[device.physicalDeviceIndex], VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
					VK_CHECK_RESULT("	", vkAllocateMemory(device.handle, &stagingMemoryAllocateInfo, nullptr, &stagingBuffer.memory), " - ", stagingBuffer.memory, " - vkAllocateMemory");

					VK_CHECK_RESULT("	", vkBindBufferMemory(device.handle, stagingBuffer.handle, stagingBuffer.memory, 0), " - ", 0, " - vkBindBufferMemory");

					void* data;
					VK_CHECK_RESULT("	", vkMapMemory(device.handle, stagingBuffer.memory, 0, mesh.vertexSize, 0, &data), " - ", data, " - vkMapMemory");
					memcpy(data, mesh.vertexData, mesh.vertexSize);
					vkUnmapMemory(device.handle, stagingBuffer.memory);
				}

				// transfer
				{
					VkCommandBufferBeginInfo commandBufferBeginInfo = VkU::GetVkCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

					VK_CHECK_RESULT("	", vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo), " - ", 0, " - vkBeginCommandBuffer");

					VkBufferCopy copyRegion;
					copyRegion.srcOffset = 0;
					copyRegion.dstOffset = 0;
					copyRegion.size = mesh.vertexSize;

					vkCmdCopyBuffer(setupCommandBuffer, stagingBuffer.handle, vertexBuffer.handle, 1, &copyRegion);

					VK_CHECK_RESULT("	", vkEndCommandBuffer(setupCommandBuffer), " - ", 0, " - vkMapMemory");

					VkSubmitInfo submitInfo = VkU::GetVkSubmitInfo(0, nullptr, nullptr, 1, &setupCommandBuffer, 0, nullptr);

					VK_CHECK_RESULT("	", vkQueueSubmit(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0], 1, &submitInfo, VK_NULL_HANDLE), " - ", 0, " - vkMapMemory");
					VK_CHECK_RESULT("	", vkQueueWaitIdle(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0]), " - ", 0, " - vkMapMemory");

					vkDestroyBuffer(device.handle, stagingBuffer.handle, nullptr);
					vkFreeMemory(device.handle, stagingBuffer.memory, nullptr);

				}

				vertexBuffers.push_back(vertexBuffer);
			}

			// indexBuffer
			{
				// buffer
				VkU::IndexBuffer indexBuffer;
				{
					VkBufferCreateInfo bufferCreateInfo = VkU::GetVkBufferCreateInfo(mesh.indexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
					VK_CHECK_RESULT("	", vkCreateBuffer(device.handle, &bufferCreateInfo, nullptr, &indexBuffer.handle), " - ", indexBuffer.handle, " - vkCreateBuffer");

					VkMemoryRequirements memoryRequirements;
					vkGetBufferMemoryRequirements(device.handle, indexBuffer.handle, &memoryRequirements);

					VkMemoryAllocateInfo memoryAllocateInfo = VkU::GetVkMemoryAllocateInfo(memoryRequirements, physicalDevices[device.physicalDeviceIndex], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
					VK_CHECK_RESULT("	", vkAllocateMemory(device.handle, &memoryAllocateInfo, nullptr, &indexBuffer.memory), " - ", indexBuffer.memory, " - vkAllocateMemory");

					VK_CHECK_RESULT("	", vkBindBufferMemory(device.handle, indexBuffer.handle, indexBuffer.memory, 0), " - ", 0, " - vkBindBufferMemory");
				}

				// staging
				VkU::IndexBuffer stagingBuffer;
				{
					VkBufferCreateInfo stagingBufferCreateInfo = VkU::GetVkBufferCreateInfo(mesh.indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
					VK_CHECK_RESULT("	", vkCreateBuffer(device.handle, &stagingBufferCreateInfo, nullptr, &stagingBuffer.handle), " - ", stagingBuffer.handle, " - vkCreateBuffer");

					VkMemoryRequirements stagingMemoryRequirements;
					vkGetBufferMemoryRequirements(device.handle, stagingBuffer.handle, &stagingMemoryRequirements);

					VkMemoryAllocateInfo stagingMemoryAllocateInfo = VkU::GetVkMemoryAllocateInfo(stagingMemoryRequirements, physicalDevices[device.physicalDeviceIndex], VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
					VK_CHECK_RESULT("	", vkAllocateMemory(device.handle, &stagingMemoryAllocateInfo, nullptr, &stagingBuffer.memory), " - ", stagingBuffer.memory, " - vkAllocateMemory");

					VK_CHECK_RESULT("	", vkBindBufferMemory(device.handle, stagingBuffer.handle, stagingBuffer.memory, 0), " - ", 0, " - vkBindBufferMemory");

					void* data;
					VK_CHECK_RESULT("	", vkMapMemory(device.handle, stagingBuffer.memory, 0, mesh.indexSize, 0, &data), " - ", data, " - vkMapMemory");
					memcpy(data, mesh.indexData, mesh.indexSize);
					vkUnmapMemory(device.handle, stagingBuffer.memory);
				}

				// transfer
				{
					VkCommandBufferBeginInfo commandBufferBeginInfo = VkU::GetVkCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

					VK_CHECK_RESULT("	", vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo), " - ", 0, " - vkBeginCommandBuffer");

					VkBufferCopy copyRegion;
					copyRegion.srcOffset = 0;
					copyRegion.dstOffset = 0;
					copyRegion.size = mesh.indexSize;

					vkCmdCopyBuffer(setupCommandBuffer, stagingBuffer.handle, indexBuffer.handle, 1, &copyRegion);

					VK_CHECK_RESULT("	", vkEndCommandBuffer(setupCommandBuffer), " - ", 0, " - vkMapMemory");

					VkSubmitInfo submitInfo = VkU::GetVkSubmitInfo(0, nullptr, nullptr, 1, &setupCommandBuffer, 0, nullptr);

					VK_CHECK_RESULT("	", vkQueueSubmit(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0], 1, &submitInfo, VK_NULL_HANDLE), " - ", 0, " - vkQueueSubmit");
					VK_CHECK_RESULT("	", vkQueueWaitIdle(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0]), " - ", 0, " - vkQueueWaitIdle");

					vkDestroyBuffer(device.handle, stagingBuffer.handle, nullptr);
					vkFreeMemory(device.handle, stagingBuffer.memory, nullptr);
				}

				// count
				indexBuffer.count = mesh.indexCount;

				indexBuffer.type = VK_INDEX_TYPE_UINT16;

				indexBuffers.push_back(indexBuffer);
			}

			delete[] mesh.vertexData;
			delete[] mesh.indexData;
		}
	};

	// Load Uniform Buffers
	{
		/// Camera buffer
		VkU::UniformBuffer cameraBuffer;
		{
			VkBufferCreateInfo bufferCreateInfo = VkU::GetVkBufferCreateInfo(sizeof(camera), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
			VK_CHECK_RESULT("	", vkCreateBuffer(device.handle, &bufferCreateInfo, nullptr, &cameraBuffer.handle), " - ", cameraBuffer.handle, " - vkCreateBuffer");

			VkMemoryRequirements memoryRequirements;
			vkGetBufferMemoryRequirements(device.handle, cameraBuffer.handle, &memoryRequirements);

			VkMemoryAllocateInfo memoryAllocateInfo = VkU::GetVkMemoryAllocateInfo(memoryRequirements, physicalDevices[device.physicalDeviceIndex], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT("	", vkAllocateMemory(device.handle, &memoryAllocateInfo, nullptr, &cameraBuffer.memory), " - ", cameraBuffer.memory, " - vkAllocateMemory");

			VK_CHECK_RESULT("	", vkBindBufferMemory(device.handle, cameraBuffer.handle, cameraBuffer.memory, 0), " - ", 0, " - vkBindBufferMemory");

			uniformBuffers.push_back(cameraBuffer);
		}

		/// Model Matrices buffer
		VkU::UniformBuffer modelBuffer;
		{
			modelMatrices.resize(_dataPackProperties.maxModelMatrixCount);
			modelMatrices[0] = glm::translate(glm::mat4(), glm::vec3(MODEL_POSITION_X, MODEL_POSITION_Y, MODEL_POSITION_Z));
			modelMatrices[1] = glm::mat4(1);
			modelMatrices[2] = glm::scale(modelMatrices[1], glm::vec3(10, 10, 10));

			VkBufferCreateInfo bufferCreateInfo = VkU::GetVkBufferCreateInfo(modelMatrices.size() * sizeof(glm::mat4), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
			VK_CHECK_RESULT("	", vkCreateBuffer(device.handle, &bufferCreateInfo, nullptr, &modelBuffer.handle), " - ", modelBuffer.handle, " - vkCreateBuffer");

			VkMemoryRequirements memoryRequirements;
			vkGetBufferMemoryRequirements(device.handle, modelBuffer.handle, &memoryRequirements);

			VkMemoryAllocateInfo memoryAllocateInfo = VkU::GetVkMemoryAllocateInfo(memoryRequirements, physicalDevices[device.physicalDeviceIndex], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT("	", vkAllocateMemory(device.handle, &memoryAllocateInfo, nullptr, &modelBuffer.memory), " - ", modelBuffer.memory, " - vkAllocateMemory");

			VK_CHECK_RESULT("	", vkBindBufferMemory(device.handle, modelBuffer.handle, modelBuffer.memory, 0), " - ", 0, " - vkBindBufferMemory");

			uniformBuffers.push_back(modelBuffer);
		}

		/// Skeleton Buffer
		VkU::UniformBuffer skeletonBuffer;
		{
			skeletons.resize(_dataPackProperties.maxAnimationCount);
			for (size_t i = 0; i != 64; ++i)
				skeletons[0].bones[i] = glm::mat4(1);

			VkBufferCreateInfo bufferCreateInfo = VkU::GetVkBufferCreateInfo(skeletons.size() * sizeof(skeleton), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
			VK_CHECK_RESULT("	", vkCreateBuffer(device.handle, &bufferCreateInfo, nullptr, &skeletonBuffer.handle), " - ", skeletonBuffer.handle, " - vkCreateBuffer");

			VkMemoryRequirements memoryRequirements;
			vkGetBufferMemoryRequirements(device.handle, skeletonBuffer.handle, &memoryRequirements);

			VkMemoryAllocateInfo memoryAllocateInfo = VkU::GetVkMemoryAllocateInfo(memoryRequirements, physicalDevices[device.physicalDeviceIndex], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT("	", vkAllocateMemory(device.handle, &memoryAllocateInfo, nullptr, &skeletonBuffer.memory), " - ", skeletonBuffer.memory, " - vkAllocateMemory");

			VK_CHECK_RESULT("	", vkBindBufferMemory(device.handle, skeletonBuffer.handle, skeletonBuffer.memory, 0), " - ", 0, " - vkBindBufferMemory");

			uniformBuffers.push_back(skeletonBuffer);
		}
	}
}
void Renderer::Setup(SetupProperties _setupProperties)
{
	/// DescriptorSetLayout creation
	{
		VkDescriptorSetLayoutBinding cameraDescriptorSetLayoutBinding;
		cameraDescriptorSetLayoutBinding.binding = CAMERA_UNIFORM_BINDING;
		cameraDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		cameraDescriptorSetLayoutBinding.descriptorCount = 1;
		cameraDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		cameraDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding modelMatricesDescriptorSetLayoutBinding;
		modelMatricesDescriptorSetLayoutBinding.binding = MODEL_UNIFORM_BINDING;
		modelMatricesDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		modelMatricesDescriptorSetLayoutBinding.descriptorCount = 1;
		modelMatricesDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		modelMatricesDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding skeletonMatricesDescriptorSetLayoutBinding;
		skeletonMatricesDescriptorSetLayoutBinding.binding = SKELETON_UNIFORM_BINDING;
		skeletonMatricesDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		skeletonMatricesDescriptorSetLayoutBinding.descriptorCount = 1;
		skeletonMatricesDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		skeletonMatricesDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding textureDescriptorSetLayoutBinding;
		textureDescriptorSetLayoutBinding.binding = TEXTURE_BINDING;
		textureDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureDescriptorSetLayoutBinding.descriptorCount = 1;
		textureDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		textureDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding[] = { cameraDescriptorSetLayoutBinding, modelMatricesDescriptorSetLayoutBinding, skeletonMatricesDescriptorSetLayoutBinding, textureDescriptorSetLayoutBinding };

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext = nullptr;
		descriptorSetLayoutCreateInfo.flags = VK_NULL_HANDLE;
		descriptorSetLayoutCreateInfo.bindingCount = sizeof(descriptorSetLayoutBinding) / sizeof(VkDescriptorSetLayoutBinding);
		descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBinding;

		VK_CHECK_RESULT("	", vkCreateDescriptorSetLayout(device.handle, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout), " - ", descriptorSetLayout, " - vkCreateDescriptorSetLayout");
	}

	/// pipelineLayout creation
	{
		VkPushConstantRange modelMatrixPushConstantRange;
		modelMatrixPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		modelMatrixPushConstantRange.offset = 0;
		modelMatrixPushConstantRange.size = sizeof(float) * 4;

		VkPushConstantRange pushConstantRanges[] = { modelMatrixPushConstantRange };

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext = nullptr;
		pipelineLayoutCreateInfo.flags = VK_RESERVED_FOR_FUTURE_USE;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutCreateInfo.pushConstantRangeCount = sizeof(pushConstantRanges) / sizeof(VkPushConstantRange);
		pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges;

		VK_CHECK_RESULT("	", vkCreatePipelineLayout(device.handle, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout), " - ", pipelineLayout, " - vkCreatePipelineLayout");
	}

	/// pipelines creation
	{
		// Shader State
		{
			pipelineShaderStagesCreateInfos.resize(_setupProperties.shaders.size());
			for (size_t i = 0; i != _setupProperties.shaders.size(); ++i)
			{
				pipelineShaderStagesCreateInfos[i].resize(_setupProperties.shaders[i].size());

				for (size_t j = 0; j != pipelineShaderStagesCreateInfos[i].size(); ++j)
				{
					size_t index = _setupProperties.shaders[i][j];

					pipelineShaderStagesCreateInfos[i][j].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					pipelineShaderStagesCreateInfos[i][j].pNext = nullptr;
					pipelineShaderStagesCreateInfos[i][j].flags = VK_RESERVED_FOR_FUTURE_USE;
					pipelineShaderStagesCreateInfos[i][j].stage = shaderModules[index].stage;
					pipelineShaderStagesCreateInfos[i][j].module = shaderModules[index].handle;
					pipelineShaderStagesCreateInfos[i][j].pName = shaderModules[index].entryPointName;
					pipelineShaderStagesCreateInfos[i][j].pSpecializationInfo = nullptr;
				}
			}
		}

		// Binding Description
		{
			vertexInputBindingDescriptions = {
				VkU::VertexP3UNTBS4::GetVkVertexInputBindingDescription(),
				VkU::VertexP3UNTB::GetVkVertexInputBindingDescription(),
			};
		}

		// Input Attribute Description
		{
			vertexInputAttributeDescriptions = {
				VkU::VertexP3UNTBS4::GetVkVertexInputAttributeDescription(),
				VkU::VertexP3UNTB::GetVkVertexInputAttributeDescription(),
			};
		}

		// Vertex Input State
		{
			pipelineVertexInputStateCreateInfos.resize(2);

			pipelineVertexInputStateCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			pipelineVertexInputStateCreateInfos[0].pNext = nullptr;
			pipelineVertexInputStateCreateInfos[0].flags = VK_RESERVED_FOR_FUTURE_USE;
			pipelineVertexInputStateCreateInfos[0].vertexBindingDescriptionCount = 1;
			pipelineVertexInputStateCreateInfos[0].pVertexBindingDescriptions = &vertexInputBindingDescriptions[0];
			pipelineVertexInputStateCreateInfos[0].vertexAttributeDescriptionCount = (uint32_t)vertexInputAttributeDescriptions[0].size();
			pipelineVertexInputStateCreateInfos[0].pVertexAttributeDescriptions = vertexInputAttributeDescriptions[0].data();

			pipelineVertexInputStateCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			pipelineVertexInputStateCreateInfos[1].pNext = nullptr;
			pipelineVertexInputStateCreateInfos[1].flags = VK_RESERVED_FOR_FUTURE_USE;
			pipelineVertexInputStateCreateInfos[1].vertexBindingDescriptionCount = 1;
			pipelineVertexInputStateCreateInfos[1].pVertexBindingDescriptions = &vertexInputBindingDescriptions[1];
			pipelineVertexInputStateCreateInfos[1].vertexAttributeDescriptionCount = (uint32_t)vertexInputAttributeDescriptions[1].size();
			pipelineVertexInputStateCreateInfos[1].pVertexAttributeDescriptions = vertexInputAttributeDescriptions[1].data();
		}

		// Input Assembly State
		{
			pipelineInputAssemblyStateCreateInfos.resize(1);
			pipelineInputAssemblyStateCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			pipelineInputAssemblyStateCreateInfos[0].pNext = nullptr;
			pipelineInputAssemblyStateCreateInfos[0].flags = VK_RESERVED_FOR_FUTURE_USE;
			pipelineInputAssemblyStateCreateInfos[0].topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			pipelineInputAssemblyStateCreateInfos[0].primitiveRestartEnable = VK_FALSE;
		}

		// Tessalation State
		{
			pipelineTessellationStateCreateInfos.resize(0);
		}

		// Viewport
		{
			viewports.resize(1);

			viewports[0].x = 0.0f;
			viewports[0].y = 0.0f;
			viewports[0].width = (float)swapchain.extent.width;
			viewports[0].height = (float)swapchain.extent.height;
			viewports[0].minDepth = 0.0f;
			viewports[0].maxDepth = 1.0f;
		}

		// Scissor
		{
			scissors.resize(1);

			scissors[0].offset = { 0, 0 };
			scissors[0].extent = swapchain.extent;
		}

		// Viewport State
		{
			pipelineViewportStateCreateInfos.resize(1);

			pipelineViewportStateCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			pipelineViewportStateCreateInfos[0].pNext = nullptr;
			pipelineViewportStateCreateInfos[0].flags = VK_RESERVED_FOR_FUTURE_USE;
			pipelineViewportStateCreateInfos[0].viewportCount = 1;
			pipelineViewportStateCreateInfos[0].pViewports = &viewports[0];
			pipelineViewportStateCreateInfos[0].scissorCount = 1;
			pipelineViewportStateCreateInfos[0].pScissors = &scissors[0];
		}

		// Rasterization State
		{
			pipelineRasterizationStateCreateInfos.resize(2);

			pipelineRasterizationStateCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			pipelineRasterizationStateCreateInfos[0].pNext = nullptr;
			pipelineRasterizationStateCreateInfos[0].flags = VK_RESERVED_FOR_FUTURE_USE;
			pipelineRasterizationStateCreateInfos[0].depthClampEnable = VK_FALSE;
			pipelineRasterizationStateCreateInfos[0].rasterizerDiscardEnable = VK_FALSE;
			pipelineRasterizationStateCreateInfos[0].polygonMode = VK_POLYGON_MODE_FILL;
			pipelineRasterizationStateCreateInfos[0].cullMode = VK_CULL_MODE_FRONT_BIT;
			pipelineRasterizationStateCreateInfos[0].frontFace = VK_FRONT_FACE_CLOCKWISE;
			pipelineRasterizationStateCreateInfos[0].depthBiasEnable = VK_FALSE;
			pipelineRasterizationStateCreateInfos[0].depthBiasConstantFactor = 0.0f;
			pipelineRasterizationStateCreateInfos[0].depthBiasClamp = 0.0f;
			pipelineRasterizationStateCreateInfos[0].depthBiasSlopeFactor = 0.0f;
			pipelineRasterizationStateCreateInfos[0].lineWidth = 1.0f;

			pipelineRasterizationStateCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			pipelineRasterizationStateCreateInfos[1].pNext = nullptr;
			pipelineRasterizationStateCreateInfos[1].flags = VK_RESERVED_FOR_FUTURE_USE;
			pipelineRasterizationStateCreateInfos[1].depthClampEnable = VK_FALSE;
			pipelineRasterizationStateCreateInfos[1].rasterizerDiscardEnable = VK_FALSE;
			pipelineRasterizationStateCreateInfos[1].polygonMode = VK_POLYGON_MODE_LINE;
			pipelineRasterizationStateCreateInfos[1].cullMode = VK_CULL_MODE_NONE;
			pipelineRasterizationStateCreateInfos[1].frontFace = VK_FRONT_FACE_CLOCKWISE;
			pipelineRasterizationStateCreateInfos[1].depthBiasEnable = VK_FALSE;
			pipelineRasterizationStateCreateInfos[1].depthBiasConstantFactor = 0.0f;
			pipelineRasterizationStateCreateInfos[1].depthBiasClamp = 0.0f;
			pipelineRasterizationStateCreateInfos[1].depthBiasSlopeFactor = 0.0f;
			pipelineRasterizationStateCreateInfos[1].lineWidth = 1.0f;
		}

		// Multisample State
		{
			pipelineMultisampleStateCreateInfos.resize(1);

			pipelineMultisampleStateCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			pipelineMultisampleStateCreateInfos[0].pNext = nullptr;
			pipelineMultisampleStateCreateInfos[0].flags = VK_RESERVED_FOR_FUTURE_USE;
			pipelineMultisampleStateCreateInfos[0].rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			pipelineMultisampleStateCreateInfos[0].sampleShadingEnable = VK_FALSE;
			pipelineMultisampleStateCreateInfos[0].minSampleShading = 0.0f;
			pipelineMultisampleStateCreateInfos[0].pSampleMask = nullptr;
			pipelineMultisampleStateCreateInfos[0].alphaToCoverageEnable = VK_FALSE;
			pipelineMultisampleStateCreateInfos[0].alphaToOneEnable = VK_FALSE;
		}

		// Depth Stencil State
		{
			pipelineDepthStencilStateCreateInfos.resize(1);

			pipelineDepthStencilStateCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			pipelineDepthStencilStateCreateInfos[0].pNext = nullptr;
			pipelineDepthStencilStateCreateInfos[0].flags = VK_RESERVED_FOR_FUTURE_USE;
			pipelineDepthStencilStateCreateInfos[0].depthTestEnable = VK_TRUE;
			pipelineDepthStencilStateCreateInfos[0].depthWriteEnable = VK_TRUE;
			pipelineDepthStencilStateCreateInfos[0].depthCompareOp = VK_COMPARE_OP_LESS;
			pipelineDepthStencilStateCreateInfos[0].depthBoundsTestEnable = VK_FALSE;
			pipelineDepthStencilStateCreateInfos[0].stencilTestEnable = VK_FALSE;
			pipelineDepthStencilStateCreateInfos[0].front = {};
			pipelineDepthStencilStateCreateInfos[0].back = {};
			pipelineDepthStencilStateCreateInfos[0].minDepthBounds = 0.0f;
			pipelineDepthStencilStateCreateInfos[0].maxDepthBounds = 1.0f;
		}

		// Color Blend State
		{
			pipelineColorBlendAttachmentState.resize(1);

			pipelineColorBlendAttachmentState[0].blendEnable = VK_FALSE;
			pipelineColorBlendAttachmentState[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			pipelineColorBlendAttachmentState[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			pipelineColorBlendAttachmentState[0].colorBlendOp = VK_BLEND_OP_ADD;
			pipelineColorBlendAttachmentState[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			pipelineColorBlendAttachmentState[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			pipelineColorBlendAttachmentState[0].alphaBlendOp = VK_BLEND_OP_ADD;
			pipelineColorBlendAttachmentState[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		}

		// Color Blend State
		{
			pipelineColorBlendStateCreateInfos.resize(1);

			pipelineColorBlendStateCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			pipelineColorBlendStateCreateInfos[0].pNext = nullptr;
			pipelineColorBlendStateCreateInfos[0].flags = VK_RESERVED_FOR_FUTURE_USE;
			pipelineColorBlendStateCreateInfos[0].logicOpEnable = VK_FALSE;
			pipelineColorBlendStateCreateInfos[0].logicOp = VK_LOGIC_OP_COPY;
			pipelineColorBlendStateCreateInfos[0].attachmentCount = 1;
			pipelineColorBlendStateCreateInfos[0].pAttachments = &pipelineColorBlendAttachmentState[0];
			pipelineColorBlendStateCreateInfos[0].blendConstants[0] = 0.0f;
			pipelineColorBlendStateCreateInfos[0].blendConstants[1] = 0.0f;
			pipelineColorBlendStateCreateInfos[0].blendConstants[2] = 0.0f;
			pipelineColorBlendStateCreateInfos[0].blendConstants[3] = 0.0f;
		}

		// Dynamic State
		{
			pipelineDynamicStateCreateInfos.resize(0);
		}

		// Pipeline Info
		graphicsPipelineCreateInfos.resize(_setupProperties.pipelines.size());
		for (size_t i = 0; i != graphicsPipelineCreateInfos.size(); ++i)
		{
			graphicsPipelineCreateInfos[i].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineCreateInfos[i].pNext = nullptr;
			graphicsPipelineCreateInfos[i].flags = 0;
			graphicsPipelineCreateInfos[i].stageCount = (uint32_t)pipelineShaderStagesCreateInfos[_setupProperties.pipelines[i].shaderStateIndex].size();
			graphicsPipelineCreateInfos[i].pStages = pipelineShaderStagesCreateInfos[_setupProperties.pipelines[i].shaderStateIndex].data();
			graphicsPipelineCreateInfos[i].pVertexInputState = &pipelineVertexInputStateCreateInfos[_setupProperties.pipelines[i].vertexInputStateIndex];
			graphicsPipelineCreateInfos[i].pInputAssemblyState = &pipelineInputAssemblyStateCreateInfos[_setupProperties.pipelines[i].inputAssemblyStateIndex];
			graphicsPipelineCreateInfos[i].pTessellationState = nullptr;
			graphicsPipelineCreateInfos[i].pViewportState = &pipelineViewportStateCreateInfos[_setupProperties.pipelines[i].viewportStateIndex];
			graphicsPipelineCreateInfos[i].pRasterizationState = &pipelineRasterizationStateCreateInfos[_setupProperties.pipelines[i].rasterizationStateIndex];
			graphicsPipelineCreateInfos[i].pMultisampleState = &pipelineMultisampleStateCreateInfos[_setupProperties.pipelines[i].multisampleStateIndex];
			graphicsPipelineCreateInfos[i].pDepthStencilState = &pipelineDepthStencilStateCreateInfos[_setupProperties.pipelines[i].depthStencilStateIndex];
			graphicsPipelineCreateInfos[i].pColorBlendState = &pipelineColorBlendStateCreateInfos[_setupProperties.pipelines[i].colorBlendStateIndex];
			graphicsPipelineCreateInfos[i].pDynamicState = nullptr;
			graphicsPipelineCreateInfos[i].layout = pipelineLayout;
			graphicsPipelineCreateInfos[i].renderPass = renderPass;
			graphicsPipelineCreateInfos[i].subpass = 0;
			graphicsPipelineCreateInfos[i].basePipelineHandle = VK_NULL_HANDLE;
			graphicsPipelineCreateInfos[i].basePipelineIndex = 0;
		}

		pipelines.resize(graphicsPipelineCreateInfos.size());
		VK_CHECK_RESULT("	", vkCreateGraphicsPipelines(device.handle, VK_NULL_HANDLE, (uint32_t)graphicsPipelineCreateInfos.size(), graphicsPipelineCreateInfos.data(), nullptr, pipelines.data()), " - ", 0, " - vkCreateGraphicsPipelines");
	};

	// Sampler
	{
		VkSamplerCreateInfo samplerCreateInfo;
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.pNext = nullptr;
		samplerCreateInfo.flags = VK_RESERVED_FOR_FUTURE_USE;
		samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
		samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = 16;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 0.0f;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

		VK_CHECK_RESULT("	", vkCreateSampler(device.handle, &samplerCreateInfo, nullptr, &sampler), " - ", sampler, " - vkCreateSampler");
	}

	// Descriptor Pool
	{
		VkDescriptorPoolSize cameraDescriptorPoolSize;
		cameraDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		cameraDescriptorPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize modelMatricesDescriptorPoolSize;
		modelMatricesDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		modelMatricesDescriptorPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize skeletonMatricesDescriptorPoolSize;
		skeletonMatricesDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		skeletonMatricesDescriptorPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize textureDescriptorPoolSize;
		textureDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureDescriptorPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize descriptorPoolSize[] = { cameraDescriptorPoolSize, modelMatricesDescriptorPoolSize, skeletonMatricesDescriptorPoolSize, textureDescriptorPoolSize };

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.pNext = nullptr;
		descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		descriptorPoolCreateInfo.maxSets = 1;
		descriptorPoolCreateInfo.poolSizeCount = sizeof(descriptorPoolSize) / sizeof(VkDescriptorPoolSize);
		descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize;

		VK_CHECK_RESULT("	", vkCreateDescriptorPool(device.handle, &descriptorPoolCreateInfo, nullptr, &descriptorPool), " - ", descriptorPool, " - vkCreateDescriptorPool");
	}

	// Descriptor Sets
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.pNext = nullptr;
		descriptorSetAllocateInfo.descriptorPool = descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

		VK_CHECK_RESULT("	", vkAllocateDescriptorSets(device.handle, &descriptorSetAllocateInfo, &descriptorSet), " - ", descriptorSet, " - vkAllocateDescriptorSets");

		VkDescriptorBufferInfo cameraDescriptorBufferInfo;
		cameraDescriptorBufferInfo.buffer = uniformBuffers[CAMERA_UNIFORM_BINDING].handle;
		cameraDescriptorBufferInfo.offset = 0;
		cameraDescriptorBufferInfo.range = sizeof(camera);

		VkWriteDescriptorSet cameraWriteDescriptorSet;
		cameraWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		cameraWriteDescriptorSet.pNext = nullptr;
		cameraWriteDescriptorSet.dstSet = descriptorSet;
		cameraWriteDescriptorSet.dstBinding = CAMERA_UNIFORM_BINDING;
		cameraWriteDescriptorSet.dstArrayElement = 0;
		cameraWriteDescriptorSet.descriptorCount = 1;
		cameraWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		cameraWriteDescriptorSet.pImageInfo = nullptr;
		cameraWriteDescriptorSet.pBufferInfo = &cameraDescriptorBufferInfo;
		cameraWriteDescriptorSet.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo modelMatricesDescriptorBufferInfo;
		modelMatricesDescriptorBufferInfo.buffer = uniformBuffers[MODEL_UNIFORM_BINDING].handle;
		modelMatricesDescriptorBufferInfo.offset = 0;
		modelMatricesDescriptorBufferInfo.range = sizeof(glm::mat4) * modelMatrices.size();

		VkWriteDescriptorSet modelMatricesWriteDescriptorSet;
		modelMatricesWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		modelMatricesWriteDescriptorSet.pNext = nullptr;
		modelMatricesWriteDescriptorSet.dstSet = descriptorSet;
		modelMatricesWriteDescriptorSet.dstBinding = MODEL_UNIFORM_BINDING;
		modelMatricesWriteDescriptorSet.dstArrayElement = 0;
		modelMatricesWriteDescriptorSet.descriptorCount = 1;
		modelMatricesWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		modelMatricesWriteDescriptorSet.pImageInfo = nullptr;
		modelMatricesWriteDescriptorSet.pBufferInfo = &modelMatricesDescriptorBufferInfo;
		modelMatricesWriteDescriptorSet.pTexelBufferView = nullptr;

		VkDescriptorImageInfo descriptorImageInfo;
		descriptorImageInfo.sampler = sampler;
		descriptorImageInfo.imageView = textures[TEXTURE_INDEX].view;
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet imageWriteDescriptorSet;
		imageWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		imageWriteDescriptorSet.pNext = nullptr;
		imageWriteDescriptorSet.dstSet = descriptorSet;
		imageWriteDescriptorSet.dstBinding = TEXTURE_BINDING;
		imageWriteDescriptorSet.dstArrayElement = 0;
		imageWriteDescriptorSet.descriptorCount = 1;
		imageWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageWriteDescriptorSet.pImageInfo = &descriptorImageInfo;
		imageWriteDescriptorSet.pBufferInfo = nullptr;
		imageWriteDescriptorSet.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo skeletonMatricesDescriptorBufferInfo;
		skeletonMatricesDescriptorBufferInfo.buffer = uniformBuffers[SKELETON_UNIFORM_BINDING].handle;
		skeletonMatricesDescriptorBufferInfo.offset = 0;
		skeletonMatricesDescriptorBufferInfo.range = sizeof(skeleton) * this->skeletons.size();

		VkWriteDescriptorSet skeletonMatricesWriteDescriptorSet;
		skeletonMatricesWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		skeletonMatricesWriteDescriptorSet.pNext = nullptr;
		skeletonMatricesWriteDescriptorSet.dstSet = descriptorSet;
		skeletonMatricesWriteDescriptorSet.dstBinding = SKELETON_UNIFORM_BINDING;
		skeletonMatricesWriteDescriptorSet.dstArrayElement = 0;
		skeletonMatricesWriteDescriptorSet.descriptorCount = 1;
		skeletonMatricesWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		skeletonMatricesWriteDescriptorSet.pImageInfo = nullptr;
		skeletonMatricesWriteDescriptorSet.pBufferInfo = &skeletonMatricesDescriptorBufferInfo;
		skeletonMatricesWriteDescriptorSet.pTexelBufferView = nullptr;

		VkWriteDescriptorSet writeDescriptorSet[] = { cameraWriteDescriptorSet, imageWriteDescriptorSet, modelMatricesWriteDescriptorSet, skeletonMatricesWriteDescriptorSet };

		vkUpdateDescriptorSets(device.handle, sizeof(writeDescriptorSet) / sizeof(VkWriteDescriptorSet), writeDescriptorSet, 0, nullptr);
	}

	// Render command buffer
	{
		renderCommandBuffers.resize(swapchain.framebuffers.size());
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = VkU::GetVkCommandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, (uint32_t)renderCommandBuffers.size());

		VK_CHECK_RESULT("	", vkAllocateCommandBuffers(device.handle, &commandBufferAllocateInfo, renderCommandBuffers.data()), " - ", 0, " - vkAllocateCommandBuffers");
	}

	// Render Fences
	{
		renderFences.resize(renderCommandBuffers.size());

		VkFenceCreateInfo fenceCreateInfo;
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.pNext = nullptr;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i != renderFences.size(); ++i)
		{
			VK_CHECK_RESULT("	", vkCreateFence(device.handle, &fenceCreateInfo, nullptr, &renderFences[i]), " - ", renderFences[i], " - vkCreateFence");
		}
	}
}

void GetAnimatedSkeleton(std::array<aiMatrix4x4, 64>& _bones, Scene::Skeleton _assimpAnimation, float AnimationTime)
{
	std::array<aiMatrix4x4, 64> bones;

	for (size_t i = 0; i != _assimpAnimation.joints.size(); ++i)
	{
		aiMatrix4x4 transformation = _assimpAnimation.joints[i].transformation;

		if (_assimpAnimation.joints[i].translateChannels.size() > 0 && _assimpAnimation.joints[i].rotateChannels.size() > 0 && _assimpAnimation.joints[i].scaleChannels.size() > 0)
		{
			// translation
			aiMatrix4x4 translationMat;// = pNode->mTransformation;
			{
				aiVector3D translation;

				if (_assimpAnimation.joints[i].translateChannels.size() == 1)
					translation = _assimpAnimation.joints[i].translateChannels[0].channel;
				else
				{
					size_t frameIndex = 0;
					for (size_t j = 0; j != _assimpAnimation.joints[i].translateChannels.size() - 1; ++j)
					{
						if (AnimationTime < (float)_assimpAnimation.joints[i].translateChannels[j + 1].time)
						{
							frameIndex = j;
							break;
						}
					}

					aiVectorKey currentFrame;
					currentFrame.mValue = _assimpAnimation.joints[i].translateChannels[frameIndex].channel;
					currentFrame.mTime = _assimpAnimation.joints[i].translateChannels[frameIndex].time;

					aiVectorKey nextFrame;
					nextFrame.mValue = _assimpAnimation.joints[i].translateChannels[(frameIndex + 1) % _assimpAnimation.joints[i].translateChannels.size()].channel;
					nextFrame.mTime = _assimpAnimation.joints[i].translateChannels[(frameIndex + 1) % _assimpAnimation.joints[i].translateChannels.size()].time;

					float delta = (AnimationTime - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

					const aiVector3D& start = currentFrame.mValue;
					const aiVector3D& end = nextFrame.mValue;

					translation = (start + delta * (end - start));
				}

				aiMatrix4x4::Translation(translation, translationMat);
			}

			// rotation
			aiMatrix4x4 rotationMat;
			if (i == 5)
			{
				static float a = 0.0f;
				a += 0.001f;
				float b = sinf(a) / 3.0f + 1.4f;

				rotationMat.RotationY(b, rotationMat);
			}
			else if (i == 6)
			{
				static float b = 0.0f;
				b += 0.001f;
				float c = sinf(b) * 1.5f + 0.5f;

				rotationMat.RotationY(c, rotationMat);
			}
			else
			{
				aiQuaternion rotation;

				if (_assimpAnimation.joints[i].rotateChannels.size() == 1)
					rotation = _assimpAnimation.joints[i].rotateChannels[0].channel;
				else
				{
					size_t frameIndex = 0;
					for (size_t j = 0; j != _assimpAnimation.joints[i].rotateChannels.size() - 1; ++j)
					{
						if (AnimationTime < (float)_assimpAnimation.joints[i].rotateChannels[j + 1].time)
						{
							frameIndex = j;
							break;
						}
					}

					aiQuatKey currentFrame;
					currentFrame.mValue = _assimpAnimation.joints[i].rotateChannels[frameIndex].channel;
					currentFrame.mTime = _assimpAnimation.joints[i].rotateChannels[frameIndex].time;

					aiQuatKey nextFrame;
					nextFrame.mValue = _assimpAnimation.joints[i].rotateChannels[(frameIndex + 1) % _assimpAnimation.joints[i].rotateChannels.size()].channel;
					nextFrame.mTime = _assimpAnimation.joints[i].rotateChannels[(frameIndex + 1) % _assimpAnimation.joints[i].rotateChannels.size()].time;

					float delta = (AnimationTime - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

					const aiQuaternion& start = currentFrame.mValue;
					const aiQuaternion& end = nextFrame.mValue;

					aiQuaternion::Interpolate(rotation, start, end, delta);
					rotation.Normalize();
				}

				rotationMat = aiMatrix4x4(rotation.GetMatrix());
			}


			// scale
			aiMatrix4x4 scaleMat;
			{
				aiVector3D scale;

				if (_assimpAnimation.joints[i].scaleChannels.size() == 1)
					scale = _assimpAnimation.joints[i].scaleChannels[0].channel;
				else
				{
					size_t frameIndex = 0;
					for (size_t j = 0; j != _assimpAnimation.joints[i].scaleChannels.size() - 1; ++j)
					{
						if (AnimationTime < (float)_assimpAnimation.joints[i].scaleChannels[j + 1].time)
						{
							frameIndex = j;
							break;
						}
					}

					aiVectorKey currentFrame;
					currentFrame.mValue = _assimpAnimation.joints[i].scaleChannels[frameIndex].channel;
					currentFrame.mTime = _assimpAnimation.joints[i].scaleChannels[frameIndex].time;

					aiVectorKey nextFrame;
					nextFrame.mValue = _assimpAnimation.joints[i].scaleChannels[(frameIndex + 1) % _assimpAnimation.joints[i].scaleChannels.size()].channel;
					nextFrame.mTime = _assimpAnimation.joints[i].scaleChannels[(frameIndex + 1) % _assimpAnimation.joints[i].scaleChannels.size()].time;

					float delta = (AnimationTime - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

					const aiVector3D& start = currentFrame.mValue;
					const aiVector3D& end = nextFrame.mValue;

					scale = (start + delta * (end - start));
				}

				aiMatrix4x4::Scaling(scale, scaleMat);
			}

			transformation = translationMat * rotationMat * scaleMat;
		}

		size_t boneID = _assimpAnimation.joints[i].boneID;
		size_t parentIndex = _assimpAnimation.joints[i].parentIndex;
		size_t parentBoneID;
		if (parentIndex != -1)
			parentBoneID = _assimpAnimation.joints[parentIndex].boneID;
		else
			parentBoneID = -1;

		aiMatrix4x4 ParentTransform;
		if (parentBoneID != -1)
			ParentTransform = bones[parentBoneID];
		aiMatrix4x4 GlobalTransformation = ParentTransform * transformation;

		bones[boneID] = GlobalTransformation;

		if (boneID != -1 && boneID < _assimpAnimation.joints.size())
		{
			_bones[boneID] = _assimpAnimation.globalInverseTransform * GlobalTransformation * _assimpAnimation.joints[i].offset;
			_bones[boneID] = _bones[boneID];
		}
	}
}

void GetAnimatedSkeleton(std::array<aiMatrix4x4, 64>& _bones, Scene::Skeleton _assimpAnimation, Scene::AnimationModifier _modifier, float AnimationTime)
{
	std::array<aiMatrix4x4, 64> bones;

	for (size_t i = 0; i != _assimpAnimation.joints.size(); ++i)
	{
		aiMatrix4x4 transformation = _assimpAnimation.joints[i].transformation;

		if (_assimpAnimation.joints[i].translateChannels.size() > 0 && _assimpAnimation.joints[i].rotateChannels.size() > 0 && _assimpAnimation.joints[i].scaleChannels.size() > 0)
		{
			// translation
			aiMatrix4x4 translationMat;// = pNode->mTransformation;
			{
				aiVector3D translation;

				if (_assimpAnimation.joints[i].translateChannels.size() == 1)
					translation = _assimpAnimation.joints[i].translateChannels[0].channel;
				else
				{
					size_t frameIndex = 0;
					for (size_t j = 0; j != _assimpAnimation.joints[i].translateChannels.size() - 1; ++j)
					{
						if (AnimationTime < (float)_assimpAnimation.joints[i].translateChannels[j + 1].time)
						{
							frameIndex = j;
							break;
						}
					}

					aiVectorKey currentFrame;
					currentFrame.mValue = _assimpAnimation.joints[i].translateChannels[frameIndex].channel;
					currentFrame.mTime = _assimpAnimation.joints[i].translateChannels[frameIndex].time;

					aiVectorKey nextFrame;
					nextFrame.mValue = _assimpAnimation.joints[i].translateChannels[(frameIndex + 1) % _assimpAnimation.joints[i].translateChannels.size()].channel;
					nextFrame.mTime = _assimpAnimation.joints[i].translateChannels[(frameIndex + 1) % _assimpAnimation.joints[i].translateChannels.size()].time;

					float delta = (AnimationTime - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

					const aiVector3D& start = currentFrame.mValue;
					const aiVector3D& end = nextFrame.mValue;

					translation = (start + delta * (end - start));
				}

				aiMatrix4x4::Translation(translation, translationMat);
			}

			// rotation
			aiMatrix4x4 rotationMat;
			{
				aiQuaternion rotation;

				if (_assimpAnimation.joints[i].rotateChannels.size() == 1)
					rotation = _assimpAnimation.joints[i].rotateChannels[0].channel;
				else
				{
					size_t frameIndex = 0;
					for (size_t j = 0; j != _assimpAnimation.joints[i].rotateChannels.size() - 1; ++j)
					{
						if (AnimationTime < (float)_assimpAnimation.joints[i].rotateChannels[j + 1].time)
						{
							frameIndex = j;
							break;
						}
					}

					if (_modifier.modifiers[i].weight == 1.0f)
					{
						rotation.x = _modifier.modifiers[i].rotX;
						rotation.y = _modifier.modifiers[i].rotY;
						rotation.z = _modifier.modifiers[i].rotZ;
						rotation.w = _modifier.modifiers[i].rotW;

						rotation.Normalize();
					}
					else
					{
						aiQuatKey currentFrame;
						currentFrame.mValue = _assimpAnimation.joints[i].rotateChannels[frameIndex].channel;
						currentFrame.mTime = _assimpAnimation.joints[i].rotateChannels[frameIndex].time;

						aiQuatKey nextFrame;
						nextFrame.mValue = _assimpAnimation.joints[i].rotateChannels[(frameIndex + 1) % _assimpAnimation.joints[i].rotateChannels.size()].channel;
						nextFrame.mTime = _assimpAnimation.joints[i].rotateChannels[(frameIndex + 1) % _assimpAnimation.joints[i].rotateChannels.size()].time;

						float delta = (AnimationTime - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

						const aiQuaternion& start = currentFrame.mValue;
						const aiQuaternion& end = nextFrame.mValue;

						aiQuaternion::Interpolate(rotation, start, end, delta);
						rotation.Normalize();

						if (_modifier.modifiers[i].weight != 0.0f)
						{
							const aiQuaternion& start = rotation;
							const aiQuaternion& end = aiQuaternion(_modifier.modifiers[i].rotX, _modifier.modifiers[i].rotY, _modifier.modifiers[i].rotZ, _modifier.modifiers[i].rotW);

							aiQuaternion::Interpolate(rotation, start, end, _modifier.modifiers[i].weight);
							rotation.Normalize();
						}
					}
				}

				rotationMat = aiMatrix4x4(rotation.GetMatrix());
			}

			// scale
			aiMatrix4x4 scaleMat;
			{
				aiVector3D scale;

				if (_assimpAnimation.joints[i].scaleChannels.size() == 1)
					scale = _assimpAnimation.joints[i].scaleChannels[0].channel;
				else
				{
					size_t frameIndex = 0;
					for (size_t j = 0; j != _assimpAnimation.joints[i].scaleChannels.size() - 1; ++j)
					{
						if (AnimationTime < (float)_assimpAnimation.joints[i].scaleChannels[j + 1].time)
						{
							frameIndex = j;
							break;
						}
					}

					aiVectorKey currentFrame;
					currentFrame.mValue = _assimpAnimation.joints[i].scaleChannels[frameIndex].channel;
					currentFrame.mTime = _assimpAnimation.joints[i].scaleChannels[frameIndex].time;

					aiVectorKey nextFrame;
					nextFrame.mValue = _assimpAnimation.joints[i].scaleChannels[(frameIndex + 1) % _assimpAnimation.joints[i].scaleChannels.size()].channel;
					nextFrame.mTime = _assimpAnimation.joints[i].scaleChannels[(frameIndex + 1) % _assimpAnimation.joints[i].scaleChannels.size()].time;

					float delta = (AnimationTime - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

					const aiVector3D& start = currentFrame.mValue;
					const aiVector3D& end = nextFrame.mValue;

					scale = (start + delta * (end - start));
				}

				aiMatrix4x4::Scaling(scale, scaleMat);
			}

			transformation = translationMat * rotationMat * scaleMat;
		}

		size_t boneID = _assimpAnimation.joints[i].boneID;
		size_t parentIndex = _assimpAnimation.joints[i].parentIndex;
		size_t parentBoneID;
		if (parentIndex != -1)
			parentBoneID = _assimpAnimation.joints[parentIndex].boneID;
		else
			parentBoneID = -1;

		aiMatrix4x4 ParentTransform;
		if (parentBoneID != -1)
			ParentTransform = bones[parentBoneID];
		aiMatrix4x4 GlobalTransformation = ParentTransform * transformation;

		bones[boneID] = GlobalTransformation;

		if (boneID != -1 && boneID < _assimpAnimation.joints.size())
		{
			_bones[boneID] = _assimpAnimation.globalInverseTransform * GlobalTransformation * _assimpAnimation.joints[i].offset;
			_bones[boneID] = _bones[boneID];
		}
	}
}

void Renderer::UpdateRenderList(Scene& _scene)
{
	// model matrices data
	{
		singleTimeTransformCount = 0;
		for (size_t i = 0; i != _scene.rootObjects.size(); ++i)
		{
			if (_scene.rootObjects[i].visible = true)
				memcpy(&modelMatrices[singleTimeTransformCount], &_scene.rootObjects[i].matrix.mat, sizeof(float) * 16);

			++singleTimeTransformCount;
		}

		//for (size_t i = 0; i != _physics.rigidbodies.size(); ++i)
		//{
		//	if (_physics.rigidbodies[i]->getCollisionShape()->getShapeType() == SPHERE_SHAPE_PROXYTYPE)
		//	{
		//		btTransform transform;
		//		_physics.rigidbodies[i]->getMotionState()->getWorldTransform(transform);
		//		float mat[16];
		//		transform.getOpenGLMatrix(mat);
		//		memcpy(&modelMatrices[1], mat, sizeof(float)*16);
		//	}
		//}
	}
}

void Renderer::UpdateUniforms(Scene& _scene)
{
	// camera
	{
		static float x = CAMERA_POSITION_X, y = CAMERA_POSITION_Y, z = CAMERA_POSITION_Z;

		if (GetAsyncKeyState('E'))
			z += CAMERA_SPEED;
		if (GetAsyncKeyState('Q'))
			z -= CAMERA_SPEED;

		if (GetAsyncKeyState('W'))
			y += CAMERA_SPEED;
		if (GetAsyncKeyState('S'))
			y -= CAMERA_SPEED;

		if (GetAsyncKeyState('A'))
			x -= CAMERA_SPEED;
		if (GetAsyncKeyState('D'))
			x += CAMERA_SPEED;

		camera.view = glm::lookAt(glm::vec3(cos((float)timer.GetTime())*CAMERA_OFFSET_FROM_CENTER + x, sin((float)timer.GetTime())*CAMERA_OFFSET_FROM_CENTER + y, z), glm::vec3(0.0f, 0.0f, CAMERA_VIEW_HEIGHT), glm::vec3(0.0f, 0.0f, 1.0f));
		camera.projection = glm::perspective(glm::radians(45.0f), swapchain.extent.width / (float)swapchain.extent.height, 0.1f, 20000.0f);
		camera.projection[1][1] *= -1;

		// staging
		VkU::UniformBuffer stagingBuffer;
		{
			VkBufferCreateInfo stagingBufferCreateInfo = VkU::GetVkBufferCreateInfo(sizeof(camera), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
			VK_CHECK_RESULT("	", vkCreateBuffer(device.handle, &stagingBufferCreateInfo, nullptr, &stagingBuffer.handle), " - ", stagingBuffer.handle, " - vkCreateBuffer");

			VkMemoryRequirements stagingMemoryRequirements;
			vkGetBufferMemoryRequirements(device.handle, stagingBuffer.handle, &stagingMemoryRequirements);

			VkMemoryAllocateInfo stagingMemoryAllocateInfo = VkU::GetVkMemoryAllocateInfo(stagingMemoryRequirements, physicalDevices[device.physicalDeviceIndex], VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			VK_CHECK_RESULT("	", vkAllocateMemory(device.handle, &stagingMemoryAllocateInfo, nullptr, &stagingBuffer.memory), " - ", stagingBuffer.memory, " - vkAllocateMemory");

			VK_CHECK_RESULT("	", vkBindBufferMemory(device.handle, stagingBuffer.handle, stagingBuffer.memory, 0), " - ", 0, " - vkBindBufferMemory");

			void* data;
			VK_CHECK_RESULT("	", vkMapMemory(device.handle, stagingBuffer.memory, 0, sizeof(camera), 0, &data), " - ", data, " - vkMapMemory");
			memcpy(data, &camera, sizeof(camera));
			vkUnmapMemory(device.handle, stagingBuffer.memory);
		}

		// transfer
		{
			VkCommandBufferBeginInfo commandBufferBeginInfo = VkU::GetVkCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VK_CHECK_RESULT("	", vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo), " - ", 0, " - vkBeginCommandBuffer");

			VkBufferCopy copyRegion;
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = sizeof(camera);

			vkCmdCopyBuffer(setupCommandBuffer, stagingBuffer.handle, uniformBuffers[CAMERA_UNIFORM_BINDING].handle, 1, &copyRegion);

			VK_CHECK_RESULT("	", vkEndCommandBuffer(setupCommandBuffer), " - ", 0, " - vkMapMemory");

			VkSubmitInfo submitInfo = VkU::GetVkSubmitInfo(0, nullptr, nullptr, 1, &setupCommandBuffer, 0, nullptr);

			VK_CHECK_RESULT("	", vkQueueSubmit(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0], 1, &submitInfo, VK_NULL_HANDLE), " - ", 0, " - vkMapMemory");
			VK_CHECK_RESULT("	", vkQueueWaitIdle(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0]), " - ", 0, " - vkMapMemory");

			vkDestroyBuffer(device.handle, stagingBuffer.handle, nullptr);
			vkFreeMemory(device.handle, stagingBuffer.memory, nullptr);
		}
	}

#define MODEL_MATRICES_SIZE singleTimeTransformCount

	// model matrices
	{
		// staging
		VkU::UniformBuffer stagingBuffer;
		{
			VkBufferCreateInfo stagingBufferCreateInfo = VkU::GetVkBufferCreateInfo(MODEL_MATRICES_SIZE * sizeof(glm::mat4), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
			VK_CHECK_RESULT("	", vkCreateBuffer(device.handle, &stagingBufferCreateInfo, nullptr, &stagingBuffer.handle), " - ", stagingBuffer.handle, " - vkCreateBuffer");

			VkMemoryRequirements stagingMemoryRequirements;
			vkGetBufferMemoryRequirements(device.handle, stagingBuffer.handle, &stagingMemoryRequirements);

			VkMemoryAllocateInfo stagingMemoryAllocateInfo = VkU::GetVkMemoryAllocateInfo(stagingMemoryRequirements, physicalDevices[device.physicalDeviceIndex], VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			VK_CHECK_RESULT("	", vkAllocateMemory(device.handle, &stagingMemoryAllocateInfo, nullptr, &stagingBuffer.memory), " - ", stagingBuffer.memory, " - vkAllocateMemory");

			VK_CHECK_RESULT("	", vkBindBufferMemory(device.handle, stagingBuffer.handle, stagingBuffer.memory, 0), " - ", 0, " - vkBindBufferMemory");

			void* data;
			VK_CHECK_RESULT("	", vkMapMemory(device.handle, stagingBuffer.memory, 0, MODEL_MATRICES_SIZE * sizeof(glm::mat4), 0, &data), " - ", data, " - vkMapMemory");
			memcpy(data, modelMatrices.data(), MODEL_MATRICES_SIZE * sizeof(glm::mat4));
			vkUnmapMemory(device.handle, stagingBuffer.memory);
		}

		// transfer
		{
			VkCommandBufferBeginInfo commandBufferBeginInfo = VkU::GetVkCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VK_CHECK_RESULT("	", vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo), " - ", 0, " - vkBeginCommandBuffer");

			VkBufferCopy copyRegion;
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = MODEL_MATRICES_SIZE * sizeof(glm::mat4);

			vkCmdCopyBuffer(setupCommandBuffer, stagingBuffer.handle, uniformBuffers[MODEL_UNIFORM_BINDING].handle, 1, &copyRegion);

			VK_CHECK_RESULT("	", vkEndCommandBuffer(setupCommandBuffer), " - ", 0, " - vkMapMemory");

			VkSubmitInfo submitInfo = VkU::GetVkSubmitInfo(0, nullptr, nullptr, 1, &setupCommandBuffer, 0, nullptr);

			VK_CHECK_RESULT("	", vkQueueSubmit(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0], 1, &submitInfo, VK_NULL_HANDLE), " - ", 0, " - vkMapMemory");
			VK_CHECK_RESULT("	", vkQueueWaitIdle(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0]), " - ", 0, " - vkMapMemory");

			vkDestroyBuffer(device.handle, stagingBuffer.handle, nullptr);
			vkFreeMemory(device.handle, stagingBuffer.memory, nullptr);
		}
	}

	// skeleton
	{
		// Update
		{
			float tempTime = sinf((float)timer.GetTime()*0.5f) * 1.5f + 1.5f;

			std::array<aiMatrix4x4, 64> bones;
			GetAnimatedSkeleton(bones, _scene.skeletons[0], _scene.animationModifiers[0], tempTime);

			for (size_t i = 0; i != _scene.skeletons[0].joints.size(); ++i)
			{
				skeletons[0].bones[i] = Loader::aiMatrix4x4ToGlmMat4(bones[i]);
				skeletons[0].bones[i] = glm::transpose(skeletons[0].bones[i]);
			}
		}

		// staging
		VkU::UniformBuffer stagingBuffer;
		{
			VkBufferCreateInfo stagingBufferCreateInfo = VkU::GetVkBufferCreateInfo(skeletons.size() * sizeof(skeleton), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
			VK_CHECK_RESULT("	", vkCreateBuffer(device.handle, &stagingBufferCreateInfo, nullptr, &stagingBuffer.handle), " - ", stagingBuffer.handle, " - vkCreateBuffer");

			VkMemoryRequirements stagingMemoryRequirements;
			vkGetBufferMemoryRequirements(device.handle, stagingBuffer.handle, &stagingMemoryRequirements);

			VkMemoryAllocateInfo stagingMemoryAllocateInfo = VkU::GetVkMemoryAllocateInfo(stagingMemoryRequirements, physicalDevices[device.physicalDeviceIndex], VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			VK_CHECK_RESULT("	", vkAllocateMemory(device.handle, &stagingMemoryAllocateInfo, nullptr, &stagingBuffer.memory), " - ", stagingBuffer.memory, " - vkAllocateMemory");

			VK_CHECK_RESULT("	", vkBindBufferMemory(device.handle, stagingBuffer.handle, stagingBuffer.memory, 0), " - ", 0, " - vkBindBufferMemory");

			void* data;
			VK_CHECK_RESULT("	", vkMapMemory(device.handle, stagingBuffer.memory, 0, skeletons.size() * sizeof(skeleton), 0, &data), " - ", data, " - vkMapMemory");
			memcpy(data, skeletons.data(), skeletons.size() * sizeof(skeleton));
			vkUnmapMemory(device.handle, stagingBuffer.memory);
		}

		// transfer
		{
			VkCommandBufferBeginInfo commandBufferBeginInfo = VkU::GetVkCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VK_CHECK_RESULT("	", vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo), " - ", 0, " - vkBeginCommandBuffer");

			VkBufferCopy copyRegion;
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = skeletons.size() * sizeof(skeleton);

			vkCmdCopyBuffer(setupCommandBuffer, stagingBuffer.handle, uniformBuffers[SKELETON_UNIFORM_BINDING].handle, 1, &copyRegion);

			VK_CHECK_RESULT("	", vkEndCommandBuffer(setupCommandBuffer), " - ", 0, " - vkMapMemory");

			VkSubmitInfo submitInfo = VkU::GetVkSubmitInfo(0, nullptr, nullptr, 1, &setupCommandBuffer, 0, nullptr);

			VK_CHECK_RESULT("	", vkQueueSubmit(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0], 1, &submitInfo, VK_NULL_HANDLE), " - ", 0, " - vkMapMemory");
			VK_CHECK_RESULT("	", vkQueueWaitIdle(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0]), " - ", 0, " - vkMapMemory");

			vkDestroyBuffer(device.handle, stagingBuffer.handle, nullptr);
			vkFreeMemory(device.handle, stagingBuffer.memory, nullptr);
		}
	}
}
void Renderer::HandleWindow()
{
	gRenderer = this;

	static double lastTime = 0.0f;
	static uint64_t frameCount = 0;
	static uint64_t sumFPS = 0;
	++frameCount;
	sumFPS += (int)(1 / (timer.GetTime() - lastTime));

	SetWindowText(window.hWnd, (std::to_string((int)(1 / (timer.GetTime() - lastTime))) + std::string(" - FPS    ") + std::to_string(sumFPS / frameCount) + std::string(" - AVG")).c_str());

	lastTime = timer.GetTime();

	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
void Renderer::PrepareToDraw()
{
	VK_CHECK_RESULT("	", vkAcquireNextImageKHR(device.handle, swapchain.handle, -1, semaphoreImageAvailable, VK_NULL_HANDLE, &swapchainImageIndex), " - ", 0, " - vkAcquireNextImageKHR");
}
void Renderer::Draw(Scene& _scene)
{
	VkCommandBufferBeginInfo commandBufferBeginInfo = VkU::GetVkCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

	VkClearValue clearColor[2];
	clearColor[0].color = { 0.15f, 0.2f, 0.25f, 1.0f };
	clearColor[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = VkU::GetVkRenderPassBeginInfo(renderPass, swapchain.framebuffers[swapchainImageIndex], { 0, 0 }, { swapchain.extent.width, swapchain.extent.height }, sizeof(clearColor) / sizeof(VkClearValue), clearColor);

	VkDeviceSize offset = 0;

	VK_CHECK_RESULT("	", vkWaitForFences(device.handle, 1, &renderFences[swapchainImageIndex], VK_TRUE, -1), " - ", 0, " - vkWaitForFences");
	VK_CHECK_RESULT("	", vkResetFences(device.handle, 1, &renderFences[swapchainImageIndex]), " - ", 0, " - vkResetFences");

	VK_CHECK_RESULT("	", vkBeginCommandBuffer(renderCommandBuffers[swapchainImageIndex], &commandBufferBeginInfo), " - ", 0, " - vkBeginCommandBuffer");
	{
		vkCmdBeginRenderPass(renderCommandBuffers[swapchainImageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		uint32_t matrixCounter = 0;
		for (size_t i = 0; i != _scene.rootObjects.size(); ++i)
		{
			if (_scene.rootObjects[i].visible == true)
			{
				_scene.models[_scene.rootObjects[i].modelID].pipelineIndex;

				uint32_t pushConstantData[4] = { matrixCounter, 1, 1, 1 };
				vkCmdPushConstants(renderCommandBuffers[swapchainImageIndex], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstantData), &pushConstantData);

				vkCmdBindPipeline(renderCommandBuffers[swapchainImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[_scene.models[_scene.rootObjects[i].modelID].pipelineIndex]);
				vkCmdBindDescriptorSets(renderCommandBuffers[swapchainImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
				vkCmdBindVertexBuffers(renderCommandBuffers[swapchainImageIndex], 0, 1, &vertexBuffers[_scene.models[_scene.rootObjects[i].modelID].vertexBufferIndex].handle, &offset);
				vkCmdBindIndexBuffer(renderCommandBuffers[swapchainImageIndex], indexBuffers[_scene.models[_scene.rootObjects[i].modelID].indexBufferIndex].handle, 0, indexBuffers[_scene.models[_scene.rootObjects[i].modelID].indexBufferIndex].type);
				vkCmdDrawIndexed(renderCommandBuffers[swapchainImageIndex], indexBuffers[_scene.models[_scene.rootObjects[i].modelID].indexBufferIndex].count, 1, 0, 0, 1);
				++matrixCounter;
			}
		}
		vkCmdEndRenderPass(renderCommandBuffers[swapchainImageIndex]);
	}
	VK_CHECK_RESULT("	", vkEndCommandBuffer(renderCommandBuffers[swapchainImageIndex]), " - ", 0, " - vkEndCommandBuffer");
}
void Renderer::Render()
{
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &semaphoreImageAvailable;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &renderCommandBuffers[swapchainImageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &semaphoreRenderDone;

	VK_CHECK_RESULT("	", vkQueueSubmit(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0], 1, &submitInfo, renderFences[swapchainImageIndex]), " - ", 0, " - vkQueueSubmit");

	VkPresentInfoKHR presentInfoKHR;
	presentInfoKHR.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfoKHR.pNext = nullptr;
	presentInfoKHR.waitSemaphoreCount = 1;
	presentInfoKHR.pWaitSemaphores = &semaphoreRenderDone;
	presentInfoKHR.swapchainCount = 1;
	presentInfoKHR.pSwapchains = &swapchain.handle;
	presentInfoKHR.pImageIndices = &swapchainImageIndex;
	presentInfoKHR.pResults = nullptr;

	VK_CHECK_RESULT("	", vkQueuePresentKHR(device.queues[GRAPHICS_PRESENT_QUEUE_INDEX].handles[0], &presentInfoKHR), " - ", 0, " - vkQueuePresentKHR");
}
void Renderer::ShutDown()
{
	VK_CHECK_RESULT("	", vkDeviceWaitIdle(device.handle), " - ", 0, " - vkDeviceWaitIdle");

	if (depthImage.view != nullptr)
		VK_CHECK_CLEANUP("	", depthImage.view, vkDestroyImageView(device.handle, depthImage.view, nullptr), " - vkDestroyImageView");
	if (depthImage.handle != nullptr)
		VK_CHECK_CLEANUP("	", depthImage.handle, vkDestroyImage(device.handle, depthImage.handle, nullptr), " - vkDestroyBuffer");
	if (depthImage.memory != nullptr)
		VK_CHECK_CLEANUP("	", depthImage.memory, vkFreeMemory(device.handle, depthImage.memory, nullptr), " - vkFreeMemory");

	VK_CHECK_CLEANUP("	", sampler, vkDestroySampler(device.handle, sampler, nullptr), " - vkDestroySampler");

	for (size_t i = 0; i != textures.size(); ++i)
	{
		VK_CHECK_CLEANUP("	", textures[i].view, vkDestroyImageView(device.handle, textures[i].view, nullptr), " - vkDestroyImageView");
		VK_CHECK_CLEANUP("	", textures[i].handle, vkDestroyImage(device.handle, textures[i].handle, nullptr), " - vkDestroyBuffer");
		VK_CHECK_CLEANUP("	", textures[i].memory, vkFreeMemory(device.handle, textures[i].memory, nullptr), " - vkFreeMemory");
	}
	textures.clear();

	VK_CHECK_CLEANUP("	", descriptorPool, vkDestroyDescriptorPool(device.handle, descriptorPool, nullptr), " - vkDestroyDescriptorSetLayout");

	VK_CHECK_CLEANUP("	", descriptorSetLayout, vkDestroyDescriptorSetLayout(device.handle, descriptorSetLayout, nullptr), " - vkDestroyDescriptorSetLayout");

	VK_CHECK_CLEANUP("	", setupFence, vkDestroyFence(device.handle, setupFence, nullptr), " - vkDestroyFence");
	for (size_t i = 0; i != renderFences.size(); ++i)
	{
		VK_CHECK_CLEANUP("	", renderFences[i], vkDestroyFence(device.handle, renderFences[i], nullptr), " - vkDestroyFence");
	}
	renderFences.clear();

	VK_CHECK_CLEANUP("	", semaphoreImageAvailable, vkDestroySemaphore(device.handle, semaphoreImageAvailable, nullptr), " - vkDestroySemaphore");
	VK_CHECK_CLEANUP("	", semaphoreRenderDone, vkDestroySemaphore(device.handle, semaphoreRenderDone, nullptr), " - vkDestroySemaphore");

	for (size_t i = 0; i != uniformBuffers.size(); ++i)
	{
		VK_CHECK_CLEANUP("	", uniformBuffers[i].handle, vkDestroyBuffer(device.handle, uniformBuffers[i].handle, nullptr), " - vkDestroyBuffer");
		VK_CHECK_CLEANUP("	", uniformBuffers[i].memory, vkFreeMemory(device.handle, uniformBuffers[i].memory, nullptr), " - vkFreeMemory");
	}
	uniformBuffers.clear();
	for (size_t i = 0; i != indexBuffers.size(); ++i)
	{
		VK_CHECK_CLEANUP("	", indexBuffers[i].handle, vkDestroyBuffer(device.handle, indexBuffers[i].handle, nullptr), " - vkDestroyBuffer");
		VK_CHECK_CLEANUP("	", indexBuffers[i].memory, vkFreeMemory(device.handle, indexBuffers[i].memory, nullptr), " - vkFreeMemory");
	}
	indexBuffers.clear();
	for (size_t i = 0; i != vertexBuffers.size(); ++i)
	{
		VK_CHECK_CLEANUP("	", vertexBuffers[i].handle, vkDestroyBuffer(device.handle, vertexBuffers[i].handle, nullptr), " - vkDestroyBuffer");
		VK_CHECK_CLEANUP("	", vertexBuffers[i].memory, vkFreeMemory(device.handle, vertexBuffers[i].memory, nullptr), " - vkFreeMemory");
	}
	vertexBuffers.clear();

	for (size_t i = 0; i != pipelines.size(); ++i)
	{
		VK_CHECK_CLEANUP("	", pipelines[i], vkDestroyPipeline(device.handle, pipelines[i], nullptr), " - vkDestroyPipeline");
	}
	pipelines.clear();

	VK_CHECK_CLEANUP("	", pipelineLayout, vkDestroyPipelineLayout(device.handle, pipelineLayout, nullptr), " - vkDestroyPipelineLayout");

	for (size_t i = 0; i != shaderModules.size(); ++i)
	{
		VK_CHECK_CLEANUP("	", shaderModules[i].handle, vkDestroyShaderModule(device.handle, shaderModules[i].handle, nullptr), " - vkDestroyShaderModule");
	}
	shaderModules.clear();

	for (size_t i = 0; i != swapchain.framebuffers.size(); ++i)
	{
		VK_CHECK_CLEANUP("	", swapchain.framebuffers[i], vkDestroyFramebuffer(device.handle, swapchain.framebuffers[i], nullptr), " - vkDestroyFramebuffer");
	}
	swapchain.framebuffers.clear();
	for (size_t i = 0; i != swapchain.views.size(); ++i)
	{
		VK_CHECK_CLEANUP("	", swapchain.views[i], vkDestroyImageView(device.handle, swapchain.views[i], nullptr), " - vkDestroyImageView");
	}
	VK_CHECK_CLEANUP("	", swapchain.handle, vkDestroySwapchainKHR(device.handle, swapchain.handle, nullptr), " - vkDestroySwapchainKHR");
	swapchain.views.clear();

	VK_CHECK_CLEANUP("	", renderPass, vkDestroyRenderPass(device.handle, renderPass, nullptr), " - vkDestroyRenderPass");

	VK_CHECK_CLEANUP("	", commandPool, vkDestroyCommandPool(device.handle, commandPool, nullptr), " - vkDestroyCommandPool");

	VK_CHECK_CLEANUP("	", device.handle, vkDestroyDevice(device.handle, nullptr), " - vkDestroyDevice");

	VK_CHECK_CLEANUP("	", surface.handle, vkDestroySurfaceKHR(instance, surface.handle, nullptr), " - vkDestroySurfaceKHR");

	DestroyWindow(window.hWnd);
	UnregisterClass(window.name, GetModuleHandle(NULL));

	if (debugReportCallback != VK_NULL_HANDLE)
	{
		PFN_vkDestroyDebugReportCallbackEXT FP_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		VK_CHECK_CLEANUP("	", debugReportCallback, FP_vkDestroyDebugReportCallbackEXT(instance, debugReportCallback, nullptr), " - FP_vkDestroyDebugReportCallbackEXT");
	}

	VK_CHECK_CLEANUP("	", instance, vkDestroyInstance(instance, nullptr), " - vkDestroyInstance");
}

void Renderer::Update(Scene& _scene)
{
	UpdateRenderList(_scene);
	UpdateUniforms(_scene);
	HandleWindow();
	PrepareToDraw();
	Draw(_scene);
}
