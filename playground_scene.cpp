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
	v2f32 test_character_anim_cells[4][4] = {
		{v2f32(0, 3), v2f32(1, 3), v2f32(2, 3), v2f32(3, 3)},
		{v2f32(0, 1), v2f32(1, 1), v2f32(2, 1), v2f32(3, 1)},
		{v2f32(0, 0), v2f32(1, 0), v2f32(2, 0), v2f32(3, 0)},
		{v2f32(0, 2), v2f32(1, 2), v2f32(2, 2), v2f32(3, 2)},
	};
} assets;

struct Entity : public EntitySlot {
	enum : u64 {
		Sound = UserFlag << 0,
		Draw = UserFlag << 1,
		Collider = UserFlag << 2,
		Physical = UserFlag << 3,
		Controllable = UserFlag << 4,
		Animated = UserFlag << 5,
		Rigidbody = Collider | Physical
	};
	static constexpr string Flags[] = {
		"None",
		"Allocated",
		"Pending Release",
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
	SidescrollControl ssctrl;
	Shape2D shape;
	Body body;
	controls::TopDownControl ctrl;
};

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
		changed |= EditorWidget("SSController", ent.ssctrl);
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
	AnimationGrid<rtu32> anim;
	AnimationGrid<Shape2D> shape_anim;
	SpriteCursor spritesheet;

	EntityHandle player;

