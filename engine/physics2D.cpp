#ifndef GPHYSICS2D
# define GPHYSICS2D

#include <box2d/box2d.h>
#include <imgui_extension.cpp>
#include <b2d_debug_draw.cpp>
#include <transform.cpp>

b2Vec2 glm_to_b2d(v2f32 vec) { return { vec.x, vec.y }; }
v2f32 b2d_to_glm(b2Vec2 vec) { return { vec.x, vec.y }; }

void override_body(b2Body& body, v2f32 position, f32 rotation) {
	auto pos = b2Vec2(position.x, position.y);
	body.SetTransform(pos, -glm::radians(rotation));
}

void override_transform(const b2Body& body, v2f32& position, f32& rotation) {
	auto pos = body.GetPosition();
	position = glm::vec2(pos.x, pos.y);
	rotation = -glm::degrees(body.GetAngle());
}

struct PhysicsConfig {
	f32 time_step = 1.f / 60.f;
	u8 velocity_iterations = 8;
	u8 position_iterations = 3;
};

void update_physics(b2World& world, PhysicsConfig config, Array<tuple<Transform2D*, b2Body*>> entities) {
	for (auto [transform, body] : entities) {
		override_body(*body, transform->translation, transform->rotation);
		//TODO awake only when needed
		body->SetAwake(true);
	}

	world.Step(config.time_step, config.velocity_iterations, config.position_iterations);

	for (auto [transform, body] : entities)
		override_transform(*body, transform->translation, transform->rotation);
}

bool EditorWidget(const cstr label, PhysicsConfig& config) {
	auto changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("Physics time step", config.time_step);
		changed |= EditorWidget("Velocity iterations", config.velocity_iterations);
		changed |= EditorWidget("Position iterations", config.position_iterations);
		ImGui::TreePop();
	}
	return changed;
}

bool physics_controls(
	b2World& world,
	PhysicsConfig& config,
	f32 time_point
) {
	auto changed = false;
	if (ImGui::TreeNode("Physics")) {
		auto gravity = b2d_to_glm(world.GetGravity());
		if (changed |= EditorWidget("Gravity", gravity))
			world.SetGravity(glm_to_b2d(gravity));
		changed |= EditorWidget("Config", config);
		ImGui::Text("time point = %f", time_point);
		ImGui::TreePop();
	}
	return changed;
}

bool EditorWidget(const cstr label, b2Body* body) {
	if (body == null) return false;
	bool body_changed = false;
	if (ImGui::TreeNode(label)) {
		{
			b2MassData mass_data;
			body->GetMassData(&mass_data);
			auto changed = EditorWidget("Mass", mass_data.mass);
			if (changed)
				body->SetMassData(&mass_data);
			body_changed |= changed;
		}

		{
			auto linear_vel = b2d_to_glm(body->GetLinearVelocity());
			auto changed = EditorWidget("Linear Velocity", linear_vel);
			if (changed)
				body->SetLinearVelocity(glm_to_b2d(linear_vel));
			body_changed |= changed;
		}

		{
			auto linear_damping = body->GetLinearDamping();
			auto changed = EditorWidget("Linear Damping", linear_damping);
			if (changed)
				body->SetLinearDamping(linear_damping);
			body_changed |= changed;
		}

		{
			auto angular_vel = body->GetAngularVelocity();
			auto changed = EditorWidget("Angular Velocity", angular_vel);
			if (changed)
				body->SetAngularVelocity(angular_vel);
			body_changed |= changed;
		}

		{
			auto angular_damping = body->GetAngularDamping();
			auto changed = EditorWidget("Angular Damping", angular_damping);
			if (changed)
				body->SetAngularDamping(angular_damping);
			body_changed |= changed;
		}
		ImGui::TreePop();
	}

	return body_changed;
}

#endif