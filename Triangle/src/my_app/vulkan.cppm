#include <logging.hxx>;

import MainApplication.MyApp;

import <vector>;
import <array>;
import <set>;
#include <limits>;

import <vulkan/vulkan.h>;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;

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

bool is_vk_device_suitable (VkPhysicalDevice device, void* surface_ptr);

struct QueueFamilyIndices {
	QueueFamilyIndices (VkPhysicalDevice device, VkSurfaceKHR surface) {
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
SwapChainSupportDetails query_swap_chain_support (VkPhysicalDevice device, VkSurfaceKHR surface);

static MyApp::Vertex s_TriangleVertices[] = {
	    {glm::vec2{-0.5, -0.5}, glm::vec3{1.0, 0.0, 1.0}, glm::vec2{1.0, 0.0}}, // A  A+-----------+B	
	    {glm::vec2{ 0.5, -0.5}, glm::vec3{0.0, 1.0, 1.0}, glm::vec2{0.0, 0.0}}, // B   |           |  
	    {glm::vec2{ 0.5,  0.5}, glm::vec3{1.0, 1.0, 0.0}, glm::vec2{0.0, 1.0}}, // C  D+-----------+C
	    {glm::vec2{-0.5,  0.5}, glm::vec3{1.0, 1.0, 1.0}, glm::vec2{1.0, 1.0}}, // D
	};

static uint16_t s_TriangleIndices[] = {1, 3, 0, /**/ 2, 3, 1};

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
			Helper::readFile (std::string() + PROJECT_ROOT_LOCATION + "/assets/2d_color-with-texture/vert.sprv"));
	m_Context.Vk.Modules.Fragment = Helper::createShaderModule (m_Context.Vk.LogicalDevice, 
			Helper::readFile (std::string() + PROJECT_ROOT_LOCATION + "/assets/2d_color-with-texture/frag.sprv"));

	// Create command pool & buffer
	Helper::createCommandPoolAndBuffer (m_Context.Vk.CommandPool, m_Context.Vk.CommandBuffers
		,m_Context.Vk.LogicalDevice, queue_family_indices.GraphicsFamily.value ());

	create_buffers (); // uniform buffers needs to be created before creating descriptor sets and we also need command pool for transferring staged buffer to device local
	create_swapchain_and_related ();

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
	
