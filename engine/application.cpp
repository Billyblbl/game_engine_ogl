#ifndef GAPPLICATION
# define GAPPLICATION

#include <cstdio>
#include <GL/glew.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <inputs.cpp>
#include <glutils.cpp>
#include <blblstd.hpp>
#include <math.cpp>
#include <framebuffer.cpp>
#include <spall/profiling.cpp>

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
	for (auto&& i : OGLLogSeverity) if (i == severity)
		fprintf(stderr, "[%s] OpenGL Debug %u, %s on %u: %s\n", GLtoString(i).data(), type, GLtoString(type).data(), id, message);
}

static void glfw_error_callback(int error, const cstr description) {
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

struct App {
	GLFWwindow* window;
	Input::Context* inputs;
	v2u32 pixel_dimensions;
	u64 scene_id;

	static constexpr u64 ID_EXIT = 0;
	static App create(const cstr window_title, v2u32 window_dimensions, u64 start_scene) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		//TODO Proper error handling
		glfwSetErrorCallback(glfw_error_callback);
		assert(glfwInit());
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		auto window = glfwCreateWindow(window_dimensions.x, window_dimensions.y, window_title, NULL, NULL);
		assert(window);
		auto& input_context = Input::init_context(window);
		glfwMakeContextCurrent(window);
		glfwSwapInterval(1); //* Enable vsync

		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		default_framebuffer.dimensions = v2u32(display_w, display_h);
		default_framebuffer.clear_attachement = clear_bit(Color0Attc) | clear_bit(DepthAttc);
		return App{ window, &input_context, default_framebuffer.dimensions, start_scene };
	}

	void release() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	bool request_change_scene(u64 target_scene) {
		scene_id = target_scene;
		return true;
	}

	bool request_exit() { return request_change_scene(App::ID_EXIT); }

};

bool init_ogl(App& app, bool debug = DEBUG_GL) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
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
	// GL_GUARD(glEnable(GL_CULL_FACE));
	GL_GUARD(glDisable(GL_CULL_FACE));
	// GL_GUARD(glCullFace(GL_BACK));
	// GL_GUARD(glCullFace(GL_FRONT));
	GL_GUARD(glEnable(GL_DEPTH_TEST));
	GL_GUARD(glDepthFunc(GL_LEQUAL));
	GL_GUARD(glEnable(GL_BLEND));
	GL_GUARD(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	return true;
}

bool update(App& app, u64 target_scene) {
	PROFILE_SCOPE("Window update");
	defer{ glfwSwapBuffers(app.window); };
	int display_w, display_h;
	glfwGetFramebufferSize(app.window, &display_w, &display_h);
	app.pixel_dimensions.x = display_w;
	app.pixel_dimensions.y = display_h;

	if (glfwWindowShouldClose(app.window)) {
		app.scene_id = 0;
		return false;
	}
	PROFILE_SCOPE("Inputs");
	Input::poll(*app.inputs);
	return app.scene_id == target_scene;
}

#endif
