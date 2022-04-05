module;

export import <span>;
import <time.h>;


import <vulkan\vulkan.h>;
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

export module MainApplication;

namespace Application
{
	export struct Context {
		public:
			void UpdateTime() {
				clock_t time_now = clock();
				RenderLatency = UpdateLatency = double(time_now - frame_timestamp) / CLOCKS_PER_SEC;
				frame_timestamp = time_now;
			}
		public:
			const int WIDTH = 800, HEIGHT = 600;
			double UpdateLatency   = 0.0;
			double RenderLatency   = 0.0;
			
			GLFWwindow* MainWindow = nullptr;
			
			struct {
				VkInstance MainInstance;
				VkDebugUtilsMessengerEXT DebugMessenger;
			} Vk;
		private:
			clock_t frame_timestamp = clock();
	};
	Context g_Context{};

	export {
		const Context*	GetContext		() { return &g_Context; }

		void Setup (const std::span<char*> &argument_list, Context& current_context  = g_Context);
		void InitializeVk (Context& current_context = g_Context);	
		
		bool WindowsShouldClose (const Context& current_context);
		void Update (double update_latency);
		void Render (double render_latency);
		void Run    (Context& current_context = g_Context) {
			current_context.UpdateTime ();
			while (!WindowsShouldClose(current_context))
				current_context.UpdateTime (), Update(current_context.UpdateLatency), Render(current_context.RenderLatency);
		}
		
		void TerminateVk (Context& current_context  = g_Context);
		void Cleanup (Context& current_context  = g_Context);
	}
};