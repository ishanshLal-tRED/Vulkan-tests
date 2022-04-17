#include <logging.hxx>;

import MainApplication.MyApp;

import <vector>;
import <array>;
import <set>;
#include <limits>;

import <vulkan/vulkan.h>;
import Helpers.Vk;
import Helpers.GLFW;
import Helpers.Files;

static std::array<const char*, 1> s_DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static std::array<const char*, 1> s_ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

constexpr bool g_EnableValidationLayers = ( _DEBUG ? true : false );

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT,VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT*, void*);

bool is_vk_device_suitable (VkPhysicalDevice &device, void* surface_ptr);

struct QueueFamilyIndices {
	QueueFamilyIndices (VkPhysicalDevice &device, VkSurfaceKHR &surface) {
		// Find required queue families
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
		for (int i = 0; auto &queue_family: queue_families) {
			if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				this->GraphicsFamily = i;
			VkBool32 presentation_support_extension = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentation_support_extension);
			if (presentation_support_extension)
				this->PresentationFamily = i;
			if (this->AreAllFamiliesPresent()) break;
			++i;
		}
	}
	std::optional<uint32_t> GraphicsFamily;
	std::optional<uint32_t> PresentationFamily;
	inline bool AreAllFamiliesPresent() const {
		return true 
			&& GraphicsFamily.has_value ()
			&& PresentationFamily.has_value ()
	;}
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice &device, VkSurfaceKHR &surface);

