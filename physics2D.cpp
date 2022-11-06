#ifndef GPHYSICS2D
# define GPHYSICS2D

#include <box2d/box2d.h>
#include <imgui_extension.cpp>
#include <b2d_debug_draw.cpp>

b2Vec2 glmToB2d(glm::f32vec2 vec) { return { vec.x, vec.y }; }
glm::f32vec2 b2dToGlm(b2Vec2 vec) { return { vec.x, vec.y }; }

bool PhysicsControls(
	b2World& world,
	float& physicsTimeStep,
	int& velocityIterations,
	int& positionIterations,
	float& physicsTimePoint
) {
	auto changed = false;

	auto gravity = b2dToGlm(world.GetGravity());
	changed |= EditorWidget("Gravity", gravity);
	if (changed)
		world.SetGravity(glmToB2d(gravity));

	changed |= EditorWidget("Physics time step", physicsTimeStep);
	changed |= EditorWidget("Velocity Iterations", velocityIterations);
	changed |= EditorWidget("Position Iterations", positionIterations);

	ImGui::Text("Physics time point = %f", physicsTimePoint);
	return changed;
}

bool EditorWidget(const char* label, b2Body* body) {
	bool bodyChanged = false;
	if (ImGui::TreeNode(label)) {
		{
			b2MassData massData;
			body->GetMassData(&massData);
			auto changed = EditorWidget("Mass", massData.mass);
			if (changed)
				body->SetMassData(&massData);
			bodyChanged |= changed;
		}

		{
			auto linearVel = b2dToGlm(body->GetLinearVelocity());
			auto changed = EditorWidget("Linear Velocity", linearVel);
			if (changed)
				body->SetLinearVelocity(glmToB2d(linearVel));
			bodyChanged |= changed;
		}

		{
			auto linearDamping = body->GetLinearDamping();
			auto changed = EditorWidget("Linear Damping", linearDamping);
			if (changed)
				body->SetLinearDamping(linearDamping);
			bodyChanged |= changed;
		}

		{
			auto angularVel = body->GetAngularVelocity();
			auto changed = EditorWidget("Angular Velocity", angularVel);
			if (changed)
				body->SetAngularVelocity(angularVel);
			bodyChanged |= changed;
		}

		{
			auto angularDamping = body->GetAngularDamping();
			auto changed = EditorWidget("Angular Damping", angularDamping);
			if (changed)
				body->SetAngularDamping(angularDamping);
			bodyChanged |= changed;
		}
		ImGui::TreePop();
	}

	return bodyChanged;
}

#endif
