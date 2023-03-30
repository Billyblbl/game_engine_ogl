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
#include <blblstd.hpp>

#include <top_down_controls.cpp>

#define MAX_ENTITIES 10
#define MAX_DRAW_BATCH MAX_ENTITIES

struct Entity {
	enum FlagIndex: u64 { Dynbody, Collision, Sprite, Player };
	u64 flags = 0;
	Transform2D transform;
	b2Body* body;
	//TODO add shape for physics
	RenderMesh* mesh;
	Texture* texture;
	f32 speed = 10.f;
	f32 accel = 100.f;
	string name = "__entity__";
};

bool EditorWidget(const cstr label, Entity& entity) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= ImGui::bit_flags("flags", entity.flags, { "Dynbody", "Collision", "Sprite", "Player" });
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

const struct {
	cstrp ship1 = "C:/Users/billy/Documents/assets/boss-spaceship-2d-sprites-pixel-art/PNG_Parts&Spriter_Animation/Boss_ship1/Boss_ship7.png";
	cstrp ship2 = "C:/Users/billy/Documents/assets/boss-spaceship-2d-sprites-pixel-art/PNG_Parts&Spriter_Animation/Boss_ship2/Boss_ship7.png";
	cstrp draw_pipeline = "./shaders/textured_instanced.glsl";
} assets;


bool playground(App& app) {
	auto entities = List{ alloc_array<Entity>(std_allocator, MAX_ENTITIES), 0 }; defer{ dealloc_array(std_allocator, entities.capacity); };

	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };

	struct {
		OrthoCamera camera = { v3f32(16, 9, -2) / 2.f, v3f32(0) };
		MappedObject<m4x4f32> view_projection_matrix = map_object(m4x4f32(1));
		MappedBuffer<m4x4f32> draw_batch_matrices_buffer = map_buffer<m4x4f32>(MAX_DRAW_BATCH);
	} rendering;

	defer{
		unmap(rendering.view_projection_matrix);
		unmap(rendering.draw_batch_matrices_buffer);
	};

	struct {
		PhysicsConfig config;
		b2World world = b2World(b2Vec2(0.f, -9.f));
		B2dDebugDraw debug_draw;
		f32 time_point = 0.f;
	} physics;
	physics.world.SetDebugDraw(&physics.debug_draw);
	physics.debug_draw.SetFlags(b2Draw::e_shapeBit);

	auto clock = Time::start();
	auto draw_pipeline = load_pipeline(assets.draw_pipeline); defer{ destroy_pipeline(draw_pipeline); };
	auto texture = load_texture(assets.ship1, Linear, Clamp); defer{ unload(texture); };
	auto texture2 = load_texture(assets.ship2, Linear, Clamp); defer{ unload(texture2); };
	auto rect = create_rect_mesh(texture.dimensions, min(texture.dimensions.x, texture.dimensions.y)); defer{ delete_mesh(rect); };

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

	auto& some_sprite = entities.push({ mask<u64>(Entity::Sprite) });
	some_sprite.texture = &texture2;
	some_sprite.mesh = &rect;

	fflush(stdout);
	while (update(app, playground)) {
		update(clock);
		Input::poll(app.inputs);

		// Using a 'while' here instead of an 'if' makes sure that if a frame was slow the physics simulation can catch up
		// by doing multiple steps in 1 frame
		// otherwise slowness in the rendering would also slow down the physics
		// although not sure how pertinent this is since slow rendering would probably be a bigger problem
		while (Time::metronome(clock.current, physics.config.time_step, physics.time_point)) { // Physics tick

			using namespace Input::Keyboard;
			if (has_all(player.flags, mask<u64>(Entity::Player, Entity::Dynbody)))
				controls::move_top_down(player.body, controls::keyboard_plane(W, A, S, D), player.speed, player.accel * physics.config.time_step);

			Entity* pbuff[entities.allocated().size()];
			auto to_sim = List{ carray(pbuff, entities.allocated().size()), 0 };
			for (auto& ent : entities.allocated()) if (has_all(ent.flags, mask<u64>(Entity::Dynbody)) && ent.body != null)
				to_sim.push(&ent);
			for (auto ent : to_sim.allocated())
				override_body(ent->body, ent->transform.translation, ent->transform.rotation);
			update_sim(physics.world, physics.config);
			for (auto ent : to_sim.allocated())
				override_transform(ent->body, ent->transform.translation, ent->transform.rotation);
		}

		{// Build Imgui overlay
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGui::Begin("Controls");
			physics_controls(physics.world, physics.config, physics.time_point);
			EditorWidget("Camera", rendering.camera);
			EditorWidget("Entities", entities.allocated());
			ImGui::Text("Time = %f", clock.current);
			ImGui::End();
			ImGui::Render();
		}

		// Rendering
		//TODO instantiate camera entity somewhere
		rendering.view_projection_matrix.obj = view_project(project(rendering.camera), trs_2d(player.transform));
		sync(rendering.view_projection_matrix);
		render(0, { v2u32(0), app.pixel_dimensions }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
			[&]() {
				//Draw entities
				//TODO batching
				for (auto&& ent : entities.allocated()) if (has_all(ent.flags, mask<u64>(Entity::Sprite))) {
					auto draw_batch_matrices = List{ rendering.draw_batch_matrices_buffer.content, 0 };
					draw_batch_matrices.push(trs_2d(ent.transform));
					sync(rendering.draw_batch_matrices_buffer);
					draw(draw_pipeline, *ent.mesh, draw_batch_matrices.current,
						{
							bind_to(*ent.texture, 0), 												//texture
							bind_to(rendering.draw_batch_matrices_buffer, 0), //ssbo -> entities
							bind_to(rendering.view_projection_matrix, 0) 			//ubo -> scene
						}
					);
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
