#ifndef GPHYSICS2D
# define GPHYSICS2D

#include <box2d/box2d.h>
#include <imgui_extension.cpp>
#include <b2d_debug_draw.cpp>
#include <transform.cpp>

b2Vec2 glm_to_b2d(v2f32 vec) { return { vec.x, vec.y }; }
v2f32 b2d_to_glm(b2Vec2 vec) { return { vec.x, vec.y }; }

bool override_body(b2Body* body, v2f32 position, f32 rotation) {
	if (body == null)
		return fail_ret("Physics entity has no body", false);
	auto pos = b2Vec2(position.x, position.y);
	body->SetTransform(pos, glm::radians(rotation));
	//TODO awake only when needed
	body->SetAwake(true);
	return true;
}

bool override_transform(const b2Body* body, v2f32& position, f32& rotation) {
	if (body == null)
		return fail_ret("Physics entity has no body", false);
	position = b2d_to_glm(body->GetPosition());
	rotation = glm::degrees(body->GetAngle());
	return true;
}

struct PhysicsConfig {
	f32 time_step = 1.f / 60.f;
	u8 velocity_iterations = 8;
	u8 position_iterations = 3;
};

void update_sim(b2World& world, PhysicsConfig config) {
	world.Step(config.time_step, config.velocity_iterations, config.position_iterations);
}

bool EditorWidget(const cstr label, PhysicsConfig& config) {
	auto changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("Physics time step", config.time_step);
		changed |= EditorWidget("Velocity iterations", config.velocity_iterations);
		changed |= EditorWidget("Position iterations", config.position_iterations);
		ImGui::TreePop();
	}
	if (config.time_step <= 0.0000f)
		config.time_step = 0.0001f;
	return changed;
}

bool physics_controls(
	const cstr label,
	b2World& world,
	PhysicsConfig& config,
	f32 time_point,
	bool& draw_debug,
	bool& wireframe
) {
	ImGui::Begin(label); defer {ImGui::End();};
	auto changed = false;
	auto gravity = b2d_to_glm(world.GetGravity());
	if (changed |= EditorWidget("Gravity", gravity))
		world.SetGravity(glm_to_b2d(gravity));
	changed |= EditorWidget("Config", config);
	changed |= EditorWidget("Debug draw", draw_debug);
	changed |= EditorWidget("Wireframe", wireframe);
	ImGui::Text("time point = %f", time_point);
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

bool EditorWidget(const cstr label, b2PolygonShape* shape) {
	if (shape == null) return false;
	bool changed = false;

	ImGui::Text(label);
	changed |= EditorWidget("Vertices", cast<v2f32>(carray(shape->m_vertices, shape->m_count)));
	changed |= EditorWidget("Normals", cast<v2f32>(carray(shape->m_normals, shape->m_count)));
	ImGui::Text("Immutable count: would break some caching in b2d");
	return changed;
}

bool EditorWidget(const cstr label, b2Fixture* fixture) {
	if (fixture == null) return false;
	bool fixture_changed = false;
	if (ImGui::TreeNode(label)) {
		static const string types[] = { "Circle", "Edge", "Polygon", "Chain" };

		ImGui::Text("Type : %s", types[fixture->GetType()].data());
		if (fixture->GetType() == b2Shape::e_polygon) {
			fixture_changed |= EditorWidget("Shape", (b2PolygonShape*)fixture->GetShape());
		} else {
			ImGui::Text("TODO  : Shape manipulation");
		}

		{
			auto density = fixture->GetDensity();
			auto changed = EditorWidget("Density", density);
			if (changed)
				fixture->SetDensity(density);
			fixture_changed |= changed;
		}

		{
			auto friction = fixture->GetFriction();
			auto changed = EditorWidget("Friction", friction);
			if (changed)
				fixture->SetFriction(friction);
			fixture_changed |= changed;
		}

		{
			auto restitution = fixture->GetRestitution();
			auto changed = EditorWidget("Restitution", restitution);
			if (changed)
				fixture->SetRestitution(restitution);
			fixture_changed |= changed;
		}

		{
			auto restitution_threshold = fixture->GetRestitutionThreshold();
			auto changed = EditorWidget("Restitution Threshold", restitution_threshold);
			if (changed)
				fixture->SetRestitutionThreshold(restitution_threshold);
			fixture_changed |= changed;
		}

		{
			auto sensor = fixture->IsSensor();
			auto changed = EditorWidget("Sensor", sensor);
			if (changed)
				fixture->SetSensor(sensor);
			fixture_changed |= changed;
		}

		ImGui::TreePop();
	}
	return fixture_changed;
}

#endif
