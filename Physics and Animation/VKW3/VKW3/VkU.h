#ifndef	VK_UTILITY_H
#define VK_UTILITY_H

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>
#define VK_RESERVED_FOR_FUTURE_USE 0

#include <vector>
#include <array>

#include "Logger.h"
#include "Timer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef LOG_RENDERER

#define VK_CHECK_RESULT(preText, call, midText, variable, postText) VkU::vkResult = call;	\
VkU::logger << "Time: " << VkU::timer.GetTime() << preText << VkU::vkResult << midText << variable << postText << '\n'

#define VK_CHECK_CLEANUP(preText, variable, call, postText) call;	\
VkU::cleanupLogger << "Time: " << VkU::timer.GetTime() << preText << variable << postText << '\n'; \
variable = VK_NULL_HANDLE

#else
#define VK_CHECK_RESULT(preText, call, midText, variable, postText) VkU::vkResult = call
#define VK_CHECK_CLEANUP(preText, variable, call, postText) call
#endif

//#include "Loader.h"

namespace VkU
{
	static VkResult vkResult = VK_SUCCESS;

#ifdef LOG_RENDERER
	static Timer timer;
	static Logger logger;
	static Logger debugReportCallbackLogger;
	static Logger cleanupLogger;
#endif

	struct PhysicalDevice
	{
		VkPhysicalDevice						handle;

		VkPhysicalDeviceFeatures				features;
		VkPhysicalDeviceMemoryProperties		memoryProperties;
		VkPhysicalDeviceProperties				properties;
		std::vector<VkQueueFamilyProperties>	queueFamilyProperties;
		std::vector<VkBool32>					queueFamilyPresentable;

		VkFormat depthFormat;
	};
	struct Window
	{
		HWND		hWnd;
		HINSTANCE	hInstance;
		const char*	name;
	};
	struct Surface
	{
		VkSurfaceKHR				handle;
		VkSurfaceFormatKHR			colorFormat;
		VkCompositeAlphaFlagBitsKHR	compositeAlpha;
		VkPresentModeKHR			presentMode;
	};
	struct Queue
	{
		std::vector<VkQueue>	handles;

		uint32_t				queueFamilyIndex;
		uint32_t				queueIndex;

		VkQueueFlags			flags;
		VkBool32				presentability;
		float					priority;
		uint32_t				count;

		static inline Queue GetQueue(VkBool32 _presentability, VkQueueFlags _flags, float _priority, uint32_t _count)
		{
			return{ {}, (uint32_t)-1, (uint32_t)-1, _flags, _presentability, _priority, _count };
		}
	};
	struct Device
	{
		VkDevice handle;
		uint32_t physicalDeviceIndex;
		std::vector<Queue> queues;
	};
	struct Swapchain
	{
		VkSwapchainKHR handle;

		VkExtent2D extent;

		std::vector<VkImage>		images;
		std::vector<VkImageView>	views;
		std::vector<VkFramebuffer>	framebuffers;
	};
	struct Image
	{
		VkImage handle;
		VkDeviceMemory memory;
		VkImageView view;
	};
	struct ShaderModule
	{
		VkShaderModule			handle;
		VkShaderStageFlagBits	stage;
		const char*				entryPointName;
	};
	struct UniformBuffer
	{
		VkBuffer handle;
		VkDeviceMemory memory;
	};
	struct VertexBuffer
	{
		VkBuffer handle;
		VkDeviceMemory memory;
	};
	struct IndexBuffer
	{
		VkBuffer handle;
		VkDeviceMemory memory;
		VkIndexType type;
		uint32_t count;
	};
	struct Skeleton
	{
		std::vector<glm::mat4> joints;
	};
	struct Keyframe
	{
		float time;
		glm::mat4 frame;
	};
	struct Animation
	{
		std::vector<Keyframe> keyframes;
	};

	struct Camera
	{
		glm::mat4 view;
		glm::mat4 projection;
	};

	// Instance
	static VkApplicationInfo GetVkApplicationInfo(const char* _appName, uint32_t _appVersion, const char* _engineName, uint32_t _engineVersion)
	{
		VkApplicationInfo applicationInfo;
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pNext = nullptr;
		applicationInfo.pApplicationName = _appName;
		applicationInfo.applicationVersion = _appVersion;
		applicationInfo.pEngineName = _engineName;
		applicationInfo.engineVersion = _engineVersion;
		applicationInfo.apiVersion = VK_API_VERSION_1_0;

		return applicationInfo;
	}
	static VkInstanceCreateInfo GetVkInstanceCreateInfo(const VkApplicationInfo* _applicationInfo, uint32_t _instanceLayerCount, const char* const* _instanceLayerNames, uint32_t _instanceExtensionCount, const char* const* _instanceExtensionNames)
	{
		VkInstanceCreateInfo instanceCreateInfo;
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = nullptr;
		instanceCreateInfo.flags = VK_RESERVED_FOR_FUTURE_USE;
		instanceCreateInfo.pApplicationInfo = _applicationInfo;
		instanceCreateInfo.enabledLayerCount = _instanceLayerCount;
		instanceCreateInfo.ppEnabledLayerNames = _instanceLayerNames;
		instanceCreateInfo.enabledExtensionCount = _instanceExtensionCount;
		instanceCreateInfo.ppEnabledExtensionNames = _instanceExtensionNames;

		return instanceCreateInfo;
	}

