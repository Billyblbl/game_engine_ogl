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

#include <system_editor.cpp>

/*
SceneNode {
	Create(...)
	Destroy()
	Update(...)
	MakeEditor()? -> ed
	EditorUpdate(ed?)
}
*/

#define MAX_SPRITES MAX_ENTITIES

const struct {
	cstrp test_character_spritesheet_path = "test_character.png";
	cstrp test_character_anim_path = "test_character.anim";
	cstrp draw_pipeline = "./shaders/sprite.glsl";
	cstrp test_sound = "./audio/file_example_OOG_1MG.ogg";
} assets;

struct Physics {
	struct Collision {
		v2f32 penetration;
		rtf32 aabbi;
		EntityHandle e1;
		EntityHandle e2;
	};
	List<Collision> collisions;

	void operator()(
		ComponentRegistry<Collider2D>& colliders,
		ComponentRegistry<Spacial2D>& spacials,
		const Time::Clock& clock) {

		//TODO chose another way for this buffer
		static Collision _buff[MAX_ENTITIES * MAX_ENTITIES];

		for (auto [_, sp] : spacials.iter())
			euler_integrate(*sp, clock.dt);

		collisions = List {larray(_buff), 0};
		for (auto [i, j] : self_combinatronic_idx(colliders.handles.current)) {
			auto ent1 = colliders.handles.allocated()[i];
			auto ent2 = colliders.handles.allocated()[j];

			auto col1 = colliders[ent1];
			auto col2 = colliders[ent2];
			auto sp1 = spacials[ent1];
			auto sp2 = spacials[ent2];

			auto [collided, aabbi, pen] = intersect(col1->shape, col2->shape, trs_2d(sp1->transform), trs_2d(sp2->transform));
			if (collided)
				collisions.push({ pen, aabbi, ent1, ent2 });

			if (glm::length2(pen) < penetration_threshold * penetration_threshold || !can_resolve_collision(col1->material.inverse_mass, col2->material.inverse_mass, col1->material.inverse_inertia, col2->material.inverse_inertia))
				continue;

			// Correct penetration
			auto [o1, o2] = correction_offset(pen, col1->material.inverse_mass, col2->material.inverse_mass);
			sp1->transform.translation += o1;
			sp2->transform.translation += o2;

			//TODO friction
			// Compute contact points with corrected positions
			auto normal = glm::normalize(pen);
			auto [ctct1, ctct2] = contacts_points(col1->shape, col2->shape, trs_2d(sp1->transform), trs_2d(sp2->transform), normal);
			v2f32 contacts[2] = { ctct1, ctct2 };

			// Bounce
			PhysicalDescriptor desc1 = { sp1->velocity, sp1->transform.translation, col1->material.inverse_mass, col1->material.inverse_inertia };
			PhysicalDescriptor desc2 = { sp2->velocity, sp2->transform.translation, col2->material.inverse_mass, col2->material.inverse_inertia };
			auto [delta1, delta2] = bounce_contacts(desc1, desc2, larray(contacts), normal, min(col1->material.restitution, col2->material.restitution));
			sp1->velocity = sp1->velocity + delta1;
			sp2->velocity = sp2->velocity + delta2;
		}
	}

	struct Editor : public SystemEditor {
		ShapeRenderer debug_draw = load_shape_renderer();
		bool debug = false;
		bool wireframe = true;

		Editor() : SystemEditor("Physics", "Alt+P", { Input::KB::K_LEFT_ALT, Input::KB::K_P }) {}

		void draw_debug(
			Array<Collision> collisions,
			ComponentRegistry<Collider2D>& colliders,
			ComponentRegistry<Spacial2D>& spacials,
			MappedObject<m4x4f32>& view_projection_matrix
		) {
			for (auto [ent, col] : colliders.iter()) {
				auto spacial = spacials[*ent];
				auto mat = trs_2d(spacial->transform);
				debug_draw(col->shape, mat, view_projection_matrix, v4f32(1, 0, 0, 1), wireframe);
				v2f32 local_vel = glm::transpose(mat) * v4f32(spacial->velocity.translation, 0, 1);

				sync(debug_draw.render_info, { mat, v4f32(1, 1, 0, 1) });
				debug_draw.draw_line(
					Segment<v2f32>{v2f32(0), local_vel},
					view_projection_matrix,
					wireframe
				);
			}

			for (auto [pen, aabbi, ent1, ent2] : collisions) {
				v2f32 verts[4];
				sync(debug_draw.render_info, { m4x4f32(1), v4f32(0, 1, 0, 1) });
				debug_draw.draw_polygon(make_box_poly(larray(verts), dims_p2(aabbi), (aabbi.max + aabbi.min) / 2.f), view_projection_matrix, wireframe);
			}
		}

		void editor_window(Physics& system) {
			EditorWidget("Draw debug", debug);
			if (debug)
				EditorWidget("Wireframe", wireframe);
			if (ImGui::TreeNode("Collisions")) {
				defer{ ImGui::TreePop(); };
				for (auto [penetration, aabbi, i, j] : system.collisions.allocated()) {
					char buffer[999];
					snprintf(buffer, sizeof(buffer), "%s:%s", i.desc->name.data(), j.desc->name.data());
					if (ImGui::TreeNode(buffer)) {
						EditorWidget("aabbi", aabbi);
						EditorWidget("Penetration", penetration);
					}
				}
			}
		}
	};

