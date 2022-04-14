#include <logging.hxx>

export import MainApplication.Base;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>;

export module MainApplication.MyApp;

namespace MyApp {
	export struct Context: public Application::BaseContext {

		virtual bool KeepContextRunning() override;

		const int WIDTH = 800, HEIGHT = 600;
		double UpdateLatency   = 0.0;
		double RenderLatency   = 0.0;
		
		GLFWwindow* MainWindow = nullptr;
		
		struct struct_Vk {
			VkInstance MainInstance;
			VkDebugUtilsMessengerEXT DebugMessenger;
			VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
			VkDevice LogicalDevice;
			
			VkSurfaceKHR  Surface;

			struct {
				VkQueue Graphics;
				VkQueue Presentation;
			} Queues;

			VkSwapchainKHR Swapchain;
			std::vector<VkImage> SwapchainImages;
			std::vector<VkImageView> SwapchainImagesView;
			VkFormat SwapchainImageFormat;
			VkExtent2D SwapchainImageExtent;
		} Vk;
	};

	export class Instance: public Application::BaseInstance<MyApp::Context> {
	public:
		virtual void setup (const std::span<char*> &argument_list) override;
		virtual void initializeVk () override;
		virtual void update (double update_latency) override;
		virtual void render (double render_latency) override;
		virtual void terminateVk () override;
		virtual void cleanup () override;

		int val = 10;
	};
}