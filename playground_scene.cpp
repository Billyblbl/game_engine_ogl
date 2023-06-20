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

// struct PlaygroundScene {
// 	Rendering rendering;
// 	Physics2D physics;
// 	Audio audio;
// 	FrameBuffer fbf;

// 	EntityRegistry entities;
// 	ComponentRegistry<Spacial2D> spacials;
// 	ComponentRegistry<AudioSource> audio_sources;
// 	ComponentRegistry<SpriteCursor> sprites;
// 	ComponentRegistry<Body2D> bodies;

// 	AnimationGrid<rtf32, 2> anim;
// 	controls::TopDownControl ctrl;
// 	Time::Clock clock;
// 	EntityHandle player;

// 	PlaygroundScene(FrameBuffer _fbf = default_framebuffer) {
// 		fbf = _fbf;
// 		entities = create_entity_registry(std_allocator, MAX_ENTITIES);
// 		spacials = register_new_component<Spacial2D>(entities, std_allocator, MAX_ENTITIES / 2, "Spacial");
// 		audio_sources = register_new_component<AudioSource>(entities, std_allocator, MAX_ENTITIES / 2, "Audio Source");
// 		sprites = register_new_component<SpriteCursor>(entities, std_allocator, MAX_ENTITIES / 2, "Sprite");
// 		bodies = register_new_component<Body2D>(entities, std_allocator, MAX_ENTITIES / 2, "Body");

// 		anim = load_animation_grid<rtf32, 2>(assets.test_character_anim_path, std_allocator);
// 		ctrl = { 1, 1, 1, v2f32(0), 0 };
// 		clock = Time::start();
// 		player = (
// 			[&]() {
// 				static v2f32 player_polygon[] = { v2f32(-1, -1) / 2.f, v2f32(+1, -1) / 2.f, v2f32(+1, +1) / 2.f, v2f32(-1, +1) / 2.f };
// 				auto ent = allocate_entity(entities, "player");
// 				spacials.add_to(ent, {});
// 				sprites.add_to(ent, load_into(assets.test_character_spritesheet_path, rendering.atlas, v2u32(0), 0));
// 				bodies.add_to(ent, { {}, make_shape_2d<Shape2D::Polygon>(larray(player_polygon)) });
// 				assert(ent.valid());
// 				return ent;
// 			}
// 		());

// 		fflush(stdout);
// 		wait_gpu();
// 	}

// 	~PlaygroundScene() {
// 		defer{ delete_registry(std_allocator, entities); };
// 		defer{ delete_registry(std_allocator, spacials); };
// 		defer{ delete_registry(std_allocator, audio_sources); };
// 		defer{ delete_registry(std_allocator, sprites); };
// 		defer{ delete_registry(std_allocator, bodies); };
// 		defer{ unload(anim, std_allocator); };
// 	}

// 	bool operator()() {
// 		update(clock);
// 		clear_stale(bodies, audio_sources, sprites, spacials);

// 		{// player controller
// 			ctrl.input = controls::keyboard_plane(Input::KB::K_W, Input::KB::K_A, Input::KB::K_S, Input::KB::K_D);
// 			auto pl_sprite = sprites[player];
// 			if (pl_sprite)
// 				pl_sprite->uv_rect = controls::animate(ctrl, anim, clock.current);
// 			auto pl_spacial = spacials[player];
// 			if (pl_spacial)
// 				controls::move_top_down(pl_spacial->velocity.translation, ctrl.input, ctrl.speed, ctrl.accel, clock.dt);
// 		}

// 		physics(bodies, shapes, spacials, clock.dt);
// 		audio(audio_sources, spacials, spacials[player]);
// 		rendering(sprites, spacials, fbf, (player.valid() ? spacials[player]->transform : identity_2d));
// 		return true;
// 	}

// 	static auto default_editor() {
// 		return tuple(
// 			Rendering::default_editor(),
// 			Audio::default_editor(),
// 			Physics2D::default_editor(),
// 			SystemEditor("Entities", "Alt+E", { Input::KB::K_LEFT_ALT, Input::KB::K_E })
// 		);
// 	}

// 	void editor(tuple<SystemEditor, SystemEditor, Physics2D::Editor, SystemEditor>& ed) {
// 		auto& [rd, au, ph, ent] = ed;
// 		if (ph.debug)
// 			ph.draw_debug(physics.collisions.allocated(), bodies, spacials, rendering.view_projection_matrix);

// 		if (rd.show_window) {
// 			if (begin_editor(rd)) {
// 				rendering.editor_window();
// 			} end_editor();
// 		}

// 		if (au.show_window) {
// 			if (begin_editor(au)) {
// 				audio.editor_window();
// 			} end_editor();
// 		}

// 		if (ph.show_window) {
// 			if (begin_editor(ph)) {
// 				ph.editor_window(physics);
// 			} end_editor();
// 		}

