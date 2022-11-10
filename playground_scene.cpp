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

bool playground(App& app) {

	ImGui::Init_OGL_GLFW(app.window);
	deferDo { ImGui::Shutdown(); };

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

			orthoCamera.dimensions.x = app.pixelDimensions.x;
			orthoCamera.dimensions.y = app.pixelDimensions.y;
			GL_GUARD(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

			{ // Draw entities
				auto drawBatchMatrices = Pool<glm::mat4>{ drawBatchMatricesBuffer.obj };
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
				// drawBatchMatrices.count = 0;
			}
		}

		ImGui::Draw();

		debugDraw.viewTransform = viewProjectionMatrix.obj;
		world.DebugDraw();
	}
	return app.focusPath.size() > 0;
}

#endif
