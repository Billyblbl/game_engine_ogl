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
#include <time.cpp>

const static GLenum OGLLogSeverity[] = {
	GL_DEBUG_SEVERITY_HIGH,
	GL_DEBUG_SEVERITY_MEDIUM,
	GL_DEBUG_SEVERITY_LOW
	// GL_DEBUG_SEVERITY_NOTIFICATION
};

static void ogl_debug_callback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const char* message,
	const void*
) {

	string s_source;
	switch (source) {
		case GL_DEBUG_SOURCE_API: 						s_source = "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:		s_source = "Window"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:	s_source = "Shader"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:			s_source = "3rd Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:			s_source = "App"; break;
		case GL_DEBUG_SOURCE_OTHER:						s_source = "Other"; break;
		default: s_source = "Unknown";
	}

	string s_type;
	switch (type) {
		case GL_DEBUG_TYPE_ERROR:								s_type = "Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:	s_type = "Deprecated Behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:	s_type = "Undefined Behaviour"; break; 
		case GL_DEBUG_TYPE_PORTABILITY:					s_type = "Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:					s_type = "Performance"; break;
		case GL_DEBUG_TYPE_MARKER:							s_type = "Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:					s_type = "Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:						s_type = "Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER:								s_type = "Other"; break;
		default: s_type = "Unknown";
	}

	string s_severity;
	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:					s_severity = "High"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:				s_severity = "Medium"; break;
		case GL_DEBUG_SEVERITY_LOW:						s_severity = "Low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:	s_severity = "Notification"; break;
		default: s_severity = "Unknown";
	}

	constexpr GLuint BUFFER_DETAILED_INFO	= 131185;
	constexpr GLuint SILENT[] = {
		BUFFER_DETAILED_INFO
	};

	for (auto ignored : SILENT) if (id == ignored)
		return;
	fprintf(stderr, "[%.*s].[%.*s].[%.*s] (%u): %.*s\n",
		(int)s_severity.size(), s_severity.data(),
		(int)s_source.size(), s_source.data(),
		(int)s_type.size(), s_type.data(),
		id,
		(int)length, message
	);
	constexpr GLuint DEBUG_IGNORED[] = {
		// BUFFER_DETAILED_INFO
	};
	for (auto ignored : DEBUG_IGNORED) if (id == ignored)
		return;
	__debugbreak();
}

static void glfw_error_callback(int error, const cstr description) {
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

struct App {
	GLFWwindow* window;//TODO maybe ? handle multiple windows
	Input::Context* inputs;
	v2u32 pixel_dimensions;

	static App create(const cstr window_title, v2u32 window_dimensions) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		//TODO Proper error handling
		glfwSetErrorCallback(glfw_error_callback);
		assert(glfwInit());
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		auto window = glfwCreateWindow(window_dimensions.x, window_dimensions.y, window_title, NULL, NULL);
		assert(window);
		auto& input_context = Input::init_context(window);
		glfwMakeContextCurrent(window);
		glfwSwapInterval(1); //* Enable vsync

		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		return App{
			.window = window,
			.inputs = &input_context,
			.pixel_dimensions = v2u32(display_w, display_h),
		};
	}

	void release() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	bool update() {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		pixel_dimensions.x = display_w;
		pixel_dimensions.y = display_h;
		if (glfwWindowShouldClose(window))
			return false;
		{
			PROFILE_SCOPE("sync");
			glfwSwapBuffers(window);
		}
		PROFILE_SCOPE("Inputs");
		Input::poll(*inputs);
		return true;
	}

};

bool init_ogl(bool debug = DEBUG_GL) {
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

	return true;
}

RenderPass window_renderpass(GLFWwindow* window) {
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	return {
		.framebuffer = 0,
		.viewport = { v2u32(0, 0), v2u32(w, h) },
		.scissor = { v2u32(0, 0), v2u32(w, h) }
	};
}

RenderTarget window_render_target(GLFWwindow* window) {
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	return {
		.dimensions = v2u32(w, h),
		.framebuffer = 0,
		.attachments = {}
	};
}

#endif
