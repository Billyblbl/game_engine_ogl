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
#include <physics_2d.cpp>
#include <physics_2d_debug.cpp>
#include <blblstd.hpp>
#include <math.cpp>
#include <animation.cpp>

#include <top_down_controls.cpp>
#include <entity.cpp>
#include <sprite.cpp>
#include <audio.cpp>

#define MAX_SPRITES MAX_ENTITIES

ImVec2 fit_to_area(ImVec2 area, v2f32 dimensions) {
	auto ratios = ImGui::to_glm(area) / dimensions;
	return ImGui::from_glm(dimensions * min(ratios.x, ratios.y));
}

const struct {
	cstrp test_character_spritesheet_path = "test_character.png";
	cstrp test_character_anim_path = "test_character.anim";
	cstrp draw_pipeline = "./shaders/sprite.glsl";
	cstrp test_sound = "./audio/file_example_OOG_1MG.ogg";
} assets;

bool playground(App& app) {
	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };

	struct {
		AudioData data = init_audio();
		AudioBuffer buffer = create_audio_buffer();
	} audio;
	defer{
		destroy(audio.buffer);
		deinit_audio(audio.data);
	};
	auto clip = load_clip_file(assets.test_sound); defer{ unload_clip(clip); };
	write_audio_clip(audio.buffer, clip);

	struct {
		OrthoCamera camera = { v3f32(16.f, 9.f, 1000.f) / 2.f, v3f32(0) };
		MappedObject<m4x4f32> view_projection_matrix = map_object(m4x4f32(1));
		SpriteRenderer draw = load_sprite_renderer(assets.draw_pipeline, MAX_DRAW_BATCH);
		TexBuffer atlas = create_texture(TX2DARR, v4u32(256, 256, MAX_SPRITES, 1));
		AnimationGrid<rtf32, 2> player_character_animation = load_animation_grid<rtf32, 2>(assets.test_character_anim_path, std_allocator);
		RenderMesh rect = create_rect_mesh(v2f32(1));
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
		f32 time_point = 0.f;
		f32 time_step = 1.f / 60.f;
		bool draw_debug = false;
		bool wireframe = true;
		ShapeRenderer debug_draw = load_collider_renderer();
		v2f32 gravity = v2f32(0);
	} physics;

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
	//TODO test point collider

	auto clock = Time::start();
	auto entities = List{ alloc_array<Entity>(std_allocator, MAX_ENTITIES), 0 }; defer{ dealloc_array(std_allocator, entities.capacity); };

	auto sprite = load_into(assets.test_character_spritesheet_path, rendering.atlas, v2u32(0), 0);
	v2f32 player_polygon[] = { v2f32(-1, -1) / 2.f, v2f32(1, -1) / 2.f, v2f32(1,  1) / 2.f, v2f32(-1,  1) / 2.f };
	v2f32 ground_polygon[] = { v2f32(-10, -1), v2f32(+10, -1), v2f32(+10, +1), v2f32(-10, +1), };

	auto& player = entities.push(
		[&]() {
			Entity ent;
			ent.name = "player";
			ent.transform.translation = v2f32(1.5f);
			add_sprite(ent, sprite, &rendering.rect);
			add_dynbody(ent, {});
			add_collider(ent, { make_shape_2d<Shape2D::Polygon>(larray(player_polygon)), 0, 1, false });
			add_sound(ent, create_audio_source().set<BUFFER>(audio.buffer.id));
			add_controls(ent, 10, 100, 2);
			return ent;
		}());
	defer{ destroy(player.audio_source); };

	// entities.push(
	// 	[&]() {
	// 		Entity ent;
	// 		ent.name = "circle";
	// 		ent.transform.translation = v2f32(-1.5f);
	// 		add_collider(ent, { make_shape_2d<Shape2D::Circle>(v3f32(0, 0, 1)), 0, 1, false });
	// 		return ent;
	// 	}());

	entities.push(
		[&]() {
			Entity ent;
			ent.name = "line";
			ent.transform.translation = v2f32(-1.5f, 1.5f);
			add_collider(ent, { make_shape_2d<Shape2D::Line>(Segment<v2f32> { v2f32(-1), v2f32(1) }), 1, 1, false });
			return ent;
		}());

	entities.push(
		[&]() {
			Entity ent;
			ent.name = "box";
			ent.transform.translation = v2f32(0, -3.f);
			add_collider(ent, { make_shape_2d<Shape2D::Polygon>(larray(ground_polygon)), 0.1, 1, false });
			return ent;
		}());

	update_bodies(entities.allocated());
	printf("Finished loading scene with %lu entities\n", entities.current);
	fflush(stdout);
	wait_gpu();

	while (update(app, playground)) {
		update(clock);

		{
			using namespace Input::Keyboard;
			if (Input::get_key(K_LEFT_CONTROL) & Input::Down)
				editor.active = !editor.active;

			// Audio test controls
			// if (Input::get_key(K_SPACE) & Input::Up) {
			// 	if (player.audio_source.get<SOURCE_STATE>() == PLAYING)
			// 		player.audio_source.pause();
			// 	else
			// 		player.audio_source.play();
			// }
			// if (Input::get_key(K_R) & Input::Up) {
			// 	player.audio_source.rewind();
			// }

			// player.controls.input = controls::keyboard_plane(K_W, K_A, K_S, K_D);
			// player.sprite.uv_rect = controls::animate(player.controls, rendering.player_character_animation, clock.current);
		}

		update_audio_sources(entities.allocated());
		update_audio_listener(player);

		Array<Collision> collisions;
		while (Time::metronome(clock.current, physics.time_step, physics.time_point)) { // Advance physics sim until it caught up to real time
			// controls::move_top_down(player.body, player.controls.input, player.controls.speed, player.controls.accel * physics.time_step);
			apply_global_force(entities.allocated(), physics.time_step, physics.gravity);
			for (auto& ent : entities.allocated()) if (has_all(ent.flags, mask<u64>(Entity::Dynbody)))
				ent.body.transform = euler_integrate(ent.body.transform, filter_locks(ent.body.velocity_transform, ent.body.locks), physics.time_step);
			collisions = simulate_collisions(entities.allocated());
		}
		interpolate_bodies(entities.allocated(), clock.current, physics.time_point, clock.dt);

		// Scene
		render(editor.active ? editor.scene_panel : 0, { v2u32(0), editor.scene_texture.dimensions }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, v4f32(v3f32(0.3), 1),
			[&]() {
				*rendering.view_projection_matrix.obj = view_project(project(rendering.camera), trs_2d(player.transform));
				draw_entities(entities.allocated(), rendering.rect, rendering.view_projection_matrix, rendering.atlas, rendering.draw);
				if (physics.draw_debug) {
					for (auto& ent : entities.allocated()) if (has_all(ent.flags, mask<u64>(Entity::Solid))) {
						physics.debug_draw(
							ent.collider.shape,
							trs_2d(ent.transform),
							rendering.view_projection_matrix,
							v4f32(1, 0, 0, 1),
							physics.wireframe
						);

						v2f32 local_vel = glm::transpose(trs_2d(ent.body.transform)) * v4f32(ent.body.velocity_transform.translation, 0, 1);
						physics.debug_draw(
							make_shape_2d<Shape2D::Line>(Segment<v2f32>{v2f32(0), local_vel}),
							trs_2d(ent.transform),
							rendering.view_projection_matrix,
							v4f32(1, 0, 1, 1),
							physics.wireframe
						);

					}
					for (auto [_1, _2, aabbi, pen, ctct1, ctct2] : collisions) {
						auto box = make_box_shape(std_allocator, dims_p2(aabbi), (aabbi.max + aabbi.min) / 2.f); defer{ dealloc_array(std_allocator, box.polygon); };
						physics.debug_draw(box, m4x4f32(1), rendering.view_projection_matrix, v4f32(0, 1, 0, 1), physics.wireframe);
						physics.debug_draw(make_shape_2d<Shape2D::Line>(Segment<v2f32>{ctct1, ctct2}), m4x4f32(1), rendering.view_projection_matrix, v4f32(0, 1, 1, 1), physics.wireframe);
					}
				}
			}
		);

		// UI
		bool should_exit = false;
		if (editor.active) {
			editor.draw_data = ImGui::RenderNewFrame(
				[&]() {
					ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

					{
						ImGui::Begin("Scene"); defer{ ImGui::End(); };
						ImGui::Image((ImTextureID)(u64)editor.scene_texture.id, fit_to_area(ImGui::GetWindowContentSize(), editor.scene_texture.dimensions), ImVec2(0, 1), ImVec2(1, 0));
					}

					{
						ImGui::Begin("Misc"); defer{ ImGui::End(); };
						EditorWidget("Camera", rendering.camera);
						ImGui::Text("Real Time = %f", clock.current);
						ImGui::Text("Physics Time = %f", physics.time_point);
						ImGui::Text("Physics advance = %f", physics.time_point - clock.current);
						if (ImGui::Button("Break"))
							printf("Breaking\n");
						if (ImGui::Button("Restart"))
							should_exit = true;
					}

					{
						ImGui::Begin("Physics"); defer{ ImGui::End(); };
						EditorWidget("Draw debug", physics.draw_debug);
						EditorWidget("Wireframe", physics.wireframe);
						EditorWidget("Sim time", physics.time_point);
						EditorWidget("Time step", physics.time_step);
						EditorWidget("Gravity", physics.gravity);
						if (ImGui::TreeNode("Collisions")) {
							for (auto [i, j, aabbi, penetration, ctct1, ctct2] : collisions) {
								ImGui::PushID(i * entities.current + j);
								ImGui::Text("%u:%u", i, j);
								EditorWidget("aabbi", aabbi);
								EditorWidget("Penetration", penetration);
								EditorWidget("Contact 1", ctct1);
								EditorWidget("Contact 2", ctct2);
								ImGui::PopID();
							}
							ImGui::TreePop();
						}
					}

					{
						ImGui::Begin("Audio"); defer{ ImGui::End(); };
						ImGui::Text("Device : %p", audio.data.device);
						ImGui::Text("%u", audio.data.extensions.size()); ImGui::SameLine(); EditorWidget("Extensions", audio.data.extensions);
						ALListener::EditorWidget("Listener");
					}

					EditorWindow("Entities", entities);
				}
			);

			if (should_exit)
				return true;
			render(0, { v2u32(0), app.pixel_dimensions }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, v4f32(v3f32(0), 1), [&]() { ImGui::Draw(editor.draw_data); });
		}
	}
	return true;
}

#endif
