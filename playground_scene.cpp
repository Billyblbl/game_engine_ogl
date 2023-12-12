#ifndef GPLAYGROUND_SCENE
# define GPLAYGROUND_SCENE

#include <imgui_extension.cpp>
#include <application.cpp>
#include <rendering.cpp>
#include <time.cpp>
#include <transform.cpp>
#include <physics_2d.cpp>
#include <blblstd.hpp>
#include <math.cpp>
#include <animation.cpp>
#include <top_down_controls.cpp>
#include <sidescroll_controls.cpp>
#include <entity.cpp>
#include <sprite.cpp>
#include <audio.cpp>
#include <system_editor.cpp>

#include <high_order.cpp>

#include <texture_shape_generation.cpp>
#include <spall/profiling.cpp>

#include <tilemap.cpp>

#define MAX_SPRITES MAX_ENTITIES

const struct {
	cstrp test_character_spritesheet_path = "test_character.png";
	cstrp test_character_anim_path = "test_character.anim";
	cstrp draw_pipeline = "./shaders/sprite.glsl";
	cstrp test_sound = "./audio/file_example_OOG_1MG.ogg";
	cstrp test_sidescroll_path = "12_Animated_Character_Template.png";
	cstrp sidescroll_character_animation_recipe_path = "test_sidescroll_character.xml";
	cstrp tilemap_pipeline = "./shaders/tilemap.glsl";
	cstrp level = "./test.tmx";
} assets;

struct Entity : public EntitySlot {
	enum : u64 {
		Sound = UserFlag << 0,
		Draw = UserFlag << 1,
		Collider = UserFlag << 2,
		Physical = UserFlag << 3,
		Controllable = UserFlag << 4,
		Animated = UserFlag << 5,
		// Tilemap = UserFlag << 6,
	};

	static constexpr string Flags[] = {
		BaseFlags[0],
		BaseFlags[1],
		BaseFlags[2],
		"Sound",
		"Draw",
		"Collider",
		"Physical",
		"Controllable",
		"Animated",
		// "Tilemap"
	};

	Spacial2D space;
	AudioSource audio_source;
	Sprite sprite;
	SidescrollControl ctrl;
	Shape2D shape[1];
	Body body;
	Animator anim;
	Array<SpriteAnimation> animations;
};

template<> tuple<bool, RigidBody> use_as<RigidBody>(EntityHandle handle) {
	if (!has_all(handle->flags, Entity::Collider)) return tuple(false, RigidBody{});
	return tuple(true, RigidBody{
		handle,
		larray(handle->content<Entity>().shape),
		&handle->content<Entity>().space,
		(has_all(handle->flags, Entity::Physical) ? &handle->content<Entity>().body : null),
		(has_all(handle->flags, Entity::Controllable && handle->content<Entity>().ctrl.falling) ? handle->content<Entity>().ctrl.fall_multiplier : 1.f)
		}
	);
}

template<> tuple<bool, SidescrollCharacter> use_as<SidescrollCharacter>(EntityHandle handle) {
	if (!has_all(handle->flags, Entity::Controllable)) return tuple(false, SidescrollCharacter{});
	return tuple(true, SidescrollCharacter{
			&handle->content<Entity>().ctrl,
			&handle->content<Entity>().anim,
			&handle->content<Entity>().sprite,
			&handle->content<Entity>().space,
			handle->content<Entity>().animations,
		}
	);
}

template<> tuple<bool, SpriteRenderer::Instance> use_as<SpriteRenderer::Instance>(EntityHandle handle) {
	if (!has_all(handle->flags, Entity::Draw)) return tuple(false, SpriteRenderer::Instance{});
	return tuple(true, SpriteRenderer::make_instance(handle->content<Entity>().sprite, handle->content<Entity>().space.transform));
}

template<> tuple<bool, Sound> use_as<Sound>(EntityHandle handle) {
	if (!has_all(handle->flags, Entity::Sound)) return tuple(false, Sound{});
	return tuple(true, Sound{ &handle->content<Entity>().audio_source, &handle->content<Entity>().space });
}

