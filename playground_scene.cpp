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
#include <entity.cpp>
#include <sprite.cpp>
#include <audio.cpp>
#include <system_editor.cpp>

#include <high_order.cpp>

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

struct Entity : public EntitySlot {
	enum : u64 {
		Sound = UserFlag << 0,
		Sprite = UserFlag << 1,
		Collider = UserFlag << 2,
		Physical = UserFlag << 3,
		Controllable = UserFlag << 4,
		Rigidbody = Collider | Physical
	};
	static constexpr string Flags[] = {
		"None",
		"Allocated",
		"Pending Release",
		"Sound",
		"Sprite",
		"Collider",
		"Physical",
		"Controllable"
	};
	Spacial2D space;
	AudioSource audio_source;
	SpriteCursor sprite;
	struct { v2f32 dimensions; f32 depth; } render_rect;
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
		if (ImGui::TreeNode("Render Rect")) {
			changed |= EditorWidget("Dimensions", ent.render_rect.dimensions);
			changed |= EditorWidget("Depth", ent.render_rect.depth);
			ImGui::TreePop();
		}
		changed |= EditorWidget("Shape", ent.shape);
		changed |= EditorWidget("Body", ent.body);
		changed |= EditorWidget("Controller", ent.ctrl);
	}
	return changed;
}

struct PlaygroundScene {
	Rendering rendering;
	Physics2D physics;
	Audio audio;

	List<Entity> entities;

	AnimationGrid<rtf32, 2> anim;
	Time::Clock clock;
	EntityHandle player;

	PlaygroundScene() {
		rendering.camera = { v3f32(16.f, 9.f, 1000.f) * 4.f, v3f32(0) };
		entities = List{ alloc_array<Entity>(std_allocator, MAX_ENTITIES), 0 };

		anim = load_animation_grid<rtf32, 2>(assets.test_character_anim_path, std_allocator);
		clock = Time::start();
		player = (
			[&]() {
				static v2f32 player_polygon[] = { v2f32(-1, -1) / 2.f, v2f32(+1, -1) / 2.f, v2f32(+1, +1) / 2.f, v2f32(-1, +1) / 2.f };
				auto& ent = allocate_entity(entities, "player", Entity::Sprite | Entity::Rigidbody | Entity::Controllable);
				ent.space = { identity_2d, null_transform_2d, null_transform_2d };
				ent.sprite = load_into(assets.test_character_spritesheet_path, rendering.atlas, v2u32(0), 0);
				ent.render_rect = { v2f32(1), 1 };
				ent.body.inverse_inertia = .1f;
				ent.body.inverse_mass = 1.f;
				ent.body.restitution = .3f;
				ent.body.friction = .8f;
				ent.shape = make_shape_2d<Shape2D::Polygon>(larray(player_polygon));
				ent.ctrl = { 1, 1, 1, v2f32(0), 0 };
				return get_entity_genhandle(ent);
			}
		());



		{// misc scene content
			static v2f32 test_polygon[] = { v2f32(-1, -1), v2f32(+1, -1), v2f32(+1, +1), v2f32(-1, +1) };
			static v2f32 test_polygon2[] = { v2f32(-1000, -1), v2f32(+1000, -1), v2f32(+1000, +1), v2f32(-1000, +1) };

			static v2f32 test_polygon_concave[] = { v2f32(-1, -1), v2f32(+1, -1), v2f32(+1, +1), v2f32(-1, +1), v2f32(-.5, 0) };
			auto [test_poly_decomposed, test_poly_decomposed_vertices] = decompose_concave_poly(larray(test_polygon_concave));
			static List<Shape2D> test_poly_decomposed_shapes = { alloc_array<Shape2D>(std_allocator, test_poly_decomposed.size()), 0 };

			test_poly_decomposed_shapes.current = 0;
			for (auto sub_poly : test_poly_decomposed)
				test_poly_decomposed_shapes.push(make_shape_2d<Shape2D::Polygon>(sub_poly));

			{
				auto& ent = allocate_entity(entities, "concave1", Entity::Rigidbody);
				ent.space.transform.translation = v2f32(0, 3);
				ent.body.inverse_inertia = .1f;
				ent.body.inverse_mass = 1.f;
				ent.body.restitution = .3f;
				ent.body.friction = .8f;
				ent.shape = make_shape_2d<Shape2D::Concave>(test_poly_decomposed_shapes.allocated());
			}

			{
				auto& ent = allocate_entity(entities, "body1", Entity::Rigidbody);
				ent.space.transform.translation = test_polygon[0] * 2.f + v2f32(0, 10);
				ent.body.inverse_inertia = 1;
				ent.body.inverse_mass = 1.f;
				ent.body.restitution = .5f;
				ent.body.friction = .5f;
				ent.shape = make_shape_2d<Shape2D::Polygon>(larray(test_polygon));
			}

			{
				auto& ent = allocate_entity(entities, "body2", Entity::Rigidbody);
				ent.space.transform.translation = test_polygon[1] * 2.f + v2f32(0, 10);
				ent.body.inverse_inertia = 1;
				ent.body.inverse_mass = 1.f;
				ent.body.restitution = .5f;
				ent.body.friction = .5f;
				ent.shape = make_shape_2d<Shape2D::Circle>(v3f32(0, 0, 1));
			}

			{
				auto& ent = allocate_entity(entities, "body3", Entity::Rigidbody);
				ent.space.transform.translation = test_polygon[2] * 2.f + v2f32(0, 10);
				ent.body.inverse_inertia = 1;
				ent.body.inverse_mass = 1.f;
				ent.body.restitution = .5f;
				ent.body.friction = .5f;
				ent.shape = make_shape_2d<Shape2D::Polygon>(larray(test_polygon));
			}

			{
				auto& ent = allocate_entity(entities, "body4", Entity::Rigidbody);
				ent.space.transform.translation = test_polygon[3] * 2.f + v2f32(0, 10);
				ent.body.inverse_inertia = 1;
				ent.body.inverse_mass = 1.f;
				ent.body.restitution = .5f;
				ent.body.friction = .5f;
				ent.shape = make_shape_2d<Shape2D::Polygon>(larray(test_polygon));
			}

			{
				auto& ent = allocate_entity(entities, "static1", Entity::Rigidbody);
				ent.space.transform.translation = v2f32(0, -2.5) - v2f32(0, -5);
				ent.body.inverse_inertia = 0;
				ent.body.inverse_mass = 0;
				ent.body.restitution = .5f;
				ent.body.friction = .5f;
				ent.shape = make_shape_2d<Shape2D::Polygon>(larray(test_polygon2));
			}

			{
				auto& ent = allocate_entity(entities, "static2", Entity::Rigidbody);
				Transform2D transform;
				transform.translation = v2f32(10, 3) - v2f32(0, -5);
				transform.rotation = 0;
				transform.scale = v2f32(3);
				ent.space.transform = transform;
				ent.body.inverse_inertia = 0;
				ent.body.inverse_mass = 0;
				ent.body.restitution = .5f;
				ent.body.friction = .5f;
				ent.shape = make_shape_2d<Shape2D::Line>(Segment<v2f32> { v2f32(-1), v2f32(1) });
			}

			{
				auto& ent = allocate_entity(entities, "static3", Entity::Rigidbody);
				Transform2D transform;
				transform.translation = v2f32(-10, 3) - v2f32(0, -5);
				transform.rotation = -90;
				transform.scale = v2f32(3);
				ent.space.transform = transform;
				ent.body.inverse_inertia = 0;
				ent.body.inverse_mass = 0;
				ent.body.restitution = .5f;
				ent.body.friction = .5f;
				ent.shape = make_shape_2d<Shape2D::Line>(Segment<v2f32> { v2f32(-1), v2f32(1) });
			}
		}
		fflush(stdout);
		wait_gpu();
	}

