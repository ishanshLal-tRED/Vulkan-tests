import MainApplication;

import <iostream>;
import <vulkan\vulkan.h>;

void Application::InitializeVk (Context& current_context) { 
	std::cout << __FUNCSIG__ << std::endl; 
	
	uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	
	std::cout << extensionCount << " extensions supported\n";
}

void Application::Render (double latency) {
	std::cout << __FUNCSIG__ << ' ' << latency << std::endl;
}

void Application::TerminateVk (Context& the_context) {
	std::cout << __FUNCSIG__ << std::endl;
}