#include <logging.hxx>;
import <thread>;
import <chrono>;

import MainApplication.Instancing;

import <GLFW/glfw3.h>;
import Helpers.GLFW;

// already very clean
bool Instancing::Context::KeepContextRunning () {
	return !glfwWindowShouldClose(MainWindow);
}

void Instancing::Instance::setup (const std::span<char*> &argument_list) { 
	LOG_trace(__FUNCSIG__);

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	auto framebuffer_resize_callback = [](GLFWwindow* window, int width, int height) {
			auto app_instance = (Instancing::Instance*)(glfwGetWindowUserPointer(window));
			app_instance->recreate_swapchain_and_related ();
		};

	if (m_Context.MainWindow == nullptr){
		m_Context.MainWindow = glfwCreateWindow(m_Context.WIDTH, m_Context.HEIGHT, "Vulkan window", nullptr, nullptr);
	}

	glfwSetWindowUserPointer(m_Context.MainWindow, this);
	glfwSetFramebufferSizeCallback(m_Context.MainWindow, framebuffer_resize_callback);
}

void Instancing::Instance::update (double latency) {
	LOG_trace("{:s} {:f}", __FUNCSIG__, latency);

	if (m_Context.UiText.refresh) { // update quads
		m_Context.UiText.destroy_instance_buffer (m_Context.Vk.LogicalDevice);
		create_ui_data ();
		m_Context.UiText.create_instance_buffer (m_Context.Vk.LogicalDevice, m_Context.Vk.PhysicalDevice);
		m_Context.UiText.refresh = false;
	}
	glfwPollEvents ();
}

void Instancing::Instance::cleanup () {
	LOG_trace(__FUNCSIG__);

	glfwDestroyWindow(m_Context.MainWindow);

    glfwTerminate();
}

void Instancing::Instance::create_ui_data () {
	const Helper::FontAtlas& atlas = m_Context.ShonenPunkFont;
	std::string text = "THE QUICK BROWN FOX JUMPS OVER LAZY DOG";
	double total_advance_x = 0.0;
	double total_advance_y /*baseline*/ = atlas.MetricsLineHeight () + atlas.MetricsUnderlineY (); // UnderlineY is negetive
	double scale_by = 0.5 * atlas.AtlasFontsize () / atlas.AtlasHeight ();
	auto unicode_to_ascii = [](char in) -> unicode_t { return in; };
	auto& quads_storage = m_Context.UiText.Quads; m_Context.UiText.Quads.clear ();
	glm::vec2 dimensions_of_atlas = {atlas.AtlasWidth (), atlas.AtlasHeight ()};

	for (char charec: text) {
		if (const auto* glyph = atlas.getGlyph (unicode_to_ascii (charec))) {

			if (glyph->planeBounds.right - glyph->planeBounds.left > 0.01f) {
				glm::vec2 position {total_advance_x + (glyph->planeBounds.right - glyph->planeBounds.left)*0.5, total_advance_y + (glyph->planeBounds.top + glyph->planeBounds.bottom)*0.5};
				float rotation = 0.0;
				glm::vec2 scale = {glyph->planeBounds.right - glyph->planeBounds.left, glyph->planeBounds.top - glyph->planeBounds.bottom};
				glm::vec2 texCoord00 = {glyph->atlasBounds.left, dimensions_of_atlas.y - glyph->atlasBounds.top};
				glm::vec2 texCoordDelta  = {glyph->atlasBounds.right - glyph->atlasBounds.left, glyph->atlasBounds.top - glyph->atlasBounds.bottom};

				quads_storage.push_back ({
					.rotn_scale = {rotation, scale[0], scale[1]},
					.position = position,
					.texCoord00 = texCoord00,
					.texCoordDelta = texCoordDelta,
				});
			}
			total_advance_x += glyph->advance;
		} else {
			total_advance_x += 0.5;
		}
	}

	glm::vec2 mid = { total_advance_x * 0.5, (total_advance_y - atlas.MetricsUnderlineY ()) * 0.5 }; // UnderlineY is negetive
	for (auto& instance: quads_storage) {
		instance.position = mid - instance.position;
		instance.rotn_scale[1] *= scale_by;
		instance.rotn_scale[2] *= scale_by;
		instance.texCoord00 /= dimensions_of_atlas;
		instance.texCoordDelta /= dimensions_of_atlas;
		instance.position *= scale_by;
		instance.texID = 1;
	}
}