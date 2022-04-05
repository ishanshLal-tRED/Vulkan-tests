import MainApplication;

import <iostream>;
import <vector>;

import <vulkan\vulkan.h>;
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

void Application::InitializeVk (Context& current_context) { 
	std::cout << __FUNCSIG__ << std::endl; 
	VkApplicationInfo app_info{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0
	};

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	VkInstanceCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledLayerCount = 0,
		.enabledExtensionCount = glfwExtensionCount,
		.ppEnabledExtensionNames = glfwExtensions
	};

	if (vkCreateInstance(&create_info, nullptr, &current_context.MainVkInstance) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create instance!");
	}

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data ());

	std::cout << "available extensions:\n";

	for (const auto& extension : extensions) {
	    std::cout << '\t' << extension.extensionName << '\n';
	}
	
	std::cout << extensionCount << " extensions supported\n";
}

void Application::Render (double latency) {
	std::cout << __FUNCSIG__ << ' ' << latency << std::endl;
}

void Application::TerminateVk (Context& the_context) {
	std::cout << __FUNCSIG__ << std::endl;
	vkDestroyInstance(the_context.MainVkInstance, nullptr);
}