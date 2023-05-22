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

struct Physics {
	f32 time_point = 0.f;
	f32 time_step = 1.f / 60.f;
	bool draw_debug = false;
	bool wireframe = true;
	ShapeRenderer debug_draw = load_collider_renderer();
	v2f32 gravity = v2f32(0);
	Array<Collision> collisions;

	void operator()(Array<Entity> entities, Time::Clock& clock) {
		while (Time::metronome(clock.current, time_step, time_point)) { // Advance physics sim until it caught up to real time
			// controls::move_top_down(player.body, player.controls.input, player.controls.speed, player.controls.accel * time_step);
			apply_global_force(entities, time_step, gravity);
			for (auto& ent : entities) if (has_all(ent.flags, mask<u64>(Entity::Dynbody)))
				ent.body.transform = euler_integrate(ent.body.transform, filter_locks(ent.body.velocity, ent.body.locks), time_step);
			collisions = simulate_collisions(entities);
		}
		interpolate_bodies(entities, clock.current, time_point, clock.dt);
	}

	void editor_window() {
		ImGui::Begin("Physics"); defer{ ImGui::End(); };
		EditorWidget("Draw debug", draw_debug);
		EditorWidget("Wireframe", wireframe);
		EditorWidget("Sim time", time_point);
		EditorWidget("Time step", time_step);
		EditorWidget("Gravity", gravity);
		if (ImGui::TreeNode("Collisions")) {
			for (auto [i, j, aabbi, penetration, ctct1, ctct2] : collisions) {
				ImGui::PushID(i * MAX_ENTITIES + j);
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
};

struct Rendering {
	OrthoCamera camera = { v3f32(16.f, 9.f, 1000.f) / 2.f, v3f32(0) };
	MappedObject<m4x4f32> view_projection_matrix = map_object(m4x4f32(1));
	SpriteRenderer draw = load_sprite_renderer(assets.draw_pipeline, MAX_DRAW_BATCH);
	TexBuffer atlas = create_texture(TX2DARR, v4u32(256, 256, MAX_SPRITES, 1));
	AnimationGrid<rtf32, 2> player_character_animation = load_animation_grid<rtf32, 2>(assets.test_character_anim_path, std_allocator);
	RenderMesh rect = create_rect_mesh(v2f32(1));

	Rendering() {
		atlas.conf_sampling({ Nearest, Nearest });
	}

	~Rendering() {
		delete_mesh(rect);
		dealloc_array(std_allocator, player_character_animation.keyframes);
		unload(atlas);
		unload(draw);
		unmap(view_projection_matrix);
	}

	void operator()(Array<Entity> entities, GLuint framebuffer, v2f32 dimensions, Transform2D& viewpoint, const Physics& physics) {
		render(framebuffer, { v2u32(0), dimensions }, clear_bit(Color0Attc) | clear_bit(DepthAttc), v4f32(v3f32(0.3), 1),
			[&]() {
				*view_projection_matrix.obj = view_project(project(camera), trs_2d(viewpoint));
				draw_entities(entities, rect, view_projection_matrix, atlas, draw);
				if (physics.draw_debug) {
					for (auto& ent : entities) if (has_all(ent.flags, mask<u64>(Entity::Solid))) {
						physics.debug_draw(
							ent.collider.shape,
							trs_2d(ent.transform),
							view_projection_matrix,
							v4f32(1, 0, 0, 1),
							physics.wireframe
						);

						v2f32 local_vel = glm::transpose(trs_2d(ent.body.transform)) * v4f32(ent.body.velocity.translation, 0, 1);
						physics.debug_draw(
							make_shape_2d<Shape2D::Line>(Segment<v2f32>{v2f32(0), local_vel}),
							trs_2d(ent.transform),
							view_projection_matrix,
							v4f32(1, 0, 1, 1),
							physics.wireframe
						);

					}
					for (auto [_1, _2, aabbi, pen, ctct1, ctct2] : physics.collisions) {
						v2f32 verts[4];
						auto box = make_box_shape(larray(verts), dims_p2(aabbi), (aabbi.max + aabbi.min) / 2.f);
						physics.debug_draw(box, m4x4f32(1), view_projection_matrix, v4f32(0, 1, 0, 1), physics.wireframe);
						physics.debug_draw(make_shape_2d<Shape2D::Line>(Segment<v2f32>{ctct1, ctct2}), m4x4f32(1), view_projection_matrix, v4f32(0, 1, 1, 1), physics.wireframe);
					}
				}
			}
		);

	}
};

struct Audio {
	static constexpr auto MAX_AUDIO_BUFFER_COUNT = 10;
	AudioData data = init_audio();
	List<AudioBuffer> buffers = { alloc_array<AudioBuffer>(std_allocator, MAX_AUDIO_BUFFER_COUNT), 0 };

	~Audio() {
		for (auto&& buffer : buffers.allocated())
			destroy(buffer);
		deinit_audio(data);
	}

	void operator()(Array<Entity> entities, const Entity& pov) {
		update_audio_sources(entities);
		update_audio_listener(pov);
	}
};

bool playground(App& app) {
	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };
	Rendering rendering;
	Physics physics;
	Audio audio;

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

	auto clock = Time::start();
	auto entities = List{ alloc_array<Entity>(std_allocator, MAX_ENTITIES), 0 }; defer{ dealloc_array(std_allocator, entities.capacity); };

	v2f32 player_polygon[] = { v2f32(-1, -1) / 2.f, v2f32(+1, -1) / 2.f, v2f32(+1, +1) / 2.f, v2f32(-1, +1) / 2.f };
	v2f32 ground_polygon[] = { v2f32(-10, -1), v2f32(+10, -1), v2f32(+10, +1), v2f32(-10, +1), };
	auto clip = load_clip_file(assets.test_sound); defer{ unload_clip(clip); };

	auto& player = entities.push(
		[&]() {
			Entity ent;
			ent.name = "player";
			ent.transform.translation = v2f32(1.5f);
			add_sprite(ent, load_into(assets.test_character_spritesheet_path, rendering.atlas, v2u32(0), 0), &rendering.rect);
			add_dynbody(ent, {});
			add_collider(ent, { make_shape_2d<Shape2D::Polygon>(larray(player_polygon)), 0, 1, false });
			auto abuff = audio.buffers.push(create_audio_buffer());
			write_audio_clip(abuff, clip);
			add_sound(ent, create_audio_source().set<BUFFER>(abuff.id));
			add_controls(ent, 10, 100, 2);
			return ent;
		}());
	defer{ destroy(player.audio_source); };

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

		if (Input::get_key(Input::Keyboard::K_LEFT_CONTROL) & Input::Down)
			editor.active = !editor.active;

		physics(entities.allocated(), clock);
		audio(entities.allocated(), player);
		rendering(entities.allocated(), editor.active ? editor.scene_panel : 0, editor.scene_texture.dimensions, player.transform, physics);

		// UI
		if (editor.active) {
			bool should_exit = false;
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

					physics.editor_window();

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
			render(0, { v2u32(0), app.pixel_dimensions }, clear_bit(Color0Attc) | clear_bit(DepthAttc), v4f32(v3f32(0), 1), [&]() { ImGui::Draw(editor.draw_data); });
		}
	}
	return true;
}

#endif
