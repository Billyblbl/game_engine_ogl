#ifndef GPHYSICS_2D
# define GPHYSICS_2D

#include <blblstd.hpp>
#include <math.cpp>
#include <imgui_extension.cpp>
#include <stdio.h>
#include <transform.cpp>

struct Body2D {
	Transform2D transform = { v2f32(0), v2f32(1), 0 };
	Transform2D derivatives = { v2f32(0), v2f32(0), 0 };
	f32 mass = 1.f;
};

struct Collider2D {
	enum : u32 { None, Circle, Polygon, Line, Point, TypeCount } type = None;
	union {
		struct { f32 radius; } circle;
		Array<v2f32> polygon;
		reg_polytope<v2f32> line;
	};
	f32 restitution = 1.f;
	f32 friction = 1.f;
	bool sensor = false;
};

struct Collision {
	Collider2D* colliders[2];
	f32 inverse_masses[2];
	v2f32 rvel;
	v2f32 normal;
	f32 depth;
	f32 elasticity;
};

v2f32 support_circle(f32 radius, v2f32 center, v2f32 direction) {
	return center + glm::normalize(direction) * radius;
}

v2f32 support_polygon(Array<v2f32> vertices, m4x4f32 transform, v2f32 direction) {
	f32 support_dot = std::numeric_limits<f32>::lowest();
	v2f32 support;

	for (auto v : vertices) {
		auto positioned_vertex = v2f32(transform * v4f32(v, 0, 1));
		auto dot = glm::dot(positioned_vertex, glm::normalize(direction));
		if (dot > support_dot) {
			support_dot = dot;
			support = positioned_vertex;
		}
	}
	return support;
}

v2f32 support(const Collider2D& collider, m4x4f32 matrix, v2f32 direction) {
	switch (collider.type) {
	case Collider2D::Circle: return support_circle(collider.circle.radius, matrix * v4f32(0, 0, 0, 1), direction);
	case Collider2D::Polygon: return support_polygon(collider.polygon, matrix, direction);
	default: assert(false);
	}
	return {};
}

// https://www.youtube.com/watch?v=ajv46BSqcK4&ab_channel=Reducible
tuple<bool, v2f32, f32> GJK(Collider2D& c1, Collider2D& c2, m4x4f32 m1, m4x4f32 m2) {
	constexpr auto no_collision = tuple<bool, v2f32, f32>{ false, v2f32(0), 0.f };

	auto minkowski_difference_support = [&](v2f32 direction) -> v2f32 {
		return support(c1, m1, direction) - support(c2, m2, -direction);
	};

	v2f32 triangle_vertices[3];
	auto triangle = List{ larray(triangle_vertices), 0 };
	auto direction = glm::normalize(v2f32(m2 * v4f32(v3f32(0), 1)) - v2f32(m1 * v4f32(v3f32(0), 1)));

	for (auto i = 0; i < 999;i++) {// inf loop safeguard
		auto new_point = minkowski_difference_support(direction);
		if (glm::dot(new_point, direction) <= 0) // Did we pass the origin to find A, return early otherwise
			return no_collision;
		triangle.push(new_point);

		if (triangle.current == 1) { // first point case
			direction = -triangle.allocated()[0];
		} else if (triangle.current == 2) { // first line case
			auto [B, A] = to_tuple<2>(triangle.allocated());
			auto AB = B - A;
			auto AO = -A;// origin - A
			auto ABperp_axis = v2f32(-AB.y, AB.x);
			if (glm::dot(AO, ABperp_axis) == 0) // origin is on AB
				return {true, v2f32(0), 0}; //TODO normal & depth
			// direction should be perpendicular to AB & go towards the origin
			direction = glm::normalize(ABperp_axis * glm::dot(AO, ABperp_axis));
		} else { // triangle case
			auto [C, B, A] = to_tuple<3>(triangle.allocated());
			auto AB = B - A;
			auto AC = C - A;
			auto AO = -A;// origin - A
			auto ABperp_axis = v2f32(-AB.y, AB.x);
			auto ACperp_axis = v2f32(-AC.y, AC.x);
			if (glm::dot(AO, ABperp_axis) == 0) // origin is on AB
				return {true, v2f32(0), 0}; //TODO normal & depth
			if (glm::dot(AO, ACperp_axis) == 0) // origin is on AC
				return {true, v2f32(0), 0}; //TODO normal & depth
			auto ABperp = glm::normalize(ABperp_axis * glm::dot(ABperp_axis, -AC)); // must go towards the exterior of triangle
			auto ACperp = glm::normalize(ACperp_axis * glm::dot(ACperp_axis, -AB)); // must go towards the exterior of triangle
			if (glm::dot(ABperp, AO) > 0) {// region AB
				triangle.remove(0);//C
				direction = ABperp;
			} else if (glm::dot(ACperp, AO) > 0) { // region AC
				triangle.remove(1);//B
				direction = ACperp;
			} else { // Inside triangle ABC
				//! wrong
				// auto out_normal = v2f32(-glm::cross(v3f32(AB, 0), v3f32(AC, 0)));
				// auto out_depth = glm::dot(out_normal, A);
				return { true, v2f32(0), 0 };
				// return { true, out_normal, out_depth };
			}
		}
	}

	return fail_ret("GJK iteration out of bounds", no_collision);
}

