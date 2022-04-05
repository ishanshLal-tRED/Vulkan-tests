#include <logging.hxx>;

import MainApplication;

import <vector>;
import <array>;

import <vulkan\vulkan.h>;
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static constexpr std::array<const char*, 1> s_ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
constexpr bool g_EnableValidationLayers = ( _DEBUG ? true : false );

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT,VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT*,void*);

void Application::InitializeVk (Context& current_context) {
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
		.pNext = (g_EnableValidationLayers ? (VkDebugUtilsMessengerCreateInfoEXT*) &debug_messenger_create_info : nullptr),
		.pApplicationInfo = &app_info,
		.enabledLayerCount = (g_EnableValidationLayers ? s_ValidationLayers.size () : 0),
		.ppEnabledLayerNames = (g_EnableValidationLayers ? s_ValidationLayers.data () : nullptr),
		.enabledExtensionCount = uint32_t(vk_extensions_to_enable.size()),
		.ppEnabledExtensionNames = vk_extensions_to_enable.data()
	};

	if (vkCreateInstance(&create_info, nullptr, &current_context.Vk.MainInstance) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create instance!");
	}

	if (g_EnableValidationLayers) { // Create Debug Utils Messenger

		VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(current_context.Vk.MainInstance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
		    result = func(current_context.Vk.MainInstance, &debug_messenger_create_info, nullptr, &current_context.Vk.DebugMessenger);
		}
		if (result != VK_SUCCESS){
			THROW_Critical("failed setting up vk_debug_messenger");
		}
	}
}

void Application::Render (double latency) {
	LOG_trace("{:s} {:f}", __FUNCSIG__, latency);
}

void Application::TerminateVk (Context& the_context) {
	LOG_trace ("{:s}", __FUNCSIG__);

	if (g_EnableValidationLayers) { // Destroy Debug Utils Messenger
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(the_context.Vk.MainInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
		    func(the_context.Vk.MainInstance, the_context.Vk.DebugMessenger, nullptr);
		}
	}

	vkDestroyInstance(the_context.Vk.MainInstance, nullptr);
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