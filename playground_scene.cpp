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

struct PlaygroundScene {
	Rendering rendering;
	Physics2D physics;
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

		physics(colliders, spacials, clock.dt);
		audio(audio_sources, spacials, spacials[player]);
		rendering(sprites, spacials, fbf, (player.valid() ? spacials[player]->transform : identity_2d));
		return true;
	}

	static auto default_editor() {
		return tuple(
			Rendering::default_editor(),
			Audio::default_editor(),
			Physics2D::default_editor(),
			SystemEditor("Entities", "Alt+E", { Input::KB::K_LEFT_ALT, Input::KB::K_E })
		);
	}

	void editor(tuple<SystemEditor, SystemEditor, Physics2D::Editor, SystemEditor>& ed) {
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