template<> tuple<bool, Spacial2D*> use_as<Spacial2D*>(EntityHandle handle) { return tuple(true, &handle->content<Entity>().space); }

bool EditorWidget(const cstr label, Entity& ent) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };

		changed |= ImGui::bit_flags("Flags", ent.flags, larray(Entity::Flags).subspan(1), false);
		changed |= EditorWidget("Space", ent.space);
		changed |= EditorWidget("Audio Source", ent.audio_source);
		changed |= EditorWidget("Sprite", ent.sprite);
		changed |= EditorWidget("Shape", ent.shape);
		changed |= EditorWidget("Body", ent.body);
		changed |= EditorWidget("Controller", ent.ctrl);
		changed |= EditorWidget("Animator", ent.anim);
	}
	return changed;
}

struct PlaygroundScene {

	struct {
		Render render;
		SpriteRenderer draw_sprites;
		TilemapRenderer draw_tilemap;
		Atlas2D sprite_atlas;
		rtu32 white;
	} gfx;

	Physics2D physics;
	Audio audio;
	Time::Clock clock;

	List<Entity> entities;

	AudioClip clip;
	Array<SpriteAnimation> animations;
	Tilemap level;
	// Transform2D tilemap_transform = identity_2d;
	rtu32 spritesheet;
	EntityHandle player;
	EntityHandle cam;
	EntityHandle level_entity;

	Arena resources_arena = Arena::from_vmem(1 << 23);

	Entity& create_test_body(string name, v2f32 position) {
		auto& ent = allocate_entity(entities, name, Entity::Draw | Entity::Collider | Entity::Physical);
		ent.space = { identity_2d, null_transform_2d, null_transform_2d };
		ent.space.transform.translation = position;
		ent.sprite.view = gfx.white;
		ent.sprite.dimensions = v2f32(1);
		ent.sprite.depth = 1;
		ent.body.inverse_inertia = 1.f;
		ent.body.inverse_mass = 1.f;
		ent.body.restitution = 1.f;
		ent.body.friction = .1f;
		ent.body.shape_index = 0;
		static v2f32 test_polygon[] = { v2f32(-1, -1) / 2.f, v2f32(+1, -1) / 2.f, v2f32(+1, +1) / 2.f, v2f32(-1, +1) / 2.f };
		ent.shape[ent.body.shape_index] = make_shape_2d(identity_2d, 0, larray(test_polygon));
		return ent;
	}

