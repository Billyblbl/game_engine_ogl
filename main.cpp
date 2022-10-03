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

#define MAX_ENTITIES 10
#define MAX_DRAW_BATCH 10
#define CAMERA_TRANSFORM_INDEX 0
#define RECT_TRANSFORM_INDEX 1

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

//TODO replace with ECS-like system
struct Renderable {
	Transform2D* transform = nullptr;
	RenderData* renderData = nullptr;
};

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

int main(int ac, char** av) {
	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	auto dimensions = glm::ivec2(1920 / 2, 1080 / 2);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(dimensions.x, dimensions.y, "Test renderer", NULL, NULL);
	if (window == NULL)
		return 1;

	auto availableGamepads = getGamepads();
	printf("Detected %lu gamepads\n", availableGamepads.size());
	auto& inputContext = allocateInputContext(window, availableGamepads);
	{
		glfwMakeContextCurrent(window);
		glfwSwapInterval(1); // Enable vsync

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
			glfwGetFramebufferSize(window, &display_w, &display_h);

			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(ogl_debug_callback, NULL);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);

			GL_GUARD(glViewport(0, 0, display_w, display_h));

			// GL_GUARD(glEnable(GL_CULL_FACE));
			GL_GUARD(glDisable(GL_CULL_FACE));
			// GL_GUARD(glCullFace(GL_BACK));
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
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL3_Init(glsl_version);

			// Load Fonts
			// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
			// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
			// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
			// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
			// - Read 'docs/FONTS.md' for more instructions and details.
			// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
			//io.Fonts->AddFontDefault();
			//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
			//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
			//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
			//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
			//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
			//IM_ASSERT(font != NULL);
		}

		auto imguiIO = ImGui::GetIO();

		//Render data
		auto drawPipeline = createRenderPipeline(
			loadShader("./shaders/camera2DRenderTexturedBatched.vert", GL_VERTEX_SHADER),
			loadShader("./shaders/camera2DRenderTexturedBatched.frag", GL_FRAGMENT_SHADER)
		);
		deferDo{ GL_GUARD(glDeleteProgram(drawPipeline)); };

		// auto whitePixel = 1.f;
		// auto whiteTexture = Textures::createFromSource(std::span(&whitePixel, 1), glm::uvec2(1));
		// glm::vec3 testTexture[] = {
		// 	glm::vec3(0, 0, 0),
		// 	glm::vec3(0, 0, 1),
		// 	glm::vec3(0, 1, 0),
		// 	glm::vec3(0, 1, 1),
		// 	glm::vec3(1, 0, 0),
		// 	glm::vec3(1, 0, 1),
		// 	glm::vec3(1, 1, 0),
		// 	glm::vec3(1, 1, 1),
		// };
		// auto texture = Textures::createFromSource(std::span(testTexture), glm::uvec2(4, 2), Textures::Nearest);
		auto texture = Textures::loadFromFile("C:/Users/billy/Documents/assets/boss-spaceship-2d-sprites-pixel-art/PNG_Parts&Spriter_Animation/Boss_ship1/Boss_ship7.png", Textures::Linear, Textures::Clamp);
		deferDo{ GL_GUARD(glDeleteTextures(1, &texture.id)); };

		//simple rect
		auto vertices = std::array{
			DefaultVertex2D { glm::vec2(-300.f, -300.f), glm::vec2(0, 1) },
			DefaultVertex2D { glm::vec2(-300.f, 300.f), glm::vec2(0, 0) },
			DefaultVertex2D { glm::vec2(300.f, 300.f), glm::vec2(1, 0) },
			DefaultVertex2D { glm::vec2(300.f, -300.f), glm::vec2(1, 1) }
		};
		auto indices = std::array{
			// 0u, 1u, 2u,
			// 0u, 2u, 3u
			0u, 1u, 3u,
			1u, 2u, 3u
		};

		auto vbo = createBufferSpan(std::span(vertices));
		auto ibo = createBufferSpan(std::span(indices));
		auto vao = recordVAO<DefaultVertex2D>(vbo, ibo);

		auto bindings = std::array{ bind(texture, 0) };

		Material matrl = { drawPipeline, std::span(bindings) };
		RenderData rdData = { vao, &matrl };

		deferDo{
			fflush(stdout);
			GL_GUARD(glDeleteVertexArrays(1, &vao));
			GLuint buffers[] = {vbo, ibo};
			GL_GUARD(glDeleteBuffers(2, buffers));
		};

		// State
		auto clock = Time::Start();
		auto rotationSpeed = 1.f;

		//Pseudo components
		Transform2D entityTransforms[MAX_ENTITIES];
		RenderData entityRenderData[MAX_ENTITIES];
		Renderable renderableEntities[MAX_ENTITIES];

		auto& cameraTransform = entityTransforms[CAMERA_TRANSFORM_INDEX];
		auto& rect1 = renderableEntities[0] = { &entityTransforms[RECT_TRANSFORM_INDEX], &entityRenderData[0] };
		auto& rect2 = renderableEntities[1] = { &entityTransforms[RECT_TRANSFORM_INDEX + 1], &entityRenderData[1] };

		const auto renderableEntitiesCount = 2;
		const auto usedTransforms = 3;

		auto orthoCamera = OrthoCamera{ glm::vec3(dimensions, 1) };

		auto drawBatchMatrices = mapBuffer<glm::mat4>(MAX_DRAW_BATCH);
		auto reservedBatchSlots = 0;

		auto viewProjectionMatrix = mapObject(glm::inverse(cameraTransform.matrix()));

		// TODO proper input targets
		auto camSpeed = 10.f;

		deferDo{
			GL_GUARD(glUnmapNamedBuffer(viewProjectionMatrix.id));
			GL_GUARD(glUnmapNamedBuffer(drawBatchMatrices.id));
		};

		fflush(stdout);

		// Main loop
		while (!glfwWindowShouldClose(window)) {
			// Poll and handle events (inputs, window resize, etc.)
			// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
			// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
			// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
			// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.

			auto camVelocity = glm::vec2(0);
			{// General Update
				clock.Update();

				Input::poll(inputContext);

				for (auto&& [id, state] : inputContext.gamepads) if (id == availableGamepads[0]) {
					camVelocity.x = state.axes[GLFW::Gamepad::LEFT_X];
					camVelocity.y = state.axes[GLFW::Gamepad::LEFT_Y];
				}

				// if (inputContext.keyStates[indexOf(GLFW::Keys::W)] & Input::Button::Pressed)
				// 	camVelocity.y += 1.f;
				// if (inputContext.keyStates[indexOf(GLFW::Keys::A)] & Input::Button::Pressed)
				// 	camVelocity.x -= 1.f;
				// if (inputContext.keyStates[indexOf(GLFW::Keys::S)] & Input::Button::Pressed)
				// 	camVelocity.y -= 1.f;
				// if (inputContext.keyStates[indexOf(GLFW::Keys::D)] & Input::Button::Pressed)
				// 	camVelocity.x += 1.f;

				int display_w, display_h;
				glfwGetFramebufferSize(window, &display_w, &display_h);
				orthoCamera.dimensions.x = display_w;
				orthoCamera.dimensions.y = display_h;
				cameraTransform.translation += camVelocity * clock.dt.count() * camSpeed;
				rect1.transform->rotation += clock.dt.count() * rotationSpeed;
				if (rect1.transform->rotation > 360 * 2)
					rect1.transform->rotation -= 360 * 2;
				GL_GUARD(glViewport(0, 0, display_w, display_h));
				GL_GUARD(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
			}

			{//Imgui overlay
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();

				ImGui::NewFrame();
				ImGui::Begin("Controls (0:camera, 1:rect1, 2:rec2)");
				{

					for (size_t i = 0; i < usedTransforms; i++) {
						auto& transform = entityTransforms[i];
						ImGui::PushID(i);
						ImGui::Text("Transform2D");
						ImGui::DragFloat2("Position", (float*)&transform.translation, .1f, .0f, .0f, "%.3f");
						ImGui::DragFloat("Rotation", &transform.rotation, .1f, .0f, .0f, "%.3f");
						ImGui::DragFloat2("Scale", (float*)&transform.scale, .1f, .0f, .0f, "%.3f");
						ImGui::PopID();
					}

					ImGui::Text("Ortho Camera");
					ImGui::DragFloat3("Dimensions", (float*)&orthoCamera.dimensions, .1f, .0f, .0f, "%.3f");
					ImGui::DragFloat3("Center", (float*)&orthoCamera.center, .1f, .0f, .0f, "%.3f");

					ImGui::Text("Animation");
					ImGui::DragFloat("Rotation Speed", &rotationSpeed, .1f, .0f, .0f, "%.3f");

					ImGui::Text("Keyboard");
					ImGui::DragFloat("Camera Speed", &camSpeed, .1f, .0f, .0f, "%.3f");
					ImGui::InputFloat2("Velocity", (float*)&camVelocity);
				}
				ImGui::End();
				ImGui::Render();

				viewProjectionMatrix.obj = orthoCamera.matrix() * glm::inverse(cameraTransform.matrix());
			}

			{//Render scene

				for (auto&& ent : std::span(renderableEntities).subspan(0, renderableEntitiesCount)) {
					//!Should only keep things with the same RenderData in the same batch
					drawBatchMatrices.obj[reservedBatchSlots++] = ent.transform->matrix();
				}

				auto ubos = std::array{ bind(viewProjectionMatrix, 0) };
				auto ssbos = std::array{ bind(drawBatchMatrices, 0) };
				auto textures = std::array{ bind(texture, 0) };
				draw(
					drawPipeline, vao, indices.size(), reservedBatchSlots,
					textures,
					ssbos,
					ubos
				);
				reservedBatchSlots = 0;
			}

			{// Draw overlay
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
				// Update and Render additional Platform Windows
				// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
				//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
				if (imguiIO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
					GLFWwindow* backup_current_context = glfwGetCurrentContext();
					ImGui::UpdatePlatformWindows();
					ImGui::RenderPlatformWindowsDefault();
					glfwMakeContextCurrent(backup_current_context);
				}
			}

			glfwSwapBuffers(window);
		}
	}

	{// Cleanup

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();

	}

	return 0;
}