	~PlaygroundScene() {
		unload(anim, std_allocator);
		dealloc_array(std_allocator, entities.capacity);
	}

	v2f32 gravity = v2f32(0);
	u64 update_count = 0;
	Spacial2D pov;
	bool operator()() {
		defer{ update_count++; };
		update(clock);

		constexpr auto SCRATCH_SIZE = (1ull << 20);
		static auto scratch = create_virtual_arena(SCRATCH_SIZE);
		auto flush_scratch = [&]() -> Alloc { return as_v_alloc(reset_virtual_arena(scratch)); };

		for (auto& i : entities.allocated()) if (has_all(i.flags, Entity::Physical) && i.body.inverse_mass > 0)
			i.space.velocity.translation += gravity * clock.dt;

		if (player.valid()) {// player controller
			player->content<Entity>().ctrl.input = controls::keyboard_plane(Input::WASD);
			if (has_all(player->flags, Entity::Sprite))
				player->content<Entity>().sprite.uv_rect = controls::animate(player->content<Entity>().ctrl, anim, clock.current);
		}

		pov = (player.valid() ? player->content<Entity>().space : Spacial2D{ identity_2d, null_transform_2d, null_transform_2d });
		pov.transform.rotation = 0;

		for (auto& i : entities.allocated()) if (has_all(i.flags, Entity::Controllable)) controls::move_top_down(i.space.velocity.translation, i.ctrl.input, i.ctrl.speed, i.ctrl.accel, clock.dt);
		for (auto& i : entities.allocated()) euler_integrate(i.space, clock.dt);
		physics(gather(flush_scratch(), entities.allocated(), Entity::Collider, [&](Entity& ent) { return RigidBody{ get_entity_genhandle(ent), &ent.shape, &ent.space, (has_all(ent.flags, Entity::Rigidbody) ? &ent.body : null) }; }));
		audio(gather(flush_scratch(), entities.allocated(), Entity::Sound, [&](Entity& ent) { return tuple(&ent.audio_source, (const Spacial2D*)&ent.space); }), &pov);
		rendering(gather(flush_scratch(), entities.allocated(), Entity::Sprite, [&](const Entity& ent) { return sprite_data(trs_2d(ent.space.transform), ent.sprite, ent.render_rect.dimensions, ent.render_rect.depth); }), pov.transform);
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
			if (ph.collisions) ph.draw_collisions(physics.collisions, vp, &Entity::space);
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
				EditorWidget("Gravity", gravity);
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
