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

ImVec2 fit_to_area(ImVec2 area, v2f32 dimensions) {
	auto ratios = ImGui::to_glm(area) / dimensions;
	return ImGui::from_glm(dimensions * min(ratios.x, ratios.y));
}

const struct {
	cstrp test_character_spritesheet_path = "test_character.png";
	cstrp test_character_anim_path = "test_character.anim";
	cstrp draw_pipeline = "./shaders/sprite.glsl";
} assets;

bool playground(App& app) {
	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };

	struct {
		OrthoCamera camera = { v3f32(16.f, 9.f, 1000.f) / 2.f, v3f32(0) };
		MappedObject<m4x4f32> view_projection_matrix = map_object(m4x4f32(1));
		SpriteRenderer draw = load_sprite_renderer(assets.draw_pipeline, MAX_DRAW_BATCH);
		TexBuffer atlas = create_texture(TX2DARR, v4u32(256, 256, MAX_SPRITES, 1));
		AnimationGrid<rtf32, 2> player_character_animation = load_animation_grid<rtf32, 2>(assets.test_character_anim_path, std_allocator);
		RenderMesh rect = get_unit_rect_mesh();
	} rendering;
	rendering.atlas.conf_sampling({ Nearest, Nearest });
	defer{
		delete_mesh(rendering.rect); //TODO replace this manual delete of a global data with another way
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
		b2PolygonShape shape;
	} physics;
	physics.shape.SetAsBox(.25f, .75f/2.f, b2Vec2(0, -.1f), 0);
	physics.world.SetDebugDraw(&physics.debug_draw);
	physics.debug_draw.SetFlags(b2Draw::e_shapeBit);
	physics.debug_draw.view_transform = &rendering.view_projection_matrix;

	struct {
		TexBuffer scene_texture;
		TexBuffer scene_texture_depth;
		GLuint scene_panel;
		ImDrawData* draw_data = null;
		bool active = true;
	} editor;
	editor.scene_texture = create_texture(TX2D, v4u32(app.pixel_dimensions, 1, 1)); defer{ unload(editor.scene_texture); };
	editor.scene_texture_depth = create_texture(TX2D, v4u32(app.pixel_dimensions, 1, 1), DEPTH_COMPONENT32); defer{ unload(editor.scene_texture_depth); };
	editor.scene_panel = create_framebuffer({
		bind_to_fb(Color0Attc, editor.scene_texture, 0, 0),
		bind_to_fb(DepthAttc, editor.scene_texture_depth, 0, 0)
	}); defer{ destroy_fb(editor.scene_panel); };

	auto entities = List{ alloc_array<Entity>(std_allocator, MAX_ENTITIES), 0 }; defer{ dealloc_array(std_allocator, entities.capacity); };
	auto& player = entities.push(create_player(load_into(assets.test_character_spritesheet_path, rendering.atlas, v2u32(0), 0), rendering.rect, physics.world, physics.shape));

	printf("Finished loading scene with %lu entities\n", entities.current);
	fflush(stdout);
	wait_gpu();

	auto clock = Time::start();
	while (update(app, playground)) {
		update(clock);

		{ // Player update
			using namespace Input::Keyboard;
			if (Input::get_key(LEFT_CONTROL) & Input::Down)
				editor.active = !editor.active;
			player.controls.input = controls::keyboard_plane(W, A, S, D);
			player.sprite.uv_rect = controls::animate(player.controls, rendering.player_character_animation, clock.current);
		}

		// Using a 'while' here instead of an 'if' makes sure that if a frame was slow the physics simulation can catch up
		// by doing multiple steps in 1 frame
		// otherwise slowness in the rendering would also slow down the physics
		// although not sure how pertinent this is since slow rendering would probably be a bigger problem
		while (Time::metronome(clock.current, physics.config.time_step, physics.time_point)) { // Physics tick
			controls::move_top_down(player.body, player.controls.input, player.controls.speed, player.controls.accel * physics.config.time_step);
			simulate_entities(entities.allocated(), physics.world, physics.config);
		}

		// Scene
		render(editor.active ? editor.scene_panel : 0, { v2u32(0), editor.scene_texture.dimensions }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, v4f32(v3f32(0.3), 1),
			[&]() {
				rendering.view_projection_matrix.obj = view_project(project(rendering.camera), trs_2d(player.transform));
				draw_entities(entities.allocated(), rendering.rect, rendering.view_projection_matrix, rendering.atlas, rendering.draw);
				if (physics.draw_debug) physics.world.DebugDraw();
			}
		);

		// UI
		if (editor.active) {
			editor.draw_data = ImGui::RenderNewFrame(
				[&]() {
					ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

					ImGui::Begin("Scene");
					ImGui::Image((ImTextureID)(u64)editor.scene_texture.id, fit_to_area(ImGui::GetWindowContentSize(), editor.scene_texture.dimensions), ImVec2(0, 1), ImVec2(1, 0));
					ImGui::End();

					ImGui::Begin("Configs");
					physics_controls(physics.world, physics.config, physics.time_point, physics.draw_debug, physics.debug_draw.wireframe);
					EditorWidget("Camera", rendering.camera);
					ImGui::Text("Time = %f", clock.current);
					ImGui::End();

					ImGui::Begin("Entities");
					ImGui::Text("Capacity : %u/%u", entities.current, entities.capacity.size());
					EditorWidget("Allocated", entities.allocated());
					if (entities.current < entities.capacity.size() && ImGui::Button("Allocate"))
						entities.push({});
					ImGui::End();
				}
			);

			render(0, { v2u32(0), app.pixel_dimensions }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, v4f32(v3f32(0), 1), [&]() { ImGui::Draw(editor.draw_data); });
		}
	}
	return true;
}

#endif
