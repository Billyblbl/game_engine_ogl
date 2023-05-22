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
	RigidBody2D body;
	ShapedCollider2D collider;
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
		if (has_one(entity.flags, mask<u64>(Entity::Dynbody)))
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

auto& add_dynbody(Entity& ent, const RigidBody2D& body) {
	ent.flags |= mask<u64>(Entity::Dynbody);
	ent.body = body;
	ent.body.transform = ent.transform;
	return ent;
}

auto& add_collider(Entity& ent, const ShapedCollider2D& collider) {
	ent.flags |= mask<u64>(Entity::Solid);
	ent.collider = collider;
	return ent;
}

auto& add_sound(Entity& ent, AudioSource source) {
	ent.flags |= mask<u64>(Entity::Sound);
	ent.audio_source = source;
	return ent;
}

auto& add_controls(Entity& ent, f32 speed, f32 acceleration, f32 walk_cycle) {
	ent.flags |= mask<u64>(Entity::Player);
	ent.controls.speed = speed;
	ent.controls.accel = acceleration;
	ent.controls.walk_cycle_duration = walk_cycle;
	return ent;
}

Array<Entity*> gather(u64 flags, Array<Entity> entities, Array<Entity*> buffer) {
	auto list = List{ buffer, 0 };
	for (auto& ent : entities) if (has_all(ent.flags, flags))
		list.push(&ent);
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

using Collision = tuple<u32, u32, rtf32, v2f32, v2f32, v2f32>;

void apply_global_force(Array<Entity> entities, f32 dt, v2f32 force) {
	for (auto& ent : entities) if (has_all(ent.flags, mask<u64>(Entity::Dynbody)))
		ent.body.velocity.translation += force * dt;;
}

Array<Collision> simulate_collisions(Array<Entity> entities) {
	auto has_body = [](const Entity& ent) { return has_all(ent.flags, mask<u64>(Entity::Dynbody)); };

	//! DEBUG
	static Collision collision_buffer[MAX_ENTITIES * MAX_ENTITIES];
	auto collisions = List{ larray(collision_buffer), 0 };
	//!

	for (auto i : u64xrange{ 0, entities.size() }) if (has_all(entities[i].flags, mask<u64>(Entity::Solid))) {
		for (auto j : u64xrange{ i + 1, entities.size() }) if (has_all(entities[j].flags, mask<u64>(Entity::Solid))) {
			auto& e1 = entities[i];
			auto& e2 = entities[j];
			auto t1 = trs_2d(has_body(e1) ? e1.body.transform : e1.transform);
			auto t2 = trs_2d(has_body(e2) ? e2.body.transform : e2.transform);
			auto [collided, aabbi, pen] = intersect(e1.collider.shape, e2.collider.shape, t1, t2);
			if (!collided)
				continue;

			auto desc1 = describe_body(has_body(e1) ? &e1.body : null, t1);
			auto desc2 = describe_body(has_body(e2) ? &e2.body : null, t2);

			if (!e1.collider.sensor && !e2.collider.sensor &&
				glm::length2(pen) > penetration_threshold * penetration_threshold &&
				can_resolve_collision(desc1, desc2)) { // resolve

				// Correct penetration
				auto [o1, o2] = correction_offset(pen, desc1.inverse_mass, desc2.inverse_mass);
				e1.body.transform.translation += o1;
				e2.body.transform.translation += o2;

				//! TODO physical rotation assumes center of mass = local origin, which requires to not touch the body's center of mass
				//TODO friction
				//TODO damping
				// Compute contact points with new positions
				auto normal = glm::normalize(pen);
				auto [ctct1, ctct2] = contacts_points(
					e1.collider.shape, e2.collider.shape,
					trs_2d(has_body(e1) ? e1.body.transform : e1.transform),
					trs_2d(has_body(e2) ? e2.body.transform : e2.transform),
					normal
				);
				v2f32 contacts[2] = { ctct1, ctct2 };

				// Bounce
				auto [delta1, delta2] = bounce_contacts(desc1, desc2, larray(contacts), normal, min(e1.collider.restitution, e2.collider.restitution));
				e1.body.velocity = e1.body.velocity + delta1;
				e2.body.velocity = e2.body.velocity + delta2;

				collisions.push({ i, j, aabbi, pen, ctct1, ctct2 });
			} else
				collisions.push({ i, j, aabbi, pen, v2f32(0), v2f32(0) });
		}
	}
	return collisions.allocated();
}

void update_audio_sources(Array<Entity> entities) {
	for (auto ent : entities) if (has_all(ent.flags, mask<u64>(Entity::Sound))) {
		ent.audio_source.set<POSITION>(v3f32(ent.transform.translation, 0));
		if (has_all(ent.flags, mask<u64>(Entity::Dynbody)))
			ent.audio_source.set<VELOCITY>(v3f32(ent.body.velocity.translation, 0));
	}
}

void update_audio_listener(const Entity& listener) {
	ALListener::set<POSITION>(v3f32(listener.transform.translation, 0));
	if (has_all(listener.flags, mask<u64>(Entity::Dynbody)))
		ALListener::set<VELOCITY>(v3f32(listener.body.velocity.translation, 0));
}

bool EditorWindow(const cstr label, List<Entity>& entities) {
	ImGui::Begin(label); defer{ ImGui::End(); };
	bool changed = false;
	ImGui::Text("Capacity : %u", entities.capacity.size());
	int count = entities.current;
	if (ImGui::InputInt("Current", &count)) {
		if (entities.current < count) {
			for (auto i : u64xrange{ 0, count - entities.current }) {
				if (entities.capacity.size() <= entities.current)
					break;
				entities.push({ 0, 0 });
			}
		} else if (count >= 0) entities.current = count;
		changed = true;
	}
	changed |= EditorWidget("Allocated", entities.allocated());
	return changed;
}
#endif
