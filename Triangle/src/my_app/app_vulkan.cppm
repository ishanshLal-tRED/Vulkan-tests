#include <logging.hxx>;

import MainApplication.MyApp;

import <vector>;
import <array>;
import <set>;
#include <limits>;

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>;

import <GLFW/glfw3.h>;
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>;

#undef max
#undef min

static constexpr std::array<const char*, 1> s_DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static constexpr std::array<const char*, 1> s_ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
constexpr bool g_EnableValidationLayers = ( _DEBUG ? true : false );

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT,VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT*,void*);

bool is_vk_device_suitable (VkPhysicalDevice &device, VkSurfaceKHR &surface);

struct QueueFamilyIndices {
	std::optional<uint32_t> GraphicsFamily;
	std::optional<uint32_t> PresentationFamily;
	inline bool AreAllFamiliesPresent() {
		return true 
			&& GraphicsFamily.has_value ()
			&& PresentationFamily.has_value ()
	;}
};
QueueFamilyIndices find_queue_families (VkPhysicalDevice &device, VkSurfaceKHR &surface);

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice &device, VkSurfaceKHR &surface);

bool check_device_extensions_support(VkPhysicalDevice &device);

void MyApp::Instance::initializeVk () {
	LOG_trace(__FUNCSIG__); 

	VkApplicationInfo app_info {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0
	};

	std::vector<const char*> vk_extensions_to_enable;

	std::span<const char*> glfwExtensions; {
		uint32_t glfwExtensionCount = 0;
		glfwExtensions = {glfwGetRequiredInstanceExtensions(&glfwExtensionCount), glfwExtensionCount};
	}
	vk_extensions_to_enable.insert(vk_extensions_to_enable.end(), glfwExtensions.begin(), glfwExtensions.end());
	if (g_EnableValidationLayers) {
		vk_extensions_to_enable.push_back (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	{ // Vk Extensions
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data ());
		
		LOG_trace("available extensions:");
		for (const auto& extension : extensions)
		    LOG_raw("\n\t {}", extension.extensionName);
		
		LOG_trace("{} extensions supported", extensionCount);
	}
	
	{ // Check validation layer support
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		std::vector<VkLayerProperties> validation_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, validation_layers.data ());

		std::vector<const char*> unavailable_layers;
		for (const char* layer_name : s_ValidationLayers) {
		    bool layerFound = false;
		
		    for (const auto& layer_properties : validation_layers) {
		        if (strcmp(layer_name, layer_properties.layerName) == 0) {
		            layerFound = true;
		            break;
		        }
		    }
		    if (!layerFound) {
		        unavailable_layers.push_back (layer_name);
		    }
		}
		if (!unavailable_layers.empty ()) {
			LOG_error("Missing Layer(s):");
			for (const char* extension_name: unavailable_layers)
				LOG_raw("\n\t {}", extension_name);
			THROW_CORE_Critical("{:d} layer(s) not found", unavailable_layers.size ());
		}
	}

	VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info { // Reusing later when creating debug messenger
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = vk_debug_callback,
		.pUserData = nullptr // Optional
	};

	VkInstanceCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		//.pNext = (g_EnableValidationLayers ? (VkDebugUtilsMessengerCreateInfoEXT*) &debug_messenger_create_info : nullptr),
		//// Disabling due to non fata error, Refer: http://disq.us/p/2o823o8
		.pApplicationInfo = &app_info,
		.enabledLayerCount = (g_EnableValidationLayers ? s_ValidationLayers.size () : 0),
		.ppEnabledLayerNames = (g_EnableValidationLayers ? s_ValidationLayers.data () : nullptr),
		.enabledExtensionCount = uint32_t(vk_extensions_to_enable.size()),
		.ppEnabledExtensionNames = vk_extensions_to_enable.data()
	};

	if (vkCreateInstance(&create_info, nullptr, &m_Context.Vk.MainInstance) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create instance!");
	}

	if (g_EnableValidationLayers) { // Create Debug Utils Messenger

		VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_Context.Vk.MainInstance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
		    result = func(m_Context.Vk.MainInstance, &debug_messenger_create_info, nullptr, &m_Context.Vk.DebugMessenger);
		}
		if (result != VK_SUCCESS)
			THROW_CORE_Critical("failed setting up vk_debug_messenger");
	}

	{ // Window surface creation // Platform specific way
		VkWin32SurfaceCreateInfoKHR surface_info{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = GetModuleHandle(nullptr),
			.hwnd = glfwGetWin32Window(m_Context.MainWindow)
		};

		if (vkCreateWin32SurfaceKHR (m_Context.Vk.MainInstance, &surface_info, nullptr, &m_Context.Vk.Surface) != VK_SUCCESS) 
			THROW_CORE_Critical("failed to create window surface");
	}	

	// Phisical device
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(m_Context.Vk.MainInstance, &device_count, nullptr);
	if (device_count == 0)
		THROW_CORE_Critical("failed to find hardware with vulkan support");
	std::vector<VkPhysicalDevice> vk_enabled_devices (device_count);
	vkEnumeratePhysicalDevices(m_Context.Vk.MainInstance, &device_count, vk_enabled_devices.data ());
	for (auto device: vk_enabled_devices){
		if (is_vk_device_suitable (device, m_Context.Vk.Surface)) {
			m_Context.Vk.PhysicalDevice = device;
			break;
		}
	}
	
	if (m_Context.Vk.PhysicalDevice == VK_NULL_HANDLE)
		THROW_CORE_Critical("failed to find hardware with vulkan support");
	
	{ // Specify queues
		QueueFamilyIndices indices = find_queue_families (m_Context.Vk.PhysicalDevice, m_Context.Vk.Surface);
		
		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queue_families = {indices.GraphicsFamily.value(), indices.PresentationFamily.value()};

		float queue_priority = 1.0f;
		for (uint32_t queue_family : unique_queue_families) {
		    queue_create_infos.push_back(VkDeviceQueueCreateInfo {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = queue_family,
				.queueCount = 1,
				.pQueuePriorities = &queue_priority
			});
		}

		VkPhysicalDeviceFeatures device_features{};
		vkGetPhysicalDeviceFeatures (m_Context.Vk.PhysicalDevice, &device_features);
		
		VkDeviceCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = uint32_t(queue_create_infos.size ()),
			.pQueueCreateInfos = queue_create_infos.data (),
			.enabledLayerCount = ( g_EnableValidationLayers ? s_ValidationLayers.size ():0 ),
			.ppEnabledLayerNames = ( g_EnableValidationLayers ? s_ValidationLayers.data ():nullptr ),
			.enabledExtensionCount = uint32_t(s_DeviceExtensions.size ()),
			.ppEnabledExtensionNames = s_DeviceExtensions.data (),
			.pEnabledFeatures = &device_features
		};
		
		if (vkCreateDevice (m_Context.Vk.PhysicalDevice, &create_info, nullptr, &m_Context.Vk.LogicalDevice) != VK_SUCCESS) 
			THROW_CORE_Critical ("Failed to vreate logical device");

		vkGetDeviceQueue (m_Context.Vk.LogicalDevice, indices.GraphicsFamily.value (), 0, &m_Context.Vk.Queues.Graphics);
		vkGetDeviceQueue (m_Context.Vk.LogicalDevice, indices.PresentationFamily.value (), 0, &m_Context.Vk.Queues.Presentation);

		VkExtent2D extent_selected;
		VkSurfaceFormatKHR format_selected;
		VkPresentModeKHR present_mode_selected;
		SwapChainSupportDetails swap_chain_support = query_swap_chain_support(m_Context.Vk.PhysicalDevice, m_Context.Vk.Surface); {
			// choose swap surface format
			for (const auto& format: swap_chain_support.formats) {
				if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					format_selected = format;
					break;
				}
				format_selected = swap_chain_support.formats[0]; // else
			}
			// choose swap presentation mode
			for (const auto& present_mode: swap_chain_support.presentModes) {
				if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
					present_mode_selected = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
				present_mode_selected = VK_PRESENT_MODE_FIFO_KHR; // else
			}
		};

		auto tmp1 = swap_chain_support.capabilities.currentExtent.width;
		auto tmp2 = std::numeric_limits<uint32_t>::max();

		if (tmp1 != tmp2) {
			extent_selected = swap_chain_support.capabilities.currentExtent;
		} else {
			int width, height;
			glfwGetFramebufferSize (m_Context.MainWindow, &width, &height);

			extent_selected = VkExtent2D {
				.width  = std::clamp (uint32_t(width) , swap_chain_support.capabilities.minImageExtent.width , swap_chain_support.capabilities.maxImageExtent.width),
				.height = std::clamp (uint32_t(height), swap_chain_support.capabilities.minImageExtent.height, swap_chain_support.capabilities.maxImageExtent.height)
			};
		}
		uint32_t image_count = swap_chain_support.capabilities.maxImageCount;
		image_count = image_count == 0 ? swap_chain_support.capabilities.minImageCount + 1 : image_count;
		VkSwapchainCreateInfoKHR swapchain_create_info {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = m_Context.Vk.Surface,
			.minImageCount = image_count,
			.imageFormat = format_selected.format,
			.imageColorSpace = format_selected.colorSpace,
			.imageExtent = extent_selected,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,

			.preTransform = swap_chain_support.capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = present_mode_selected,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE
		};
		if (indices.GraphicsFamily.value() != indices.PresentationFamily.value()) {
			swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;

			uint32_t queue_family_indices[] = {indices.GraphicsFamily.value(), indices.PresentationFamily.value()};
			swapchain_create_info.queueFamilyIndexCount = uint32_t(std::size(queue_family_indices));
			swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
		} else {
			swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchain_create_info.queueFamilyIndexCount = 0;
			swapchain_create_info.pQueueFamilyIndices = nullptr;
		}
		if (vkCreateSwapchainKHR (m_Context.Vk.LogicalDevice, &swapchain_create_info, nullptr, &m_Context.Vk.Swapchain) != VK_SUCCESS)
			THROW_Critical("failed to create swapchain !");

		uint32_t swapchain_images_count;
		vkGetSwapchainImagesKHR(m_Context.Vk.LogicalDevice, m_Context.Vk.Swapchain, &swapchain_images_count, nullptr);
		m_Context.Vk.SwapchainImages.resize(swapchain_images_count);
		vkGetSwapchainImagesKHR(m_Context.Vk.LogicalDevice, m_Context.Vk.Swapchain, &swapchain_images_count, m_Context.Vk.SwapchainImages.data());
		m_Context.Vk.SwapchainImageFormat = format_selected.format;
		m_Context.Vk.SwapchainImageExtent = extent_selected;
	}

	{ // create image views
		m_Context.Vk.SwapchainImagesView.resize(m_Context.Vk.SwapchainImages.size());
		for (size_t i = 0; i < m_Context.Vk.SwapchainImages.size (); ++i) {
			VkImageViewCreateInfo create_info {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = m_Context.Vk.SwapchainImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = m_Context.Vk.SwapchainImageFormat,
				.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
				.subresourceRange = VkImageSubresourceRange {
					    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					    .baseMipLevel = 0,
					    .levelCount = 1,
					    .baseArrayLayer = 0,
					    .layerCount = 1
				}
			};

			if (vkCreateImageView (m_Context.Vk.LogicalDevice, &create_info, nullptr, &m_Context.Vk.SwapchainImagesView[i]) != VK_SUCCESS)
				THROW_Critical ("failed to create imagfe views!");
		}
	}
}

