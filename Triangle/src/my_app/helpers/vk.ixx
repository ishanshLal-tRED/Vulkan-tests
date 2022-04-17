module;
#include <logging.hxx>

import <set>;
export import <span>;

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>;

import Helpers.GLFW;
export module Helpers.Vk;

namespace Helper{
	export {
		template<bool EnableValidationLayers = true>
		void createInstance(VkInstance&, VkDebugUtilsMessengerEXT&
			, VkApplicationInfo, const std::span<const char*>,
			VkBool32 (VKAPI_PTR *f)(
				VkDebugUtilsMessageSeverityFlagBitsEXT,VkDebugUtilsMessageTypeFlagsEXT,
			    const VkDebugUtilsMessengerCallbackDataEXT*,void*));

		void setupDebugMessenger(VkDebugUtilsMessengerEXT&
			, VkInstance&, VkDebugUtilsMessengerCreateInfoEXT&,
			VkBool32 (VKAPI_PTR *f)(
				VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
			    const VkDebugUtilsMessengerCallbackDataEXT*,void*));

		void createSurface(VkSurfaceKHR&, VkInstance&,  HWND, HINSTANCE hinstance = GetModuleHandle(nullptr));

		VkPhysicalDevice pickPhysicalDevice(VkInstance&, bool (*f)(VkPhysicalDevice&, void*), void*);

		template<bool EnableValidationLayers = true>
		void createLogicalDevice(VkDevice&
			, VkPhysicalDevice&, std::vector<std::pair<VkQueue*, uint32_t>>
			, const std::span<const char*>, const std::span<const char*>);

		void createSwapChain (VkSwapchainKHR&, std::vector<VkImage>&, VkSurfaceKHR&, VkDevice&
			, VkSurfaceCapabilitiesKHR, VkExtent2D, VkSurfaceFormatKHR, VkPresentModeKHR
			, uint32_t, uint32_t);

		void createImageViews(std::vector<VkImageView>&
			, VkDevice&, VkFormat, std::vector<VkImage>&);
		
		VkShaderModule createShaderModule (VkDevice& device, const std::vector<char>& byte_code);
	};

    template<bool EnableValidationLayers>
	void createInstance(VkInstance &instance, VkDebugUtilsMessengerEXT& debug_messenger
		, VkApplicationInfo app_info, std::span<const char*> req_validation_layers
		, VkBool32 (VKAPI_PTR *vk_debug_callback)(
			VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
		    const VkDebugUtilsMessengerCallbackDataEXT*,void*) ) 
	{
        std::vector<const char*> vk_extensions_to_enable;
        auto glfw_extensions = Helper::GetGLFWRequiredVkExtensions();
        vk_extensions_to_enable.insert(vk_extensions_to_enable.end(), glfw_extensions.begin(), glfw_extensions.end());
	    if (EnableValidationLayers) {
	    	vk_extensions_to_enable.push_back (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	    }
        
        { // Vk Extensions
			uint32_t extension_count = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
			std::vector<VkExtensionProperties> extensions(extension_count);
			vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data ());
			
			LOG_trace("available extensions:");
			for (const auto& extension : extensions)
			    LOG_raw("\n\t {}", extension.extensionName);
			
			LOG_trace("{} extensions supported", extension_count);
		}
	
		{ // Check validation layer support
			uint32_t layer_count;
			vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
			std::vector<VkLayerProperties> validation_layers(layer_count);
			vkEnumerateInstanceLayerProperties(&layer_count, validation_layers.data ());

			std::vector<const char*> unavailable_layers;
			for (const char* layer_name : req_validation_layers) {
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
			//.pNext = (EnableValidationLayers ? (VkDebugUtilsMessengerCreateInfoEXT*) &debug_messenger_create_info : nullptr),
			//// Disabling due to non fatal error, Refer: http://disq.us/p/2o823o8
			.pApplicationInfo = &app_info,
			.enabledLayerCount = uint32_t(EnableValidationLayers ? req_validation_layers.size () : 0),
			.ppEnabledLayerNames = (EnableValidationLayers ? req_validation_layers.data () : nullptr),
			.enabledExtensionCount = uint32_t(vk_extensions_to_enable.size()),
			.ppEnabledExtensionNames = vk_extensions_to_enable.data()
		};

		if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) 
			THROW_CORE_Critical("failed to create instance!");

		if (EnableValidationLayers) { // Create Debug Utils Messenger
			setupDebugMessenger (debug_messenger, instance, debug_messenger_create_info, vk_debug_callback);
		}
    }

