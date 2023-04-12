#ifndef GENTITY
# define GENTITY

#include <blblstd.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <transform.cpp>
#include <rendering.cpp>
#include <physics2d.cpp>
#include <sprite.cpp>
#include <top_down_controls.cpp>

#define MAX_ENTITIES 1000
#define MAX_DRAW_BATCH MAX_ENTITIES

struct Entity {
	enum FlagIndex: u64 { Dynbody, Collision, Sprite, Player };
	u64 flags = 0;
	Transform2D transform;
	b2Body* body;
	b2Fixture* collider;
	RenderMesh* mesh;
	SpriteCursor sprite;
	f32 draw_layer;
	controls::TopDownControl controls;
	string name = "__entity__";
};

bool EditorWidget(const cstr label, Entity& entity) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= ImGui::bit_flags("flags", entity.flags, { "Dynbody", "Collision", "Sprite", "Player" });
		if (has_one(entity.flags, mask<u64>(Entity::Dynbody, Entity::Collision, Entity::Sprite, Entity::Player)))
			changed |= EditorWidget("transform", entity.transform);
		if (has_one(entity.flags, mask<u64>(Entity::Dynbody, Entity::Collision)))
			changed |= EditorWidget("body", entity.body);
		if (has_one(entity.flags, mask<u64>(Entity::Collision)))
			changed |= EditorWidget("collider", entity.collider);
		if (has_one(entity.flags, mask<u64>(Entity::Sprite, Entity::Collision)))
			ImGui::Text("TODO : Widget for mesh reference");
		if (has_one(entity.flags, mask<u64>(Entity::Sprite))) {
			changed |= EditorWidget("sprite", entity.sprite);
			changed |= EditorWidget("draw layer", entity.draw_layer);
		}
		if (has_one(entity.flags, mask<u64>(Entity::Player)))
			changed |= EditorWidget("controls", entity.controls);
		ImGui::TreePop();
	}
	return changed;
}

auto& add_sprite(Entity& ent, SpriteCursor sprite, RenderMesh& mesh) {
	ent.flags |= mask<u64>(Entity::Sprite);
	ent.sprite = sprite;
	ent.mesh = &mesh;
	return ent;
}

auto& add_dynbody(Entity& ent, b2World& world) {
	ent.flags |= mask<u64>(Entity::Dynbody);
	b2BodyDef def;
	def.position = glm_to_b2d(ent.transform.translation);
	def.type = b2_dynamicBody;
	ent.body = world.CreateBody(&def);
	return ent;
}

auto& add_collider(Entity& ent, b2Shape& shape) {
	if (ent.body == null)
		return fail_ret("Can't add collider without body", ent);
	ent.flags |= mask<u64>(Entity::Collision);
	b2FixtureDef def;
	def.shape = &shape;
	ent.collider = ent.body->CreateFixture(&def);
	return ent;
}

auto create_player(SpriteCursor sprite, RenderMesh& mesh, b2World& world, b2Shape& shape, f32 speed = 10.f, f32 accel = 100.f) {
	Entity ent = { mask<u64>(Entity::Player) };
	add_sprite(ent, sprite, mesh);
	add_dynbody(ent, world);
	add_collider(ent, shape);
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

void draw_entities(Array<Entity> entities, const RenderMesh& rect, MappedObject<m4x4f32> view_projection, const TexBuffer atlas, SpriteRenderer draw) {
	auto batch = draw.start_batch();
	for (auto&& ent : entities) if (has_all(ent.flags, mask<u64>(Entity::Sprite)))
		batch.push(sprite_data(trs_2d(ent.transform), ent.sprite.uv_rect, ent.sprite.atlas_index, ent.draw_layer));
	draw(rect, atlas, view_projection, batch.current);
}

void simulate_entities(Array<Entity> entities, b2World& world, const PhysicsConfig& config) {
	Entity* pbuff[entities.size()];
	auto to_sim = gather(mask<u64>(Entity::Dynbody), entities, carray(pbuff, entities.size()));
	for (auto ent : to_sim)
		override_body(ent->body, ent->transform.translation, ent->transform.rotation);
	update_sim(world, config);
	for (auto ent : to_sim)
		override_transform(ent->body, ent->transform.translation, ent->transform.rotation);
}

bool EditorWindow(const cstr label, List<Entity>& entities) {
	ImGui::Begin(label); defer {ImGui::End();};
	bool changed = false;
	ImGui::Text("Capacity : %u/%u", entities.current, entities.capacity.size());
	changed |= EditorWidget("Allocated", entities.allocated());
	if (entities.current < entities.capacity.size() && ImGui::Button("Allocate")) {
		entities.push({});
		return true;
	}
	return changed;
}
#endif