	// Debug
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT _flags, VkDebugReportObjectTypeEXT _objType, uint64_t _obj, size_t _location, int32_t _code, const char* _layerPrefix, const char* _msg, void* _userData)
	{
#ifdef LOG_RENDERER
		if (_flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
			debugReportCallbackLogger << "	INFORMATION:";
		if (_flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
			debugReportCallbackLogger << "WARNING:";
		if (_flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
			debugReportCallbackLogger << "PERFORMANCE:";
		if (_flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
			debugReportCallbackLogger << "ERROR:";
		if (_flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
			debugReportCallbackLogger << "	DEBUG:";

		debugReportCallbackLogger << _msg << '\n';
#endif
		return VK_FALSE; // Don't abort the function that made this call
	}
	static VkDebugReportCallbackCreateInfoEXT GetVkDebugReportCallbackCreateInfoEXT(VkDebugReportFlagsEXT _debugFlags, PFN_vkDebugReportCallbackEXT _debugReportCallback)
	{
		VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo;
		debugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		debugReportCallbackCreateInfo.pNext = nullptr;
		debugReportCallbackCreateInfo.flags = _debugFlags;
		debugReportCallbackCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)_debugReportCallback;
		debugReportCallbackCreateInfo.pUserData = nullptr;

		return debugReportCallbackCreateInfo;
	}

	// PhysicalDevice
	static VkFormat GetDepthFormat(VkPhysicalDevice _physicalDevices, std::vector<VkFormat>* _preferedDepthFormat)
	{
		if (_preferedDepthFormat != nullptr)
		{
			for (uint32_t j = 0; j != _preferedDepthFormat->size(); ++j)
			{
				VkFormatProperties formatProperties;
				vkGetPhysicalDeviceFormatProperties(_physicalDevices, (*_preferedDepthFormat)[j], &formatProperties);

				if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				{
					return (*_preferedDepthFormat)[j];
				}
			}
		}
		else
		{
			VkFormat preferedAllDepthFormats[] = {
				VK_FORMAT_D32_SFLOAT, // 100% availability 3/d1/2017
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_X8_D24_UNORM_PACK32,
				VK_FORMAT_D16_UNORM, // 100% availability 3/d1/2017
				VK_FORMAT_D16_UNORM_S8_UINT,
			};

			for (uint32_t j = 0; j != sizeof(preferedAllDepthFormats) / sizeof(VkFormat); ++j)
			{
				VkFormatProperties formatProperties;
				vkGetPhysicalDeviceFormatProperties(_physicalDevices, preferedAllDepthFormats[j], &formatProperties);

				if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				{
					return preferedAllDepthFormats[j];
				}
			}
		}

		return VK_FORMAT_UNDEFINED;
	}
	static std::vector<PhysicalDevice> GetPhysicalDevices(VkInstance _instance, std::vector<VkFormat>* _preferedDepthFormat)
	{
		uint32_t propertyCount = 0;
		vkEnumeratePhysicalDevices(_instance, &propertyCount, nullptr);
		std::vector<VkPhysicalDevice> physicalDevicesHandles(propertyCount);
		vkEnumeratePhysicalDevices(_instance, &propertyCount, physicalDevicesHandles.data());

		std::vector<PhysicalDevice> physicalDevices(physicalDevicesHandles.size());
		for (size_t i = 0; i != physicalDevicesHandles.size(); ++i)
		{
			physicalDevices[i].handle = physicalDevicesHandles[i];

			vkGetPhysicalDeviceProperties(physicalDevices[i].handle, &physicalDevices[i].properties);
			vkGetPhysicalDeviceFeatures(physicalDevices[i].handle, &physicalDevices[i].features);
			vkGetPhysicalDeviceMemoryProperties(physicalDevices[i].handle, &physicalDevices[i].memoryProperties);

			uint32_t propertyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i].handle, &propertyCount, nullptr);
			physicalDevices[i].queueFamilyProperties.resize(propertyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i].handle, &propertyCount, physicalDevices[i].queueFamilyProperties.data());

			physicalDevices[i].queueFamilyPresentable.resize(physicalDevices[i].queueFamilyProperties.size());
			for (uint32_t j = 0; j != physicalDevices[i].queueFamilyPresentable.size(); ++j)
				physicalDevices[i].queueFamilyPresentable[j] = vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevices[i].handle, j);

			physicalDevices[i].depthFormat = GetDepthFormat(physicalDevices[i].handle, _preferedDepthFormat);
		}

		return physicalDevices;
	}
	static std::vector<VkSurfaceFormatKHR> GetVkSurfaceFormatKHRs(VkPhysicalDevice _physicalDevice, VkSurfaceKHR _surface)
	{
		std::vector<VkSurfaceFormatKHR> surfaceFormats;

		uint32_t propertyCount = 0;
		VK_CHECK_RESULT("	", vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &propertyCount, nullptr), " - ", 0, " - vkGetPhysicalDeviceSurfaceFormatsKHR");
		surfaceFormats.resize(propertyCount);
		VK_CHECK_RESULT("	", vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &propertyCount, surfaceFormats.data()), " - ", 0, " - vkGetPhysicalDeviceSurfaceFormatsKHR");

		return surfaceFormats;
	}
	static std::vector<VkPresentModeKHR> GetVkPresentModeKHRs(VkPhysicalDevice _physicalDevice, VkSurfaceKHR _surface)
	{
		std::vector<VkPresentModeKHR> presentModes;

		uint32_t propertyCount = 0;
		VK_CHECK_RESULT("	", vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &propertyCount, nullptr), " - ", 0, " - vkGetPhysicalDeviceSurfacePresentModesKHR");
		presentModes.resize(propertyCount);
		VK_CHECK_RESULT("	", vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &propertyCount, presentModes.data()), " - ", 0, " - vkGetPhysicalDeviceSurfacePresentModesKHR");

		return presentModes;
	}
	static VkSurfaceCapabilitiesKHR GetVkSurfaceCapabilitiesKHR(VkPhysicalDevice _physicalDevice, VkSurfaceKHR _surface)
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;

		VK_CHECK_RESULT("	", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &surfaceCapabilities), " - ", 0, " - vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

		return surfaceCapabilities;
	}

	// Window
	static Window GetWindow(uint32_t _width, uint32_t _height, const char* _title, const char* _name, WNDPROC _wndProc)
	{
		Window window;
		window.hInstance = GetModuleHandle(NULL);
		window.name = _name;
		window.hWnd = NULL;

		WNDCLASSEX wndClassEx;
		wndClassEx.cbSize = sizeof(WNDCLASSEX);
		wndClassEx.style = CS_HREDRAW | CS_VREDRAW;
		wndClassEx.lpfnWndProc = _wndProc;
		wndClassEx.cbClsExtra = 0;
		wndClassEx.cbWndExtra = 0;
		wndClassEx.hInstance = GetModuleHandle(NULL);
		wndClassEx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndClassEx.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wndClassEx.lpszMenuName = NULL;
		wndClassEx.lpszClassName = _name;
		wndClassEx.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

		if (!RegisterClassEx(&wndClassEx))
			return window;

		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

		RECT windowRect;
		windowRect.left = 0L;
		windowRect.top = 0L;
		windowRect.right = (long)_width;
		windowRect.bottom = (long)_height;

		AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

		window.hWnd = CreateWindowEx(
			0,
			_name,
			_title,
			dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			0,
			0,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			NULL,
			NULL,
			GetModuleHandle(NULL),
			NULL);

		if (window.hWnd == NULL)
			return window;

		uint32_t x = (screenWidth - windowRect.right) / 2;
		uint32_t y = (screenHeight - windowRect.bottom) / 2;
		SetWindowPos(window.hWnd, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

		ShowWindow(window.hWnd, SW_SHOW);
		SetForegroundWindow(window.hWnd);
		SetFocus(window.hWnd);

		return window;
	}

	// Surface
	static VkWin32SurfaceCreateInfoKHR GetVkWin32SurfaceCreateInfoKHR(HINSTANCE _hInstance, HWND _hWnd)
	{
		VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo;
		win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		win32SurfaceCreateInfo.pNext = nullptr;
		win32SurfaceCreateInfo.flags = VK_RESERVED_FOR_FUTURE_USE;
		win32SurfaceCreateInfo.hinstance = _hInstance;
		win32SurfaceCreateInfo.hwnd = _hWnd;

		return win32SurfaceCreateInfo;
	}
	static VkSurfaceFormatKHR GetVkSurfaceFormatKHR(VkPhysicalDevice _physicalDevice, Surface _surface, std::vector<VkFormat>* _preferedColorFormats)
	{
		std::vector<VkSurfaceFormatKHR> surfaceFormats = GetVkSurfaceFormatKHRs(_physicalDevice, _surface.handle);

		if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			if (_preferedColorFormats != nullptr)
				return{ (*_preferedColorFormats)[0], surfaceFormats[0].colorSpace };
			else
				return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

		}
		else if (_preferedColorFormats != nullptr)
		{
			for (uint32_t i = 0; i != (*_preferedColorFormats).size(); ++i)
			{
				for (uint32_t j = 0; j != surfaceFormats.size(); ++j)
				{
					if ((*_preferedColorFormats)[i] == surfaceFormats[j].format)
					{
						return{ surfaceFormats[i] };
						break;
					}
				}
			}
		}

		return surfaceFormats[0];
	}
	static VkCompositeAlphaFlagBitsKHR GetVkCompositeAlphaFlagBitsKHR(VkSurfaceCapabilitiesKHR _surfaceCapabilities, std::vector<VkCompositeAlphaFlagBitsKHR>* _preferedCompositeAlphas)
	{
		if (_preferedCompositeAlphas != nullptr)
		{
			for (size_t i = 0; i != _preferedCompositeAlphas->size(); ++i)
			{
				if (((*_preferedCompositeAlphas)[i] & _surfaceCapabilities.supportedCompositeAlpha) == (*_preferedCompositeAlphas)[i])
				{
					return (*_preferedCompositeAlphas)[i];
				}
			}
		}

		return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	}
	static VkPresentModeKHR GetVkPresentModeKHR(VkPhysicalDevice _physicalDevice, VkSurfaceKHR _surface, std::vector<VkPresentModeKHR>* _preferedPresentModes)
	{
		if (_preferedPresentModes != nullptr)
		{
			std::vector<VkPresentModeKHR> presentModes = VkU::GetVkPresentModeKHRs(_physicalDevice, _surface);

			for (size_t i = 0; i != _preferedPresentModes->size(); ++i)
			{
				for (size_t j = 0; j != presentModes.size(); ++j)
				{
					if (presentModes[j] == (*_preferedPresentModes)[i])
					{
						return (*_preferedPresentModes)[i];
					}
				}
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	// Queue
	static bool PickDeviceQueuesIndicesRecursively(std::vector<uint32_t>& _queueFamilyUseCount, std::vector<std::vector<uint32_t>> _deviceQueuesValidIndices, std::vector<VkQueueFamilyProperties> _queueFamilyProperties, std::vector<std::array<uint32_t, 3>>& _queueFamily_Indices_Count, size_t _depth)
	{
		if (_depth == _deviceQueuesValidIndices.size())
			return true; // nothing to left to assign, therefore we succeed

		for (size_t i = 0; i != _deviceQueuesValidIndices[_depth].size(); ++i)
		{
			if (_queueFamilyUseCount[_deviceQueuesValidIndices[_depth][i]] + _queueFamily_Indices_Count[_depth][2] <= _queueFamilyProperties[_deviceQueuesValidIndices[_depth][i]].queueCount)
			{

				uint32_t queueIndex = _queueFamilyUseCount[_deviceQueuesValidIndices[_depth][i]];
				uint32_t queueFamilyIndex = _deviceQueuesValidIndices[_depth][i];
				_queueFamilyUseCount[_deviceQueuesValidIndices[_depth][i]] += _queueFamily_Indices_Count[_depth][2];

				if (PickDeviceQueuesIndicesRecursively(_queueFamilyUseCount, _deviceQueuesValidIndices, _queueFamilyProperties, _queueFamily_Indices_Count, _depth + 1))
				{
					_queueFamily_Indices_Count[_depth][0] = queueFamilyIndex;
					_queueFamily_Indices_Count[_depth][1] = queueIndex;
					return true;
				}
				else
				{
					_queueFamilyUseCount[_deviceQueuesValidIndices[_depth][i]] -= _queueFamily_Indices_Count[_depth][2];
				}
			}
		}

		return false;
	}
	static bool CheckQueueFamilyIndexSupport(uint32_t _familyIndex, VkU::PhysicalDevice _physicalDevice, VkSurfaceKHR _surface, VkQueueFlags _flags, VkBool32 _presentability, uint32_t _count)
	{
		VkBool32 surfaceSupported = VK_FALSE;
		if (_surface != VK_NULL_HANDLE)
		{
			vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice.handle, _familyIndex, _surface, &surfaceSupported);
			if (surfaceSupported == VK_FALSE)
				return false;
		}

		if ((_flags & _physicalDevice.queueFamilyProperties[_familyIndex].queueFlags) != _flags)
			return false;

		if (_presentability == VK_TRUE && _physicalDevice.queueFamilyPresentable[_familyIndex] == VK_FALSE)
			return false;

		if (_count > _physicalDevice.queueFamilyProperties[_familyIndex].queueCount)
			return false;

		return true;
	}
	static std::vector<uint32_t> GetQueueFamilyIndicesWithSupport(Queue _deviceQueue, PhysicalDevice _physicalDevice, std::vector<Surface> _surfaces)
	{
		std::vector<uint32_t> indices;

		for (uint32_t i = 0; i != (uint32_t)_physicalDevice.queueFamilyProperties.size(); ++i)
		{
			bool allSurfacesValid = true;
			for (size_t s = 0; s != _surfaces.size(); ++s)
			{
				if (!CheckQueueFamilyIndexSupport(i, _physicalDevice, _surfaces[s].handle, _deviceQueue.flags, _deviceQueue.presentability, _deviceQueue.count))
				{
					allSurfacesValid = false;
					break;
				}
			}

			if (allSurfacesValid)
				indices.push_back(i);
		}

		return indices;
	}
	static std::vector<Queue> PickDeviceQueuesIndices(std::vector<Queue> _queues, VkU::PhysicalDevice _physicalDevice, std::vector<VkU::Surface> _surfaces, bool* _isCompatible)
	{
		std::vector<std::vector<uint32_t>> deviceQueuesValidIndices;

		// get possible indices
		for (size_t q = 0; q != _queues.size(); ++q)
		{
			std::vector<uint32_t> indices = GetQueueFamilyIndicesWithSupport(_queues[q], _physicalDevice, _surfaces);

			if (indices.size() == 0)
			{
				if (_isCompatible != nullptr)
					*_isCompatible = false;

				return _queues; // one queue cannot be represented
			}
			else
				deviceQueuesValidIndices.push_back(indices);
		}

		// assign indices
		std::vector<uint32_t> queueFamilyUseCount(_physicalDevice.queueFamilyProperties.size());
		std::vector<std::array<uint32_t, 3>> queueFamily_Indices_Count(_queues.size());
		for (size_t i = 0; i != queueFamily_Indices_Count.size(); ++i)
			queueFamily_Indices_Count[i][2] = _queues[i].count;

		*_isCompatible = PickDeviceQueuesIndicesRecursively(queueFamilyUseCount, deviceQueuesValidIndices, _physicalDevice.queueFamilyProperties, queueFamily_Indices_Count, 0);

		for (size_t q = 0; q != _queues.size(); ++q)
		{
			_queues[q].queueFamilyIndex = queueFamily_Indices_Count[q][0];
			_queues[q].queueIndex = queueFamily_Indices_Count[q][1];
		}

		return _queues;
	}

	// Device
	static VkDeviceQueueCreateInfo GetVkDeviceQueueCreateInfo(uint32_t _queueFamilyIndex, uint32_t _queueCount, float* _priority)
	{
		VkDeviceQueueCreateInfo deviceQueueCreateInfo;
		deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfo.pNext = nullptr;
		deviceQueueCreateInfo.flags = VK_RESERVED_FOR_FUTURE_USE;
		deviceQueueCreateInfo.queueFamilyIndex = _queueFamilyIndex;
		deviceQueueCreateInfo.queueCount = _queueCount;
		deviceQueueCreateInfo.pQueuePriorities = _priority;

		return deviceQueueCreateInfo;
	}
	static VkDeviceCreateInfo GetVkDeviceCreateInfo(uint32_t _queueCreateInfoCount, const VkDeviceQueueCreateInfo* _pQueueCreateInfos, uint32_t _enabledLayerCount, const char* const* _ppEnabledLayerNames, uint32_t _enabledExtensionCount, const char* const* _ppEnabledExtensionNames, const VkPhysicalDeviceFeatures* _pEnabledFeatures)
	{
		VkDeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = nullptr;
		deviceCreateInfo.flags = VK_RESERVED_FOR_FUTURE_USE;
		deviceCreateInfo.queueCreateInfoCount = _queueCreateInfoCount;
		deviceCreateInfo.pQueueCreateInfos = _pQueueCreateInfos;
		deviceCreateInfo.enabledLayerCount = _enabledLayerCount;
		deviceCreateInfo.ppEnabledLayerNames = _ppEnabledLayerNames;
		deviceCreateInfo.enabledExtensionCount = _enabledExtensionCount;
		deviceCreateInfo.ppEnabledExtensionNames = _ppEnabledExtensionNames;
		deviceCreateInfo.pEnabledFeatures = _pEnabledFeatures;

		return deviceCreateInfo;
	}

	// CommandPool
	static VkCommandPoolCreateInfo GetVkCommandPoolCreateInfo(VkCommandPoolCreateFlags _flags, uint32_t _queueFamilyIndex)
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo;
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.pNext = nullptr;
		commandPoolCreateInfo.flags = _flags;
		commandPoolCreateInfo.queueFamilyIndex = _queueFamilyIndex;

		return commandPoolCreateInfo;
	}

	// CommandBuffer
	static VkCommandBufferAllocateInfo GetVkCommandBufferAllocateInfo(VkCommandPool _commandPool, VkCommandBufferLevel _level, uint32_t _commandBufferCount)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.pNext = nullptr;
		commandBufferAllocateInfo.commandPool = _commandPool;
		commandBufferAllocateInfo.level = _level;
		commandBufferAllocateInfo.commandBufferCount = _commandBufferCount;

		return commandBufferAllocateInfo;
	}
	static VkCommandBufferBeginInfo GetVkCommandBufferBeginInfo(VkCommandBufferUsageFlags _flags)
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.pNext = nullptr;
		commandBufferBeginInfo.flags = _flags;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;

		return commandBufferBeginInfo;
	}
	static VkSubmitInfo GetVkSubmitInfo(uint32_t _waitSemaphoreCount, const VkSemaphore* _pWaitSemaphores, const VkPipelineStageFlags* _pWaitDstStageMask, uint32_t _commandBufferCount, const VkCommandBuffer* _pCommandBuffers, uint32_t _signalSemaphoreCount, const VkSemaphore* _pSignalSemaphores)
	{
		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = _waitSemaphoreCount;
		submitInfo.pWaitSemaphores = _pWaitSemaphores;
		submitInfo.pWaitDstStageMask = _pWaitDstStageMask;
		submitInfo.commandBufferCount = _commandBufferCount;
		submitInfo.pCommandBuffers = _pCommandBuffers;
		submitInfo.signalSemaphoreCount = _signalSemaphoreCount;
		submitInfo.pSignalSemaphores = _pSignalSemaphores;

		return submitInfo;
	}
	// renderPass
	static VkRenderPassBeginInfo GetVkRenderPassBeginInfo(VkRenderPass _renderPass, VkFramebuffer _framebuffer, VkOffset2D _renderAreaOffset2D, VkExtent2D _renderAreaExtent2D, uint32_t _clearValueCount, VkClearValue* _clearValue)
	{
		VkRenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = _renderPass;
		renderPassBeginInfo.framebuffer = _framebuffer;
		renderPassBeginInfo.renderArea.offset = _renderAreaOffset2D;
		renderPassBeginInfo.renderArea.extent = _renderAreaExtent2D;
		renderPassBeginInfo.clearValueCount = _clearValueCount;
		renderPassBeginInfo.pClearValues = _clearValue;

		return renderPassBeginInfo;;
	}

	// Swapchain
	static VkSwapchainCreateInfoKHR GetVkSwapchainCreateInfoKHR(VkPhysicalDevice _physicalDevice, VkU::Surface _surface, uint32_t* _targetSwapchainImageCount, VkExtent2D* _swapchainExtent)
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities = VkU::GetVkSurfaceCapabilitiesKHR(_physicalDevice, _surface.handle);

		if (*_targetSwapchainImageCount > surfaceCapabilities.maxImageCount)
			*_targetSwapchainImageCount = surfaceCapabilities.maxImageCount;
		else if (*_targetSwapchainImageCount < surfaceCapabilities.minImageCount)
			*_targetSwapchainImageCount = surfaceCapabilities.minImageCount;

		if (_swapchainExtent->width > surfaceCapabilities.maxImageExtent.width)
			_swapchainExtent->width = surfaceCapabilities.maxImageExtent.width;
		else if (_swapchainExtent->width < surfaceCapabilities.minImageExtent.width)
			_swapchainExtent->width = surfaceCapabilities.minImageExtent.width;
		if (_swapchainExtent->height > surfaceCapabilities.maxImageExtent.height)
			_swapchainExtent->height = surfaceCapabilities.maxImageExtent.height;
		else if (_swapchainExtent->height < surfaceCapabilities.minImageExtent.height)
			_swapchainExtent->height = surfaceCapabilities.minImageExtent.height;

		VkSwapchainCreateInfoKHR swapchainCreateInfoKHR;
		swapchainCreateInfoKHR.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfoKHR.pNext = nullptr;
		swapchainCreateInfoKHR.flags = VK_RESERVED_FOR_FUTURE_USE;
		swapchainCreateInfoKHR.surface = _surface.handle;
		swapchainCreateInfoKHR.minImageCount = *_targetSwapchainImageCount;
		swapchainCreateInfoKHR.imageFormat = _surface.colorFormat.format;
		swapchainCreateInfoKHR.imageColorSpace = _surface.colorFormat.colorSpace;
		swapchainCreateInfoKHR.imageExtent = *_swapchainExtent;
		swapchainCreateInfoKHR.imageArrayLayers = 1;
		swapchainCreateInfoKHR.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCreateInfoKHR.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfoKHR.queueFamilyIndexCount = 0;
		swapchainCreateInfoKHR.pQueueFamilyIndices = nullptr;
		swapchainCreateInfoKHR.preTransform = surfaceCapabilities.currentTransform;
		swapchainCreateInfoKHR.compositeAlpha = _surface.compositeAlpha;
		swapchainCreateInfoKHR.presentMode = _surface.presentMode;
		swapchainCreateInfoKHR.clipped = VK_TRUE;
		swapchainCreateInfoKHR.oldSwapchain = VK_NULL_HANDLE;

		return swapchainCreateInfoKHR;
	}

	// Image
	static VkImageCreateInfo GetVkImageCreateInfo(VkFormat _format, VkExtent3D _extent3D, VkImageUsageFlags _usage, VkImageTiling _tiling)
	{
		VkImageCreateInfo imageCreateInfo;
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.pNext = nullptr;
		imageCreateInfo.flags = 0;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = _format;
		imageCreateInfo.extent = _extent3D;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = _tiling;
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | _usage;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.queueFamilyIndexCount = 0;
		imageCreateInfo.pQueueFamilyIndices = nullptr;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		return imageCreateInfo;
	}
	static VkImageMemoryBarrier GetVkImageMemoryBarrier(VkAccessFlags _srcAccessMask, VkAccessFlags _dstAccessMask, VkImageLayout _oldLayout, VkImageLayout _newLayout, VkImageAspectFlags _aspectMask, VkImage _image)
	{
		VkImageMemoryBarrier stagingImageMemoryBarrier;
		stagingImageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		stagingImageMemoryBarrier.pNext = nullptr;
		stagingImageMemoryBarrier.srcAccessMask = _srcAccessMask;
		stagingImageMemoryBarrier.dstAccessMask = _dstAccessMask;
		stagingImageMemoryBarrier.oldLayout = _oldLayout;
		stagingImageMemoryBarrier.newLayout = _newLayout;
		stagingImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		stagingImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		stagingImageMemoryBarrier.image = _image;
		stagingImageMemoryBarrier.subresourceRange.aspectMask = _aspectMask;
		stagingImageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		stagingImageMemoryBarrier.subresourceRange.levelCount = 1;
		stagingImageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		stagingImageMemoryBarrier.subresourceRange.layerCount = 1;

		return stagingImageMemoryBarrier;
	}

	// ImageView
	static VkImageViewCreateInfo GetVkImageViewCreateInfo(VkFormat _format, VkImage _image, VkImageAspectFlags _aspectMask)
	{
		VkImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = nullptr;
		imageViewCreateInfo.flags = VK_RESERVED_FOR_FUTURE_USE;
		imageViewCreateInfo.image = _image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = _format;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = _aspectMask;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;

		return imageViewCreateInfo;
	}

	// Framebuffer
	static VkFramebufferCreateInfo GetVkFramebufferCreateInfo(VkRenderPass _renderPass, uint32_t _attachmentCount, const VkImageView* _pAttachments, uint32_t _width, uint32_t _height)
	{
		VkFramebufferCreateInfo framebufferCreateInfo;
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.pNext = nullptr;
		framebufferCreateInfo.flags = VK_RESERVED_FOR_FUTURE_USE;
		framebufferCreateInfo.renderPass = _renderPass;
		framebufferCreateInfo.attachmentCount = _attachmentCount;
		framebufferCreateInfo.pAttachments = _pAttachments;
		framebufferCreateInfo.width = _width;
		framebufferCreateInfo.height = _height;
		framebufferCreateInfo.layers = 1;

		return framebufferCreateInfo;
	}

	// Shader
	static VkShaderModuleCreateInfo GetVkShaderModuleCreateInfo(size_t _codeSize, const uint32_t* _pCode)
	{
		VkShaderModuleCreateInfo shaderModuleCreateInfo;
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.pNext = nullptr;
		shaderModuleCreateInfo.flags = VK_RESERVED_FOR_FUTURE_USE;
		shaderModuleCreateInfo.codeSize = _codeSize;
		shaderModuleCreateInfo.pCode = _pCode;

		return shaderModuleCreateInfo;
	}

	// Buffer
	static VkBufferCreateInfo GetVkBufferCreateInfo(VkDeviceSize _size, VkBufferUsageFlags _usage)
	{
		VkBufferCreateInfo bufferCreateInfo;
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.pNext = nullptr;
		bufferCreateInfo.flags = 0;
		bufferCreateInfo.size = _size;
		bufferCreateInfo.usage = _usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCreateInfo.queueFamilyIndexCount = 0;
		bufferCreateInfo.pQueueFamilyIndices = nullptr;

		return bufferCreateInfo;
	}
	static VkBufferCopy GetVkBufferCopy(VkDeviceSize _srcOffset, VkDeviceSize _dstOffset, VkDeviceSize _size)
	{
		VkBufferCopy copyRegion;
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = _size;

		return copyRegion;
	}

	// Memory
	static uint32_t FindMemoryTypeIndex(VkMemoryRequirements _memoryRequirements, PhysicalDevice _physicalDevice, VkMemoryPropertyFlags _memoryPropertyFlags)
	{
		for (uint32_t i = 0; i != _physicalDevice.memoryProperties.memoryTypeCount; ++i)
		{
			if ((_memoryRequirements.memoryTypeBits & (1 << i)) && (_physicalDevice.memoryProperties.memoryTypes[i].propertyFlags & _memoryPropertyFlags) == _memoryPropertyFlags)
			{
				return i;
			}
		}

		return -1;
	}
	static VkMemoryAllocateInfo GetVkMemoryAllocateInfo(VkMemoryRequirements _memoryRequirements, PhysicalDevice _physicalDevice, VkMemoryPropertyFlags _memoryPropertyFlags)
	{
		VkMemoryAllocateInfo memoryAllocateInfo;
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.pNext = nullptr;
		memoryAllocateInfo.allocationSize = _memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = FindMemoryTypeIndex(_memoryRequirements, _physicalDevice, _memoryPropertyFlags);

		return memoryAllocateInfo;
	}

	// other
	
	// Vertices
	enum VERTEX_TYPE
	{
		UNKNOWN = -1,

		P2 = 0,
		P2U = 2,
		P2N = 4,
		P2UN = 6,
		P2NTB = 12,
		P2UNTB = 14,
		P2S4 = 16,
		P2US4 = 18,
		P2NS4 = 20,
		P2UNS4 = 22,
		P2NTBS4 = 28,
		P2UNTBS4 = 30,

		P3 = 1,
		P3U = 3,
		P3N = 5,
		P3UN = 7,
		P3UNTB = 15,
		P3S4 = 17,
		P3US4 = 19,
		P3NS4 = 21,
		P3UNS4 = 23,
		P3UNTBS4 = 31,
	};

	struct VertexP2U
	{
		float position[2];
		float uv[2];

		static inline VkVertexInputBindingDescription GetVkVertexInputBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription;
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(VertexP2U);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}
		static inline std::vector<VkVertexInputAttributeDescription> GetVkVertexInputAttributeDescription()
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescription(2);
			attributeDescription[0].location = 0;
			attributeDescription[0].binding = 0;
			attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[0].offset = offsetof(VertexP2U, position);

			attributeDescription[1].location = 1;
			attributeDescription[1].binding = 0;
			attributeDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescription[1].offset = offsetof(VertexP2U, uv);

			return attributeDescription;
		}
	};
	struct VertexP2C3
	{
		float position[2];
		float color[3];

		static inline VkVertexInputBindingDescription GetVkVertexInputBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription;
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(VertexP2C3);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}
		static inline std::vector<VkVertexInputAttributeDescription> GetVkVertexInputAttributeDescription()
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescription(2);
			attributeDescription[0].location = 0;
			attributeDescription[0].binding = 0;
			attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[0].offset = offsetof(VertexP2C3, position);

			attributeDescription[1].location = 1;
			attributeDescription[1].binding = 0;
			attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[1].offset = offsetof(VertexP2C3, color);

			return attributeDescription;
		}
	};
	struct VertexP3U
	{
		float position[3];
		float uv[2];

		static inline VkVertexInputBindingDescription GetVkVertexInputBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription;
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(VertexP3U);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}
		static inline std::vector<VkVertexInputAttributeDescription> GetVkVertexInputAttributeDescription()
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescription(2);
			attributeDescription[0].location = 0;
			attributeDescription[0].binding = 0;
			attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[0].offset = offsetof(VertexP3U, position);

			attributeDescription[1].location = 1;
			attributeDescription[1].binding = 0;
			attributeDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescription[1].offset = offsetof(VertexP3U, uv);

			return attributeDescription;
		}
	};
	struct VertexP3UNTB
	{
		float position[3];
		float uv[2];
		float normal[3];
		float tangent[3];
		float bitangent[3];

		static inline VkVertexInputBindingDescription GetVkVertexInputBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription;
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(VertexP3UNTB);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}
		static inline std::vector<VkVertexInputAttributeDescription> GetVkVertexInputAttributeDescription()
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescription(5);
			attributeDescription[0].location = 0;
			attributeDescription[0].binding = 0;
			attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[0].offset = offsetof(VertexP3UNTB, position);

			attributeDescription[1].location = 1;
			attributeDescription[1].binding = 0;
			attributeDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescription[1].offset = offsetof(VertexP3UNTB, uv);

			attributeDescription[2].location = 2;
			attributeDescription[2].binding = 0;
			attributeDescription[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[2].offset = offsetof(VertexP3UNTB, normal);

			attributeDescription[3].location = 3;
			attributeDescription[3].binding = 0;
			attributeDescription[3].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[3].offset = offsetof(VertexP3UNTB, tangent);

			attributeDescription[4].location = 4;
			attributeDescription[4].binding = 0;
			attributeDescription[4].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[4].offset = offsetof(VertexP3UNTB, bitangent);

			return attributeDescription;
		}
	};
	struct VertexP3UNTBS4
	{
		float position[3];
		float uv[2];
		float normal[3];
		float tangent[3];
		float bitangent[3];
		float boneWieghts[4];
		uint16_t boneIDs[4];

		static inline VkVertexInputBindingDescription GetVkVertexInputBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription;
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(VertexP3UNTBS4);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}
		static inline std::vector<VkVertexInputAttributeDescription> GetVkVertexInputAttributeDescription()
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescription(7);
			attributeDescription[0].location = 0;
			attributeDescription[0].binding = 0;
			attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[0].offset = offsetof(VertexP3UNTBS4, position);

			attributeDescription[1].location = 1;
			attributeDescription[1].binding = 0;
			attributeDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescription[1].offset = offsetof(VertexP3UNTBS4, uv);

			attributeDescription[2].location = 2;
			attributeDescription[2].binding = 0;
			attributeDescription[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[2].offset = offsetof(VertexP3UNTBS4, normal);

			attributeDescription[3].location = 3;
			attributeDescription[3].binding = 0;
			attributeDescription[3].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[3].offset = offsetof(VertexP3UNTBS4, tangent);

			attributeDescription[4].location = 4;
			attributeDescription[4].binding = 0;
			attributeDescription[4].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[4].offset = offsetof(VertexP3UNTBS4, bitangent);

			attributeDescription[5].location = 5;
			attributeDescription[5].binding = 0;
			attributeDescription[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescription[5].offset = offsetof(VertexP3UNTBS4, boneWieghts);

			attributeDescription[6].location = 6;
			attributeDescription[6].binding = 0;
			attributeDescription[6].format = VK_FORMAT_R16G16B16A16_UINT;
			attributeDescription[6].offset = offsetof(VertexP3UNTBS4, boneIDs);

			return attributeDescription;
		}
	};
}

#endif