	static constexpr v4f32 white_pixel[] = { v4f32(1) };
	PlaygroundScene() {
		PROFILE_SCOPE(__FUNCTION__);
		auto [arena, scope] = scratch_push_scope(1ull << 21); defer{ scratch_pop_scope(arena, scope); };
		resources_arena = Arena::from_vmem(1ull << 23);

		{
			PROFILE_SCOPE("Rendering init");
			defer{ scratch_pop_scope(arena, scope); };
			gfx.draw_sprites = SpriteRenderer::create(assets.draw_pipeline);
			gfx.draw_tilemap = TilemapRenderer::load(assets.tilemap_pipeline);
			gfx.sprite_atlas = Atlas2D::create(v2u32(1920, 1080));
			gfx.white = gfx.sprite_atlas.push(make_image(larray(white_pixel), v2u32(1)));

			auto img = load_image(assets.test_sidescroll_path); defer{ unload(img); };
			spritesheet = gfx.sprite_atlas.push(img);

			auto layout = build_layout(arena, assets.sidescroll_character_animation_recipe_path);
			animations = build_sidescroll_character_animations(resources_arena, layout, img.dimensions);

			level = Tilemap::load(resources_arena, gfx.sprite_atlas, assets.level);
		}

		{
			PROFILE_SCOPE("Scene content init");
			srand(time(0));
			auto frand = [](f32range range) -> f32 { return range.min + fmodf(f32(rand()) / f32(rand() % 0xFFFFFF + 1), 1.f) * (range.max - range.min); };

			entities = List{ resources_arena.push_array<Entity>(MAX_ENTITIES), 0 };

			player = (
				[&]() {
					auto& ent = create_test_body("player", v2f32(0));
					ent.flags |= (Entity::Controllable | Entity::Animated);
					ent.sprite.view = spritesheet;
					ent.body.inverse_inertia = 0.f;
					ent.body.inverse_mass = 1.f;
					ent.body.restitution = 0.f;
					ent.animations = animations;
					return get_entity_genhandle(ent);
				}
			());

			level_entity = (
				[&]() {
					auto& ent = allocate_entity(entities, "Level", Entity::Collider | Entity::Physical);
					ent.body.inverse_inertia = 0;
					ent.body.inverse_mass = 0;
					ent.body.restitution = .1f;
					ent.body.friction = .1f;
					ent.body.shape_index = 0;
					auto shapes = tilemap_shapes(resources_arena, *level.tree, tile_shapeset(resources_arena, carray(level.tree->tiles, level.tree->tilecount)));
					ent.shape[0] = shapes[0];
					return get_entity_genhandle(ent);
				}
			());

			cam = get_entity_genhandle(allocate_entity(entities, "Camera", 0));

			// for (auto i : u64xrange{ 0, 5 })
			// 	create_test_body("test_ent PRE", v2f32(frand({ -10, 10 }), frand({ -10, 10 })));

			{
				auto& ent = allocate_entity(entities, "floor", Entity::Draw | Entity::Collider | Entity::Physical);
				ent.space.transform.translation = v2f32(0, -15);
				ent.sprite.view = gfx.white;
				ent.sprite.dimensions = v2f32(2000, 10);
				ent.sprite.depth = 1;
				ent.body.inverse_inertia = 0;
				ent.body.inverse_mass = 0;
				ent.body.restitution = .1f;
				ent.body.friction = .1f;
				ent.body.shape_index = 0;
				static v2f32 floor_poly[] = { v2f32(-1000, -5), v2f32(+1000, -5), v2f32(+1000, +5), v2f32(-1000, +5) };
				ent.shape[ent.body.shape_index] = make_shape_2d(identity_2d, 0, larray(floor_poly));
			}

			// for (auto i : u64xrange{ 0, 5 })
			// 	create_test_body("test_ent POST", v2f32(frand({ -10, 10 }), frand({ -10, 10 })));

		}

		{
			PROFILE_SCOPE("Flushing log buffers");
			fflush(stdout);
		}

		{
			PROFILE_SCOPE("Waiting for GPU init work");
			wait_gpu();
		}
		clock = Time::start();
	}

	u64 update_count = 0;
	bool operator()() {
		PROFILE_SCOPE("Scene update");
		defer{ update_count++; };
		update(clock);
		auto [scratch, scope] = scratch_push_scope(1ull << 21); defer{ scratch_pop_scope(scratch, scope); };

		if (player.valid())
			player_input(player->content<Entity>().ctrl);

		if (physics.manual_update && (Input::KB::get(Input::KB::K_SPACE) & Input::Down)) {
			printf("Manual stepping\n");
			scratch_pop_scope(scratch, scope);
			physics(gather<RigidBody>(scratch, entities.used()), gather<Spacial2D*>(scratch, entities.used()), 1);
		}

		if (Input::KB::get(Input::KB::K_ENTER) & Input::Down)
			create_test_body("new test body", v2f32(0));

		update_characters(gather<SidescrollCharacter>(scratch_pop_scope(scratch, scope), entities.used()), physics.collisions.used(), physics.gravity, clock);
		if (auto it_count = physics.iteration_count(clock.current); !physics.manual_update && it_count > 0) {
			scratch_pop_scope(scratch, scope);
			physics(gather<RigidBody>(scratch, entities.used()), gather<Spacial2D*>(scratch, entities.used()), it_count);
		}

		if (cam.valid()) {
			cam->content<Entity>().space.transform.translation = (player.valid() ? player->content<Entity>().space.transform.translation : v2f32(0));
			cam->content<Entity>().space.velocity.translation = (player.valid() ? player->content<Entity>().space.velocity.translation : v2f32(0));
		}

		auto pov = (cam.valid() ? cam->content<Entity>().space : Spacial2D{});
		audio(gather<Sound>(scratch_pop_scope(scratch, scope), entities.used()), &pov);
		auto sprites = gather<SpriteRenderer::Instance>(scratch_pop_scope(scratch, scope), entities.used());
		gfx.render(pov.transform,
		// gfx.render(identity_2d,
			[&](m4x4f32 mat) {
				gfx.draw_tilemap(level, level_entity->content<Entity>().space.transform, mat, gfx.sprite_atlas.texture);
				gfx.draw_sprites(sprites, mat, gfx.sprite_atlas.texture);
			}
		);
		return true;
	}

