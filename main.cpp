// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include <limits>
#include <stdio.h>
#include <chrono>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <time.cpp>
#include <glutils.cpp>
#include <rendering.cpp>
#include <vertex.cpp>
#include <transform.cpp>
#include <buffer.cpp>
#include <textures.cpp>
#include <utils.cpp>
#include <inputs.cpp>
#include <model.cpp>
#include <pool.cpp>

#define MAX_ENTITIES 10
#define MAX_DRAW_BATCH MAX_ENTITIES
#define AVERAGE_CACHE_LINE_ACCORDING_TO_THE_INTERNET 64

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

const static GLenum SeverityIncluded[] = {
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
	for (auto&& i : SeverityIncluded) if (i == severity) {
		fprintf(stderr, "OpenGL Debug %d: %s on %u, %s\n", type, GLtoString(type), id, message);
	}
}

const char* getVersion() {
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 4.6 + GLSL 130
	//TODO check if we can use more up to date glsl version
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif
	return glsl_version;
}
	
struct App {
	GLFWwindow* window;
	Input::Context& inputs;
	glm::uvec2 pixelDimensions;
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

	return App { window, inputContext, windowDimensions };
}

void destroyApp(App& app) {
	glfwDestroyWindow(app.window);
	glfwTerminate();
}

int main(int ac, char** av) {

	auto app = createApp("Test renderer", glm::uvec2(1920 / 2, 1080 / 2), 1);
	deferDo{ destroyApp(app); };

	{// Init OpenGL
		printf("Initializing OpenGL\n");
		GLenum err = glewInit();
		if (GLEW_OK != err) {
			fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
			return err;
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
	}

	{// Init DearImgui
		printf("Initializing DearImgui\n");
		const char* glsl_version = getVersion();
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(app.window, true);
		ImGui_ImplOpenGL3_Init(glsl_version);
	}
	deferDo{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	};

	auto& imguiIO = ImGui::GetIO();

	//Render data
	auto drawPipeline = createRenderPipeline(
		loadShader("./shaders/camera2DRenderTexturedBatched.vert", GL_VERTEX_SHADER),
		loadShader("./shaders/camera2DRenderTexturedBatched.frag", GL_FRAGMENT_SHADER)
	);
	deferDo{ GL_GUARD(glDeleteProgram(drawPipeline)); };

	auto texture = Textures::loadFromFile("C:/Users/billy/Documents/assets/boss-spaceship-2d-sprites-pixel-art/PNG_Parts&Spriter_Animation/Boss_ship1/Boss_ship7.png", Textures::Linear, Textures::Clamp);
	deferDo{ GL_GUARD(glDeleteTextures(1, &texture.id)); };

	//simple rect
	auto [vertices, indices] = createRect(texture.dimensions);
	auto mesh = uploadMesh(std::span(vertices.data(), vertices.size()), indices);
	deferDo{ deleteMesh(mesh); };

	// State
	auto clock = Time::Start();
	auto rotationSpeed = 1.f;

	std::byte memory[MAX_ENTITIES * AVERAGE_CACHE_LINE_ACCORDING_TO_THE_INTERNET];
	Arena allocator = { { memory, 0 } };

	uint32_t nextID = 1;

	//Pseudo components
	auto entityTransforms = LinearDatabase<Transform2D>::create(allocator, MAX_ENTITIES);
	//TODO find better way to group entities for render stage
	auto renderableEntities = LinearDatabase<Transform2D*>::create(allocator, MAX_ENTITIES);

	auto cameraEntity = nextID++;
	auto rect1Entity = nextID++;
	auto rect2Entity = nextID++;

	// Note(202210052223) : Those references are invalid when removing from the pool
	auto& cameraTransform = entityTransforms.add(cameraEntity);
	{
		auto& rect1Transform = entityTransforms.add(rect1Entity);
		auto& rect2Transform = entityTransforms.add(rect2Entity);
		renderableEntities.add(rect1Entity, &rect1Transform);
		renderableEntities.add(rect2Entity, &rect2Transform);
	}
	auto orthoCamera = OrthoCamera{ glm::vec3(app.pixelDimensions, -1) };
	auto viewProjectionMatrix = mapObject(glm::inverse(cameraTransform.matrix()));
	auto drawBatchMatricesBuffer = mapBuffer<glm::mat4>(MAX_DRAW_BATCH);
	auto drawBatchMatrices = Pool<glm::mat4>{ drawBatchMatricesBuffer.obj };

	auto camSpeed = 10.f;

	deferDo{
		GL_GUARD(glUnmapNamedBuffer(viewProjectionMatrix.id));
		GL_GUARD(glUnmapNamedBuffer(drawBatchMatricesBuffer.id));
	};
	fflush(stdout);

	// Main loop
	while (!glfwWindowShouldClose(app.window)) {
		auto camVelocity = glm::vec2(0);
		{// General Update
			clock.Update();

			Input::poll(app.inputs);
			camVelocity = Input::composite(
				app.inputs.keyStates[indexOf(GLFW::Keys::A)],
				app.inputs.keyStates[indexOf(GLFW::Keys::D)],
				app.inputs.keyStates[indexOf(GLFW::Keys::S)],
				app.inputs.keyStates[indexOf(GLFW::Keys::W)]
			);

			{ // Update viewport
				int display_w, display_h;
				glfwGetFramebufferSize(app.window, &display_w, &display_h);
				orthoCamera.dimensions.x = display_w;
				orthoCamera.dimensions.y = display_h;
				GL_GUARD(glViewport(0, 0, display_w, display_h));
			}

			{ // Test movements
				cameraTransform.translation += camVelocity * clock.dt.count() * camSpeed;
				auto rect1Transform = entityTransforms.find(rect1Entity);
				rect1Transform->rotation += clock.dt.count() * rotationSpeed;
				while (rect1Transform->rotation > 360 * 2)
					rect1Transform->rotation -= 360 * 2;
			}

			GL_GUARD(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		}

		{// Build Imgui overlay
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();

			ImGui::NewFrame();
			ImGui::Begin("Controls (0:camera, 1:rect1, 2:rec2)");
			{
				EditorWidget("Transforms", entityTransforms.pool.allocated());
				EditorWidget("Ortho Camera", orthoCamera);
				ImGui::Text("Animation");
				EditorWidget("Rotation Speed", rotationSpeed);
				ImGui::Text("Keyboard");
				EditorWidget("Camera Speed", camSpeed);
				EditorWidget("Velocity", camVelocity);
			}
			ImGui::End();
			ImGui::Render();

			viewProjectionMatrix.obj = orthoCamera.matrix() * glm::inverse(cameraTransform.matrix());
		}

		{// Render scene
			auto ubos = std::array{ bind(viewProjectionMatrix, 0) }; // Scene global data
			auto ssbos = std::array{ bind(drawBatchMatricesBuffer, 0) }; // Entities unique data
			auto textures = std::array{ bind(texture, 0) }; // Entities shared data
			for (auto&& transform : renderableEntities.pool.allocated()) {
				//!Should only keep things with the same render data in the same batch
				if (drawBatchMatrices.count >= drawBatchMatrices.buffer.size()) {
					draw(drawPipeline, mesh, drawBatchMatrices.count, textures, ssbos, ubos);
					drawBatchMatrices.count = 0;
				}
				drawBatchMatrices.add(transform->matrix());
			}
			draw(drawPipeline, mesh, drawBatchMatrices.count, textures, ssbos, ubos);
			drawBatchMatrices.count = 0;
		}

		{// Draw overlay
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			if (imguiIO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}
		}

		glfwSwapBuffers(app.window);
	}

	// Cleanup
	return 0;
}
