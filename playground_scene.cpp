#ifndef GPLAYGROUND_SCENE
# define GPLAYGROUND_SCENE

#include <GL/glew.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <application.cpp>
#include <glutils.cpp>
#include <rendering.cpp>
#include <time.cpp>
#include <transform.cpp>

#include <physics2d.cpp>

#define MAX_ENTITIES 10
#define MAX_DRAW_BATCH MAX_ENTITIES

bool playground(App& app) {

	struct Entity {
		enum FlagIndex: u64 {
			Dynbody,
			Collision,
			Sprite,
			Player
		};
		u64 flags = 0;
		b2Body* body;
		//TODO add shape for physics
		Transform2D transform;
		RenderMesh* mesh;
		Textures::Texture* texture;
	} entities[MAX_ENTITIES];

	ImGui::init_ogl_glfw(app.window);
	defer{ ImGui::shutdown_ogl_glfw(); };

	//Render data
	auto draw_pipeline = create_render_pipeline(
		load_shader("./shaders/camera2DRenderTexturedBatched.vert", GL_VERTEX_SHADER),
		load_shader("./shaders/camera2DRenderTexturedBatched.frag", GL_FRAGMENT_SHADER)
	);
	defer{ GL_GUARD(glDeleteProgram(draw_pipeline)); };
	auto texture = Textures::load_from_file(
		"C:/Users/billy/Documents/assets/boss-spaceship-2d-sprites-pixel-art/PNG_Parts&Spriter_Animation/Boss_ship1/Boss_ship7.png",
		Textures::Linear, Textures::Clamp
	);
	defer{ GL_GUARD(glDeleteTextures(1, &texture.id)); };

	//simple rect
	auto rect = create_rect_mesh(texture.dimensions);
	defer{ delete_mesh(rect); };

	// Scene state
	struct {
		Time::Clock clock = Time::start();
		f32 rotationSpeed = 1.f;
		f32 camSpeed = 10.f;
	} scene;

	struct {
		OrthoCamera camera;
		MappedObject<m4x4f32> view_projection_matrix = map_object(glm::inverse(Transform2D{}.matrix()));
		MappedBuffer<m4x4f32> draw_batch_matrices_buffer = map_buffer<m4x4f32>(MAX_DRAW_BATCH);
	} rendering;
	rendering.camera = { v3f32(app.pixel_dimensions, -1) };

	defer{
		GL_GUARD(glUnmapNamedBuffer(rendering.view_projection_matrix.id));
		GL_GUARD(glUnmapNamedBuffer(rendering.draw_batch_matrices_buffer.id));
	};

	struct {
		PhysicsConfig config;
		b2World world = b2World(b2Vec2(0.f, -9.f));
		B2dDebugDraw debug_draw;
		f32 time_point = 0.f;
	} physics;
	physics.world.SetDebugDraw(&physics.debug_draw);
	physics.debug_draw.SetFlags(b2Draw::e_shapeBit);

	fflush(stdout);
	while (update(app, playground)) {
		{// General Update
			auto cam_velocity = v2f32(0);
			update(scene.clock);
			Input::poll(app.inputs);
			cam_velocity = Input::composite(
				app.inputs.keyStates[Input::Keys::index_of(Input::Keys::A)],
				app.inputs.keyStates[Input::Keys::index_of(Input::Keys::D)],
				app.inputs.keyStates[Input::Keys::index_of(Input::Keys::S)],
				app.inputs.keyStates[Input::Keys::index_of(Input::Keys::W)]
			);
			// cameraWorldTransform.translation += cam_velocity * clock.dt.count() * camSpeed;
		}

		// Using a 'while' here instead of an 'if' makes sure that if a frame was slow the physics simulation can catch up
		// by doing multiple steps in 1 frame
		// otherwise slowness in the rendering would also slow down the physics
		// althout not sure how pertinent this is since slow rendering would probably be a bigger problem
		if (Time::metronome(scene.clock.current, physics.config.time_step, physics.time_point)) { // Physics tick
			tuple<Transform2D*, b2Body*> pbuff[MAX_ENTITIES];
			auto to_sim = List{ larray(pbuff), 0 };
			for (auto& ent : entities) if (has_all(ent.flags, mask(Entity::Dynbody)))
				to_sim.push({&ent.transform, ent.body});
			update_physics(physics.world, physics.config, to_sim.allocated());
		}

		{// Build Imgui overlay
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGui::Begin("Controls");
			physics_controls(physics.world, physics.config, physics.time_point);
			ImGui::Text("Time = %f", scene.clock.current);
			ImGui::End();
			ImGui::Render();
		}

		{// Rendering
			//TODO instanciate camera entity somewhere
			// rendering.view_projection_matrix.obj = rendering.camera.matrix() * glm::inverse(cameraWorldTransform.matrix());
			rendering.camera.dimensions.x = app.pixel_dimensions.x;
			rendering.camera.dimensions.y = app.pixel_dimensions.y;
			render(0, { v2u32(0), app.pixel_dimensions }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
				[&]() {

					//Draw entities
					for (auto&& ent : entities) if (has_all(ent.flags, mask(Entity::Sprite))) {
						auto draw_batch_matrices = List{ rendering.draw_batch_matrices_buffer.obj, 0 };
						draw_batch_matrices.push(ent.transform.matrix());
						GPUBinding textures[] = { bind(*ent.texture, 0) };
						GPUBinding ssbos[] = { bind(rendering.draw_batch_matrices_buffer, 0) };
						GPUBinding ubos[] = { bind(rendering.view_projection_matrix, 0) };
						draw(draw_pipeline, *ent.mesh, draw_batch_matrices.current, larray(textures), larray(ssbos), larray(ubos));
					}

					//Draw overlay
					ImGui::Draw();

					//Draw physics debug
					physics.debug_draw.view_transform.obj = rendering.view_projection_matrix.obj;
					physics.world.DebugDraw();
				}
			);
		}
	}
	return true;
}

#endif
