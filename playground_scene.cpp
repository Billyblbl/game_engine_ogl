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
	cstrp test_sidescroll_path = "12_Animated_Character_Template.png";
	cstrp sidescroll_character_animation_recipe_path = "test_sidescroll_character.xml";
} assets;

struct Entity : public EntitySlot {
	enum : u64 {
		Sound = UserFlag << 0,
		Draw = UserFlag << 1,
		Collider = UserFlag << 2,
		Physical = UserFlag << 3,
		Controllable = UserFlag << 4,
		Animated = UserFlag << 5,
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
		"Animated"
	};

	Spacial2D space;
	AudioSource audio_source;
	Sprite sprite;
	SidescrollControl ctrl;
	Shape2D shape;
	Body body;
	Animator anim;
	Array<SpriteAnimation> animations;
};

template<> tuple<bool, RigidBody> use_as<RigidBody>(EntityHandle handle) {
	if (!has_all(handle->flags, Entity::Collider)) return tuple(false, RigidBody{});
	return tuple(true, RigidBody{
		handle,
		&handle->content<Entity>().shape,
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

template<> tuple<bool, SpriteInstance> use_as<SpriteInstance>(EntityHandle handle) {
	if (!has_all(handle->flags, Entity::Draw)) return tuple(false, SpriteInstance{});
	return tuple(true, instance_of(handle->content<Entity>().sprite, trs_2d(handle->content<Entity>().space.transform)));
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
	Rendering rendering;
	Physics2D physics;
	Audio audio;
	Time::Clock clock;

	List<Entity> entities;

	AudioClip clip;
	SpriteCursor spritesheet;
	Array<SpriteAnimation> animations;

	EntityHandle player;

	Arena resources_arena = Arena::from_vmem(1 << 23);

	PlaygroundScene() {
		rendering.camera = { v3f32(16.f, 9.f, 1000.f) * 4.f, v3f32(0) };
		entities = List{ resources_arena.push_array<Entity>(MAX_ENTITIES), 0 };

		auto img = load_image(assets.test_sidescroll_path); defer{ unload(img); };
		spritesheet = load_into(img, rendering.atlas);

		auto layout = build_layout(resources_arena, assets.sidescroll_character_animation_recipe_path);
		animations = build_sidescroll_character_animations(resources_arena, layout, img.dimensions);

		static v2f32 test_polygon[] = { v2f32(-1, -1) / 2.f, v2f32(+1, -1) / 2.f, v2f32(+1, +1) / 2.f, v2f32(-1, +1) / 2.f };
		static v2f32 floor_poly[] = { v2f32(-1000, -1), v2f32(+1000, -1), v2f32(+1000, +1), v2f32(-1000, +1) };

		player = (
			[&]() {
				auto& ent = allocate_entity(entities, "player", Entity::Draw | Entity::Physical | Entity::Collider | Entity::Controllable | Entity::Animated);
				ent.space = { identity_2d, null_transform_2d, null_transform_2d };
				ent.sprite.cursor = spritesheet;
				ent.sprite.dimensions = v2f32(1);
				ent.sprite.depth = 1;
				ent.body.inverse_inertia = 0.f;
				ent.body.inverse_mass = 1.f;
				ent.body.restitution = .0f;
				ent.body.friction = .3f;
				ent.animations = animations;
				ent.shape = make_shape_2d<Shape2D::Polygon>(larray(test_polygon));
				return get_entity_genhandle(ent);
			}
		());

		srand(time(0));
		auto frand = [](f32range range) -> f32 { return range.min + fmodf(f32(rand()) / f32(rand() % 0xFFFFFF + 1), 1.f) * (range.max - range.min); };

		{// misc scene content

			for (auto i : u64xrange{ 0, 0 }) {
				auto& ent = allocate_entity(entities, "test_ent", Entity::Draw | Entity::Collider | Entity::Physical);
				ent.space = { identity_2d, null_transform_2d, null_transform_2d };
				ent.space.transform.translation += v2f32(frand({ -10, 10 }), frand({ -10, 10 }));
				ent.space.velocity.translation += v2f32(frand({ -1, 1 }), frand({ -1, 1 }));
				ent.sprite.cursor = spritesheet;
				ent.sprite.dimensions = v2f32(1);
				ent.sprite.depth = 1;
				ent.body.inverse_inertia = 1.f;
				ent.body.inverse_mass = 1.f;
				ent.body.restitution = .3f;
				ent.body.friction = .5f;
				ent.shape = make_shape_2d<Shape2D::Polygon>(larray(test_polygon));
			}

			{
				auto& ent = allocate_entity(entities, "static1", Entity::Collider | Entity::Physical);
				ent.space.transform.translation = v2f32(0, -15);
				ent.body.inverse_inertia = 0;
				ent.body.inverse_mass = 0;
				ent.body.restitution = .8f;
				ent.body.friction = .5f;
				ent.shape = make_shape_2d<Shape2D::Polygon>(larray(floor_poly));
			}

			// {
			// 	auto& ent = allocate_entity(entities, "ramp1", Entity::Rigidbody);
			// 	Transform2D transform;
			// 	transform.translation = v2f32(+7, -10);
			// 	transform.rotation = 0;
			// 	transform.scale = v2f32(3);
			// 	ent.space.transform = transform;
			// 	ent.body.inverse_inertia = 0;
			// 	ent.body.inverse_mass = 0;
			// 	ent.body.restitution = .5f;
			// 	ent.body.friction = .5f;
			// 	ent.shape = make_shape_2d<Shape2D::Line>(Segment<v2f32> { v2f32(-1), v2f32(1) });
			// }

			// {
			// 	auto& ent = allocate_entity(entities, "ramp2", Entity::Rigidbody);
			// 	Transform2D transform;
			// 	transform.translation = v2f32(-7, -10);
			// 	transform.rotation = -90;
			// 	transform.scale = v2f32(3);
			// 	ent.space.transform = transform;
			// 	ent.body.inverse_inertia = 0;
			// 	ent.body.inverse_mass = 0;
			// 	ent.body.restitution = .5f;
			// 	ent.body.friction = .5f;
			// 	ent.shape = make_shape_2d<Shape2D::Line>(Segment<v2f32> { v2f32(-1), v2f32(1) });
			// }
		}

		fflush(stdout);
		wait_gpu();
		clock = Time::start();
	}

	u64 update_count = 0;
	Spacial2D pov;
	bool operator()() {
		defer{ update_count++; };
		update(clock);
		constexpr u64 SCRATCH_SIZE = (1ull << 21);
		auto [scratch, scope] = scratch_push_scope(SCRATCH_SIZE); defer{ scratch_pop_scope(scratch, scope); };

		if (player.valid())
			player_input(player->content<Entity>().ctrl);

		pov = (player.valid() ? player->content<Entity>().space : Spacial2D{ identity_2d, null_transform_2d, null_transform_2d });
		pov.transform.rotation = 0;
		pov.transform.scale.x = abs(pov.transform.scale.x);

		scratch_pop_scope(scratch, scope);
		if (auto it_count = physics.iteration_count(clock.current); it_count > 0)
			physics(gather<RigidBody>(scratch, entities.used()), gather<Spacial2D*>(scratch, entities.used()), it_count );
		update_characters(gather<SidescrollCharacter>(scratch_pop_scope(scratch, scope), entities.used()), physics.collisions.used(), physics.gravity, clock);
		audio(gather<Sound>(scratch_pop_scope(scratch, scope), entities.used()), &pov);
		rendering(gather<SpriteInstance>(scratch_pop_scope(scratch, scope), entities.used()), pov.transform);
		return true;
	}

	static auto default_editor() {
		return tuple(
			Rendering::default_editor(),
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
			auto vp = rendering.get_vp_matrix(pov.transform);
			if (ph.colliders) ph.draw_shapes(gather((debug_scratch.reset()), entities.used(), Entity::Collider, [&](Entity& ent) { return tuple(&ent.shape, &ent.space); }), vp);
			if (ph.collisions) ph.draw_collisions(physics.collisions.used(), vp, &Entity::space);
		}

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

	while (update(app, editor_test)) {
		scene();
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