	static auto default_editor() { return Editor(); }

};

struct Rendering {
	OrthoCamera camera = { v3f32(16.f, 9.f, 1000.f) / 2.f, v3f32(0) };
	MappedObject<m4x4f32> view_projection_matrix = map_object(m4x4f32(1));
	SpriteRenderer draw = load_sprite_renderer(assets.draw_pipeline, MAX_DRAW_BATCH);
	TexBuffer atlas = create_texture(TX2DARR, v4u32(256, 256, MAX_SPRITES, 1));
	AnimationGrid<rtf32, 2> player_character_animation = load_animation_grid<rtf32, 2>(assets.test_character_anim_path, std_allocator);
	RenderMesh rect = create_rect_mesh(v2f32(1));

	Rendering() { atlas.conf_sampling({ Nearest, Nearest }); }

	~Rendering() {
		delete_mesh(rect);
		dealloc_array(std_allocator, player_character_animation.keyframes);
		unload(atlas);
		unload(draw);
		unmap(view_projection_matrix);
	}

	void operator()(
		ComponentRegistry<SpriteCursor>& sprites,
		ComponentRegistry<Spacial2D>& spacials,
		FrameBuffer& fbf, const Transform2D& viewpoint) {

		begin_render(fbf);
		clear(fbf, v4f32(v3f32(0.3), 1));
		*view_projection_matrix.obj = view_project(project(camera), trs_2d(viewpoint));
		auto batch = draw.start_batch();
		for (auto [ent, sprite] : sprites.iter())
			batch.push(sprite_data(trs_2d(spacials[*ent]->transform), *sprite, 0/*draw layer*/));
		draw(rect, atlas, view_projection_matrix, batch.current);
	}

	static auto default_editor() {
		return SystemEditor("Rendering", "Alt+R", { Input::KB::K_LEFT_ALT, Input::KB::K_R });
	}

	void editor_window() {
		EditorWidget("Camera", camera);
		if (ImGui::Button("Reset Camera"))
			camera = { v3f32(16.f, 9.f, 1000.f) / 2.f, v3f32(0) };
		EditorWidget("Atlas", atlas);
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

	void operator()(ComponentRegistry<AudioSource>& audio, ComponentRegistry<Spacial2D>& spacial, Spacial2D* pov) {
		for (auto&& [ent, source] : audio.iter()) {
			source->set<POSITION>(v3f32(spacial[*ent]->transform.translation, 0));
			source->set<VELOCITY>(v3f32(spacial[*ent]->velocity.translation, 0));
		}
		if (pov) {
			ALListener::set<POSITION>(v3f32(pov->transform.translation, 0));
			ALListener::set<VELOCITY>(v3f32(pov->velocity.translation, 0));
		}
	}

	static auto default_editor() { return SystemEditor("Audio", "Alt+A", { Input::KB::K_LEFT_ALT,Input::KB::K_A }); }

	void editor_window() {
		ImGui::Text("Device : %p", data.device);
		if (data.extensions.size() > 0) {
			ImGui::Text("%u", data.extensions.size());
			ImGui::SameLine();
			EditorWidget("Extensions", data.extensions);
		} else {
			ImGui::Text("No extensions");
		}
		ALListener::EditorWidget("Listener");
	};
};

struct PlaygroundScene {
	Rendering rendering;
	Physics physics;
	Audio audio;
	FrameBuffer fbf;

	EntityRegistry entities;
	ComponentRegistry<Spacial2D> spacials;
	ComponentRegistry<AudioSource> audio_sources;
	ComponentRegistry<SpriteCursor> sprites;
	ComponentRegistry<Collider2D> colliders;

	AnimationGrid<rtf32, 2> anim;
	controls::TopDownControl ctrl;
	Time::Clock clock;
	EntityHandle player;

	PlaygroundScene(FrameBuffer _fbf = default_framebuffer) {
		fbf = _fbf;
		entities = create_entity_registry(std_allocator, MAX_ENTITIES);
		spacials = register_new_component<Spacial2D>(entities, std_allocator, MAX_ENTITIES / 2, "Spacial");
		audio_sources = register_new_component<AudioSource>(entities, std_allocator, MAX_ENTITIES / 2, "Audio Source");
		sprites = register_new_component<SpriteCursor>(entities, std_allocator, MAX_ENTITIES / 2, "Sprite");
		colliders = register_new_component<Collider2D>(entities, std_allocator, MAX_ENTITIES / 2, "Collider");

		anim = load_animation_grid<rtf32, 2>(assets.test_character_anim_path, std_allocator);
		ctrl = { 1, 1, 1, v2f32(0), 0 };
		clock = Time::start();
		player = (
			[&]() {
				static v2f32 player_polygon[] = { v2f32(-1, -1) / 2.f, v2f32(+1, -1) / 2.f, v2f32(+1, +1) / 2.f, v2f32(-1, +1) / 2.f };
				auto ent = allocate_entity(entities, "player");
				spacials.add_to(ent, {});
				sprites.add_to(ent, load_into(assets.test_character_spritesheet_path, rendering.atlas, v2u32(0), 0));
				colliders.add_to(ent, { {}, make_shape_2d<Shape2D::Polygon>(larray(player_polygon)) });
				assert(ent.valid());
				return ent;
			}
		());

		fflush(stdout);
		wait_gpu();
	}

