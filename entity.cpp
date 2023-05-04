#ifndef GENTITY
# define GENTITY

#include <blblstd.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <transform.cpp>
#include <rendering.cpp>
#include <physics_2d.cpp>
#include <sprite.cpp>
#include <top_down_controls.cpp>
#include <audio.cpp>

#define MAX_ENTITIES 100
#define MAX_DRAW_BATCH MAX_ENTITIES

struct GenRef {
	u32 index;
	u32 generation;
};

template<typename T> u32 get_generation(T& t);

template<typename T> T* try_get(Array<T> arr, GenRef ref) {
	auto current_gen = get_generation(arr[ref.index]);
	if (current_gen == ref.generation)
		return &arr[ref.index];
	else
		return null;
}

struct Entity {
	using Controls = controls::TopDownControl;
	enum FlagIndex : u64 { Allocated, Dynbody, Solid, Sprite, Player, Sound };
	u64 flags = mask<u64>(Allocated);
	u32 generation = 0;
	Transform2D transform;
	RenderMesh* mesh;
	SpriteCursor sprite;
	f32 draw_layer;
	Body2D body;
	Collider2D* collider;
	Controls controls;
	AudioSource audio_source;
	string name = "__entity__";
};

template<> u32 get_generation<Entity>(Entity& t) { return t.generation; }

inline Entity& create_entity(List<Entity>& pool, const Entity& ent = {}) {
	Entity* slot = null;
	for (auto& ent : pool.allocated()) if (ent.flags == 0) {
		slot = &ent;
		break;
	}
	if (slot == null)
		slot = &pool.push({ 0, 0 });
	slot->generation++;
	return *slot;
}

bool EditorWidget(const cstr label, Entity& entity) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= ImGui::bit_flags("flags", entity.flags, { "Allocated", "Dynbody", "Collision", "Sprite", "Player", "Sound" });
		if (has_one(entity.flags, mask<u64>(Entity::Dynbody, Entity::Solid, Entity::Sprite, Entity::Player)))
			changed |= EditorWidget("transform", entity.transform);
		if (has_one(entity.flags, mask<u64>(Entity::Dynbody, Entity::Solid)))
			changed |= EditorWidget("body", entity.body);
		if (has_one(entity.flags, mask<u64>(Entity::Solid)))
			changed |= EditorWidget("collider", entity.collider);
		if (has_one(entity.flags, mask<u64>(Entity::Sprite)) && ImGui::TreeNode("sprite")) {
			defer{ ImGui::TreePop(); };
			changed |= EditorWidget("cursor", entity.sprite);
			changed |= EditorWidget("draw layer", entity.draw_layer);
		}
		if (has_one(entity.flags, mask<u64>(Entity::Player)))
			changed |= EditorWidget("controls", entity.controls);
		if (has_one(entity.flags, mask<u64>(Entity::Sound)))
			changed |= EditorWidget("audio source", entity.audio_source);
		ImGui::TreePop();
	}
	return changed;
}

auto& add_sprite(Entity& ent, SpriteCursor sprite, RenderMesh* mesh) {
	ent.flags |= mask<u64>(Entity::Sprite);
	ent.sprite = sprite;
	ent.mesh = mesh;
	return ent;
}

auto& add_dynbody(Entity& ent, const Body2D& body) {
	ent.flags |= mask<u64>(Entity::Dynbody);
	ent.body = body;
	ent.body.transform = ent.transform;
	return ent;
}

auto& add_collider(Entity& ent, Collider2D* collider) {
	ent.flags |= mask<u64>(Entity::Solid);
	ent.collider = collider;
	return ent;
}

auto& add_sound(Entity& ent, AudioSource source) {
	ent.flags |= mask<u64>(Entity::Sound);
	ent.audio_source = source;
	return ent;
}

auto create_player(SpriteCursor sprite, RenderMesh* mesh, Body2D body, Collider2D* collider, f32 speed = 10.f, f32 accel = 100.f) {
	Entity ent;
	ent.flags |= mask<u64>(Entity::Player);
	add_sprite(ent, sprite, mesh);
	add_dynbody(ent, body);
	add_collider(ent, collider);
	ent.name = "Player";
	ent.controls.accel = accel;
	ent.controls.speed = speed;
	ent.controls.walk_cycke_duration = 2.f;
	return ent;
}

Array<Entity*> gather(u64 flags, Array<Entity> entities, Array<Entity*> buffer) {
	auto list = List{ buffer, 0 };
	for (auto& ent : entities) if (has_all(ent.flags, flags))
		list.push(&ent);
	return list.allocated();
}

template<typename T> Array<T*> gather_member(
	u64 flags,
	Array<Entity> entities,
	T Entity::* member,
	Array<T> buffer
) {
	auto list = List{ buffer, 0 };
	for (auto& ent : entities) if (has_all(ent.flags, flags))
		list.push(ent.*member);
	return list.allocated();
}