void MyApp::Instance::render (double latency) {
	LOG_trace("{:s} {:f}", __FUNCSIG__, latency);
}

void MyApp::Instance::terminateVk () {
	LOG_trace ("{:s}", __FUNCSIG__);

	if (g_EnableValidationLayers) { // Destroy Debug Utils Messenger
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_Context.Vk.MainInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
		    func(m_Context.Vk.MainInstance, m_Context.Vk.DebugMessenger, nullptr);
		}
	}

	for (auto &image_view: m_Context.Vk.SwapchainImagesView) 
		vkDestroyImageView (m_Context.Vk.LogicalDevice, image_view, nullptr);
	
	vkDestroySwapchainKHR(m_Context.Vk.LogicalDevice, m_Context.Vk.Swapchain, nullptr);

	vkDestroyDevice(m_Context.Vk.LogicalDevice, nullptr);
	
	vkDestroySurfaceKHR(m_Context.Vk.MainInstance, m_Context.Vk.Surface, nullptr);

	vkDestroyInstance(m_Context.Vk.MainInstance, nullptr);
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback (VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* pUserData) 
{
	std::string msg_type;
	if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
		//if (!msg_type.empty())
		//	msg_type += ", ";
		msg_type += "VkGeneral";
	} 
	if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
		if (!msg_type.empty())
			msg_type += ", ";
		msg_type += "VkPerformance";
	} 
	if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
		if (!msg_type.empty())
			msg_type += ", ";
		msg_type += "VkValidation";
	}
	if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		LOG_error("{:s}: {:s}", msg_type, callback_data->pMessage); __debugbreak();
	} else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		LOG_warn("{:s}: {:s}", msg_type, callback_data->pMessage);
	} else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		LOG_info("{:s}: {:s}", msg_type, callback_data->pMessage);
	} else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		LOG_debug("{:s}: {:s}", msg_type, callback_data->pMessage);
	} else {
		LOG_trace("{:s}: {:s}", msg_type, callback_data->pMessage);
	}
	return VK_FALSE;
};

