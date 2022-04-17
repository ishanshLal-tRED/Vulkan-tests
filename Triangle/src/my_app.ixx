#include <logging.hxx>

export import MainApplication.Base;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>;

export module MainApplication.MyApp;

namespace MyApp {
	export constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

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

			struct {
				VkShaderModule Vertex;
				VkShaderModule Fragment;
			} Modules;

			VkRenderPass RenderPass;
			
			VkPipelineLayout PipelineLayout;
			VkPipeline GraphicsPipeline;

			std::vector<VkFramebuffer> SwapchainFramebuffers;

			VkCommandPool CommandPool;
			VkCommandBuffer CommandBuffers[MAX_FRAMES_IN_FLIGHT]; // some how array

			VkSemaphore RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
			VkSemaphore ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
			VkFence InFlightFences[MAX_FRAMES_IN_FLIGHT];
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

	private:
		void cleanupSwapchainAndRelated ();
		void createSwapchainAndRelated ();
		void recreateSwapchainAndRelated ();
	private:
		size_t m_CurrentFrame = 0;
	};
}