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
#include <math.cpp>
#include <animation.cpp>

#include <top_down_controls.cpp>
#include <entity.cpp>
#include <sprite.cpp>


#define MAX_SPRITES MAX_ENTITIES

#define ASSETS_FD "C:/Users/billy/Documents/assets/boss-spaceship-2d-sprites-pixel-art/PNG_Parts&Spriter_Animation"

const struct {
	string ships[3 * 7] = {
		ASSETS_FD"/Boss_ship1/Boss_ship1.png",
		ASSETS_FD"/Boss_ship1/Boss_ship2.png",
		ASSETS_FD"/Boss_ship1/Boss_ship3.png",
		ASSETS_FD"/Boss_ship1/Boss_ship4.png",
		ASSETS_FD"/Boss_ship1/Boss_ship5.png",
		ASSETS_FD"/Boss_ship1/Boss_ship6.png",
		ASSETS_FD"/Boss_ship1/Boss_ship7.png",
		ASSETS_FD"/Boss_ship2/Boss_ship1.png",
		ASSETS_FD"/Boss_ship2/Boss_ship2.png",
		ASSETS_FD"/Boss_ship2/Boss_ship3.png",
		ASSETS_FD"/Boss_ship2/Boss_ship4.png",
		ASSETS_FD"/Boss_ship2/Boss_ship5.png",
		ASSETS_FD"/Boss_ship2/Boss_ship6.png",
		ASSETS_FD"/Boss_ship2/Boss_ship7.png",
		ASSETS_FD"/Boss_ship3/Boss_ship1.png",
		ASSETS_FD"/Boss_ship3/Boss_ship2.png",
		ASSETS_FD"/Boss_ship3/Boss_ship3.png",
		ASSETS_FD"/Boss_ship3/Boss_ship4.png",
		ASSETS_FD"/Boss_ship3/Boss_ship5.png",
		ASSETS_FD"/Boss_ship3/Boss_ship6.png",
		ASSETS_FD"/Boss_ship3/Boss_ship7.png",
	};
	cstrp test_character_spritesheet_path = "test_character.png";
	cstrp test_character_anim_path = "test_character.anim";
	cstrp draw_pipeline = "./shaders/sprite.glsl";
} assets;

bool playground(App& app) {
	auto entities = List{ alloc_array<Entity>(std_allocator, MAX_ENTITIES), 0 }; defer{ dealloc_array(std_allocator, entities.capacity); };

	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };

	struct {
		OrthoCamera camera = { v3f32(16.f, 9.f, 1000.f) / 2.f, v3f32(0) };
		MappedObject<m4x4f32> view_projection_matrix = map_object(m4x4f32(1));
		SpriteRenderer draw = load_sprite_renderer(assets.draw_pipeline, MAX_DRAW_BATCH);
		TexBuffer atlas = create_texture(TX2DARR, v4u32(256, 256, MAX_SPRITES, 1));
		AnimationGrid<rtf32, 2> player_character_animation = load_animation_grid<rtf32, 2>(assets.test_character_anim_path, std_allocator);
	} rendering;
	rendering.atlas.conf_sampling(SamplingConfig{ Nearest, Nearest });
	defer{
		dealloc_array(std_allocator, rendering.player_character_animation.keyframes);
		unload(rendering.atlas);
		unload(rendering.draw);
		unmap(rendering.view_projection_matrix);
	};

	struct {
		PhysicsConfig config;
		b2World world = b2World(b2Vec2(0.f, -9.f));
		B2dDebugDraw debug_draw;
		f32 time_point = 0.f;
		bool draw_debug = false;
	} physics;
	physics.world.SetDebugDraw(&physics.debug_draw);
	physics.debug_draw.SetFlags(b2Draw::e_shapeBit);
	physics.debug_draw.view_transform = &rendering.view_projection_matrix;

	auto clock = Time::start();
	auto rect = create_rect_mesh(v2f32(1)); defer{ delete_mesh(rect); };
	auto& player = entities.push(create_player(load_into(assets.test_character_spritesheet_path, rendering.atlas, v2u32(0), 50), rect, physics.world));

	for (u64 i : u64xrange{ 0, array_size(assets.ships) }) {
		auto& ship = add_sprite(entities.push({}), load_into(assets.ships[i].data(), rendering.atlas, v2u32(0), i), rect);
		ship.name = "ship";
		ship.transform.translation += v2f32(cos(glm::degrees(f32(i * 10))), sin(glm::degrees(f32(i * 10)))) * f32(i) / 5.f;
		ship.transform.rotation += i * 10;
	}

	printf("Finished loading scene with %lu entities\n", entities.current);
	fflush(stdout);
	wait_gpu();
	while (update(app, playground)) {
		update(clock);
		Input::poll(app.inputs);

		{ // Player update
			using namespace Input::Keyboard;
			player.controls.input = controls::keyboard_plane(W, A, S, D);
			player.sprite.uv_rect = controls::animate(player.controls, rendering.player_character_animation, clock.current);
		}

		// Using a 'while' here instead of an 'if' makes sure that if a frame was slow the physics simulation can catch up
		// by doing multiple steps in 1 frame
		// otherwise slowness in the rendering would also slow down the physics
		// although not sure how pertinent this is since slow rendering would probably be a bigger problem
		while (Time::metronome(clock.current, physics.config.time_step, physics.time_point)) { // Physics tick

			if (has_all(player.flags, mask<u64>(Entity::Player, Entity::Dynbody)))
				controls::move_top_down(player.body, player.controls.input, player.controls.speed, player.controls.accel * physics.config.time_step);

			Entity* pbuff[entities.current];
			auto to_sim = gather(mask<u64>(Entity::Dynbody), entities.allocated(), carray(pbuff, entities.current));
			for (auto ent : to_sim)
				override_body(ent->body, ent->transform.translation, ent->transform.rotation);
			update_sim(physics.world, physics.config);
			for (auto ent : to_sim)
				override_transform(ent->body, ent->transform.translation, ent->transform.rotation);
		}

		auto imgui_draw_data = ImGui::RenderNewFrame(
			[&]() {
				ImGui::Begin("Controls");
				physics_controls(physics.world, physics.config, physics.time_point, physics.draw_debug);
				EditorWidget("Camera", rendering.camera);
				EditorWidget("Entities", entities.allocated());
				ImGui::Text("Time = %f", clock.current);
				ImGui::End();
			}
		);

		// Rendering
		render(0, { v2u32(0), app.pixel_dimensions }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
			[&]() {

				//Draw entities
				rendering.view_projection_matrix.obj = view_project(project(rendering.camera), trs_2d(player.transform));
				auto batch = rendering.draw.start_batch();
				for (auto&& ent : entities.allocated()) if (has_all(ent.flags, mask<u64>(Entity::Sprite)))
					batch.push(sprite_data(trs_2d(ent.transform), ent.sprite.uv_rect, ent.sprite.atlas_index, ent.draw_layer));
				rendering.draw(rect, rendering.atlas, rendering.view_projection_matrix, batch.current);

				//Draw overlay
				ImGui::Draw(imgui_draw_data);

				//Draw physics debug
				if (physics.draw_debug)
					physics.world.DebugDraw();
			}
		);
	}
	return true;
}

#endif
