#ifndef GPLAYGROUND_SCENE
# define GPLAYGROUND_SCENE

#define PROFILE_TRACE_ON
#include <spall/profiling.cpp>

#include <imgui_extension.cpp>
#include <application.cpp>
#include <rendering.cpp>
#include <time.cpp>
#include <transform.cpp>
#include <physics_2d.cpp>
#include <physics_2d_debug.cpp>
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

#include <tilemap.cpp>

//test
#include <text.cpp>

#define MAX_SPRITES MAX_ENTITIES

const struct {
	cstrp test_character_spritesheet_path = "test_stuff/test_character.png";
	cstrp test_character_anim_path = "test_stuff/test_character.anim";
	cstrp sprite_pipeline = "./shaders/sprite.glsl";
	cstrp test_sound = "test_stuff/file_example_OOG_1MG.ogg";
	cstrp test_sidescroll_path = "test_stuff/12_Animated_Character_Template.png";
	cstrp sidescroll_character_animation_recipe_path = "test_stuff/test_sidescroll_character.xml";
	cstrp tilemap_pipeline = "./shaders/tilemap.glsl";
	cstrp level = "test_stuff/test.tmx";
} assets;

// struct Entity : public EntitySlot {
// 	enum : u64 {
// 		AllocatedEntity = SlotAllocatedEntity,
// 		Enabled = SlotEnabled,
// 		Usable = SlotUsable,
// 		PendingRelease = SlotPendingRelease,
// 		UserFlag = SlotUserFlag,
// 		Sound = UserFlag << 0,
// 		Draw = UserFlag << 1,
// 		Collider = UserFlag << 2,
// 		Physical = UserFlag << 3,
// 		Controllable = UserFlag << 4,
// 		Animated = UserFlag << 5,
// 		TilemapTree = UserFlag << 6,
// 		Camera = UserFlag << 7,
// 		Temporary = UserFlag << 8,
// 	};

// 	static constexpr string Flags[] = {
// 		BaseFlags[0],
// 		BaseFlags[1],
// 		BaseFlags[2],
// 		BaseFlags[3],
// 		"Sound",
// 		"Draw",
// 		"Collider",
// 		"Physical",
// 		"Controllable",
// 		"Animated",
// 		"Tilemap",
// 		"Camera",
// 		"Temporary"
// 	};

// 	Spacial2D space;
// 	AudioSource audio_source;//* need release, out of arena
// 	Sprite sprite;
// 	SidescrollControl ctrl;
// 	Array<Shape2D> shapes;//* need release, shared, in arena
// 	Body body;
// 	Animator anim;
// 	Array<SpriteAnimation> animations;
// 	rtu32 spritesheet = { v2u32(0), v2u32(0) };
// 	Tilemap tilemap;
// 	OrthoCamera projection;
// 	RenderTarget render_target;
// 	Time::Timer lifetime;
// };

// template<> tuple<bool, RigidBody> use_as<RigidBody>(EntityHandle handle) {
// 	if (!has_all(handle->flags, Entity::Collider)) return tuple(false, RigidBody{});
// 	return {
// 		true,
// 		RigidBody{
// 			handle,
// 			handle->content<Entity>().shapes,
// 			&handle->content<Entity>().space,
// 			(has_all(handle->flags, Entity::Physical) ? &handle->content<Entity>().body : null),
// 			((has_all(handle->flags, Entity::Controllable) && handle->content<Entity>().ctrl.falling) ? handle->content<Entity>().ctrl.fall_multiplier : 1.f)
// 		}
// 	};
// }

// template<> tuple<bool, SidescrollCharacter> use_as<SidescrollCharacter>(EntityHandle handle) {
// 	if (!has_all(handle->flags, Entity::Controllable)) return tuple(false, SidescrollCharacter{});
// 	return {
// 		true,
// 		SidescrollCharacter{
// 			&handle->content<Entity>().ctrl,
// 			&handle->content<Entity>().anim,
// 			&handle->content<Entity>().sprite,
// 			&handle->content<Entity>().space,
// 			handle->content<Entity>().animations,
// 			handle->content<Entity>().spritesheet
// 		}
// 	};
// }