void MyApp::Instance::initializeVk () {
	LOG_trace(__FUNCSIG__); 

	Helper::createInstance(m_Context.Vk.MainInstance, m_Context.Vk.DebugMessenger
		, VkApplicationInfo {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Hello Triangle",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "No Engine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_0}, s_ValidationLayers, vk_debug_callback);

	// Window surface creation // Platform specific way
	Helper::createSurface(m_Context.Vk.Surface, m_Context.Vk.MainInstance, Helper::Win32WindowHandleFromGLFW(m_Context.MainWindow));

	// Phisical device
	m_Context.Vk.PhysicalDevice = Helper::pickPhysicalDevice(m_Context.Vk.MainInstance, is_vk_device_suitable, &m_Context.Vk.Surface);
	
	// Phisical device & Queues
	const QueueFamilyIndices indices (m_Context.Vk.PhysicalDevice, m_Context.Vk.Surface);

	Helper::createLogicalDevice(m_Context.Vk.LogicalDevice, m_Context.Vk.PhysicalDevice
		, std::vector<std::pair<VkQueue*, uint32_t>>{{&m_Context.Vk.Queues.Graphics, indices.GraphicsFamily.value ()}, {&m_Context.Vk.Queues.Presentation, indices.PresentationFamily.value ()}}
		, s_ValidationLayers, s_DeviceExtensions);

	// Swapchain related data extraction
	VkExtent2D extent_selected;
	VkSurfaceFormatKHR format_selected;
	VkPresentModeKHR present_mode_selected;
	SwapChainSupportDetails swap_chain_support = query_swap_chain_support(m_Context.Vk.PhysicalDevice, m_Context.Vk.Surface); {
		// choose swap surface format
		for (const auto& format: swap_chain_support.formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				format_selected = format;
				break;
			} else format_selected = swap_chain_support.formats[0]; // else
		}
		// choose swap presentation mode
		for (const auto& present_mode: swap_chain_support.presentModes) {
			if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				present_mode_selected = VK_PRESENT_MODE_MAILBOX_KHR;
				break;
			}
			present_mode_selected = VK_PRESENT_MODE_FIFO_KHR; // else
		}
		if (swap_chain_support.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			extent_selected = swap_chain_support.capabilities.currentExtent;
		} else {
			auto [width, height] = Helper::GLFWGetFrameBufferSize (m_Context.MainWindow);

			extent_selected = VkExtent2D {
				.width  = std::clamp (width , swap_chain_support.capabilities.minImageExtent.width , swap_chain_support.capabilities.maxImageExtent.width),
				.height = std::clamp (height, swap_chain_support.capabilities.minImageExtent.height, swap_chain_support.capabilities.maxImageExtent.height)
			};
		}

		m_Context.Vk.SwapchainImageExtent = extent_selected;
		m_Context.Vk.SwapchainImageFormat = format_selected.format;
	};
	
	// Swapchain
	Helper::createSwapChain (m_Context.Vk.Swapchain, m_Context.Vk.SwapchainImages
		, m_Context.Vk.Surface, m_Context.Vk.LogicalDevice
		, swap_chain_support.capabilities, m_Context.Vk.SwapchainImageExtent, format_selected, present_mode_selected
		, indices.GraphicsFamily.value (), indices.PresentationFamily.value ());

	Helper::createImageViews(m_Context.Vk.SwapchainImagesView, m_Context.Vk.LogicalDevice, m_Context.Vk.SwapchainImageFormat, m_Context.Vk.SwapchainImages);

	{
		VkAttachmentDescription color_attachment {
			.format = m_Context.Vk.SwapchainImageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};

		VkAttachmentReference color_attachment_ref {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		VkSubpassDescription subpass {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_ref
		};
		
		std::array<VkSubpassDescription, 1> subpasses = {subpass};
		Helper::createRenderPasses (m_Context.Vk.RenderPass
			,m_Context.Vk.LogicalDevice, color_attachment
			,subpasses);
	}

	{ // Create Graphics Pipeline
		m_Context.Vk.Modules.Vertex   = Helper::createShaderModule (m_Context.Vk.LogicalDevice, 
				Helper::readFile (std::string() + PROJECT_ROOT_LOCATION + "/assets/hardcoded_triangle/vert.sprv"));
		m_Context.Vk.Modules.Fragment = Helper::createShaderModule (m_Context.Vk.LogicalDevice, 
				Helper::readFile (std::string() + PROJECT_ROOT_LOCATION + "/assets/hardcoded_triangle/frag.sprv"));

		VkPipelineShaderStageCreateInfo vert_shader_stage_info{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = m_Context.Vk.Modules.Vertex,
			.pName = "main"
		};
		VkPipelineShaderStageCreateInfo frag_shader_stage_info{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = m_Context.Vk.Modules.Fragment,
			.pName = "main"
		};
		VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};
		
		VkPipelineVertexInputStateCreateInfo vertex_input_info {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 0,
			.pVertexBindingDescriptions = nullptr, // Optional
			.vertexAttributeDescriptionCount = 0,
			.pVertexAttributeDescriptions = nullptr // Optional
		};
		VkPipelineInputAssemblyStateCreateInfo input_assembly_info {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		};

		auto dynamic_states = std::array<VkDynamicState, 0> { // this can be of zero size
		#if 0
		    VK_DYNAMIC_STATE_VIEWPORT,
		    VK_DYNAMIC_STATE_LINE_WIDTH
		#endif
		};
		
		Helper::createGraphicsPipelineDefaultSize<2> (m_Context.Vk.PipelineLayout, m_Context.Vk.GraphicsPipeline
			,m_Context.Vk.LogicalDevice, m_Context.Vk.RenderPass, shader_stages
			,vertex_input_info, input_assembly_info
			,m_Context.Vk.SwapchainImageExtent, dynamic_states);
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

	vkDestroyPipeline(m_Context.Vk.LogicalDevice, m_Context.Vk.GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_Context.Vk.LogicalDevice, m_Context.Vk.PipelineLayout, nullptr);

	vkDestroyRenderPass(m_Context.Vk.LogicalDevice, m_Context.Vk.RenderPass, nullptr);

	vkDestroyShaderModule (m_Context.Vk.LogicalDevice, m_Context.Vk.Modules.Vertex, nullptr);
	vkDestroyShaderModule (m_Context.Vk.LogicalDevice, m_Context.Vk.Modules.Fragment, nullptr);
	
	vkDestroySwapchainKHR (m_Context.Vk.LogicalDevice, m_Context.Vk.Swapchain, nullptr);

	vkDestroyDevice (m_Context.Vk.LogicalDevice, nullptr);
	
	vkDestroySurfaceKHR (m_Context.Vk.MainInstance, m_Context.Vk.Surface, nullptr);

	vkDestroyInstance (m_Context.Vk.MainInstance, nullptr);
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

bool is_vk_device_suitable (VkPhysicalDevice &device, void* surface_ptr) {
	VkSurfaceKHR &surface = *(VkSurfaceKHR*)(surface_ptr);
	// We culd also implement a rating system for selecting most powerful GPU in system

	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;

	vkGetPhysicalDeviceProperties (device, &device_properties);
	vkGetPhysicalDeviceFeatures (device, &device_features);

	const QueueFamilyIndices indices (device, surface);

	auto check_device_extensions_support = [](VkPhysicalDevice &device) {
			uint32_t extension_count;
			vkEnumerateDeviceExtensionProperties (device, nullptr, &extension_count, nullptr);
			std::vector<VkExtensionProperties> properties(extension_count);
			vkEnumerateDeviceExtensionProperties (device, nullptr, &extension_count, properties.data());
			
			std::set<std::string> required_extensions (s_DeviceExtensions.begin(), s_DeviceExtensions.end());
		
			for(auto& extension: properties) {
				required_extensions.erase(extension.extensionName);
			}
			return required_extensions.empty();
		};

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