	PlaygroundScene() {
		rendering.camera = { v3f32(16.f, 9.f, 1000.f) * 4.f, v3f32(0) };
		entities = List{ alloc_array<Entity>(std_allocator, MAX_ENTITIES), 0 };

		clip = load_clip_file("audio/file_example_OOG_1MG.ogg");
		auto& buffer = audio.buffers.push(create_audio_buffer());
		write_audio_clip(buffer, clip);
		auto img = load_image(assets.test_character_spritesheet_path); defer{ unload(img); };
		anim = load_animation_grid<rtu32>(assets.test_character_anim_path, std_allocator);
		clock = Time::start();
		spritesheet = load_into(img, rendering.atlas);
		shape_anim = create_animated_shape(std_allocator, img, anim, alpha_filter()); //TODO free resource / make an arena for this resource ?

		static v2f32 test_polygon[] = { v2f32(-1, -1) / 2.f, v2f32(+1, -1) / 2.f, v2f32(+1, +1) / 2.f, v2f32(-1, +1) / 2.f };
		static v2f32 floor_poly[] = { v2f32(-1000, -1), v2f32(+1000, -1), v2f32(+1000, +1), v2f32(-1000, +1) };

		player = (
			[&]() {
				auto& ent = allocate_entity(entities, "player", Entity::Draw | Entity::Physical | Entity::Collider | Entity::Controllable | Entity::Sound);
				ent.space = { identity_2d, null_transform_2d, null_transform_2d };
				ent.sprite.cursor = spritesheet;
				ent.sprite.dimensions = v2f32(1);
				ent.sprite.depth = 1;
				ent.body.inverse_inertia = 0.f;
				ent.body.inverse_mass = 1.f;
				ent.body.restitution = .0f;
				ent.body.friction = .3f;
				ent.ctrl = { 10, 100, 1, v2f32(0), 0 };
				ent.audio_source = create_audio_source();//TODO delete_audio_source -> change into a resource ? wont be shared tho, could instead deal with it with the reintroduction of the 'pending deletion' flag
				ent.audio_source.set<BUFFER>(buffer.id);
				ent.shape = make_shape_2d<Shape2D::Polygon>(larray(test_polygon));
				return get_entity_genhandle(ent);
			}
		());

		srand(time(0));
		auto frand = [](f32range range)->f32 { return range.min + fmodf(f32(rand()) / f32(rand() % 0xFFFFFF + 1), 1.f) * (range.max - range.min); };

		{// misc scene content

			for (auto i : u64xrange{ 0, 0 }) {
				auto& ent = allocate_entity(entities, "test_ent", Entity::Draw | Entity::Rigidbody);
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
				auto& ent = allocate_entity(entities, "static1", Entity::Rigidbody);
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
	}

	~PlaygroundScene() {
		unload(anim, std_allocator);
		dealloc_array(std_allocator, entities.capacity);
	}

	u64 update_count = 0;
	Spacial2D pov;
	bool operator()() {
		defer{ update_count++; };
		update(clock);
		constexpr u64 SCRATCH_SIZE = (1ull << 21);
		static auto scratch = create_virtual_arena(SCRATCH_SIZE);
		auto flush_scratch = [&]() -> Alloc { return as_v_alloc(reset_virtual_arena(scratch)); };

		if (player.valid()) // player controller
			player_input(player->content<Entity>().ssctrl);

		pov = (player.valid() ? player->content<Entity>().space : Spacial2D{ identity_2d, null_transform_2d, null_transform_2d });
		pov.transform.rotation = 0;

		for (auto [ctrl, sprite, shape] : gather(flush_scratch(), entities.allocated(), Entity::Animated, [](Entity& ent) { return tuple(&ent.ctrl, has_all(ent.flags, Entity::Draw) ? &ent.sprite.cursor : null, has_all(ent.flags, Entity::Collider) ? &ent.shape : null); })) {
			//TODO dispatch to the right animation function
			//? the animation system probably can't just be given every possible piece of data it might need like that, how do we gather ?
			//* possible "subsystem" animation scheme, where an animation component informs which subsytem needs to be run
			animate_character(*ctrl, sprite, shape, spritesheet, &anim, &shape_anim, clock.current);
		}

		if (flush_scratch(); auto it_count = physics.iteration_count(clock.current) > 0) { // TODO test this
			using namespace glm;
			physics.flush_state();
			auto gravity_dir = normalize(physics.gravity);
			auto ctrls = gather(as_v_alloc(scratch), entities.allocated(), Entity::Controllable, [](Entity& ent) { return tuple(&ent.ssctrl, &ent.space.velocity.translation);});
			auto physical = gather(as_v_alloc(scratch), entities.allocated(), Entity::Physical, [](Entity& ent) { return tuple(&ent.body, &ent.space, has_all(ent.flags, Entity::Controllable) && ent.ssctrl.falling ? ent.ssctrl.fall_multiplier : 1.f); });
			auto spacial = map(as_v_alloc(scratch), entities.allocated(), [](Entity& ent) { return &ent.space; });
			auto rigidbodies = gather(as_v_alloc(scratch), entities.allocated(), Entity::Collider, [](Entity& ent) { return RigidBody{ get_entity_genhandle(ent), &ent.shape, &ent.space, (has_all(ent.flags, Entity::Rigidbody) ? &ent.body : null) }; });
			for (auto i : u64xrange{ 0, it_count }) {
				for (auto [ctrl, vel] : ctrls) *vel = control(*ctrl, *vel, physics.gravity, clock.current);
				physics.apply_gravity(physical);
				physics.step_sim(spacial);
				physics.resolve_collisions(rigidbodies);
			}
			for (auto [ctrl, _] : ctrls) ctrl->grounded = false;
			for (auto& col : physics.collisions.allocated()) {
				for (auto i : u64xrange{ 0, 2 }) if (has_all(col.entities[i]->flags, Entity::Controllable)) {
					auto& ent = col.entities[i]->content<Entity>();
					auto mat = trs_2d(ent.space.transform);
					auto grounding_contact = (
						[&](const Contact2D& ctc) {
							auto lever = normalize(ctc.levers[i]);
							auto normal = normalize(ctc.penetration);
							return dot(lever, gravity_dir) > 0 && angle(normal, gravity_dir) < ent.ssctrl.max_slope;
						}
					);
					if (ent.ssctrl.grounded = (linear_search(col.contacts, grounding_contact) >= 0))
						ent.ssctrl.falling = false;
				}
			}
		}

		audio(gather(flush_scratch(), entities.allocated(), Entity::Sound, [](Entity& ent) { return tuple(&ent.audio_source, (const Spacial2D*)&ent.space); }), &pov);
		rendering(gather(flush_scratch(), entities.allocated(), Entity::Draw, [&](const Entity& ent) { return instance_of(ent.sprite, trs_2d(ent.space.transform), rendering.atlas.dimensions); }), pov.transform);
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
		static auto debug_scratch = create_virtual_arena(1 << 19);
		if (ph.debug) {
			auto vp = rendering.get_vp_matrix(pov.transform);
			if (ph.colliders) ph.draw_shapes(gather(as_v_alloc(reset_virtual_arena(debug_scratch)), entities.allocated(), Entity::Collider, [&](Entity& ent) { return tuple(&ent.shape, &ent.space); }), vp);
			if (ph.collisions) ph.draw_collisions(physics.collisions.allocated(), vp, &Entity::space);
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
				EditorWidget("Entities", entities.allocated(), false);
			} end_editor();
		}

		if (misc.show_window) {
			if (begin_editor(misc)) {
				ImGui::Text("Update index : %llu", update_count);
				EditorWidget("Clock", clock);
				EditorWidget("Animation Grid", anim);
			} end_editor();
		}
	}

};

bool editor_test(App& app) {
	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };
	auto editor = create_editor("Editor", "Alt+X", { Input::KB::K_LEFT_ALT, Input::KB::K_X });
	editor.show_window = true;
	auto sub_editors = List{ alloc_array<SystemEditor*>(std_allocator, 10), 0 }; defer{ dealloc_array(std_allocator, sub_editors.capacity); };
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
		shortcut_sub_editors(sub_editors.allocated());
		if (editor.show_window) {
			ImGui::NewFrame_OGL_GLFW(); defer{
				ImGui::Render();
				ImGui::Draw();
			};
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
			ImGui::DockSpaceOverViewport(ImGui::GetWindowViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
			scene.editor(pg_ed);
		}
	}
	return true;
}

#endif