// template<> tuple<bool, Camera> use_as<Camera>(EntityHandle handle) {
// 	if (!has_all(handle->flags, Entity::Camera)) return tuple(false, Camera{});
// 	return {
// 		true,
// 		Camera{
// 			&handle->content<Entity>().space,
// 			&handle->content<Entity>().projection,
// 			&handle->content<Entity>().render_target
// 		}
// 	};
// }

// template<> tuple<bool, SpriteRenderer::Instance> use_as<SpriteRenderer::Instance>(EntityHandle handle) {
// 	if (!has_all(handle->flags, Entity::Draw)) return tuple(false, SpriteRenderer::Instance{});
// 	return { true, SpriteRenderer::make_instance(handle->content<Entity>().sprite, handle->content<Entity>().space.transform) };
// }

// template<> tuple<bool, Sound> use_as<Sound>(EntityHandle handle) {
// 	if (!has_all(handle->flags, Entity::Sound)) return tuple(false, Sound{});
// 	return { true, Sound{ &handle->content<Entity>().audio_source, &handle->content<Entity>().space } };
// }

// template<> tuple<bool, Spacial2D*> use_as<Spacial2D*>(EntityHandle handle) { return { true, &handle->content<Entity>().space }; }

// bool EditorWidget(const cstr label, Entity& ent) {
// 	bool changed = false;
// 	if (ImGui::TreeNode(label)) {
// 		defer{ ImGui::TreePop(); };

// 		changed |= ImGui::bit_flags("Flags", ent.flags, larray(Entity::Flags).subspan(1), false);
// 		changed |= EditorWidget("Space", ent.space);
// 		changed |= EditorWidget("Audio Source", ent.audio_source);
// 		changed |= EditorWidget("Sprite", ent.sprite);
// 		changed |= EditorWidget("Shape", ent.shapes);
// 		changed |= EditorWidget("Body", ent.body);
// 		changed |= EditorWidget("Controller", ent.ctrl);
// 		changed |= EditorWidget("Animator", ent.anim);
// 		changed |= EditorWidget("animations", ent.animations);
// 		changed |= EditorWidget("spritesheet", ent.spritesheet);
// 		changed |= EditorWidget("tilemap", ent.tilemap);
// 		changed |= EditorWidget("projection", ent.projection);
// 		changed |= EditorWidget("render_target", ent.render_target);

// 	}
// 	return changed;
// }

struct RefactorScene {

	struct {
		Atlas2D sprite_atlas;
		SpriteMesh::Pipeline draw_sprite_meshes;
		SpriteMesh::Renderer sm_rd;

		Tilemap::Pipeline draw_tilemap;
		Tilemap::Renderer tm_rd;

		UI::Pipeline draw_ui;
		UI::Renderer ui_rd;
		Text::Font font;
	} gfx;

	Time::Clock clock;
	struct {
		Spacial2D space;
		OrthoCamera proj;
		ClearCommand clear;
		RenderTarget target;
	} cam;

	static constexpr u32 ENTITY_COUNT = 3;
	struct {
		Tilemap::Terrain terrain;
		struct {
			Spacial2D space;
			rtu32 sprite;
			v4f32 color;
			Physics2D::Convex* shape;
			Physics2D::Momentum momentum;
			Physics2D::Properties props;
		} entities[ENTITY_COUNT];
		u32 mesh_index;
	} test;

