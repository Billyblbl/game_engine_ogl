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

struct Entity : public EntitySlot {
	enum : u64 {
		AllocatedEntity = SlotAllocatedEntity,
		Enabled = SlotEnabled,
		Usable = SlotUsable,
		PendingRelease = SlotPendingRelease,
		UserFlag = SlotUserFlag,
		Sound = UserFlag << 0,
		Draw = UserFlag << 1,
		Collider = UserFlag << 2,
		Physical = UserFlag << 3,
		Controllable = UserFlag << 4,
		Animated = UserFlag << 5,
		TilemapTree = UserFlag << 6,
		Camera = UserFlag << 7,
		Temporary = UserFlag << 8,
	};

	static constexpr string Flags[] = {
		BaseFlags[0],
		BaseFlags[1],
		BaseFlags[2],
		BaseFlags[3],
		"Sound",
		"Draw",
		"Collider",
		"Physical",
		"Controllable",
		"Animated",
		"Tilemap",
		"Camera",
		"Temporary"
	};

	Spacial2D space;
	AudioSource audio_source;//* need release, out of arena
	Sprite sprite;
	SidescrollControl ctrl;
	Array<Shape2D> shapes;//* need release, shared, in arena
	Body body;
	Animator anim;
	Array<SpriteAnimation> animations;
	rtu32 spritesheet = { v2u32(0), v2u32(0) };
	Tilemap tilemap;
	OrthoCamera projection;
	RenderTarget render_target;
	Time::Timer lifetime;
};

template<> tuple<bool, RigidBody> use_as<RigidBody>(EntityHandle handle) {
	if (!has_all(handle->flags, Entity::Collider)) return tuple(false, RigidBody{});
	return {
		true,
		RigidBody{
			handle,
			handle->content<Entity>().shapes,
			&handle->content<Entity>().space,
			(has_all(handle->flags, Entity::Physical) ? &handle->content<Entity>().body : null),
			((has_all(handle->flags, Entity::Controllable) && handle->content<Entity>().ctrl.falling) ? handle->content<Entity>().ctrl.fall_multiplier : 1.f)
		}
	};
}

template<> tuple<bool, SidescrollCharacter> use_as<SidescrollCharacter>(EntityHandle handle) {
	if (!has_all(handle->flags, Entity::Controllable)) return tuple(false, SidescrollCharacter{});
	return {
		true,
		SidescrollCharacter{
			&handle->content<Entity>().ctrl,
			&handle->content<Entity>().anim,
			&handle->content<Entity>().sprite,
			&handle->content<Entity>().space,
			handle->content<Entity>().animations,
			handle->content<Entity>().spritesheet
		}
	};
}

template<> tuple<bool, Camera> use_as<Camera>(EntityHandle handle) {
	if (!has_all(handle->flags, Entity::Camera)) return tuple(false, Camera{});
	return {
		true,
		Camera{
			&handle->content<Entity>().space,
			&handle->content<Entity>().projection,
			&handle->content<Entity>().render_target
		}
	};
}

template<> tuple<bool, SpriteRenderer::Instance> use_as<SpriteRenderer::Instance>(EntityHandle handle) {
	if (!has_all(handle->flags, Entity::Draw)) return tuple(false, SpriteRenderer::Instance{});
	return { true, SpriteRenderer::make_instance(handle->content<Entity>().sprite, handle->content<Entity>().space.transform) };
}

template<> tuple<bool, Sound> use_as<Sound>(EntityHandle handle) {
	if (!has_all(handle->flags, Entity::Sound)) return tuple(false, Sound{});
	return { true, Sound{ &handle->content<Entity>().audio_source, &handle->content<Entity>().space } };
}

template<> tuple<bool, Spacial2D*> use_as<Spacial2D*>(EntityHandle handle) { return { true, &handle->content<Entity>().space }; }

bool EditorWidget(const cstr label, Entity& ent) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };

		changed |= ImGui::bit_flags("Flags", ent.flags, larray(Entity::Flags).subspan(1), false);
		changed |= EditorWidget("Space", ent.space);
		changed |= EditorWidget("Audio Source", ent.audio_source);
		changed |= EditorWidget("Sprite", ent.sprite);
		changed |= EditorWidget("Shape", ent.shapes);
		changed |= EditorWidget("Body", ent.body);
		changed |= EditorWidget("Controller", ent.ctrl);
		changed |= EditorWidget("Animator", ent.anim);
		changed |= EditorWidget("animations", ent.animations);
		changed |= EditorWidget("spritesheet", ent.spritesheet);
		changed |= EditorWidget("tilemap", ent.tilemap);
		changed |= EditorWidget("projection", ent.projection);
		changed |= EditorWidget("render_target", ent.render_target);

	}
	return changed;
}

struct PlaygroundScene {

	struct {
		SpriteRenderer draw_sprites;
		TilemapRenderer draw_tilemap;
		Atlas2D sprite_atlas;
		rtu32 white;
	} gfx;

	Physics2D physics;
	AudioDevice audio;