	~PlaygroundScene() {
		defer{ delete_registry(std_allocator, entities); };
		defer{ delete_registry(std_allocator, spacials); };
		defer{ delete_registry(std_allocator, audio_sources); };
		defer{ delete_registry(std_allocator, sprites); };
		defer{ delete_registry(std_allocator, colliders); };
		defer{ unload(anim, std_allocator); };
	}

	bool operator()() {
		update(clock);
		clear_stale(colliders, audio_sources, sprites, spacials);

		{// player controller
			ctrl.input = controls::keyboard_plane(Input::KB::K_W, Input::KB::K_A, Input::KB::K_S, Input::KB::K_D);
			auto pl_sprite = sprites[player];
			if (pl_sprite)
				pl_sprite->uv_rect = controls::animate(ctrl, anim, clock.current);
			auto pl_spacial = spacials[player];
			if (pl_spacial)
				controls::move_top_down(pl_spacial->velocity.translation, ctrl.input, ctrl.speed, ctrl.accel, clock.dt);
		}

		physics(colliders, spacials, clock);
		audio(audio_sources, spacials, spacials[player]);
		rendering(sprites, spacials, fbf, (player.valid() ? spacials[player]->transform : identity_2d));
		return true;
	}

	static auto default_editor() {
		return tuple(
			Rendering::default_editor(),
			Audio::default_editor(),
			Physics::default_editor(),
			SystemEditor("Entities", "Alt+E", { Input::KB::K_LEFT_ALT, Input::KB::K_E })
		);
	}

	void editor(tuple<SystemEditor, SystemEditor, Physics::Editor, SystemEditor>& ed) {
		auto& [rd, au, ph, ent] = ed;
		if (ph.debug)
			ph.draw_debug(physics.collisions.allocated(), colliders, spacials, rendering.view_projection_matrix);

		if (rd.show_window) {
			if (begin_editor(rd)) {
				rendering.editor_window();
			} end_editor();
		}

		if (au.show_window) {
			if (begin_editor(au)) {
				audio.editor_window();
			} end_editor();
		}

		if (ph.show_window) {
			if (begin_editor(ph)) {
				ph.editor_window(physics);
			} end_editor();
		}

		if (ent.show_window) {
			if (begin_editor(ent)) {
				entity_registry_editor(entities, spacials, audio_sources, sprites, colliders);
			} end_editor();
		}
	}

};

bool editor_test(App& app) {
	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };
	auto scene_texture = create_texture(TX2D, v4u32(v2f32(1920, 1080), 1, 1)); defer{ unload(scene_texture); };
	auto scene_texture_depth = create_texture(TX2D, v4u32(v2f32(1920, 1080), 1, 1), DEPTH_COMPONENT32); defer{ unload(scene_texture_depth); };
	auto scene_panel = create_framebuffer({ bind_to_fb(Color0Attc, scene_texture, 0, 0), bind_to_fb(DepthAttc, scene_texture_depth, 0, 0) }); defer{ destroy_fb(scene_panel); };
	auto scene_window = create_editor("Scene", "Alt+S", { Input::KB::K_LEFT_ALT, Input::KB::K_S });
	auto editor = create_editor("Editor", "Alt+X", { Input::KB::K_LEFT_ALT, Input::KB::K_X });

	auto sub_editors = List{ alloc_array<SystemEditor*>(std_allocator, 10), 0 };
	sub_editors.push(&editor);
	sub_editors.push(&scene_window);

	PlaygroundScene pg;
	auto pg_ed = pg.default_editor();
	add_editors(sub_editors, pg_ed);

	while (update(app, editor_test)) {
		pg.fbf = editor.show_window ? scene_panel : default_framebuffer;
		pg();
		shortcut_sub_editors(sub_editors.allocated());
		if (!editor.show_window) continue;
		ImGui::NewFrame_OGL_GLFW(); defer{ render_to(default_framebuffer); };
		if (ImGui::BeginMainMenuBar()) {
			defer{ ImGui::EndMainMenuBar(); };
			sub_editor_menu("Windows", sub_editors.allocated());
			if (ImGui::BeginMenu("Actions")) {
				defer{ ImGui::EndMenu(); };
				if (ImGui::MenuItem("Break"))
					fprintf(stderr, "breaking!\n");
				if (ImGui::MenuItem("Restart"))
					return true;
			}
		}
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
		if (scene_window.show_window) {
			if (begin_editor(scene_window)) {
				EditorWidget("Texture", scene_texture);
			} end_editor();
		}
		pg.editor(pg_ed);
	}
	return true;
}

#endif