	static RefactorScene create(GLScope& ctx) {
		auto [scratch, scope] = scratch_push_scope(0, &ctx.arena); defer{ scratch_pop_scope(scratch, scope); };
		auto atlas = Atlas2D::create(ctx, v2u32(256 * 8, 256 * 8));

		auto sprite_pipeline = SpriteMesh::Pipeline::create(ctx);
		auto sprite_renderer = sprite_pipeline.make_renderer(ctx);
		auto tex = sprite_renderer.push_texture(atlas.texture.id);
		(void)tex;
		SpriteMesh::Quad q[] = { {
			.info = {
				.albedo_index = 0,
				.depth = 0,
			},
			.rect = rtu32{.min = v2u32(0), .max = v2u32(5) },
		} };
		auto mesh_index = sprite_renderer.push_quad_mesh(larray(q), 16);

		auto tm_ppl = Tilemap::Pipeline::create(ctx);
		auto [tm_rd, tm_terrain] = Tilemap::load_proc(assets.level, [&](auto map) {
			return tuple(tm_ppl.make_renderer(ctx, map), Tilemap::Terrain::create(ctx.arena, map));
		});

		printf("Terrain layer count : %llu\n", tm_terrain.layers.size());
		for (auto& l : tm_terrain.layers) {
			printf("Terrain layer : %p collision : %u\n", &l, l.collision_layers);
		}

		auto ui_ppl = UI::Pipeline::create(ctx);
		auto ui_rd = ui_ppl.make_renderer(ctx);
		auto font = Text::Font::load(ctx, Text::FT_Global(), "test_stuff/test_font.ttf");

		RefactorScene scene = {
			.gfx = {
				.sprite_atlas = atlas,

				.draw_sprite_meshes = sprite_pipeline,
				.sm_rd = sprite_renderer,

				.draw_tilemap = tm_ppl,
				.tm_rd = tm_rd,

				.draw_ui = ui_ppl,
				.ui_rd = ui_rd,
				.font = font,
			},
			.clock = Time::Clock::start(),
			.cam = {
				.space = {
					.transform = {
						.translation = v2f32(0, 0),
						.scale = v2f32(1),
						.rotation = 0
					},
					.velocity = null_transform_2d,
					.accel = null_transform_2d
				},
				.proj = {
					.dimensions = v3f32(16, 9, 1000),
					.center = v3f32(0)
				},
				.clear = {
					.attachements = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
					.color = v4f32(v3f32(0.2), 1),
					.depth = 10,
					.stencil = 0
				},
				.target = RenderTarget::make_default(ctx)
			},
			.test = {}
		};

		auto& shape = ctx.arena.push(Physics2D::Convex{
			.type = Physics2D::Convex::CIRCLE,
			.radius = 1,
			.center = { 0, 0 },
		});

		scene.test = {
			.terrain = tm_terrain,
			.entities = {
				{
					.space = {
						.transform = {
							.translation = v2f32(2, 0),
							.scale = v2f32(1),
							.rotation = 0
						},
						.velocity = null_transform_2d,
						.accel = null_transform_2d
					},
					.sprite = { v2u32(0), v2u32(1) },
					.color = v4f32(1, 0, 0, 1),
					.shape = &shape,
					.momentum = {.vec = v3f32(0) },
					.props = {.vec = v4f32(1, 1, 1, 1) },
				},
				{
					.space = {
						.transform = {
							.translation = v2f32(-2, 0),
							.scale = v2f32(1),
							.rotation = 0
						},
						.velocity = null_transform_2d,
						.accel = null_transform_2d
					},
					.sprite = { v2u32(0), v2u32(1) },
					.color = v4f32(0, 1, 0, 1),
					.shape = &shape,
					.momentum = {.vec = v3f32(0) },
					.props = {.vec = v4f32(1, 1, 1, 1) },
				},
				{
					.space = {
						.transform = {
							.translation = v2f32(0, -2),
							.scale = v2f32(1),
							.rotation = 0
						},
						.velocity = null_transform_2d,
						.accel = null_transform_2d
					},
					.sprite = { v2u32(0), v2u32(1) },
					.color = v4f32(0, 0, 1, 1),
					.shape = &shape,
					.momentum = {.vec = v3f32(0) },
					.props = {.vec = v4f32(1, 1, 1, 1) },
				}
			},
			.mesh_index = mesh_index
		};

		return scene;
	}

	tuple<rtu32, RenderTarget&> operator()(bool debug = DEBUG_GL) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		static auto gravity_scale = 0.0f;
		if (debug) {
			if (ImGui::Begin("Test")) {
				EditorWidget("Gravity Scale", gravity_scale);
				// EditorWidget("Tilemap", gfx.level);
				for (u32 i = 0; auto & ent : test.entities) {
					ImGui::PushID(i); defer{ ImGui::PopID(); };
					ImGui::Text("Sprite [%u]", i++);
					ImGui::Indent(); defer{ ImGui::Unindent(); };
					ImGui::BeginGroup(); defer{ ImGui::EndGroup(); };
					EditorWidget("space", ent.space);
					EditorWidget("momentum", ent.momentum);
					EditorWidget("props", ent.props);
					EditorWidget("sprite", ent.sprite);
					ImGui::ColorEdit4("color", glm::value_ptr(ent.color));
				}
				EditorWidget("mesh_index", test.mesh_index);
			} ImGui::End();
			if (ImGui::Begin("Misc")) {
				EditorWidget("Clock", clock);
				if (ImGui::TreeNode("Camera")) {
					defer{ ImGui::TreePop(); };
					EditorWidget("Space", cam.space);
					EditorWidget("Projection", cam.proj);
					EditorWidget("Clear", cam.clear);
					EditorWidget("Target", cam.target);
				}
			} ImGui::End();
		}
		auto [scratch, scope] = scratch_push_scope(); defer{ scratch_pop_scope(scratch, scope); };
		clock.update();

