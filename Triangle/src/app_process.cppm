import MainApplication;

import <iostream>;
import <thread>;
import <chrono>;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

static int val = 10;
void Application::Setup (const std::span<char*> &argument_list, Context& current_context) { 
	std::cout << __FUNCSIG__ << std::endl;
	
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
	std::cout << __FUNCSIG__ << ' ' << latency << std::endl;

	glfwPollEvents ();
}

void Application::Cleanup (Context& the_context) {
	std::cout << __FUNCSIG__ << std::endl;

	glfwDestroyWindow(the_context.MainWindow);

    glfwTerminate();
}
