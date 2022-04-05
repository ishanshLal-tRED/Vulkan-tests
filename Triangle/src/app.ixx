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
			VkInstance MainVkInstance;
		private:
			clock_t frame_timestamp = clock();
	};
	Context g_context{};

	export {
		const Context*	GetContext		() { return &g_context; }

		void Setup (const std::span<char*> &argument_list, Context& current_context  = g_context);
		void InitializeVk (Context& current_context = g_context);	
		
		bool WindowsShouldClose (const Context& current_context);
		void Update (double update_latency);
		void Render (double render_latency);
		void Run    (Context& current_context = g_context) {
			while (!WindowsShouldClose(current_context))
				Update(current_context.UpdateLatency), Render(current_context.RenderLatency), current_context.UpdateTime ();
		}
		
		void TerminateVk (Context& current_context  = g_context);
		void Cleanup (Context& current_context  = g_context);
	}
};