		static struct {
			Arena arena;
			f32 time;
			f32 target_dt;
			struct {
				Array<Physics2D::Manifold> collisions;
				Array<Physics2D::Delta> deltas;
				Physics2D::SimStep step;
			} last_update;
		} phx_tests = { Arena::from_vmem(1 << 24, Arena::COMMIT_ON_PUSH | Arena::ALLOW_CHAIN_GROWTH | Arena::ALLOW_MOVE_MORPH), 0, 1.f / 60.f, {} };

		//* Physics Simulation iterations
		auto phx_it_this_frame = Physics2D::step_count(phx_tests.time, clock.app_time, phx_tests.target_dt, { 0, 5 });
		for (auto phx_it : u32xrange{ 0, phx_it_this_frame }) {
			(void)phx_it;
			phx_tests.arena.reset();
			auto step = Physics2D::SimStep::create(&phx_tests.arena, phx_tests.target_dt);
			phx_tests.time += phx_tests.target_dt;

			auto first_ent_body = step.bodies.current;
			for (auto& ent : test.entities) {
				ent.momentum.velocity += v2f32(0, -1) * Physics2D::EARTH_GRAVITY * step.dt * gravity_scale;

				//* integrate
				ent.space.transform.translation += ent.momentum.velocity * step.dt;
				ent.space.transform.rotation += ent.momentum.angular_velocity * step.dt;

				//* submit to simulation
				auto bd = step.push_body({
					.center_mass = ent.space.transform.translation,
					.momentum = ent.momentum,
					.props = ent.props
				});
				step.push_collider({
					.transform = ent.space.transform,
					.aabb = Physics2D::aabb_convex(*ent.shape, ent.space.transform),
					.shape = ent.shape,
					.body_id = i32(bd),
					.layers = 1
				});
			}

			//* Broadphase
			static auto detections = Physics2D::FlagMatrix<u32>::create_fill();
			Physics2D::broadphase_naive(step, detections);
			Tilemap::terrain_broadphase(step, test.terrain, detections);

			//* Processing
			static auto physical_collisions = Physics2D::FlagMatrix<u32>::create_fill();
			auto manifolds = Physics2D::query_collisions(phx_tests.arena, step.tests.used(), step.colliders.used());
			auto physical = Physics2D::filter_physical(phx_tests.arena, step.bodies.used(), step.colliders.used(), manifolds, physical_collisions);
			auto deltas = Physics2D::solve_collisions(phx_tests.arena, step.bodies.used(), step.colliders.used(), physical, step.dt);
			auto bodies = Physics2D::apply_resolution(step.bodies.used(), deltas, { u32(first_ent_body) , u32(step.bodies.current) });

			for (auto i : u32xrange{ 0, ENTITY_COUNT }) {
				test.entities[i].momentum = bodies[i].momentum;
				test.entities[i].space.transform.translation = bodies[i].center_mass;
			}

			phx_tests.last_update = {
				.collisions = manifolds,
				.deltas = deltas,
				.step = step
			};
		}