bool should_resolve(const Collision& col) {
	for (auto collider : col.colliders) if (collider->sensor) return false;
	return true;
}

v2f32 impulse_resolution(const Collision& col) {
	return
		-col.normal *
		(1 + col.elasticity) *
		glm::dot(col.rvel, col.normal) /
		(col.inverse_masses[0] + col.inverse_masses[1]);
}

v2f32 correction_offset(const Collision& col) {
	return col.normal * col.depth / 2.f;
}

#ifndef DEFAULT_GRAVITY
#define DEFAULT_GRAVITY v2f32(0, -9.81f)
#endif

constexpr auto GRAVITY = DEFAULT_GRAVITY;

void apply_force(Array<Body2D> bodies, f32 dt, v2f32 force) {
	for (auto&& body : bodies)
		body.derivatives.translation += force * dt;
}

Transform2D euler_integrate(const Transform2D& transform, const Transform2D& derivatives, f32 dt) {
	return {
		transform.translation + derivatives.translation * dt,
		transform.scale + derivatives.scale * dt,
		transform.rotation + derivatives.rotation * dt
	};
}

bool EditorWidget(const cstr label, Collider2D& collider) {
	bool changed = false;

	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };

		const cstrp types[] = { "None", "Circle", "Polygon", "Line", "Point" };
		changed |= ImGui::Combo("Type", (i32*)&collider.type, types, array_size(types));

		switch (collider.type) {
		case Collider2D::Circle:	changed |= EditorWidget("Radius", collider.circle.radius);	break;
		case Collider2D::Polygon:	changed |= EditorWidget("Polygon", collider.polygon);				break;
		case Collider2D::Line:		changed |= EditorWidget("Line", collider.line);							break;
		}

		changed |= EditorWidget("Restitution", collider.restitution);
		changed |= EditorWidget("Friction", collider.friction);
		changed |= EditorWidget("Is Sensor", collider.sensor);
	}
	return changed;
}

bool EditorWidget(const cstr label, Body2D& body) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("Physics tick transform", body.transform);
		changed |= EditorWidget("Derivatives", body.derivatives);
		changed |= EditorWidget("Mass", body.mass);
		// changed |= EditorWidget("Shape", body.shape);
	}
	return changed;
}

bool EditorWidget(const cstr label, Collision& col) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("Colliders", larray(col.colliders));
		changed |= EditorWidget("Normal", col.normal);
		changed |= EditorWidget("Depth", col.depth);
	}
	return changed;
}

tuple<Array<v2f32>, usize> read_polygon(FILE* file, Alloc allocator) {
	usize read = 0;
	usize count = 0;
	read += fread(&count, sizeof(usize), 1, file);
	auto array = alloc_array<v2f32>(allocator, count);
	read += fread(array.data(), sizeof(v2f32), count, file);
	return tuple(array, read);
}

usize write_polygon(FILE* file, Array<v2f32> polygon) {
	usize wrote = 0;
	usize count = polygon.size();
	wrote += fwrite(&count, sizeof(usize), 1, file);
	wrote += fwrite(polygon.data(), sizeof(v2f32), count, file);
	return wrote;
}

struct PhysicsConfig {
	f32 time_step = 1.f / 60.f;
	u8 velocity_iterations = 8;
	u8 position_iterations = 3;
};

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


#endif
