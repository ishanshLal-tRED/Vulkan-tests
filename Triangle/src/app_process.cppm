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
	
	glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;
}

bool Application::WindowsShouldClose (const Context& current_context) {
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	return (val-- ? false : true);
}

void Application::Update (double latency) {
	std::cout << __FUNCSIG__ << ' ' << latency << std::endl;
}

void Application::Cleanup (Context& the_context) {
	std::cout << __FUNCSIG__ << std::endl;
}