		Physics2D::Debug::Batch debug_batch;
		if (debug) {
			Physics2D::Debug::debug_config.draw_edit_window("Physics debug config");

			if (ImGui::Begin("Physics test last update")) {
				EditorWidget("Target dt",  phx_tests.target_dt);
				EditorWidget("Step",  phx_tests.last_update.step);
				EditorWidgetArray("Collisions", phx_tests.last_update.collisions, [](auto l, auto e) { return EditorWidget(l, e); });
				EditorWidgetArray("Deltas", phx_tests.last_update.deltas, [](auto l, auto e) { return EditorWidget(l, e); });

				auto it_color = phx_it_this_frame > 0 ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1);
				ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, it_color);
				ImGui::Text("Physics Iterations this frame : %u", phx_it_this_frame);
				ImGui::PopStyleColor();

			} ImGui::End();

			debug_batch = Physics2D::Debug::Batch::create(&scratch);
			debug_batch.push_sim_step(phx_tests.last_update.step);
			for (auto& man : phx_tests.last_update.collisions)
				debug_batch.push_manifold(man);
		}

		//* accumulate
		{//* sprite mesh
			auto batch = gfx.sm_rd.start_batch(); defer{ gfx.sm_rd.consume_batch(batch); };
			for (auto& ent : test.entities)
				batch.push_entity(ent.space.transform, ent.color, test.mesh_index, carray(&ent.sprite, 1));
		}
		// {//* ui
		// 	auto batch = UI::Batch::start(scratch, 1024, 1024 * 1024, {
		// 		.canvas_projection = OrthoCamera{
		// 			.dimensions = v3f32(cam.target.dimensions, 100),
		// 			// .center = v3f32(0)
		// 			.center = v3f32(-v2f32(cam.target.dimensions) / 2.f, 0)
		// 		},
		// 		.alpha_discard = 0.1f,
		// 		.padding = {}
		// 		}); defer{ gfx.ui_rd.apply_batch(batch); };

		// 		static rtf32 rect = { v2f32(100, 100), v2f32(500, 500) };
		// 		static Text::Style style = {
		// 			.color = v4f32(1, 1, 1, 1),
		// 			.scale = 1,
		// 			.linespace = 1,
		// 			.axis = Text::Axis::H
		// 		};
		// 		static char text_buffer[1024] = "test hello world";
		// 		ImGui::Begin("Test text"); defer{ ImGui::End(); };
		// 		ImGui::InputTextMultiline("Text", text_buffer, IM_ARRAYSIZE(text_buffer));
		// 		EditorWidget("Rect", rect);
		// 		EditorWidget("Style", style);
		// 		batch.push_text(text_buffer, rect, 0, gfx.font, style);
		// }

		//* submit draws
		auto drawn = start_render_pass(render_target_pass(cam.target, flex_viewport(cam.target.dimensions, cam.proj.dimensions, FLEX_CONTAINED))); {
			auto vp = m4x4f32(cam.proj) * glm::inverse(m4x4f32(cam.space.transform));
			clear(cam.clear);
			render_cmd(gfx.draw_sprite_meshes(scratch, gfx.sm_rd, {
				.view_projection = vp,
				.alpha_discard = 0.01f,
				.padding = {}
				}));
			render_cmd(gfx.draw_tilemap(scratch, gfx.tm_rd, {
				.view_projection = vp,
				.parallax_pov = cam.space.transform.translation,
				.alpha_discard = 0.1f,
				.padding = {}
				}));
			if (debug)
				render_cmd(Physics2D::Debug::render(scratch, debug_batch, vp));
			// render_cmd(gfx.draw_ui(scratch, gfx.ui_rd));
		}
		return { drawn, cam.target };
	}

};

// struct PlaygroundScene {

// 	struct {
// 		SpriteRenderer draw_sprites;
// 		TilemapRenderer draw_tilemap;
// 		Atlas2D sprite_atlas;
// 		rtu32 white;
// 	} gfx;

// 	Physics2D physics;
// 	AudioDevice audio;

// 	Arena resources_arena;
// 	List<Entity> entities;
// 	Array<SpriteAnimation> animations;
// 	rtu32 spritesheet;
// 	Tilemap level;

// 	Time::Clock clock;
// 	EntityHandle player;
// 	EntityHandle cam;
// 	EntityHandle level_entity;

// 	struct {
// 		SpriteMeshRenderer draw_sprite_meshes;
// 		UIRenderer draw_texts;
// 		Font font;
// 		rtu32 sp[1];
// 		u32 mesh_index;
// 	} test;