bool is_vk_device_suitable (VkPhysicalDevice &device, VkSurfaceKHR &surface) {
	// We culd also implement a rating system for selecting most powerful GPU in system

	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;

	vkGetPhysicalDeviceProperties (device, &device_properties);
	vkGetPhysicalDeviceFeatures (device, &device_features);

	QueueFamilyIndices indices = find_queue_families (device, surface);

	bool extensions_supported = check_device_extensions_support(device);
	bool swap_chain_adequate = false;
	if (extensions_supported) {
		SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device,surface);
		swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.presentModes.empty();
	};

	LOG_info("Testing device for suitability: {:s}", device_properties.deviceName);
	return true
		&&	device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
		&&	device_features.geometryShader
		&&	indices.AreAllFamiliesPresent()
		&&	extensions_supported
		&&	swap_chain_adequate
	;
}

QueueFamilyIndices find_queue_families (VkPhysicalDevice &device, VkSurfaceKHR &surface) { 
	// Find required queue families
	QueueFamilyIndices indices;
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
	for (int i = 0; auto &queue_family: queue_families) {
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.GraphicsFamily = i;
		VkBool32 presentation_support_extension = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentation_support_extension);
		if (presentation_support_extension)
			indices.PresentationFamily = i;
		if (indices.AreAllFamiliesPresent()) break;
		++i;
	}
	return indices;
}

bool check_device_extensions_support(VkPhysicalDevice &device) {
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties (device, nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> properties(extension_count);
	vkEnumerateDeviceExtensionProperties (device, nullptr, &extension_count, properties.data());
	
	std::set<std::string> required_extensions (s_DeviceExtensions.begin(), s_DeviceExtensions.end());

	for(auto& extension: properties) {
		required_extensions.erase(extension.extensionName);
	}
	return required_extensions.empty();
}

SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice &device, VkSurfaceKHR &surface) {
    SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
	
	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR (device, surface, &format_count, nullptr);
	if (format_count != 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR (device, surface, &format_count, details.formats.data());
	}
	
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface, &present_mode_count, nullptr);
	if (present_mode_count != 0) {
		details.presentModes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface, &present_mode_count, details.presentModes.data());
	}

    return details;
}
