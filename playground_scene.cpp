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
	ComponentRegistry<Shape2D> shapes;
	ComponentRegistry<Body> bodies;

	AnimationGrid<rtf32, 2> anim;
	controls::TopDownControl ctrl;
	Time::Clock clock;
	EntityHandle player;

	PlaygroundScene(FrameBuffer _fbf = default_framebuffer) {
		fbf = _fbf;
		rendering.camera = { v3f32(16.f, 9.f, 1000.f) * 4.f, v3f32(0) };
		entities = create_entity_registry(std_allocator, MAX_ENTITIES);
		spacials = register_new_component<Spacial2D>(entities, std_allocator, MAX_ENTITIES / 2, "Spacial");
		audio_sources = register_new_component<AudioSource>(entities, std_allocator, MAX_ENTITIES / 2, "Audio Source");
		sprites = register_new_component<SpriteCursor>(entities, std_allocator, MAX_ENTITIES / 2, "Sprite");
		shapes = register_new_component<Shape2D>(entities, std_allocator, MAX_ENTITIES / 2, "Shape");
		bodies = register_new_component<Body>(entities, std_allocator, MAX_ENTITIES / 2, "Body");

		anim = load_animation_grid<rtf32, 2>(assets.test_character_anim_path, std_allocator);
		ctrl = { 1, 1, 1, v2f32(0), 0 };
		clock = Time::start();
		player = (
			[&]() {
				static v2f32 player_polygon[] = { v2f32(-1, -1) / 2.f, v2f32(+1, -1) / 2.f, v2f32(+1, +1) / 2.f, v2f32(-1, +1) / 2.f };
				auto ent = allocate_entity(entities, "player");
				spacials.add_to(ent, {});
				sprites.add_to(ent, load_into(assets.test_character_spritesheet_path, rendering.atlas, v2u32(0), 0));
				Body body;
				body.inverse_inertia = .1f;
				body.inverse_mass = 1.f;
				body.restitution = .3f;
				body.friction = .8f;
				bodies.add_to(ent, std::move(body));
				shapes.add_to(ent, make_shape_2d<Shape2D::Polygon>(larray(player_polygon)));
				assert(ent.valid());
				return ent;
			}
		());

		static v2f32 test_polygon[] = { v2f32(-1, -1), v2f32(+1, -1), v2f32(+1, +1), v2f32(-1, +1) };
		static v2f32 test_polygon2[] = { v2f32(-1000, -1), v2f32(+1000, -1), v2f32(+1000, +1), v2f32(-1000, +1) };

		static v2f32 test_polygon_concave[] = { v2f32(-1, -1), v2f32(+1, -1), v2f32(+1, +1), v2f32(-1, +1), v2f32(-.5, 0) };
		auto [test_poly_decomposed, test_poly_decomposed_vertices] = decompose_concave_poly(larray(test_polygon_concave));
		static List<Shape2D> test_poly_decomposed_shapes = { alloc_array<Shape2D>(std_allocator, test_poly_decomposed.size()), 0 };

		test_poly_decomposed_shapes.current = 0;
		for (auto sub_poly : test_poly_decomposed)
			test_poly_decomposed_shapes.push(make_shape_2d<Shape2D::Polygon>(sub_poly));

		{
			auto ent = allocate_entity(entities, "concave1");
			spacials.add_to(ent, {}).transform.translation = v2f32(0, 3);
			Body body;
			body.inverse_inertia = .1f;
			body.inverse_mass = 1.f;
			body.restitution = .3f;
			body.friction = .8f;
			bodies.add_to(ent, std::move(body));
			shapes.add_to(ent, make_shape_2d<Shape2D::Concave>(test_poly_decomposed_shapes.allocated()));
			assert(ent.valid());
		}

		{
			auto ent = allocate_entity(entities, "body1");
			spacials.add_to(ent, {}).transform.translation = test_polygon[0] * 2.f + v2f32(0, 10);
			Body body;
			body.inverse_inertia = 1;
			body.inverse_mass = 1.f;
			body.restitution = .5f;
			body.friction = .5f;
			bodies.add_to(ent, std::move(body));
			shapes.add_to(ent, make_shape_2d<Shape2D::Polygon>(larray(test_polygon)));
			assert(ent.valid());
		}

		{
			auto ent = allocate_entity(entities, "body2");
			spacials.add_to(ent, {}).transform.translation = test_polygon[1] * 2.f + v2f32(0, 10);
			Body body;
			body.inverse_inertia = 1;
			body.inverse_mass = 1.f;
			body.restitution = .5f;
			body.friction = .5f;
			bodies.add_to(ent, std::move(body));
			shapes.add_to(ent, make_shape_2d<Shape2D::Circle>(v3f32(0, 0, 1)));
			assert(ent.valid());
		}

		{
			auto ent = allocate_entity(entities, "body3");
			spacials.add_to(ent, {}).transform.translation = test_polygon[2] * 2.f + v2f32(0, 10);
			Body body;
			body.inverse_inertia = 1;
			body.inverse_mass = 1.f;
			body.restitution = .5f;
			body.friction = .5f;
			bodies.add_to(ent, std::move(body));
			shapes.add_to(ent, make_shape_2d<Shape2D::Polygon>(larray(test_polygon)));
			assert(ent.valid());
		}

		{
			auto ent = allocate_entity(entities, "body4");
			spacials.add_to(ent, {}).transform.translation = test_polygon[3] * 2.f + v2f32(0, 10);
			Body body;
			body.inverse_inertia = 1;
			body.inverse_mass = 1.f;
			body.restitution = .5f;
			body.friction = .5f;
			bodies.add_to(ent, std::move(body));
			shapes.add_to(ent, make_shape_2d<Shape2D::Polygon>(larray(test_polygon)));
			assert(ent.valid());
		}

		{
			auto ent = allocate_entity(entities, "static1");
			spacials.add_to(ent, {}).transform.translation = v2f32(0, -2.5) - v2f32(0, -5);
			Body body;
			body.inverse_inertia = 0;
			body.inverse_mass = 0;
			body.restitution = .5f;
			body.friction = .5f;
			bodies.add_to(ent, std::move(body));
			shapes.add_to(ent, make_shape_2d<Shape2D::Polygon>(larray(test_polygon2)));
			assert(ent.valid());
		}

		{
			auto ent = allocate_entity(entities, "static2");
			Transform2D transform;
			transform.translation = v2f32(10, 3) - v2f32(0, -5);
			transform.rotation = 0;
			transform.scale = v2f32(3);
			spacials.add_to(ent, {}).transform = transform;
			Body body;
			body.inverse_inertia = 0;
			body.inverse_mass = 0;
			body.restitution = .5f;
			body.friction = .5f;
			bodies.add_to(ent, std::move(body));
			shapes.add_to(ent, make_shape_2d<Shape2D::Line>(Segment<v2f32> { v2f32(-1), v2f32(1) }));
			assert(ent.valid());
		}

		{
			auto ent = allocate_entity(entities, "static3");
			Transform2D transform;
			transform.translation = v2f32(-10, 3) - v2f32(0, -5);
			transform.rotation = -90;
			transform.scale = v2f32(3);
			spacials.add_to(ent, {}).transform = transform;
			Body body;
			body.inverse_inertia = 0;
			body.inverse_mass = 0;
			body.restitution = .5f;
			body.friction = .5f;
			bodies.add_to(ent, std::move(body));
			shapes.add_to(ent, make_shape_2d<Shape2D::Line>(Segment<v2f32> { v2f32(-1), v2f32(1) }));
			assert(ent.valid());
			spacials[ent]->transform.rotation = -90;
		}

		fflush(stdout);
		wait_gpu();
	}

	~PlaygroundScene() {
		defer{ delete_registry(std_allocator, entities); };
		defer{ delete_registry(std_allocator, spacials); };
		defer{ delete_registry(std_allocator, audio_sources); };
		defer{ delete_registry(std_allocator, sprites); };
		defer{ delete_registry(std_allocator, shapes); };
		defer{ delete_registry(std_allocator, bodies); };
		defer{ unload(anim, std_allocator); };
	}

	v2f32 gravity = v2f32(0);
	bool operator()() {
		constexpr auto SCRATCH_SIZE = (1 << 16) * sizeof(void*);
		static auto scratch = create_virtual_arena(SCRATCH_SIZE);

		update(clock);
		clear_stale(shapes, spacials, bodies);
		reset_virtual_arena(scratch);

		for (auto [ent, body] : bodies.iter()) if (body->inverse_mass > 0) if (auto sp = spacials[*ent])
			sp->velocity.translation += gravity * clock.dt;

		{// player controller
			ctrl.input = controls::keyboard_plane(Input::KB::K_W, Input::KB::K_A, Input::KB::K_S, Input::KB::K_D);
			auto pl_sprite = sprites[player];
			if (pl_sprite)
				pl_sprite->uv_rect = controls::animate(ctrl, anim, clock.current);
			auto pl_spacial = spacials[player];
			if (pl_spacial)
				controls::move_top_down(pl_spacial->velocity.translation, ctrl.input, ctrl.speed, ctrl.accel, clock.dt);
		}

		for (auto& sp : spacials.buffer.allocated()) euler_integrate(sp, clock.dt);
		physics(gather_as<RigidBody>(as_v_alloc(scratch), shapes.handles.allocated(), [](const RigidBodyComp& comp) { return tuple_as<RigidBody>(comp); }, shapes, spacials, bodies));
		reset_virtual_arena(scratch);
		audio(audio_sources, spacials, spacials[player]);
		rendering(sprites, spacials, fbf, (player.valid() ? spacials[player]->transform : identity_2d));
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
		if (ph.debug)
			ph.draw_debug(physics.collisions, shapes, spacials, rendering.view_projection_matrix);

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
				entity_registry_editor(entities, spacials, audio_sources, sprites, bodies);
			} end_editor();
		}

		if (misc.show_window) {
			if (begin_editor(misc)) {
				EditorWidget("Player controls", ctrl);
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