// 	Entity& create_test_body(string name, v2f32 position) {
// 		auto& ent = allocate_entity(entities, name, Entity::Draw | Entity::Collider | Entity::Physical);
// 		ent.space = Spacial2D{
// 			.transform = { .translation = position, .scale = v2f32(1), .rotation = 0 },
// 			.velocity = null_transform_2d,
// 			.accel = null_transform_2d
// 		};
// 		ent.sprite = Sprite{
// 			.view = gfx.white,
// 			.dimensions = v2f32(1),
// 			.depth = 1
// 		};
// 		ent.body = Body{
// 			.inverse_mass = 1.f,
// 			.inverse_inertia = 1.f,
// 			.restitution = 1.f,
// 			.friction = .1f,
// 			.shape_index = 0
// 		};
// 		static v2f32 test_polygon[] = { v2f32(-1, -1) / 2.f, v2f32(+1, -1) / 2.f, v2f32(+1, +1) / 2.f, v2f32(-1, +1) / 2.f };
// 		static Shape2D test_body_shape = make_shape_2d(identity_2d, 0, larray(test_polygon));
// 		ent.shapes = carray(&test_body_shape, 1);
// 		return ent;
// 	}

// 	void release() {
// 		PROFILE_SCOPE(__PRETTY_FUNCTION__);

// 		//test
// 		// test.draw_sprite_meshes.release();
// 		test.font.release();
// 		// test.draw_texts.release();

// 		level.release();
// 		// gfx.sprite_atlas.release();
// 		gfx.draw_tilemap.release();
// 		gfx.draw_sprites.release();
// 		audio.release();
// 		physics.release();
// 		resources_arena.vmem_release();
// 	}

// 	static constexpr v4f32 white_pixel[] = { v4f32(1) };

// 	static PlaygroundScene create(u64 resource_capacity = 1ull << 23) {
// 		PROFILE_SCOPE(__PRETTY_FUNCTION__);
// 		PlaygroundScene scene;
// 		scene.resources_arena = Arena::from_vmem(resource_capacity);

// 		{
// 			PROFILE_SCOPE("Systems init");
// 			scene.physics = Physics2D::create();
// 			scene.audio = AudioDevice::init();
// 			scene.gfx.draw_sprites = SpriteRenderer::load(assets.sprite_pipeline);
// 			scene.gfx.draw_tilemap = TilemapRenderer::load(assets.tilemap_pipeline);
// 		}

// 		{
// 			PROFILE_SCOPE("Ressources init");
// 			auto [scratch, scope] = scratch_push_scope(1ull << 18); defer{ scratch_pop_scope(scratch, scope); };

// 			scene.gfx.sprite_atlas = Atlas2D::create(GLScope::global(), v2u32(1920, 1080));
// 			scene.gfx.white = scene.gfx.sprite_atlas.push(make_image(larray(white_pixel), v2u32(1)));

// 			auto img = load_image(assets.test_sidescroll_path); defer{ unload(img); };
// 			scene.spritesheet = scene.gfx.sprite_atlas.push(img);

// 			auto layout = build_layout(scratch, assets.sidescroll_character_animation_recipe_path);
// 			scene.animations = build_sidescroll_character_animations(scene.resources_arena, layout, img.dimensions);
// 		}

// 		{
// 			PROFILE_SCOPE("Scene content init");
// 			scene.entities = List{ scene.resources_arena.push_array<Entity>(MAX_ENTITIES), 0 };
// 			scene.player = { null, 0 };
// 			scene.level = Tilemap::load(scene.resources_arena, scene.gfx.sprite_atlas, assets.level,
// 				[&](tmx_object_group* group, v2f32 tile_dimensions) {
// 					for (auto& obj : traverse_by<tmx_object, &tmx_object::next>(group->head)) {
// 						if (string(obj.type) == string("test_entity_construct")) {
// 							u64 flags = 0;
// 							for (auto i : u64xrange{ 0, array_size(Entity::Flags) }) if (auto prop = tmx_get_property(obj.properties, Entity::Flags[i].data()); prop && prop->type == PT_BOOL && prop->value.boolean)
// 								flags |= 1 << i;
// 							auto& ent = allocate_entity(scene.entities, obj.name, flags);
// 							ent.enable(obj.visible);
// 							ent.space.transform.translation = v2f32(obj.x, -obj.y) / tile_dimensions;
// 							ent.space.transform.rotation = glm::radians(obj.rotation);
// 						} else if (string(obj.type) == string("test_body")) {
// 							auto& ent = scene.create_test_body(obj.name, v2f32(obj.x, -obj.y) / tile_dimensions);
// 							ent.space.transform.rotation = glm::radians(obj.rotation);
// 							ent.enable(obj.visible);
// 						} else if (string(obj.type) == string("player")) {
// 							scene.player = (
// 								[&]() {
// 									auto& ent = scene.create_test_body(obj.name, v2f32(obj.x, -obj.y) / tile_dimensions);
// 									ent.space.transform.rotation = glm::radians(obj.rotation);
// 									ent.flags |= (Entity::Controllable | Entity::Animated);
// 									ent.spritesheet = scene.spritesheet;
// 									ent.sprite.view = { v2u32(0), dim_vec(scene.spritesheet) };
// 									ent.animations = scene.animations;
// 									ent.body.inverse_inertia = 0.f;
// 									ent.body.inverse_mass = 1.f;
// 									ent.body.restitution = 0.f;
// 									ent.enable(obj.visible);
// 									return get_entity_genhandle(ent);
// 								}
// 							());
// 						}
// 					}
// 				}
// 			);

