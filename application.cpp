#ifndef GAPPLICATION
# define GAPPLICATION

#include <string_view>
#include <cstdio>
#include <GL/glew.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <inputs.cpp>
#include <glutils.cpp>

const static GLenum OGLLogSeverity[] = {
	GL_DEBUG_SEVERITY_HIGH,
	GL_DEBUG_SEVERITY_MEDIUM,
	GL_DEBUG_SEVERITY_LOW
	// GL_DEBUG_SEVERITY_NOTIFICATION
};

static void ogl_debug_callback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam
) {
	for (auto&& i : OGLLogSeverity) if (i == severity) {
		fprintf(stderr, "OpenGL Debug %d: %s on %u, %s\n", type, GLtoString(type), id, message);
	}
}

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

	return App{ window, inputContext, windowDimensions, "" };
}

bool initOpenGL(App& app) {
	//TODO error handling
	printf("Initializing OpenGL\n");
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
		return false;
	}
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	auto clear_color = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);

	int display_w, display_h;
	glfwGetFramebufferSize(app.window, &display_w, &display_h);

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(ogl_debug_callback, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);

	GL_GUARD(glViewport(0, 0, display_w, display_h));
	GL_GUARD(glEnable(GL_CULL_FACE));
	// GL_GUARD(glDisable(GL_CULL_FACE));
	GL_GUARD(glCullFace(GL_BACK));
	// GL_GUARD(glCullFace(GL_FRONT));
	GL_GUARD(glEnable(GL_DEPTH_TEST));
	GL_GUARD(glDepthFunc(GL_LEQUAL));
	GL_GUARD(glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w));
	GL_GUARD(glEnable(GL_BLEND));
	GL_GUARD(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	return true;
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