	static auto default_editor() {
		return tuple(
			Render::default_editor(),
			Audio::default_editor(),
			Physics2D::default_editor(),
			SystemEditor("Entities", "Alt+E", { Input::KB::K_LEFT_ALT, Input::KB::K_E }),
			SystemEditor("Misc", "Alt+M", { Input::KB::K_LEFT_ALT, Input::KB::K_M })
		);
	}

	void editor(tuple<SystemEditor, SystemEditor, Physics2D::Editor, SystemEditor, SystemEditor>& ed) {
		auto& [rd, au, ph, ent, misc] = ed;
		static auto debug_scratch = Arena::from_vmem(1 << 19);
		if (ph.debug) {
			PROFILE_SCOPE("Physics Debug");
			auto vp = project(gfx.render.camera) * glm::inverse(m4x4f32(cam->content<Entity>().space.transform));
			if (ph.colliders) ph.draw_shapes(gather<RigidBody>(debug_scratch.reset(), entities.used()), vp);
			if (ph.collisions) ph.draw_collisions(physics.collisions.used(), vp);
		}

		if (rd.show_window) {
			if (begin_editor(rd)) {
				gfx.render.editor_window();
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
				EditorWidget("Entities", entities.used(), false);
			} end_editor();
		}

		if (misc.show_window) {
			if (begin_editor(misc)) {
				ImGui::Text("Update index : %llu", update_count);
				EditorWidget("Clock", clock);
				EditorWidget("Animations", animations);
			} end_editor();
		}
	}

};

bool editor_test(App& app) {
	profile_scope_begin("Initialisation");
	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };
	auto editor = create_editor("Editor", "Alt+X", { Input::KB::K_LEFT_ALT, Input::KB::K_X });
	editor.show_window = true;
	auto sub_editors = List{ cast<SystemEditor*>(virtual_reserve(10 * sizeof(SystemEditor*), true)), 0 };
	sub_editors.push(&editor);

	PlaygroundScene scene;
	auto pg_ed = scene.default_editor();
	auto& [rd, au, ph, ent, misc] = pg_ed;
	rd.show_window = true;
	au.show_window = true;
	ph.show_window = true;
	ent.show_window = true;
	misc.show_window = true;

	add_editors(sub_editors, pg_ed);
	profile_scope_end();

	while (update(app, editor_test)) {
		PROFILE_SCOPE("Frame");
		scene();
		PROFILE_SCOPE("Editor");
		shortcut_sub_editors(sub_editors.used());
		if (editor.show_window) {
			ImGui::NewFrame_OGL_GLFW(); defer{
				ImGui::Render();
				ImGui::Draw();
			};
			if (ImGui::BeginMainMenuBar()) {
				defer{ ImGui::EndMainMenuBar(); };
				sub_editor_menu("Windows", sub_editors.used());
				if (ImGui::BeginMenu("Actions")) {
					defer{ ImGui::EndMenu(); };
					if (ImGui::MenuItem("Break"))
						fprintf(stderr, "breaking!\n");
					if (ImGui::MenuItem("Restart"))
						return true;
				}
			}
			ImGui::DockSpaceOverViewport(ImGui::GetWindowViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
			scene.editor(pg_ed);
		}
	}
	return true;
}

#endif
