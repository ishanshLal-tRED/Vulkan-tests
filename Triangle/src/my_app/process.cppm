#include <logging.hxx>;
import <thread>;
import <chrono>;

import MainApplication.MyApp;

import <GLFW/glfw3.h>;
import Helpers.GLFW;

// already very clean
bool MyApp::Context::KeepContextRunning () {	
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	return !glfwWindowShouldClose(MainWindow);
}

void MyApp::Instance::setup (const std::span<char*> &argument_list) { 
	LOG_trace(__FUNCSIG__);

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	auto framebuffer_resize_callback = [](GLFWwindow* window, int width, int height) {
			auto app_instance = (MyApp::Instance*)(glfwGetWindowUserPointer(window));
			app_instance->recreate_swapchain_and_related ();
		};

	if (m_Context.MainWindow == nullptr){
		m_Context.MainWindow = glfwCreateWindow(m_Context.WIDTH, m_Context.HEIGHT, "Vulkan window", nullptr, nullptr);
	}

	glfwSetWindowUserPointer(m_Context.MainWindow, this);
	glfwSetFramebufferSizeCallback(m_Context.MainWindow, framebuffer_resize_callback);
}

void MyApp::Instance::update (double latency) {
	LOG_trace("{:s} {:f}", __FUNCSIG__, latency);

	glfwPollEvents ();
}

void MyApp::Instance::cleanup () {
	LOG_trace(__FUNCSIG__);

	glfwDestroyWindow(m_Context.MainWindow);

    glfwTerminate();
}