// 			if (!scene.player.valid()) {
// 				scene.player = (
// 					[&]() {
// 						auto& ent = scene.create_test_body("player", v2f32(0));
// 						ent.flags |= (Entity::Controllable | Entity::Animated);
// 						ent.spritesheet = scene.spritesheet;
// 						ent.sprite.view = { v2u32(0), dim_vec(scene.spritesheet) };
// 						ent.animations = scene.animations;
// 						ent.body.inverse_inertia = 0.f;
// 						ent.body.inverse_mass = 1.f;
// 						ent.body.restitution = 0.f;
// 						ent.enable();
// 						return get_entity_genhandle(ent);
// 					}
// 				());
// 			}

// 			scene.level_entity = (
// 				[&]() {
// 					auto& ent = allocate_entity(scene.entities, "Level", Entity::Collider | Entity::Physical);
// 					ent.body.inverse_inertia = 0;
// 					ent.body.inverse_mass = 0;
// 					ent.body.restitution = .1f;
// 					ent.body.friction = .1f;
// 					ent.body.shape_index = 0;
// 					ent.tilemap = scene.level;
// 					ent.shapes = tilemap_shapes(scene.resources_arena, *scene.level.tree, tile_shapeset(scene.resources_arena, carray(scene.level.tree->tiles, scene.level.tree->tilecount)));
// 					ent.enable();
// 					return get_entity_genhandle(ent);
// 				}
// 			());

// 			scene.cam = (
// 				[&]() {
// 					auto& ent = allocate_entity(scene.entities, "Camera", Entity::Camera);
// 					ent.render_target.fbf = FB_DFLT;
// 					ent.render_target.clear_color = v4f32(v3f32(0.3), 1);
// 					ent.projection.dimensions = v3f32(16, 9, 1000);
// 					ent.projection.center = v3f32(0);
// 					ent.enable();
// 					return get_entity_genhandle(ent);
// 				}
// 			());
// 		}

// 		{
// 			PROFILE_SCOPE("Flushing log buffers");
// 			fflush(stdout);
// 		}

// 		//test
// 		scene.test.draw_texts = UIRenderer::load("./shaders/ui.glsl");
// 		// font = Font::load(resources_arena, FT_Global(), "test_font.ttf");
// 		scene.test.font = Font::load(scene.resources_arena, FT_Global(), "test_stuff/Arial.ttf");
// 		scene.test.draw_sprite_meshes = SpriteMeshRenderer::load(GLScope::global(), "./shaders/sprite_mesh_2d.glsl");

// 		auto tex = scene.test.draw_sprite_meshes.push_texture(&scene.gfx.sprite_atlas.texture);
// 		SpriteMeshRenderer::Quad q[] = {{
// 			.albedo_index = tex,
// 			.depth = 0,
// 			.rect = rtu32{ .min = v2u32(0), .max = v2u32(5) },
// 		}};
// 		scene.test.sp[0] = scene.player->content<Entity>().sprite.view;
// 		scene.test.mesh_index = scene.test.draw_sprite_meshes.push_quad_mesh(larray(q), 16);

// 		{
// 			PROFILE_SCOPE("Waiting for GPU init work");
// 			wait_gpu();
// 		}
// 		scene.clock = Time::start();
// 		return scene;
// 	}

