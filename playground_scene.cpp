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
#include <entity.cpp>
#include <sprite.cpp>

#define MAX_SPRITES MAX_ENTITIES

const struct {
	cstrp ship1 = "C:/Users/billy/Documents/assets/boss-spaceship-2d-sprites-pixel-art/PNG_Parts&Spriter_Animation/Boss_ship1/Boss_ship7.png";
	cstrp ship2 = "C:/Users/billy/Documents/assets/boss-spaceship-2d-sprites-pixel-art/PNG_Parts&Spriter_Animation/Boss_ship2/Boss_ship7.png";
	cstrp draw_pipeline = "./shaders/sprite.glsl";
} assets;

bool playground(App& app) {
	auto entities = List{ alloc_array<Entity>(std_allocator, MAX_ENTITIES), 0 }; defer{ dealloc_array(std_allocator, entities.capacity); };

	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };

	struct {
		OrthoCamera camera = { v3f32(16.f, 9.f, -2.f) / 2.f, v3f32(0) };
		MappedObject<m4x4f32> view_projection_matrix = map_object(m4x4f32(1));
		SpriteRenderer draw = load_sprite_renderer(assets.draw_pipeline, MAX_DRAW_BATCH);
		TexBuffer atlas = create_texture(TX2DARR, v4u32(256 * 2, 256 * 2, MAX_SPRITES, 1));
	} rendering;
	defer{
		unload(rendering.atlas);
		unload(rendering.draw);
		unmap(rendering.view_projection_matrix);
	};

	struct {
		PhysicsConfig config;
		b2World world = b2World(b2Vec2(0.f, -9.f));
		B2dDebugDraw debug_draw;
		f32 time_point = 0.f;
	} physics;
	physics.world.SetDebugDraw(&physics.debug_draw);
	physics.debug_draw.SetFlags(b2Draw::e_shapeBit);
	physics.debug_draw.view_transform = &rendering.view_projection_matrix;

	auto clock = Time::start();

	auto ship1_sprite = load_into(assets.ship1, rendering.atlas, v2u32(0), 0);
	auto ship2_sprite = load_into(assets.ship2, rendering.atlas, v2u32(0), 1);

	auto rect = create_rect_mesh(v2f32(1)); defer{ delete_mesh(rect); };

	auto& player = entities.push(create_player(ship1_sprite, rect, physics.world));
	auto& some_sprite = entities.push({});
	add_sprite(some_sprite, ship2_sprite, rect);

	printf("Finished loading scene with %lu entities\n", entities.allocated().size());
	fflush(stdout);
	sync(rendering.view_projection_matrix);
	sync(rendering.draw.instances_buffer);
	wait_gpu();
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

			Entity* pbuff[entities.current];
			auto to_sim = List{ carray(pbuff, entities.current), 0 };
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
		render(0, { v2u32(0), app.pixel_dimensions }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
			[&]() {

				//Draw entities
				rendering.view_projection_matrix.obj = view_project(project(rendering.camera), m4x4f32(1));
				auto batch = rendering.draw.start_batch();
				for (auto&& ent : entities.allocated()) if (has_all(ent.flags, mask<u64>(Entity::Sprite)))
					batch.push({ trs_2d(ent.transform), ent.sprite.uv_rect, v2u64(ent.sprite.atlas_index, 0) });
				rendering.draw(rect, rendering.atlas, rendering.view_projection_matrix, batch.current);

				//Draw overlay
				ImGui::Draw();

				//Draw physics debug
				physics.world.DebugDraw();
			}
		);
	}
	return true;
}

#endif
