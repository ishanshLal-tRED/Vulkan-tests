#include <logging.hxx>

export import MainApplication.Base;

import <vulkan/vulkan.h>;
import <GLFW/glfw3.h>;
export import <glm/glm.hpp>;

export import Helpers.FontAtlasLoader;


export module MainApplication.Instancing;

namespace Instancing {
	export constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

	export constexpr bool g_EnableValidationLayers = ( _DEBUG ? true : false );
	export constexpr bool VK_VERTEX_BUFFER_BIND_ID = 0;
	export constexpr bool VK_INSTANCE_BUFFER_BIND_ID = 1;

	// Custom structs
	export struct Vertex {
		glm::vec2 position;
		glm::vec3 color;
		glm::vec2 texCoord;

		static constexpr VkVertexInputBindingDescription getBindingDiscription () {
			return VkVertexInputBindingDescription {
				.binding = VK_VERTEX_BUFFER_BIND_ID,
				.stride = sizeof (Vertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			};
		}
		static constexpr std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		    return {
				VkVertexInputAttributeDescription {
					.location = 0,
					.binding = VK_VERTEX_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(Vertex, position),
				}, 
				VkVertexInputAttributeDescription {
					.location = 1,
					.binding = VK_VERTEX_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(Vertex, color),
				}, 
				VkVertexInputAttributeDescription {
					.location = 2,
					.binding = VK_VERTEX_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(Vertex, texCoord),
				},
			};
		}
	};

	// Custom structs
	export struct AnInstance {
		glm::vec3 rotn_scale; // vkDeviceWaitIdle (m_Context.Vk.LogicalDevice);
		glm::vec2 position;
		glm::vec2 texCoord00;
		glm::vec2 texCoordDelta;
		float texID = 0;

		static constexpr VkVertexInputBindingDescription getBindingDiscription () {
			return VkVertexInputBindingDescription {
				.binding = VK_INSTANCE_BUFFER_BIND_ID,
				.stride = sizeof (AnInstance),
				.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
			};
		}
		template<uint32_t startfrom = 3>
		static constexpr std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions () {
		    return { 
				VkVertexInputAttributeDescription {
					.location = startfrom + 0,
					.binding = VK_INSTANCE_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(AnInstance, rotn_scale),
				},
				VkVertexInputAttributeDescription {
					.location = startfrom + 1,
					.binding = VK_INSTANCE_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(AnInstance, position),
				},
				VkVertexInputAttributeDescription {
					.location = startfrom + 2,
					.binding = VK_INSTANCE_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(AnInstance, texCoord00),
				},
				VkVertexInputAttributeDescription {
					.location = startfrom + 3,
					.binding = VK_INSTANCE_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(AnInstance, texCoordDelta),
				},
				VkVertexInputAttributeDescription {
					.location = startfrom + 4,
					.binding = VK_INSTANCE_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32_SFLOAT,
					.offset = offsetof(AnInstance, texCoordDelta),
				},
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

		const int WIDTH = 800, HEIGHT = 800;

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
				VkBuffer InstanceBuffer;
				VkDeviceMemory InstanceBufferMemory;
				VkBuffer VertexBuffer;
				VkDeviceMemory VertexBufferMemory;
				VkBuffer IndexBuffer;
				VkDeviceMemory IndexBufferMemory;
				VkBuffer UniformBuffer [MAX_FRAMES_IN_FLIGHT];
				VkDeviceMemory UniformBufferMemory [MAX_FRAMES_IN_FLIGHT];

				VkDescriptorPool DescriptorPool;
				VkDescriptorSet DescriptorSets [MAX_FRAMES_IN_FLIGHT];

				VkImage TextureImage;
				VkDeviceMemory TextureImageMemory;
				VkImageView TextureImageView;

				VkSampler TextureSampler;
			} Extras;
		} Vk;
		
		struct struct_uitext {
			void create_instance_buffer (VkDevice logical_device, VkPhysicalDevice physical_device);
			void destroy_instance_buffer (VkDevice logical_device);

			struct {
				VkBuffer InstanceBuffer;
				VkDeviceMemory InstanceBufferMemory;

				VkDeviceMemory FontTextureMemory;
				VkImage FontTexture;
				VkImageView FontTextureView;
				VkSampler AtlasSampler;
			} Vk;
			std::vector<AnInstance> Quads;
			bool refresh = false;
		} UiText;
		Helper::FontAtlas ShonenPunkFont {PROJECT_ROOT_LOCATION "/assets/fonts_atlas/shonen_punk.png", PROJECT_ROOT_LOCATION "/assets/fonts_atlas/shonen_punk.json"};
	};

	export class Instance: public Application::BaseInstance<Context> {
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

		void create_ui_data ();
	private:
		size_t m_CurrentFrame = 0;
	};
}