	// did you notice, these struct ex. VkCommandBuffer, VkPipeline, VkRenderPass, VkFramebuffer are typdefs to a pointer to real_object, so there's no need to pass them as refrence
	auto record_command_buffer = [](VkCommandBuffer command_buffer
			,VkPipeline graphics_pipeline, VkRenderPass render_pass, VkFramebuffer framebuffer
			,VkBuffer vertex_buffer, VkBuffer index_buffer, VkPipelineLayout pipeline_layout, VkDescriptorSet descriptor_set
			,VkExtent2D extent) 
	{
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

		VkBuffer vertex_buffers[] = {vertex_buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers (command_buffer, 0, 1, vertex_buffers, offsets);
		vkCmdBindIndexBuffer (command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

 		vkCmdDrawIndexed (command_buffer, std::size(s_TriangleIndices) /*no. of vertices*/, 1, 0, 0, 0);

		vkCmdEndRenderPass (command_buffer);

		if (vkEndCommandBuffer (command_buffer) != VK_SUCCESS)
		    THROW_Critical("failed to record command buffer!");
	};
	
	uint32_t image_index;
    switch (vkAcquireNextImageKHR(m_Context.Vk.LogicalDevice, m_Context.Vk.Swapchain, UINT64_MAX, m_Context.Vk.ImageAvailableSemaphores[current_frame_flight], VK_NULL_HANDLE, &image_index)) {
		case VK_SUCCESS: break;
		case VK_ERROR_OUT_OF_DATE_KHR:			
		case VK_SUBOPTIMAL_KHR: // Recreate swapchain and related
			recreate_swapchain_and_related ();
			return;
		default:
			THROW_Critical ("failed to acquire swapchain image");
	}
	
	vkResetFences (m_Context.Vk.LogicalDevice, 1, &m_Context.Vk.InFlightFences[current_frame_flight]);

	vkResetCommandBuffer(m_Context.Vk.CommandBuffers[current_frame_flight], 0);
	record_command_buffer(m_Context.Vk.CommandBuffers[current_frame_flight]
		,m_Context.Vk.GraphicsPipeline, m_Context.Vk.RenderPass, m_Context.Vk.SwapchainFramebuffers[image_index]
		,m_Context.Vk.Extras.VertexBuffer, m_Context.Vk.Extras.IndexBuffer
		,m_Context.Vk.PipelineLayout, m_Context.Vk.Extras.DescriptorSets[current_frame_flight]
		,m_Context.Vk.SwapchainImageExtent);

	{ // update uniform info
		MyApp::UniformBufferObject ubo {
			.model = glm::scale(glm::rotate(glm::mat4(1.0f), float(GetRenderTimestamp () * glm::radians(90.0)), glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(3)),
			.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
			.projection = glm::perspective(glm::radians(45.0f), m_Context.Vk.SwapchainImageExtent.width / (float) m_Context.Vk.SwapchainImageExtent.height, 0.1f, 10.0f)
		};
		ubo.projection[1][1] *= -1; // inverting y axis
		void* data = nullptr;
		vkMapMemory(m_Context.Vk.LogicalDevice, m_Context.Vk.Extras.UniformBufferMemory[current_frame_flight], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		//memcpy_s(data, sizeof(ubo), &ubo, sizeof(ubo));
		vkUnmapMemory(m_Context.Vk.LogicalDevice, m_Context.Vk.Extras.UniformBufferMemory[current_frame_flight]);
	}

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

	vkDestroySampler (m_Context.Vk.LogicalDevice, m_Context.Vk.Extras.TextureSampler, nullptr);
	vkDestroyImageView (m_Context.Vk.LogicalDevice, m_Context.Vk.Extras.TextureImageView, nullptr);
	vkDestroyImage (m_Context.Vk.LogicalDevice, m_Context.Vk.Extras.TextureImage, nullptr);
	vkFreeMemory (m_Context.Vk.LogicalDevice, m_Context.Vk.Extras.TextureImageMemory, nullptr);
	vkDestroyBuffer (m_Context.Vk.LogicalDevice, m_Context.Vk.Extras.VertexBuffer, nullptr);
	vkFreeMemory (m_Context.Vk.LogicalDevice, m_Context.Vk.Extras.VertexBufferMemory, nullptr);
	vkDestroyBuffer (m_Context.Vk.LogicalDevice, m_Context.Vk.Extras.IndexBuffer, nullptr);
	vkFreeMemory (m_Context.Vk.LogicalDevice, m_Context.Vk.Extras.IndexBufferMemory, nullptr);
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroyBuffer (m_Context.Vk.LogicalDevice, m_Context.Vk.Extras.UniformBuffer[i], nullptr);
		vkFreeMemory (m_Context.Vk.LogicalDevice, m_Context.Vk.Extras.UniformBufferMemory[i], nullptr);
	}

	vkDestroyShaderModule (m_Context.Vk.LogicalDevice, m_Context.Vk.Modules.Vertex, nullptr);
	vkDestroyShaderModule (m_Context.Vk.LogicalDevice, m_Context.Vk.Modules.Fragment, nullptr);

	cleanup_swapchain_and_related ();

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

void MyApp::Instance::cleanup_swapchain_and_related () {
	for (auto &framebuffer: m_Context.Vk.SwapchainFramebuffers) 
		vkDestroyFramebuffer (m_Context.Vk.LogicalDevice, framebuffer, nullptr);

	vkDestroyPipeline (m_Context.Vk.LogicalDevice, m_Context.Vk.GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout (m_Context.Vk.LogicalDevice, m_Context.Vk.PipelineLayout, nullptr);

	vkDestroyDescriptorPool (m_Context.Vk.LogicalDevice, m_Context.Vk.Extras.DescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout (m_Context.Vk.LogicalDevice, m_Context.Vk.DescriptorSetLayout, nullptr);

	vkDestroyRenderPass (m_Context.Vk.LogicalDevice, m_Context.Vk.RenderPass, nullptr);
	
	for (auto &image_view: m_Context.Vk.SwapchainImagesView) 
		vkDestroyImageView (m_Context.Vk.LogicalDevice, image_view, nullptr);
	
	vkDestroySwapchainKHR (m_Context.Vk.LogicalDevice, m_Context.Vk.Swapchain, nullptr);
}

void MyApp::Instance::create_swapchain_and_related () {
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

	{ // Create Discriptor set layout
		VkDescriptorSetLayoutBinding sampler_layout_binding {
			.binding = 1,
			.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		};
		VkDescriptorSetLayoutBinding ubo_layout_binding {
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = nullptr,
		};
		VkDescriptorSetLayoutBinding bindings[] = {ubo_layout_binding, sampler_layout_binding};
		VkDescriptorSetLayoutCreateInfo layout_info {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = uint32_t (std::size (bindings)),
			.pBindings = bindings,
		};

		if (vkCreateDescriptorSetLayout(m_Context.Vk.LogicalDevice, &layout_info, nullptr, &m_Context.Vk.DescriptorSetLayout) != VK_SUCCESS)
		   THROW_Critical("failed to create descriptor set layout!");
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
		
		// vertex discription
		auto binding_description = Vertex::geBindingDiscription();
		auto attribute_descriptions = Vertex::getAttributeDescriptions();
		{
			// Create discription pool
			VkDescriptorPoolSize pool_sizes[] = {
				{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = uint32_t (MAX_FRAMES_IN_FLIGHT)}, 
				{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = uint32_t (MAX_FRAMES_IN_FLIGHT)}
			};
			VkDescriptorPoolCreateInfo pool_info {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				.maxSets = MAX_FRAMES_IN_FLIGHT,
				.poolSizeCount = uint32_t (std::size (pool_sizes)),
				.pPoolSizes = pool_sizes,
			};
			if (vkCreateDescriptorPool (m_Context.Vk.LogicalDevice, &pool_info, nullptr, &m_Context.Vk.Extras.DescriptorPool) != VK_SUCCESS)
				THROW_Critical ("failed to create descriptor pool");

			// Create discription sets
			VkDescriptorSetLayout set_layouts[MAX_FRAMES_IN_FLIGHT];
			for (auto& set_layout: set_layouts) {
				set_layout = m_Context.Vk.DescriptorSetLayout;
			} 
			
			VkDescriptorSetAllocateInfo alloc_info {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = m_Context.Vk.Extras.DescriptorPool,
				.descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
				.pSetLayouts = set_layouts
			};
			if (vkAllocateDescriptorSets(m_Context.Vk.LogicalDevice, &alloc_info, m_Context.Vk.Extras.DescriptorSets) != VK_SUCCESS)
				THROW_Critical("failed to allocate descriptor sets!");

			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
				VkDescriptorBufferInfo buffer_info {
					.buffer = m_Context.Vk.Extras.UniformBuffer[i],
					.offset = 0,
					.range = sizeof(MyApp::UniformBufferObject)
				};
				VkDescriptorImageInfo image_info {
					.sampler = m_Context.Vk.Extras.TextureSampler,
					.imageView = m_Context.Vk.Extras.TextureImageView,
					.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				};

				VkWriteDescriptorSet descriptor_writes[] = {{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_Context.Vk.Extras.DescriptorSets[i],
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pBufferInfo = &buffer_info,
				}, {
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_Context.Vk.Extras.DescriptorSets[i],
					.dstBinding = 1,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &image_info,
				}};

				vkUpdateDescriptorSets(m_Context.Vk.LogicalDevice, uint32_t (std::size (descriptor_writes)), descriptor_writes, 0, nullptr);
			}
		}

		VkPipelineLayoutCreateInfo pipeline_layout_info {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &m_Context.Vk.DescriptorSetLayout,
			.pushConstantRangeCount = 0, // Optional
			.pPushConstantRanges = nullptr, // Optional
		};
			
		VkPipelineVertexInputStateCreateInfo vertex_input_info {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &binding_description, // Optional
			.vertexAttributeDescriptionCount = uint32_t (std::size (attribute_descriptions)),
			.pVertexAttributeDescriptions = attribute_descriptions.data () // Optional
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
			,m_Context.Vk.LogicalDevice, m_Context.Vk.RenderPass
			,pipeline_layout_info, shader_stages
			,vertex_input_info, input_assembly_info
			,m_Context.Vk.SwapchainImageExtent, dynamic_states);
	}

	// Create framebuffers
	Helper::createFramebuffers (m_Context.Vk.SwapchainFramebuffers
		,m_Context.Vk.LogicalDevice, m_Context.Vk.RenderPass, m_Context.Vk.SwapchainImagesView
		,m_Context.Vk.SwapchainImageExtent);
}

void MyApp::Instance::recreate_swapchain_and_related () {
	LOG_trace (__FUNCSIG__);
	vkDeviceWaitIdle (m_Context.Vk.LogicalDevice);
	cleanup_swapchain_and_related ();
	create_swapchain_and_related ();
}

void MyApp::Instance::create_buffers () {
	{ // vertex buffer for triangles
		VkDeviceSize buffer_size = sizeof(s_TriangleVertices);

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;
		Helper::createBuffer (staging_buffer, staging_buffer_memory
			,m_Context.Vk.LogicalDevice, m_Context.Vk.PhysicalDevice, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		
		void* staging_buffer_ptr = nullptr;
		vkMapMemory (m_Context.Vk.LogicalDevice, staging_buffer_memory, 0, buffer_size, 0, &staging_buffer_ptr);
		memcpy (staging_buffer_ptr, s_TriangleVertices, buffer_size);
		vkUnmapMemory(m_Context.Vk.LogicalDevice, staging_buffer_memory);
		// memcpy_s (buffer_ptr, alloc_info.allocationSize, s_TriangleVertices, sizeof(s_TriangleVertices)); // bug in microsoft stl header units

		Helper::createBuffer (m_Context.Vk.Extras.VertexBuffer, m_Context.Vk.Extras.VertexBufferMemory
			,m_Context.Vk.LogicalDevice, m_Context.Vk.PhysicalDevice, buffer_size
			,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
			,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Helper::copyBuffer (m_Context.Vk.Extras.VertexBuffer, staging_buffer
			,buffer_size, m_Context.Vk.LogicalDevice, m_Context.Vk.CommandPool, m_Context.Vk.Queues.Graphics);

		vkDestroyBuffer (m_Context.Vk.LogicalDevice, staging_buffer, nullptr);
		vkFreeMemory (m_Context.Vk.LogicalDevice, staging_buffer_memory, nullptr);
	}
	{ // index buffer for triangles
		VkDeviceSize buffer_size = sizeof(s_TriangleIndices);

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;
		Helper::createBuffer (staging_buffer, staging_buffer_memory
			,m_Context.Vk.LogicalDevice, m_Context.Vk.PhysicalDevice, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		
		void* staging_buffer_ptr = nullptr;
		vkMapMemory (m_Context.Vk.LogicalDevice, staging_buffer_memory, 0, buffer_size, 0, &staging_buffer_ptr);
		memcpy (staging_buffer_ptr, s_TriangleIndices, buffer_size);
		vkUnmapMemory(m_Context.Vk.LogicalDevice, staging_buffer_memory);
		// memcpy_s (buffer_ptr, alloc_info.allocationSize, s_TriangleVertices, sizeof(s_TriangleVertices)); // bug in microsoft stl header units

		Helper::createBuffer (m_Context.Vk.Extras.IndexBuffer, m_Context.Vk.Extras.IndexBufferMemory
			,m_Context.Vk.LogicalDevice, m_Context.Vk.PhysicalDevice, buffer_size
			,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
			,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Helper::copyBuffer (m_Context.Vk.Extras.IndexBuffer, staging_buffer
			,buffer_size, m_Context.Vk.LogicalDevice, m_Context.Vk.CommandPool, m_Context.Vk.Queues.Graphics);

		vkDestroyBuffer (m_Context.Vk.LogicalDevice, staging_buffer, nullptr);
		vkFreeMemory (m_Context.Vk.LogicalDevice, staging_buffer_memory, nullptr);
	}
	{ // uniform buffers
		VkDeviceSize buffer_size = sizeof(UniformBufferObject);

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			Helper::createBuffer (m_Context.Vk.Extras.UniformBuffer[i], m_Context.Vk.Extras.UniformBufferMemory[i]
				,m_Context.Vk.LogicalDevice, m_Context.Vk.PhysicalDevice, buffer_size
				,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
				,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		}
	}
	{ // texture image
		// image loading
		auto [image_data, tex_width, tex_height, tex_channels] = Helper::readImageRGBA (PROJECT_ROOT_LOCATION "/assets/textures/hatsune_miku.jpg");

		VkDeviceSize buffer_size = tex_width * tex_height * 4;//tex_channels * sizeof(image_data[0]);

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;
		Helper::createBuffer (staging_buffer, staging_buffer_memory
			,m_Context.Vk.LogicalDevice, m_Context.Vk.PhysicalDevice, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		
		void* staging_buffer_ptr = nullptr;
		vkMapMemory (m_Context.Vk.LogicalDevice, staging_buffer_memory, 0, buffer_size, 0, &staging_buffer_ptr);
		memcpy (staging_buffer_ptr, image_data, buffer_size);
		vkUnmapMemory(m_Context.Vk.LogicalDevice, staging_buffer_memory);
		// memcpy_s (buffer_ptr, alloc_info.allocationSize, s_TriangleVertices, sizeof(s_TriangleVertices)); // bug in microsoft stl header units

		Helper::freeImage (image_data);

		Helper::createImage (m_Context.Vk.Extras.TextureImage, m_Context.Vk.Extras.TextureImageMemory
			,m_Context.Vk.LogicalDevice, m_Context.Vk.PhysicalDevice, {tex_width, tex_height}
			,VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Helper::copyBuffer (m_Context.Vk.Extras.TextureImage
			,staging_buffer, {tex_width, tex_height}
			,m_Context.Vk.LogicalDevice, m_Context.Vk.CommandPool, m_Context.Vk.Queues.Graphics);

		vkDestroyBuffer (m_Context.Vk.LogicalDevice, staging_buffer, nullptr);
		vkFreeMemory (m_Context.Vk.LogicalDevice, staging_buffer_memory, nullptr);

		// image view for texture image
		VkImageViewCreateInfo view_info {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = m_Context.Vk.Extras.TextureImage,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_SRGB,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
				.baseMipLevel = 0, 
				.levelCount = 1,
				.baseArrayLayer = 0, 
				.layerCount = 1 
			}
		};
		if (vkCreateImageView (m_Context.Vk.LogicalDevice, &view_info, nullptr, &m_Context.Vk.Extras.TextureImageView) != VK_SUCCESS) 
			THROW_Critical ("failed to create textureimage view!");
		
		// texture image sampler
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties (m_Context.Vk.PhysicalDevice, &properties);
		VkSamplerCreateInfo sampler_info {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,

			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,

			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_TRUE, // VK_FALSE
			.maxAnisotropy = properties.limits.maxSamplerAnisotropy, // 1.0f

			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,

			.minLod = 0.0f,
			.maxLod = 0.0f,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE,
		};

		if (vkCreateSampler (m_Context.Vk.LogicalDevice, &sampler_info, nullptr, &m_Context.Vk.Extras.TextureSampler) != VK_SUCCESS) 
			THROW_Critical ("failed to create texture sampler");
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback (VkDebugUtilsMessageSeverityFlagBitsEXT message_severity
	,VkDebugUtilsMessageTypeFlagsEXT message_type
	,const VkDebugUtilsMessengerCallbackDataEXT* callback_data
	,void* pUserData) 
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

bool is_vk_device_suitable (VkPhysicalDevice device, void* surface_ptr) {
	VkSurfaceKHR surface = *(VkSurfaceKHR*)(surface_ptr);
	// We culd also implement a rating system for selecting most powerful GPU in system

	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features {.samplerAnisotropy = VK_TRUE};

	vkGetPhysicalDeviceProperties (device, &device_properties);
	vkGetPhysicalDeviceFeatures (device, &device_features);

	const QueueFamilyIndices indices (device, surface);

	auto check_device_extensions_support = [](VkPhysicalDevice device) {
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
		&&	device_features.samplerAnisotropy
		&&	indices.AreAllFamiliesPresent()
		&&	extensions_supported
		&&	swap_chain_adequate
	;
}

SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
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