// 		if (ent.show_window) {
// 			if (begin_editor(ent)) {
// 				entity_registry_editor(entities, spacials, audio_sources, sprites, bodies);
// 			} end_editor();
// 		}
// 	}

// };

struct PhysicsTestBed {
	OrthoCamera camera;
	Transform2D view = identity_2d;
	MappedObject<m4x4f32> view_projection_matrix;
	Physics2D physics;

	FrameBuffer fbf;

	EntityRegistry entities;
	ComponentRegistry<Spacial2D> spacials;
	ComponentRegistry<Shape2D> shapes;
	ComponentRegistry<Body> bodies;

	Time::Clock clock;

	PhysicsTestBed(FrameBuffer _fbf = default_framebuffer) {
		camera = { v3f32(16.f, 9.f, 1000.f) * 4.f, v3f32(0) };
		view_projection_matrix = map_object(m4x4f32(1));
		fbf = _fbf;
		entities = create_entity_registry(std_allocator, MAX_ENTITIES);
		spacials = register_new_component<Spacial2D>(entities, std_allocator, MAX_ENTITIES / 2, "Spacial");
		shapes = register_new_component<Shape2D>(entities, std_allocator, MAX_ENTITIES / 2, "Shape");
		bodies = register_new_component<Body>(entities, std_allocator, MAX_ENTITIES / 2, "Body");

		clock = Time::start();
		static v2f32 test_polygon[] = { v2f32(-1, -1), v2f32(+1, -1), v2f32(+1, +1), v2f32(-1, +1) };
		static v2f32 test_polygon2[] = { v2f32(-1000, -1), v2f32(+1000, -1), v2f32(+1000, +1), v2f32(-1000, +1) };

		static v2f32 test_polygon3[] = { v2f32(-10, -1) / 3.f, v2f32(+10, -1) / 3.f, v2f32(+10, +1) / 3.f, v2f32(-10, +1) / 3.f };
		static v2f32 test_polygon4[] = { v2f32(-1, -10) / 3.f, v2f32(+1, -10) / 3.f, v2f32(+1, +10) / 3.f, v2f32(-1, +10) / 3.f };

		static Shape2D subshapes[] = {
			make_shape_2d<Shape2D::Circle>(v3f32(0, 0, 5) / 3.f),
			make_shape_2d<Shape2D::Polygon>(larray(test_polygon3)),
			make_shape_2d<Shape2D::Polygon>(larray(test_polygon4))
		};

		{
			auto ent = allocate_entity(entities, "concave1");
			spacials.add_to(ent, {}).transform.translation = v2f32(0, 3);
			Body body;
			body.inverse_inertia = .1f;
			body.inverse_mass = 1.f;
			body.restitution = .5f;
			body.friction = .5f;
			bodies.add_to(ent, std::move(body));
			shapes.add_to(ent, make_shape_2d<Shape2D::Concave>(larray(subshapes)));
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

		// {
		// 	auto ent = allocate_entity(entities, "static4");
		// 	spacials.add_to(ent, {}).transform.translation = v2f32(0, 0);
		// 	shapes.add_to(ent, make_shape_2d<Shape2D::Polygon>(larray(test_polygon2)));
		// 	assert(ent.valid());
		// }

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

	~PhysicsTestBed() {
		defer{ delete_registry(std_allocator, entities); };
		defer{ delete_registry(std_allocator, spacials); };
		defer{ delete_registry(std_allocator, shapes); };
		defer{ delete_registry(std_allocator, bodies); };
	}

	v2f32 gravity = v2f32(0);
	bool operator()() {
		update(clock);
		clear_stale(shapes, spacials);

		for (auto [ent, body] : bodies.iter()) if (body->inverse_mass > 0) if (auto sp = spacials[*ent])
			sp->velocity.translation += gravity * clock.dt;

		physics(bodies, shapes, spacials, clock.dt);
		sync(view_projection_matrix, view_project(project(camera), trs_2d(view)));
		begin_render(fbf);
		clear(fbf, v4f32(v3f32(0.3), 1));
		return true;
	}

	static auto default_editor() {
		auto ph = Physics2D::default_editor();
		ph.debug = true;
		ph.show_window = true;
		auto ent = create_editor("Entities", "Alt+E", { Input::KB::K_LEFT_ALT, Input::KB::K_E });
		ent.show_window = true;
		return tuple(ph, ent);
	}

	void editor(tuple<Physics2D::Editor, SystemEditor>& ed) {
		auto& [ph, ent] = ed;
		if (ph.debug)
			ph.draw_debug(physics.collisions_pool.allocated(), shapes, spacials, view_projection_matrix);

		if (ph.show_window) {
			if (begin_editor(ph)) {
				EditorWidget("Gravity", gravity);
				EditorWidget("View", view);
				EditorWidget("Camera", camera);
				ph.editor_window(physics);
			} end_editor();
		}

		if (ent.show_window) {
			if (begin_editor(ent)) {
				entity_registry_editor(entities, spacials, shapes, bodies);
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

	PhysicsTestBed scene;
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