// 	u64 update_count = 0;
// 	bool operator()(bool debug = false) {
// 		PROFILE_SCOPE(__PRETTY_FUNCTION__);
// 		{
// 			PROFILE_SCOPE("Editor");
// 			static auto debug_scratch = Arena::from_vmem(1 << 19);
// 			debug_scratch.reset();

// 			if (debug) {
// 				if (ImGui::Begin("Entities")) for (auto ent : gather(debug_scratch, entities.used(), Entity::AllocatedEntity)) {
// 					ImGui::PushID(ent); defer{ ImGui::PopID(); };
// 					EditorWidget(ent->name.data(), *ent);
// 				} ImGui::End();

// 				if (ImGui::Begin("Misc")) {
// 					ImGui::Text("Update index : %llu", update_count++);
// 					EditorWidget("Clock", clock);
// 				} ImGui::End();
// 			}
// 		}

// 		update(clock);
// 		auto [scratch, scope] = scratch_push_scope(1ull << 19); defer{ scratch_pop_scope(scratch, scope); };

// 		if (Input::KB::get(Input::KB::K_ENTER) & Input::Down) for (auto i = 0; i < 10; i++)
// 			create_test_body("new test body", player->content<Entity>().space.transform.translation + v2f32(0, 2)).enable();

// 		if (player.valid())
// 			player_input(player->content<Entity>().ctrl);

// 		update_characters(gather<SidescrollCharacter>(scratch, entities.used()), physics.collisions.used(), physics.gravity, clock);
// 		if (auto it_count = physics.iteration_count(clock.app_time); it_count > 0)
// 			physics(gather<RigidBody>(scratch, entities.used()), gather<Spacial2D*>(scratch, entities.used()), it_count);

// 		if (cam.valid() && player.valid())
// 			follow(cam->content<Entity>().space, player->content<Entity>().space);

// 		update_audio(audio, gather<Sound>(scratch, entities.used()), cam->content<Entity>().space);

// 		for (auto& cam : gather<Camera>(scratch, entities.used())) render(cam,
// 			[&](m4x4f32 mat) {
// 				if (level_entity.valid() && level_entity->enabled()) gfx.draw_tilemap(
// 					level_entity->content<Entity>().tilemap,
// 					level_entity->content<Entity>().space.transform,
// 					mat, gfx.sprite_atlas.texture
// 				);
// 				gfx.draw_sprites(gather<SpriteRenderer::Instance>(scratch, entities.used()), mat, gfx.sprite_atlas.texture);
// 				//test
// 				{
// 					static char text_buffer[4096] = "";
// 					static f32 test_text_scale = 1;
// 					if (ImGui::Begin("Test Text")) {
// 						EditorWidget("Test font", test.font);
// 						ImGui::InputTextMultiline("Text", text_buffer, array_size(text_buffer));
// 						ImGui::DragFloat("Scale", &test_text_scale, max(0.0001f, test_text_scale * 0.01f));
// 					} ImGui::End();

// 					Text text;
// 					text.str = text_buffer;
// 					text.rect = rtu32{ v2u32(0), cam.target->fbf.dimensions };
// 					text.color = v4f32(1);
// 					text.font = &test.font;
// 					text.scale = test_text_scale;
// 					text.linespace = 1.f;
// 					text.orient = Text::H;
// 					test.draw_texts(carray(&text, 1), cam.target->fbf.dimensions);

// 					test.draw_sprite_meshes.push_entity(m4x4f32(1), v4f32(1), test.mesh_index, larray(test.sp));
// 					test.draw_sprite_meshes(mat);
// 					test.draw_sprite_meshes.clear_entities();
// 				}
// 			}
// 		);

// 		for (auto temp : gather(scratch, entities.used(), Entity::Usable | Entity::Temporary)) if (temp->lifetime.over(clock.app_time))
// 			temp->discard();

// 		//Resource cleanup
// 		AudioSource::batch_release(map(scratch, gather(scratch, entities.used(), Entity::PendingRelease | Entity::Sound), [](Entity* ent) -> ALuint { return ent->audio_source.id; }));
// 		for (auto slot : gather(scratch, entities.used(), Entity::PendingRelease))
// 			slot->recycle();
// 		return true;
// 	}

// };

#endif
