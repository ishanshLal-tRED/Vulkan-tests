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

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback (
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
	const QueueFamilyIndices queue_family_indices (m_Context.Vk.PhysicalDevice, m_Context.Vk.Surface);
	Helper::createLogicalDevice(m_Context.Vk.LogicalDevice, m_Context.Vk.PhysicalDevice
		, std::vector<std::pair<VkQueue*, uint32_t>>{{&m_Context.Vk.Queues.Graphics, queue_family_indices.GraphicsFamily.value ()}, {&m_Context.Vk.Queues.Presentation, queue_family_indices.PresentationFamily.value ()}}
		, s_ValidationLayers, s_DeviceExtensions);

	// Create shader modules	
	m_Context.Vk.Modules.Vertex   = Helper::createShaderModule (m_Context.Vk.LogicalDevice, 
			Helper::readFile (std::string() + PROJECT_ROOT_LOCATION + "/assets/hardcoded_triangle/vert.sprv"));
	m_Context.Vk.Modules.Fragment = Helper::createShaderModule (m_Context.Vk.LogicalDevice, 
			Helper::readFile (std::string() + PROJECT_ROOT_LOCATION + "/assets/hardcoded_triangle/frag.sprv"));

	createSwapchainAndRelated ();

	// Create command pool & buffer
	Helper::createCommandPoolAndBuffer (m_Context.Vk.CommandPool, m_Context.Vk.CommandBuffers
		,m_Context.Vk.LogicalDevice, queue_family_indices.GraphicsFamily.value ());

	// Create syncronisation objects
	VkSemaphoreCreateInfo semaphore_info { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fence_info { 
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if ( vkCreateSemaphore (m_Context.Vk.LogicalDevice, &semaphore_info, nullptr, &m_Context.Vk.ImageAvailableSemaphores[i]) != VK_SUCCESS
		  || vkCreateSemaphore (m_Context.Vk.LogicalDevice, &semaphore_info, nullptr, &m_Context.Vk.RenderFinishedSemaphores[i]) != VK_SUCCESS
		  || vkCreateFence (m_Context.Vk.LogicalDevice, &fence_info, nullptr, &m_Context.Vk.InFlightFences[i]) != VK_SUCCESS)
			THROW_Critical ("failed to create sync locks!");
	}
}

void MyApp::Instance::render (double latency) {
	LOG_trace("{:s} {:f}", __FUNCSIG__, latency);
	const uint32_t current_frame_flight = m_CurrentFrame++ % MAX_FRAMES_IN_FLIGHT;

	vkWaitForFences (m_Context.Vk.LogicalDevice, 1, &m_Context.Vk.InFlightFences[current_frame_flight], VK_TRUE, UINT64_MAX);
	
	auto record_command_buffer = [](VkCommandBuffer& command_buffer, VkPipeline &graphics_pipeline, VkRenderPass& render_pass, VkFramebuffer &framebuffer/*uint32_t image_index*/, VkExtent2D extent) {
		VkCommandBufferBeginInfo begin_info {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0, // Optional
			.pInheritanceInfo = nullptr // Optional
		};

		if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
		    THROW_Critical("failed to begin recording command buffer!");
		}
		
		VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

		VkRenderPassBeginInfo render_pass_info{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = render_pass,
			.framebuffer = framebuffer,
			.renderArea = {{0, 0}, extent},
			.clearValueCount = 1,
			.pClearValues = &clear_color
		};
		
		vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
		vkCmdDraw(command_buffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(command_buffer);

		if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
		    THROW_Critical("failed to record command buffer!");
	};
	
	uint32_t image_index;
    switch(vkAcquireNextImageKHR(m_Context.Vk.LogicalDevice, m_Context.Vk.Swapchain, UINT64_MAX, m_Context.Vk.ImageAvailableSemaphores[current_frame_flight], VK_NULL_HANDLE, &image_index)) {
		case VK_SUCCESS: break;
		case VK_ERROR_OUT_OF_DATE_KHR:			
		case VK_SUBOPTIMAL_KHR: // Recreate swapchain and related
			recreateSwapchainAndRelated ();
			return;
		default:
			THROW_Critical ("failed to acquire swapchain image");
	}
	
	vkResetFences (m_Context.Vk.LogicalDevice, 1, &m_Context.Vk.InFlightFences[current_frame_flight]);

	vkResetCommandBuffer(m_Context.Vk.CommandBuffers[current_frame_flight], 0);
	record_command_buffer(m_Context.Vk.CommandBuffers[current_frame_flight], m_Context.Vk.GraphicsPipeline, m_Context.Vk.RenderPass, m_Context.Vk.SwapchainFramebuffers[image_index], m_Context.Vk.SwapchainImageExtent);

	VkSemaphore wait_semaphores[] = {m_Context.Vk.ImageAvailableSemaphores[current_frame_flight]};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore signal_semaphores[] = {m_Context.Vk.RenderFinishedSemaphores[current_frame_flight]};
	VkSubmitInfo submitInfo {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = wait_semaphores,
		.pWaitDstStageMask = wait_stages,

		.commandBufferCount = 1,
		.pCommandBuffers = &m_Context.Vk.CommandBuffers[current_frame_flight],

		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signal_semaphores
	};

	if (vkQueueSubmit(m_Context.Vk.Queues.Graphics, 1, &submitInfo, m_Context.Vk.InFlightFences[current_frame_flight]) != VK_SUCCESS)
	    THROW_Critical("failed to submit draw command buffer!");

	VkPresentInfoKHR presentInfo {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signal_semaphores,
		.swapchainCount = 1,
		.pSwapchains = &m_Context.Vk.Swapchain,
		.pImageIndices = &image_index,
		.pResults = nullptr // Optional
	};

	vkQueuePresentKHR(m_Context.Vk.Queues.Presentation, &presentInfo);
}

void MyApp::Instance::terminateVk () {
	LOG_trace ("{:s}", __FUNCSIG__);
	vkDeviceWaitIdle (m_Context.Vk.LogicalDevice);

	vkDestroyShaderModule (m_Context.Vk.LogicalDevice, m_Context.Vk.Modules.Vertex, nullptr);
	vkDestroyShaderModule (m_Context.Vk.LogicalDevice, m_Context.Vk.Modules.Fragment, nullptr);

	cleanupSwapchainAndRelated ();

	for (int  i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroySemaphore (m_Context.Vk.LogicalDevice, m_Context.Vk.ImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore (m_Context.Vk.LogicalDevice, m_Context.Vk.RenderFinishedSemaphores[i], nullptr);
		vkDestroyFence (m_Context.Vk.LogicalDevice, m_Context.Vk.InFlightFences[i], nullptr);
	}
	
	vkDestroyCommandPool (m_Context.Vk.LogicalDevice, m_Context.Vk.CommandPool, nullptr);

	vkDestroyDevice (m_Context.Vk.LogicalDevice, nullptr);

	if (g_EnableValidationLayers) { // Destroy Debug Utils Messenger
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_Context.Vk.MainInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
		    func(m_Context.Vk.MainInstance, m_Context.Vk.DebugMessenger, nullptr);
		}
	}
	
	vkDestroySurfaceKHR (m_Context.Vk.MainInstance, m_Context.Vk.Surface, nullptr);

	vkDestroyInstance (m_Context.Vk.MainInstance, nullptr);
}

void MyApp::Instance::cleanupSwapchainAndRelated () {
	for (auto &framebuffer: m_Context.Vk.SwapchainFramebuffers) 
		vkDestroyFramebuffer (m_Context.Vk.LogicalDevice, framebuffer, nullptr);

	vkDestroyPipeline(m_Context.Vk.LogicalDevice, m_Context.Vk.GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_Context.Vk.LogicalDevice, m_Context.Vk.PipelineLayout, nullptr);

	vkDestroyRenderPass(m_Context.Vk.LogicalDevice, m_Context.Vk.RenderPass, nullptr);
	
	for (auto &image_view: m_Context.Vk.SwapchainImagesView) 
		vkDestroyImageView (m_Context.Vk.LogicalDevice, image_view, nullptr);
	
	vkDestroySwapchainKHR (m_Context.Vk.LogicalDevice, m_Context.Vk.Swapchain, nullptr);
}
void MyApp::Instance::createSwapchainAndRelated () {
	const QueueFamilyIndices queue_family_indices (m_Context.Vk.PhysicalDevice, m_Context.Vk.Surface);

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
		, queue_family_indices.GraphicsFamily.value (), queue_family_indices.PresentationFamily.value ());
    
	// Create Image Views
	Helper::createImageViews(m_Context.Vk.SwapchainImagesView, m_Context.Vk.LogicalDevice, m_Context.Vk.SwapchainImageFormat, m_Context.Vk.SwapchainImages);
    
	// Create Render Pass
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

	// Create framebuffers
	Helper::createFramebuffers (m_Context.Vk.SwapchainFramebuffers
		,m_Context.Vk.LogicalDevice, m_Context.Vk.RenderPass, m_Context.Vk.SwapchainImagesView
		,m_Context.Vk.SwapchainImageExtent);
}

void MyApp::Instance::recreateSwapchainAndRelated () {
	LOG_trace (__FUNCSIG__);
	vkDeviceWaitIdle (m_Context.Vk.LogicalDevice);
	cleanupSwapchainAndRelated ();
	createSwapchainAndRelated ();
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