	Arena resources_arena;
	List<Entity> entities;
	Array<SpriteAnimation> animations;
	rtu32 spritesheet;
	Tilemap level;

	Time::Clock clock;
	EntityHandle player;
	EntityHandle cam;
	EntityHandle level_entity;

	struct {
		TextRenderer draw_texts;
		Font font;
	} test;

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
		static Shape2D test_body_shape = make_shape_2d(identity_2d, 0, larray(test_polygon));
		ent.shapes = carray(&test_body_shape, 1);
		return ent;
	}

	void release() {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);

		//test
		test.font.release();
		test.draw_texts.release();

		level.release();
		gfx.sprite_atlas.release();
		resources_arena.vmem_release();
		gfx.draw_tilemap.release();
		gfx.draw_sprites.release();
		audio.release();
		physics.release();
	}

	static constexpr v4f32 white_pixel[] = { v4f32(1) };

	static PlaygroundScene create(u64 resource_capacity = 1ull << 23) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		PlaygroundScene scene;
		scene.resources_arena = Arena::from_vmem(resource_capacity);

		{
			PROFILE_SCOPE("Systems init");
			scene.physics = Physics2D::create();
			scene.audio = AudioDevice::init();
			scene.gfx.draw_sprites = SpriteRenderer::load(assets.sprite_pipeline);
			scene.gfx.draw_tilemap = TilemapRenderer::load(assets.tilemap_pipeline);
		}

		{
			PROFILE_SCOPE("Ressources init");
			auto [scratch, scope] = scratch_push_scope(1ull << 18); defer{ scratch_pop_scope(scratch, scope); };

			scene.gfx.sprite_atlas = Atlas2D::create(v2u32(1920, 1080));
			scene.gfx.white = scene.gfx.sprite_atlas.push(make_image(larray(white_pixel), v2u32(1)));

			auto img = load_image(assets.test_sidescroll_path); defer{ unload(img); };
			scene.spritesheet = scene.gfx.sprite_atlas.push(img);

			auto layout = build_layout(scratch, assets.sidescroll_character_animation_recipe_path);
			scene.animations = build_sidescroll_character_animations(scene.resources_arena, layout, img.dimensions);
		}

		{
			PROFILE_SCOPE("Scene content init");
			scene.entities = List{ scene.resources_arena.push_array<Entity>(MAX_ENTITIES), 0 };
			scene.player = { null, 0 };
			scene.level = Tilemap::load(scene.resources_arena, scene.gfx.sprite_atlas, assets.level,
				[&](tmx_object_group* group, v2f32 tile_dimensions) {
					for (auto& obj : traverse_by<tmx_object, &tmx_object::next>(group->head)) {
						if (string(obj.type) == string("test_entity_construct")) {
							u64 flags = 0;
							for (auto i : u64xrange{ 0, array_size(Entity::Flags) }) if (auto prop = tmx_get_property(obj.properties, Entity::Flags[i].data()); prop && prop->type == PT_BOOL && prop->value.boolean)
								flags |= 1 << i;
							auto& ent = allocate_entity(scene.entities, obj.name, flags);
							ent.enable(obj.visible);
							ent.space.transform.translation = v2f32(obj.x, -obj.y) / tile_dimensions;
							ent.space.transform.rotation = glm::radians(obj.rotation);
						} else if (string(obj.type) == string("test_body")) {
							auto& ent = scene.create_test_body(obj.name, v2f32(obj.x, -obj.y) / tile_dimensions);
							ent.space.transform.rotation = glm::radians(obj.rotation);
							ent.enable(obj.visible);
						} else if (string(obj.type) == string("player")) {
							scene.player = (
								[&]() {
									auto& ent = scene.create_test_body(obj.name, v2f32(obj.x, -obj.y) / tile_dimensions);
									ent.space.transform.rotation = glm::radians(obj.rotation);
									ent.flags |= (Entity::Controllable | Entity::Animated);
									ent.spritesheet = scene.spritesheet;
									ent.sprite.view = { v2u32(0), dim_vec(scene.spritesheet) };
									ent.animations = scene.animations;
									ent.body.inverse_inertia = 0.f;
									ent.body.inverse_mass = 1.f;
									ent.body.restitution = 0.f;
									ent.enable(obj.visible);
									return get_entity_genhandle(ent);
								}
							());
						}
					}
				}
			);

			if (!scene.player.valid()) {
				scene.player = (
					[&]() {
						auto& ent = scene.create_test_body("player", v2f32(0));
						ent.flags |= (Entity::Controllable | Entity::Animated);
						ent.spritesheet = scene.spritesheet;
						ent.sprite.view = { v2u32(0), dim_vec(scene.spritesheet) };
						ent.animations = scene.animations;
						ent.body.inverse_inertia = 0.f;
						ent.body.inverse_mass = 1.f;
						ent.body.restitution = 0.f;
						ent.enable();
						return get_entity_genhandle(ent);
					}
				());
			}

			scene.level_entity = (
				[&]() {
					auto& ent = allocate_entity(scene.entities, "Level", Entity::Collider | Entity::Physical);
					ent.body.inverse_inertia = 0;
					ent.body.inverse_mass = 0;
					ent.body.restitution = .1f;
					ent.body.friction = .1f;
					ent.body.shape_index = 0;
					ent.tilemap = scene.level;
					ent.shapes = tilemap_shapes(scene.resources_arena, *scene.level.tree, tile_shapeset(scene.resources_arena, carray(scene.level.tree->tiles, scene.level.tree->tilecount)));
					ent.enable();
					return get_entity_genhandle(ent);
				}
			());

			scene.cam = (
				[&]() {
					auto& ent = allocate_entity(scene.entities, "Camera", Entity::Camera);
					ent.render_target.fbf = default_framebuffer;
					ent.render_target.clear_color = v4f32(v3f32(0.3), 1);
					ent.projection.dimensions = v3f32(16, 9, 1000);
					ent.projection.center = v3f32(0);
					ent.enable();
					return get_entity_genhandle(ent);
				}
			());
		}

		{
			PROFILE_SCOPE("Flushing log buffers");
			fflush(stdout);
		}

		//test
		scene.test.draw_texts = TextRenderer::load("./shaders/text.glsl");
		// font = Font::load(resources_arena, test.draw_texts.lib, "test_font.ttf");
		scene.test.font = Font::load(scene.resources_arena, scene.test.draw_texts.lib, "test_stuff/Arial.ttf");

		{
			PROFILE_SCOPE("Waiting for GPU init work");
			wait_gpu();
		}
		scene.clock = Time::start();
		return scene;
	}

	u64 update_count = 0;
	bool operator()(bool debug = false) {
		{
			PROFILE_SCOPE(__PRETTY_FUNCTION__".Editor");
			static auto debug_scratch = Arena::from_vmem(1 << 19);
			debug_scratch.reset();

			if (debug) {
				if (ImGui::Begin("Entities")) for (auto ent : gather(debug_scratch, entities.used(), Entity::AllocatedEntity)) {
					ImGui::PushID(ent); defer{ ImGui::PopID(); };
					EditorWidget(ent->name.data(), *ent);
				} ImGui::End();

				if (ImGui::Begin("Misc")) {
					ImGui::Text("Update index : %llu", update_count++);
					EditorWidget("Clock", clock);
				} ImGui::End();
			}
		}

		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		update(clock);
		auto [scratch, scope] = scratch_push_scope(1ull << 19); defer{ scratch_pop_scope(scratch, scope); };

		if (Input::KB::get(Input::KB::K_ENTER) & Input::Down) for (auto i = 0; i < 10; i++)
			create_test_body("new test body", player->content<Entity>().space.transform.translation + v2f32(0, 2)).enable();

		if (player.valid())
			player_input(player->content<Entity>().ctrl);

		update_characters(gather<SidescrollCharacter>(scratch, entities.used()), physics.collisions.used(), physics.gravity, clock);
		if (auto it_count = physics.iteration_count(clock.current); it_count > 0)
			physics(gather<RigidBody>(scratch, entities.used()), gather<Spacial2D*>(scratch, entities.used()), it_count);

		if (cam.valid() && player.valid())
			follow(cam->content<Entity>().space, player->content<Entity>().space);

		update_audio(audio, gather<Sound>(scratch, entities.used()), cam->content<Entity>().space);

		for (auto& cam : gather<Camera>(scratch, entities.used())) render(cam,
			[&](m4x4f32 mat) {
				if (level_entity.valid() && level_entity->enabled()) gfx.draw_tilemap(
					level_entity->content<Entity>().tilemap,
					level_entity->content<Entity>().space.transform,
					mat, gfx.sprite_atlas.texture
				);
				gfx.draw_sprites(gather<SpriteRenderer::Instance>(scratch, entities.used()), mat, gfx.sprite_atlas.texture);
				//test
				{
					static char text_buffer[4096];
					static f32 test_text_scale = 1;
					memset(text_buffer, 0, sizeof(text_buffer));
					if (ImGui::Begin("Test Text")) {
						EditorWidget("Test font", test.font);
						ImGui::InputTextMultiline("Text", text_buffer, array_size(text_buffer));
						ImGui::InputFloat("Scale", &test_text_scale);
					} ImGui::End();

					Text text;
					text.str = text_buffer;
					text.rect = rtu32{ v2u32(0), cam.target->fbf.dimensions };
					text.color = v4f32(1);
					text.font = &test.font;
					text.scale = test_text_scale;
					text.linespace = 1.f;
					text.orient = Text::H;
					test.draw_texts(carray(&text, 1), cam.target->fbf.dimensions);
				}
			}
		);

		for (auto temp : gather(scratch, entities.used(), Entity::Usable | Entity::Temporary)) if (temp->lifetime.over(clock.current))
			temp->discard();

		//Resource cleanup
		AudioSource::batch_release(map(scratch, gather(scratch, entities.used(), Entity::PendingRelease | Entity::Sound), [](Entity* ent) -> ALuint { return ent->audio_source.id; }));
		for (auto slot : gather(scratch, entities.used(), Entity::PendingRelease))
			slot->recycle();
		return true;
	}

};

#endif
