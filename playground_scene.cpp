#ifndef GPLAYGROUND_SCENE
# define GPLAYGROUND_SCENE

#include <GL/glew.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <application.cpp>
#include <glutils.cpp>
#include <utils.cpp>
#include <rendering.cpp>
#include <time.cpp>
#include <pool.cpp>
#include <transform.cpp>

#include <physics2d.cpp>

#define MAX_ENTITIES 10
#define MAX_DRAW_BATCH MAX_ENTITIES
#define AVERAGE_CACHE_LINE_ACCORDING_TO_THE_INTERNET 64

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


bool playground(App& app) {

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
	auto texture = Textures::loadFromFile(
		"C:/Users/billy/Documents/assets/boss-spaceship-2d-sprites-pixel-art/PNG_Parts&Spriter_Animation/Boss_ship1/Boss_ship7.png",
		Textures::Linear, Textures::Clamp
	);
	deferDo{ GL_GUARD(glDeleteTextures(1, &texture.id)); };

	//simple rect
	auto rect = createRectMesh(texture.dimensions);
	deferDo{ deleteMesh(rect); };

	// State
	auto clock = Time::Start();
	auto rotationSpeed = 1.f;
	auto camSpeed = 10.f;

	std::byte memory[MAX_ENTITIES * AVERAGE_CACHE_LINE_ACCORDING_TO_THE_INTERNET * 3];
	Arena allocator = { { memory, 0 } };

	auto entities = EntityRegistry::create(allocator, MAX_ENTITIES, 1);
	auto transforms = LinearDatabase<Transform2D>::create(allocator, MAX_ENTITIES);
	//TODO find better way to group entities for render stage
	auto toRender = LinearDatabase<Transform2D*>::create(allocator, MAX_ENTITIES);

	auto cameraEntity = entities.allocate();
	auto rect1Entity = entities.allocate();
	auto rect2Entity = entities.allocate();

	// Note(202210052223) : Those references are invalid when removing from the pool
	auto& cameraWorldTransform = transforms.add(cameraEntity);
	{
		auto& rect1Transform = transforms.add(rect1Entity);
		auto& rect2Transform = transforms.add(rect2Entity);
		toRender.add(rect1Entity, &rect1Transform);
		toRender.add(rect2Entity, &rect2Transform);
	}
	auto orthoCamera = OrthoCamera{ glm::vec3(app.pixelDimensions, -1) };
	auto viewProjectionMatrix = mapObject(glm::inverse(cameraWorldTransform.matrix()));
	auto drawBatchMatricesBuffer = mapBuffer<glm::mat4>(MAX_DRAW_BATCH);
	auto drawBatchMatrices = Pool<glm::mat4>{ drawBatchMatricesBuffer.obj };
	deferDo{
		GL_GUARD(glUnmapNamedBuffer(viewProjectionMatrix.id));
		GL_GUARD(glUnmapNamedBuffer(drawBatchMatricesBuffer.id));
	};


#pragma region physics

	auto toSimulate = LinearDatabase<b2Body*>::create(allocator, MAX_ENTITIES);

	auto gravity = b2Vec2(0.f, -9.f);
	auto world = b2World(gravity);

	{
		b2BodyDef bodyDef;
		b2PolygonShape box;
		b2FixtureDef fixtureDef;

		// Static body
		bodyDef.position.Set(0, -10);
		auto* staticBody = toSimulate.add(rect1Entity, world.CreateBody(&bodyDef));
		box.SetAsBox(100, 10);
		staticBody->CreateFixture(&box, 0);

		// dynamic body
		auto tr = transforms.find(rect2Entity);
		tr->translation += glm::vec2(0, 200);
		bodyDef.position.Set(tr->translation.x, tr->translation.y);
		bodyDef.type = b2_dynamicBody;
		auto* dynamicBody = toSimulate.add(rect2Entity, world.CreateBody(&bodyDef));
		box.SetAsBox(20, 20);
		fixtureDef.shape = &box;
		fixtureDef.density = 1;
		fixtureDef.friction = .3f;
		dynamicBody->CreateFixture(&fixtureDef);
	}

	auto physicsTimeStep = 1.f / 60.f;
	auto velocityIterations = 8;
	auto positionIterations = 3;
	auto physicsTimePoint = clock.totalElapsedTime.count();
	auto debugDraw = B2dDebugDraw();

	world.SetDebugDraw(&debugDraw);
	debugDraw.SetFlags(b2Draw::e_shapeBit);

#pragma endregion physics

	// Main loop
	fflush(stdout);
	while (update(app, __func__)) {
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

			// Test movements
			cameraWorldTransform.translation += camVelocity * clock.dt.count() * camSpeed;

			entities.reclaim();
		}

		if (clock.totalElapsedTime.count() - physicsTimePoint >= 0.f) {// Physics Update
			for (auto i = 0; i < toSimulate.pool.allocated().size(); i++) {
				auto body = toSimulate.pool[i];
				auto id = toSimulate.ids[i];

				auto transform = transforms.find(id);
				if (!transform)
					fprintf(stderr, "transform-less physics body %u", id);

				auto pos = b2Vec2(transform->translation.x, transform->translation.y);
				body->SetTransform(pos, -glm::radians(transform->rotation));

				//TODO proper awake on tranform change
				body->SetAwake(true);
			}

			world.Step(physicsTimeStep, velocityIterations, positionIterations);
			physicsTimePoint += physicsTimeStep;


			for (auto i = 0; i < toSimulate.pool.allocated().size(); i++) {
				auto body = toSimulate.pool[i];
				auto id = toSimulate.ids[i];

				auto transform = transforms.find(id);
				if (!transform)
					fprintf(stderr, "transform-less physics body %u", id);
				auto pos = body->GetPosition();
				transform->translation = glm::vec2(pos.x, pos.y);
				transform->rotation = -glm::degrees(body->GetAngle());
			}
		}

		{// Build Imgui overlay
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();

			ImGui::NewFrame();
			ImGui::Begin("Controls (0:camera, 1:rect1, 2:rec2)");
			{

				EditorWidget("Transforms", transforms.pool.allocated());
				EditorWidget("Rigidbodies 2D", toSimulate.pool.allocated());
				EditorWidget("Ortho Camera", orthoCamera);
				ImGui::Text("Movement");
				EditorWidget("Camera Speed", camSpeed);
				EditorWidget("Velocity", camVelocity);

				ImGui::Separator();
				ImGui::Text("Physics");
				PhysicsControls(
					world,
					physicsTimeStep,
					velocityIterations,
					positionIterations,
					physicsTimePoint
				);
				ImGui::Separator();

				ImGui::Text("Time = %f", clock.totalElapsedTime.count());
			}
			ImGui::End();
			ImGui::Render();

			viewProjectionMatrix.obj = orthoCamera.matrix() * glm::inverse(cameraWorldTransform.matrix());
		}

		{// Render scene

			{ // Update viewport
				int display_w, display_h;
				glfwGetFramebufferSize(app.window, &display_w, &display_h);
				orthoCamera.dimensions.x = display_w;
				orthoCamera.dimensions.y = display_h;
				GL_GUARD(glViewport(0, 0, display_w, display_h));
			}

			GL_GUARD(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

			{ // Draw entities
				auto ubos = std::array{ bind(viewProjectionMatrix, 0) }; // Scene global data
				auto ssbos = std::array{ bind(drawBatchMatricesBuffer, 0) }; // Entities unique data
				auto textures = std::array{ bind(texture, 0) }; // Entities shared data
				for (auto&& transform : toRender.pool.allocated()) {
					//!Should only keep things with the same render data in the same batch
					if (drawBatchMatrices.count >= drawBatchMatrices.buffer.size()) {
						draw(drawPipeline, rect, drawBatchMatrices.count, textures, ssbos, ubos);
						drawBatchMatrices.count = 0;
					}
					drawBatchMatrices.add(transform->matrix());
				}
				draw(drawPipeline, rect, drawBatchMatrices.count, textures, ssbos, ubos);
				drawBatchMatrices.count = 0;
			}
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

		debugDraw.viewTransform = viewProjectionMatrix.obj;
		world.DebugDraw();
	}
	return app.focusPath.size() > 0;
}

#endif
