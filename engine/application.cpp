#ifndef GAPPLICATION
# define GAPPLICATION

#include <cstdio>
#include <GL/glew.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <inputs.cpp>
#include <glutils.cpp>
#include <blblstd.hpp>
#include <math.cpp>

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
	const void* user_param
) {
	for (auto&& i : OGLLogSeverity) if (i == severity) {
		fprintf(stderr, "OpenGL Debug %u: %s on %u, %s\n", type, GLtoString(type).data(), id, message);
	}
}

static void glfw_error_callback(int error, const cstr description) {
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

struct App;

using Scene = bool(*)(App&);

struct App {
	GLFWwindow* window;
	Input::Context& inputs;
	v2u32 pixel_dimensions;
	Scene scene;
};

auto create_app(const cstr window_title, v2u32 window_dimensions, Scene start_scene = null) {
	//TODO Proper error handling
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		abort();
	// Create window with graphics context
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	auto window = glfwCreateWindow(window_dimensions.x, window_dimensions.y, window_title, NULL, NULL);
	if (window == NULL)
		abort();
	auto& input_context = Input::init_context(window);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);

	return App{ window, input_context, v2u32(display_w, display_h), start_scene };
}

bool init_ogl(App& app, bool debug = true) {
	printf("Initializing OpenGL\n");
	GLenum err = glewInit();
	if (GLEW_OK != err)
		return fail_ret(glewGetErrorString(err), false);
	printf("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	if (debug) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(ogl_debug_callback, null);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, null, GL_TRUE);
	}

	GL_GUARD(glViewport(0, 0, app.pixel_dimensions.x, app.pixel_dimensions.y));
	GL_GUARD(glEnable(GL_CULL_FACE));
	// GL_GUARD(glDisable(GL_CULL_FACE));
	GL_GUARD(glCullFace(GL_BACK));
	// GL_GUARD(glCullFace(GL_FRONT));
	GL_GUARD(glEnable(GL_DEPTH_TEST));
	GL_GUARD(glDepthFunc(GL_LEQUAL));
	GL_GUARD(glEnable(GL_BLEND));
	GL_GUARD(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	return true;
}

bool update(App& app, Scene target_scene) {
	defer{ glfwSwapBuffers(app.window); };
	int display_w, display_h;
	glfwGetFramebufferSize(app.window, &display_w, &display_h);
	app.pixel_dimensions.x = display_w;
	app.pixel_dimensions.y = display_h;

	if (glfwWindowShouldClose(app.window)) {
		app.scene = null;
		return false;
	}
	Input::poll(app.inputs);
	return app.scene == target_scene;
}

void destroy(App& app) {
	glfwDestroyWindow(app.window);
	glfwTerminate();
}

bool change_scene(App& app, Scene new_scene) {
	app.scene = new_scene;
	return true;
}

#endif
