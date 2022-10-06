#ifndef GAPPLICATION
# define GAPPLICATION

#include <GL/glew.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <inputs.cpp>
#include <string_view>
#include <cstdio>

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

struct App {
	GLFWwindow* window;
	Input::Context& inputs;
	glm::uvec2 pixelDimensions;
	std::string_view focusPath = "";
};

auto createApp(const char* windowTitle, glm::uvec2 windowDimensions, uint8_t gamepadCount = 0) {
	//TODO Proper error handling
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		abort();
	// auto dimensions = glm::ivec2(1920 / 2, 1080 / 2);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	// Create window with graphics context
	auto window = glfwCreateWindow(windowDimensions.x, windowDimensions.y, windowTitle, NULL, NULL);
	if (window == NULL)
		abort();

	auto availableGamepads = getGamepads();
	printf("Detected %lu gamepads\n", availableGamepads.size());
	auto trackedGamepads = (gamepadCount < availableGamepads.size()) ? gamepadCount : availableGamepads.size();
	auto& inputContext = allocateInputContext(window, availableGamepads.subspan(0, trackedGamepads));
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	return App{ window, inputContext, windowDimensions };
}

bool update(App& app, std::string_view focusPath) {
	if (glfwWindowShouldClose(app.window)) {
		app.focusPath = "";
		return false;
	}
	if (!app.focusPath.starts_with(focusPath)) {
		return false;
	}
	glfwSwapBuffers(app.window);
	return true;
}

void destroyApp(App& app) {
	glfwDestroyWindow(app.window);
	glfwTerminate();
}

bool exitApp(App&) { return false; }

#endif