void draw_entities(Array<Entity> entities, const RenderMesh& rect, MappedObject<m4x4f32> view_projection, const TexBuffer atlas, SpriteRenderer draw) {
	auto batch = draw.start_batch();
	for (auto&& ent : entities) if (has_all(ent.flags, mask<u64>(Entity::Sprite)))
		batch.push(sprite_data(trs_2d(ent.transform), ent.sprite.uv_rect, ent.sprite.atlas_index, ent.draw_layer));
	draw(rect, atlas, view_projection, batch.current);
}

void update_bodies(Array<Entity> entities) {
	for (auto& ent : entities) if (has_all(ent.flags, mask<u64>(Entity::Dynbody))) {
		ent.body.transform = ent.transform;
	}
}

void interpolate_bodies(Array<Entity> entities, f32 real_time, f32 physics_time, f32 dt) {
	for (auto& ent : entities) if (has_all(ent.flags, mask<u64>(Entity::Dynbody)))
		ent.transform = lerp(ent.transform, ent.body.transform, inv_lerp(real_time - dt, physics_time, real_time));
}

m4x4f32 collider_transform(const Entity& ent) {
	return trs_2d(has_all(ent.flags, mask<u64>(Entity::Dynbody)) ? ent.body.transform : ent.transform);
}

tuple<bool, Collision> test_entity_collision(Entity& e1, Entity& e2) {
	auto t1 = collider_transform(e1);
	auto t2 = collider_transform(e2);
	auto [collided, aabbi, penetration] = intersect(e1.collider->shape, e2.collider->shape, t1, t2);
	if (!collided)
		return { false, {} };
	auto b1 = has_all(e1.flags, mask<u64>(Entity::Dynbody));
	auto b2 = has_all(e2.flags, mask<u64>(Entity::Dynbody));
	auto im1 = (b1 && e1.body.mass > 0) ? 1 / e1.body.mass : 0;
	auto im2 = (b2 && e2.body.mass > 0) ? 1 / e2.body.mass : 0;
	auto v1 = b1 ? e1.body.derivatives.translation : v2f32(0);
	auto v2 = b2 ? e2.body.derivatives.translation : v2f32(0);
	Collision collision;
	collision.penetration = penetration;
	collision.colliders[0] = e1.collider;
	collision.colliders[1] = e2.collider;
	collision.inverse_masses[0] = im1;
	collision.inverse_masses[1] = im2;
	collision.rvel = v1 - v2;
	return { true, collision };
}

Array<Collision> simulate_entities(Alloc allocator, Array<Entity> entities, f32 dt) {
	for (auto& ent : entities) if (has_all(ent.flags, mask<u64>(Entity::Dynbody)))
		ent.body.transform = euler_integrate(ent.body.transform, filter_locks(ent.body.derivatives, ent.body.locks), dt);

	auto collisions = List<Collision>{ {}, 0 };
	for (auto i : u64xrange{ 0, entities.size() }) if (has_all(entities[i].flags, mask<u64>(Entity::Solid))) {
		for (auto j : u64xrange{ i + 1, entities.size() }) if (has_all(entities[j].flags, mask<u64>(Entity::Solid))) {
			assert(j > i);
			auto [collided, out] = test_entity_collision(entities[i], entities[j]);
			if (collided) {
				auto& col = collisions.push_growing(allocator, std::move(out));
				if (should_resolve(col)) {
					//TODO angular momentum

					// Bounce
					auto [v1, v2] = bounce_impulse(col);
					entities[i].body.derivatives.translation += v1;
					entities[j].body.derivatives.translation += v2;

					// Correct penetration
					auto [o1, o2] = correction_offset(col);
					entities[i].body.transform.translation += o1;
					entities[j].body.transform.translation += o2;
				}
			}
		}
	}

	return collisions.allocated();
}

void update_audio_sources(Array<Entity> entities) {
	for (auto ent : entities) if (has_all(ent.flags, mask<u64>(Entity::Sound))) {
		ent.audio_source.set<POSITION>(v3f32(ent.transform.translation, 0));
		if (has_all(ent.flags, mask<u64>(Entity::Dynbody)))
			ent.audio_source.set<VELOCITY>(v3f32(ent.body.derivatives.translation, 0));
	}
}

void update_audio_listener(Entity& listener) {
	ALListener::set<POSITION>(v3f32(listener.transform.translation, 0));
	if (has_all(listener.flags, mask<u64>(Entity::Dynbody)))
		ALListener::set<VELOCITY>(v3f32(listener.body.derivatives.translation, 0));
}

bool EditorWindow(const cstr label, List<Entity>& entities) {
	ImGui::Begin(label); defer{ ImGui::End(); };
	bool changed = false;
	ImGui::Text("Capacity : %u/%u", entities.current, entities.capacity.size());
	changed |= EditorWidget("Allocated", entities.allocated());
	if (entities.current < entities.capacity.size() && ImGui::Button("Allocate")) {
		entities.push({ 0 });
		return true;
	}
	return changed;
}
#endif