    void setupDebugMessenger (VkDebugUtilsMessengerEXT &debug_messenger
		, VkInstance &instance, VkDebugUtilsMessengerCreateInfoEXT &debug_messenger_info
		, VkBool32 (VKAPI_PTR *vk_debug_callback)(
			VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
		    const VkDebugUtilsMessengerCallbackDataEXT*,void*))
	{
		VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
		    result = func(instance, &debug_messenger_info, nullptr, &debug_messenger);
		};
		if (result != VK_SUCCESS)
			THROW_CORE_Critical("failed setting up vk_debug_messenger");
	}

    void createSurface(VkSurfaceKHR &surface
		, VkInstance &instance, HWND window_handle, HINSTANCE hinstance) 
	{
		VkWin32SurfaceCreateInfoKHR surface_info{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = hinstance,
			.hwnd = window_handle
		};

		if (vkCreateWin32SurfaceKHR (instance, &surface_info, nullptr, &surface) != VK_SUCCESS) 
			THROW_CORE_Critical("failed to create window surface");
	}

    VkPhysicalDevice pickPhysicalDevice(VkInstance &instance	
		, bool (*is_device_suitable)(VkPhysicalDevice&, void*), void* user_supplied_data) 
	{
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
		if (device_count == 0)
			THROW_CORE_Critical("failed to find hardware with vulkan support");
		std::vector<VkPhysicalDevice> vk_enabled_devices (device_count);
		vkEnumeratePhysicalDevices(instance, &device_count, vk_enabled_devices.data ());

		// Can be improved, aka when more than one GPU is available
		for (auto device: vk_enabled_devices){
			if (is_device_suitable (device, user_supplied_data)) {
				return device;
			}
		}
		
		THROW_CORE_Critical("failed to find hardware with vulkan support");
	};
	
	template<bool EnableValidationLayers>
    void createLogicalDevice(VkDevice &logical_device
		, VkPhysicalDevice &physical_device, std::vector<std::pair<VkQueue*, uint32_t>> vk_queues_and_family_indice_pairs
		, std::span<const char*> req_validation_layers, std::span<const char*> req_extensions) 
	{
		std::set<uint32_t> unique_queue_families;
		for (auto& queue_family_pair: vk_queues_and_family_indice_pairs) {
			unique_queue_families.insert(queue_family_pair.second);
		}

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		queue_create_infos.reserve (unique_queue_families.size());
		for (auto family_indice: unique_queue_families){
			float queue_priority = 1.0f; // need more information on it
			queue_create_infos.push_back(VkDeviceQueueCreateInfo {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = family_indice,
				.queueCount = 1,
				.pQueuePriorities = &queue_priority
			});
		}

		VkPhysicalDeviceFeatures device_features{};
		vkGetPhysicalDeviceFeatures (physical_device, &device_features);
		
		VkDeviceCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = uint32_t(queue_create_infos.size ()),
			.pQueueCreateInfos = queue_create_infos.data (),
			.enabledLayerCount = uint32_t(EnableValidationLayers ? req_validation_layers.size () : 0),
			.ppEnabledLayerNames = (EnableValidationLayers ? req_validation_layers.data () : nullptr),
			.enabledExtensionCount = uint32_t(req_extensions.size ()),
			.ppEnabledExtensionNames = req_extensions.data (),
			.pEnabledFeatures = &device_features
		};

		if (vkCreateDevice (physical_device, &create_info, nullptr, &logical_device) != VK_SUCCESS) 
			THROW_CORE_Critical ("Failed to vreate logical device");

		for (auto& queue_family_pair: vk_queues_and_family_indice_pairs) {
			vkGetDeviceQueue (logical_device, queue_family_pair.second, 0, queue_family_pair.first);
		}
	}

    void createSwapChain (VkSwapchainKHR &swapchain, std::vector<VkImage> &swapchain_images
		, VkSurfaceKHR &surface, VkDevice &logical_device
		, VkSurfaceCapabilitiesKHR swapchain_capabilities
		, VkExtent2D extent, VkSurfaceFormatKHR format, VkPresentModeKHR present_mode
		, uint32_t graphics_family_indice, uint32_t presentation_family_indice)
	{
		uint32_t image_count = ((swapchain_capabilities.maxImageCount == 0) ? swapchain_capabilities.minImageCount + 1 : swapchain_capabilities.maxImageCount);
		VkSwapchainCreateInfoKHR swapchain_create_info {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = surface,
			.minImageCount = image_count,
			.imageFormat = format.format,
			.imageColorSpace = format.colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,

			.preTransform = swapchain_capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = present_mode,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE
		};
		
		if (graphics_family_indice == presentation_family_indice) {
			swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchain_create_info.queueFamilyIndexCount = 0;
			swapchain_create_info.pQueueFamilyIndices = nullptr;
		} else {
			uint32_t queue_family_indices[] = {graphics_family_indice, presentation_family_indice};
			
			swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchain_create_info.queueFamilyIndexCount = uint32_t(std::size(queue_family_indices));
			swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
		}
	
		if (vkCreateSwapchainKHR (logical_device, &swapchain_create_info, nullptr, &swapchain) != VK_SUCCESS)
			THROW_Critical("failed to create swapchain !");

		uint32_t swapchain_images_count;
		vkGetSwapchainImagesKHR(logical_device, swapchain, &swapchain_images_count, nullptr);
		swapchain_images.resize(swapchain_images_count);
		vkGetSwapchainImagesKHR(logical_device, swapchain, &swapchain_images_count, swapchain_images.data());
	}

    void createImageViews(std::vector<VkImageView> &swapchain_images_view
		, VkDevice &logical_device, VkFormat swapchain_image_format
		, std::vector<VkImage> &swapchain_images) 
	{
		swapchain_images_view.resize(swapchain_images.size());
		for (size_t i = 0; i < swapchain_images.size (); ++i) {
			VkImageViewCreateInfo create_info {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = swapchain_images[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = swapchain_image_format,
				.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
				.subresourceRange = VkImageSubresourceRange {
					    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					    .baseMipLevel = 0,
					    .levelCount = 1,
					    .baseArrayLayer = 0,
					    .layerCount = 1
				}
			};

			if (vkCreateImageView (logical_device, &create_info, nullptr, &swapchain_images_view[i]) != VK_SUCCESS)
				THROW_Critical ("failed to create imagfe views!");
		}
	}

	VkShaderModule createShaderModule (VkDevice& device, const std::vector<char>& byte_code) {
		VkShaderModuleCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = byte_code.size(),
			.pCode = reinterpret_cast<const uint32_t*>(byte_code.data())
		};

		VkShaderModule shader_module;
		if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS) 
			THROW_Critical ("failed to create shader module!");
		return shader_module;
	}
};