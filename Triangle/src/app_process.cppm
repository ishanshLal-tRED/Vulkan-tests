#include <logging.hxx>;
import <thread>;
import <chrono>;

import MainApplication;


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

static int val = 10;
void Application::Setup (const std::span<char*> &argument_list, Context& current_context) { 
	LOG_trace(__FUNCSIG__);

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	if (current_context.MainWindow == nullptr){
		current_context.MainWindow = glfwCreateWindow(current_context.WIDTH, current_context.HEIGHT, "Vulkan window", nullptr, nullptr);
	}
	
	glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;
}

bool Application::WindowsShouldClose (const Context& current_context) {
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	return glfwWindowShouldClose(current_context.MainWindow);
}

void Application::Update (double latency) {
	LOG_trace("{:s} {:f}", __FUNCSIG__, latency);

	glfwPollEvents ();
}

void Application::Cleanup (Context& the_context) {
	LOG_trace(__FUNCSIG__);

	glfwDestroyWindow(the_context.MainWindow);

    glfwTerminate();
}
