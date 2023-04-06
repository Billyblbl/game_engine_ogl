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

#define MAX_ENTITIES 100
#define MAX_DRAW_BATCH MAX_ENTITIES

struct Entity {
	enum FlagIndex: u64 { Dynbody, Collision, Sprite, Player };
	u64 flags = 0;
	Transform2D transform;
	b2Body* body;
	//TODO add shape for physics
	RenderMesh* mesh;
	SpriteCursor sprite;
	f32 speed;
	f32 accel;
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
		if (has_one(entity.flags, mask<u64>(Entity::Sprite, Entity::Collision)))
			ImGui::Text("TODO : Widget for mesh reference");
		if (has_one(entity.flags, mask<u64>(Entity::Sprite)))
			changed |= EditorWidget("sprite", entity.sprite);
		if (has_one(entity.flags, mask<u64>(Entity::Player))) {
			changed |= EditorWidget("speed", entity.speed);
			changed |= EditorWidget("accel", entity.accel);
		}
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

auto create_player(SpriteCursor sprite, RenderMesh& mesh, b2World& world, f32 speed = 10.f, f32 accel = 100.f) {
	Entity ent = { mask<u64>(Entity::Player) };
	add_sprite(ent, sprite, mesh);
	add_dynbody(ent, world);
	ent.name = "Player";
	ent.speed = speed;
	ent.accel = accel;
	return ent;
}

Array<Entity*> gather(u64 flags, Array<Entity> entities, Array<Entity*> buffer) {
	auto list = List{ buffer, 0 };
	for (auto&& ent : entities) if (has_all(ent.flags, flags)) {
		list.push(&ent);
	}
	return list.allocated();
}

#endif
