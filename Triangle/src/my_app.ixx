#include <logging.hxx>

export import MainApplication.Base;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>;

import <glm/glm.hpp>;

export module MainApplication.MyApp;

namespace MyApp {
	export constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

	// Custom structs
	export struct Vertex {
		glm::vec2 position;
		glm::vec3 color;

		static constexpr VkVertexInputBindingDescription geBindingDiscription () {
			return VkVertexInputBindingDescription {
				.binding = 0,
				.stride = sizeof (Vertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			};
		}
		static constexpr std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		    return {
				VkVertexInputAttributeDescription {
					.location = 0,
					.binding = 0,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(Vertex, position),
				}, 
				VkVertexInputAttributeDescription {
					.location = 1,
					.binding = 0,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(Vertex, color),
				}
			};
		}
	};

	export struct UniformBufferObject {
		// meeting alignment requirements
		alignas(16) // we can do this or, #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
	    glm::mat4 model;
	    glm::mat4 view;
	    glm::mat4 projection;
	};

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

			VkDescriptorSetLayout DescriptorSetLayout;
			
			VkPipelineLayout PipelineLayout;
			VkPipeline GraphicsPipeline;

			std::vector<VkFramebuffer> SwapchainFramebuffers;

			VkCommandPool CommandPool;
			VkCommandBuffer CommandBuffers[MAX_FRAMES_IN_FLIGHT]; // some how array

			VkSemaphore RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
			VkSemaphore ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
			VkFence InFlightFences[MAX_FRAMES_IN_FLIGHT];

			// Thie is not the place for this struct
			struct {
				VkBuffer VertexBuffer;
				VkDeviceMemory VertexBufferMemory;
				VkBuffer IndexBuffer;
				VkDeviceMemory IndexBufferMemory;
				VkBuffer UniformBuffer [MAX_FRAMES_IN_FLIGHT];
				VkDeviceMemory UniformBufferMemory [MAX_FRAMES_IN_FLIGHT];

				VkDescriptorPool DescriptorPool;
				VkDescriptorSet DescriptorSets [MAX_FRAMES_IN_FLIGHT];
			} Extras;
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
		void cleanup_swapchain_and_related ();
		void create_swapchain_and_related ();
		void recreate_swapchain_and_related ();

		void create_buffers ();
	private:
		size_t m_CurrentFrame = 0;
	};
}