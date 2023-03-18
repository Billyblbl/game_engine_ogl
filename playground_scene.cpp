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

#include <top_down_controls.cpp>

#define MAX_ENTITIES 10
#define MAX_DRAW_BATCH MAX_ENTITIES

struct Entity {
	enum FlagIndex: u64 {
		Dynbody,
		Collision,
		Sprite,
		Player
	};
	u64 flags = 0;
	Transform2D transform;
	b2Body* body;
	//TODO add shape for physics
	RenderMesh* mesh;
	Textures::Texture* texture;
	f32 speed = 10.f;
	f32 accel = 5.f;
	str name = "__entity__";
};

bool bit_flags(const cstr label, u64& flags, Array<const str> bit_names) {
	bool changed = false;
	ImGui::Text(label);
	ImGui::SameLine();
	for (u64 bit_idx : u64range{ 0, bit_names.size() }) {
		bool checked = flags & mask<u64>(bit_idx);
		if (ImGui::Checkbox(bit_names[bit_idx].cbegin(), &checked)) {
			flags ^= mask<u64>(bit_idx);
			changed |=  true;
		}
		if (bit_idx < bit_names.size() - 1)
			ImGui::SameLine();
	}
	return changed;
}

bool EditorWidget(const cstr label, Entity& entity) {
	static const str bits[] = {
		"Dynbody",
		"Collision",
		"Sprite",
		"Player"
	};
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= bit_flags("flags", entity.flags, bits);
		if (has_one(entity.flags, mask<u64>(Entity::Dynbody, Entity::Collision, Entity::Sprite, Entity::Player)))
			changed |= EditorWidget("transform", entity.transform);
		if (has_one(entity.flags, mask<u64>(Entity::Dynbody, Entity::Collision)))
			changed |= EditorWidget("body", entity.body);
		if (has_one(entity.flags, mask<u64>(Entity::Sprite, Entity::Collision)))
			ImGui::Text("TODO : Widget for mesh reference");
		if (has_one(entity.flags, mask<u64>(Entity::Sprite)))
			ImGui::Text("TODO : Widget for texture reference");
		if (has_one(entity.flags, mask<u64>(Entity::Player))) {
			changed |= EditorWidget("speed", entity.speed);
			changed |= EditorWidget("accel", entity.accel);
		}
		ImGui::TreePop();
	}
	return changed;
}

bool playground(App& app) {
	auto entities = List{ alloc_array<Entity>(std_allocator, MAX_ENTITIES), 0 };
	defer{ dealloc_array(std_allocator, entities.capacity); };

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

	auto texture2 = Textures::load_from_file(
		"C:/Users/billy/Documents/assets/boss-spaceship-2d-sprites-pixel-art/PNG_Parts&Spriter_Animation/Boss_ship2/Boss_ship7.png",
		Textures::Linear, Textures::Clamp
	);
	defer{ GL_GUARD(glDeleteTextures(1, &texture2.id)); };

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

	auto create_player = [&]() {
		Entity ent = { mask<u64>(Entity::Player, Entity::Sprite, Entity::Dynbody) };
		ent.texture = &texture;
		ent.mesh = &rect;
		ent.name = "Player";
		b2BodyDef def;
		def.position = b2Vec2(0, 0);
		def.type = b2_dynamicBody;
		ent.body = physics.world.CreateBody(&def);
		return ent;
	};

	auto& player = entities.push(create_player());

	auto& greoihgior = entities.push({mask<u64>(Entity::Sprite)});
	greoihgior.texture = &texture2;
	greoihgior.mesh = &rect;
	greoihgior.transform.translation += v2f32(100, 0);

	fflush(stdout);
	while (update(app, playground)) {
		update(scene.clock);
		Input::poll(app.inputs);

		for (auto& ent : entities.allocated()) if (has_all(ent.flags, mask<u64>(Entity::Player, Entity::Dynbody))) {
			using namespace controls;
			using namespace Input::Keys;
			move_top_down(*ent.body, keyboard_plane(W, A, S, D), ent.speed, ent.accel * scene.clock.dt);
		}

		// Using a 'while' here instead of an 'if' makes sure that if a frame was slow the physics simulation can catch up
		// by doing multiple steps in 1 frame
		// otherwise slowness in the rendering would also slow down the physics
		// althout not sure how pertinent this is since slow rendering would probably be a bigger problem
		while (Time::metronome(scene.clock.current, physics.config.time_step, physics.time_point)) { // Physics tick
			tuple<Transform2D*, b2Body*> pbuff[MAX_ENTITIES];
			auto to_sim = List{ larray(pbuff), 0 };
			for (auto& ent : entities.allocated()) if (has_all(ent.flags, mask<u64>(Entity::Dynbody))) {
				//TODO replace pointers in entity with resource handles to automate error handling
				if (ent.body == null)
					fprintf(stderr, "Entity %s marked as dynamic body has no body\n", ent.name.data());
				else
					to_sim.push({ &ent.transform, ent.body });
			}
			update_physics(physics.world, physics.config, to_sim.allocated());
		}

		{// Build Imgui overlay
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGui::Begin("Controls");
			physics_controls(physics.world, physics.config, physics.time_point);
			EditorWidget("Camera", rendering.camera);
			EditorWidget("Entities", entities.allocated());
			ImGui::Text("Time = %f", scene.clock.current);
			ImGui::End();
			ImGui::Render();
		}

		// Rendering
		//TODO instanciate camera entity somewhere
		rendering.camera.dimensions.x = app.pixel_dimensions.x;
		rendering.camera.dimensions.y = app.pixel_dimensions.y;
		rendering.view_projection_matrix.obj = view_project(rendering.camera.matrix(), player.transform.matrix());
		sync(rendering.view_projection_matrix);
		render(0, { v2u32(0), app.pixel_dimensions }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
			[&]() {
				//Draw entities
				//TODO batching
				for (auto&& ent : entities.allocated()) if (has_all(ent.flags, mask<u64>(Entity::Sprite))) {
					auto draw_batch_matrices = List{ rendering.draw_batch_matrices_buffer.obj, 0 };
					draw_batch_matrices.push(ent.transform.matrix());
					GPUBinding textures[] = { bind(*ent.texture, 0) };
					GPUBinding ssbos[] = { bind(rendering.draw_batch_matrices_buffer, 0) };
					GPUBinding ubos[] = { bind(rendering.view_projection_matrix, 0) };
					sync(rendering.draw_batch_matrices_buffer);
					draw(draw_pipeline, *ent.mesh, draw_batch_matrices.current, larray(textures), larray(ssbos), larray(ubos));
					// Synchronisation avoids overwriting on mapped buffers before draw is executed
					wait_gpu();
				}

				//Draw overlay
				ImGui::Draw();

				//Draw physics debug
				physics.debug_draw.view_transform.obj = rendering.view_projection_matrix.obj;
				physics.world.DebugDraw();
			}
		);
	}
	return true;
}

#endif
