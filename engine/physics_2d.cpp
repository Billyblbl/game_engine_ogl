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
	enum : u32 {
		FREE = 0,
		TX = 1 << 0,
		TY = 1 << 1,
		RT = 1 << 2,
		SX = 1 << 3,
		SY = 1 << 4,
	} locks = FREE;
};

static const cstrp shape_type_names_array[] = { "None", "Circle", "Polygon", "Line", "Point" };
static const auto shape_type_names = larray(shape_type_names_array);

struct Shape2D {
	enum : u32 { None, Circle, Polygon, Line, Point, TypeCount } type = None;
	union {
		struct { v2f32 center; f32 radius; } circle;
		Array<v2f32> polygon;
		//TODO use something else than a god damn _rectangle_ to represent a _line_
		reg_polytope<v2f32> line;
		v2f32 point;
	};
};

struct Collider2D {
	Shape2D shape;
	f32 restitution = 1.f;
	f32 friction = 1.f;
	bool sensor = false;
};

struct Collision {
	Collider2D* colliders[2];
	f32 inverse_masses[2];
	v2f32 rvel;
	// v2f32 normal;
	// f32 depth;
	v2f32 penetration;
	f32 elasticity;
};

v2f32 support_circle(f32 radius, v2f32 direction) {
	return glm::normalize(direction) * radius;
}

v2f32 support_point_cloud(Array<v2f32> vertices, v2f32 direction) {
	f32 support_dot = std::numeric_limits<f32>::lowest();
	v2f32 support;

	for (auto v : vertices) {
		auto dot = glm::dot(v, glm::normalize(direction));
		if (dot > support_dot) {
			support_dot = dot;
			support = v;
		}
	}
	return support;
}

v2f32 support_polygon(Array<v2f32> vertices, v2f32 direction) {
	return support_point_cloud(vertices, direction);
}

v2f32 support_line(rtf32 line, v2f32 direction) {
	return support_point_cloud(cast<v2f32>(carray(&line, 1)), direction);
}

v2f32 support(const Shape2D& shape, v2f32 direction) {
	switch (shape.type) {
	case Shape2D::Circle: return support_circle(shape.circle.radius, direction);
	case Shape2D::Polygon: return support_polygon(shape.polygon, direction);
	case Shape2D::Line: return support_line(shape.line, direction);
	case Shape2D::Point: return shape.point;
	default: return fail_ret("Unimplemented support function", v2f32(0));
	}
	return {};
}

inline rtf32 aabb_circle(v2f32 center, f32 radius, m4x4f32 transform) {
	auto scaling_factors = v2f32(
		glm::length(v3f32(transform[0])),
		glm::length(v3f32(transform[1]))
	);
	auto center_transformed = v2f32(transform * v4f32(center, 0, 1));
	auto radius_transformed = radius * max(scaling_factors.x, scaling_factors.y);

	return {
		center_transformed - v2f32(radius_transformed),
		center_transformed + v2f32(radius_transformed)
	};
}

inline rtf32 aabb_polygon(Array<v2f32> vertices, m4x4f32 transform) {
	auto mx = v2f32(std::numeric_limits<f32>::lowest());
	auto mn = v2f32(std::numeric_limits<f32>::max());

	for (auto&& v : vertices) {
		auto tfmd = v2f32(transform * v4f32(v, 0, 1));
		mx = glm::max(mx, tfmd);
		mn = glm::min(mn, tfmd);
	}
	return { mn, mx };
}

inline rtf32 aabb_line(rtf32 line, m4x4f32 transform) {
	auto transformed = rtf32{ v2f32(transform * v4f32(line.min, 0, 1)), v2f32(transform * v4f32(line.max, 0, 1)) };
	return { glm::min(transformed.min, transformed.max), glm::max(transformed.min, transformed.max) };
}

inline rtf32 aabb_point(v2f32 point, m4x4f32 transform) {
	auto transformed = v2f32(transform * v4f32(point, 0, 1));
	return { transformed, transformed };
}

inline rtf32 aabb_shape(const Shape2D& shape, m4x4f32 transform) {
	switch (shape.type) {
	case Shape2D::Circle: return aabb_circle(shape.circle.center, shape.circle.radius, transform);
	case Shape2D::Polygon: return aabb_polygon(shape.polygon, transform);
	case Shape2D::Line: return aabb_line(shape.line, transform);
	case Shape2D::Point: return aabb_point(shape.point, transform);
	default: return { v2f32(1), v2f32(-1) };
	}
}

inline rtf32 intersect(rtf32 a, rtf32 b) {
	return { glm::max(a.min, b.min), glm::min(a.max, b.max) };
}

using Triangle = tuple<v2f32, v2f32, v2f32>;

v2f32 minkowski_diff_support(const Shape2D& s1, const Shape2D& s2, m4x4f32 m1, m4x4f32 m2, v2f32 direction) {
	return v2f32(
		m1 * v4f32(support(s1, glm::transpose(m1) * v4f32(direction, 0, 1)), 0, 1) -
		m2 * v4f32(support(s2, glm::transpose(m2) * v4f32(-direction, 0, 1)), 0, 1)
	);
}

// https://www.youtube.com/watch?v=ajv46BSqcK4&ab_channel=Reducible
// Gilbert-Johnson-Keerthi
// TODO continuous collision -> support function only selects point, should sample from the convex hull of the shapes at time t & t+dt
// TODO contact point -> depend on penetration vector
tuple<bool, Triangle> GJK(const Shape2D& s1, const Shape2D& s2, m4x4f32 m1, m4x4f32 m2) {
	constexpr tuple<bool, Triangle> no_collision = { false, {{}, {}, {}} };

	v2f32 triangle_vertices[3];
	auto triangle = List{ larray(triangle_vertices), 0 };
	auto direction = glm::normalize(v2f32(m2 * v4f32(v3f32(0), 1)) - v2f32(m1 * v4f32(v3f32(0), 1)));

	for (auto i = 0; i < 999;i++) {// inf loop safeguard
		auto new_point = minkowski_diff_support(s1, s2, m1, m2, direction);
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
			if (glm::dot(AO, ABperp_axis) == 0) {// origin is on AB
				triangle.push(v2f32(0));
				return { true, to_tuple<3>(triangle.allocated()) };
			}
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
				return { true, to_tuple<3>(triangle.allocated()) };
			if (glm::dot(AO, ACperp_axis) == 0) // origin is on AC
				return { true, to_tuple<3>(triangle.allocated()) };
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
				return { true, to_tuple<3>(triangle.allocated()) };
			}
		}
	}

	return fail_ret("GJK iteration out of bounds", no_collision);
}

// Expanding Polytope algorithm
// https://www.youtube.com/watch?v=0XQ2FSz3EK8&ab_channel=Winterdev
v2f32 EPA(const Shape2D& s1, const Shape2D& s2, m4x4f32 m1, m4x4f32 m2, Triangle t, u32 max_iteration = 64, f32 iteration_threshold = 0.1f) {
	auto [A, B, C] = t;
	v2f32 points_buffer[max_iteration + 3] = { A, B, C };
	auto points = List{ carray(points_buffer, max_iteration + 3), 3 };

	auto min_index = 0;
	auto min_dist = std::numeric_limits<f32>::max();
	auto min_normal = v2f32(0);

	for (auto iteration : u64xrange{ 0, max_iteration }) {
		for (auto i : u64xrange{ 0, points.current }) {
			auto j = (i + 1) % points.current;
			auto vi = points.allocated()[i];
			auto vj = points.allocated()[j];
			auto ij = vj - vi;
			auto normal = glm::normalize(v2f32(ij.y, -ij.x));
			// auto normal = glm::normalize(v2f32(-ij.y, ij.x));
			auto dist = glm::dot(normal, vj);

			if (dist < 0) {
				dist *= -1;
				normal *= -1;
			}

			if (dist < min_dist) {
				min_dist = dist;
				min_normal = normal;
				min_index = j;
			}
		}

		auto support_point = minkowski_diff_support(s1, s2, m1, m2, min_normal);
		auto s_distance = glm::dot(min_normal, support_point);

		if (glm::abs(s_distance - min_dist) > iteration_threshold) {
			min_dist = std::numeric_limits<f32>::max();
			points.insert_ordered(min_index, std::move(support_point));
		} else {
			break;
		}

	}
	return min_normal * (min_dist + iteration_threshold);
}

tuple<bool, rtf32, v2f32> intersect(const Shape2D& s1, const Shape2D& s2, m4x4f32 t1, m4x4f32 t2) {
	auto aabb_intersection = intersect(aabb_shape(s1, t1), aabb_shape(s2, t2));
	if (width(aabb_intersection) < 0 || height(aabb_intersection) < 0)
		return {false, aabb_intersection, v2f32(0)};
	auto [collided, triangle] = GJK(s1, s2, t1, t2);
	if (!collided)
		return {false, aabb_intersection, v2f32(0)};
	auto penetration = EPA(s1, s2, t1, t2, triangle);
	return {true, aabb_intersection, penetration};
}

inline bool should_resolve(const Collision& col, f32 min_penetration = 0.1f) {
	for (auto collider : col.colliders) if (collider->sensor) return false;
	return glm::length(col.penetration) > min_penetration && col.inverse_masses[0] + col.inverse_masses[1] > 0;
}

inline tuple<v2f32, v2f32> bounce_impulse(const Collision& col) {
	auto impulse = -glm::normalize(col.penetration) *
		(1 + col.elasticity) *
		glm::dot(-col.rvel, glm::normalize(col.penetration)) /
		(col.inverse_masses[0] + col.inverse_masses[1]);
	return tuple(-impulse * col.inverse_masses[0], impulse * col.inverse_masses[1]);
}

inline tuple<v2f32, v2f32> correction_offset(const Collision& col) {
	auto total_offset = col.penetration / (col.inverse_masses[0] + col.inverse_masses[1]) / 2.f;
	return { // those indices are normal -> O(x) = O*inv(y)/(inv(x)+inv(y))
		-total_offset * col.inverse_masses[1] / 2.f,
		 total_offset * col.inverse_masses[0] / 2.f,
	};
}

#ifndef DEFAULT_GRAVITY
#define DEFAULT_GRAVITY v2f32(0, -9.81f)
#endif

constexpr auto GRAVITY = DEFAULT_GRAVITY;

inline void apply_force(Array<Body2D> bodies, f32 dt, v2f32 force) {
	for (auto&& body : bodies)
		body.derivatives.translation += force * dt;
}

inline Transform2D euler_integrate(const Transform2D& transform, const Transform2D& derivatives, f32 dt) {
	return {
		transform.translation + derivatives.translation * dt,
		transform.scale + derivatives.scale * dt,
		transform.rotation + derivatives.rotation * dt
	};
}

inline Transform2D filter_locks(const Transform2D& transform, u32 locks) {
	Transform2D filtered = { v2f32(0), v2f32(0), 0 };
	filtered.translation.x = (locks & Body2D::TX) ? 0 : transform.translation.x;
	filtered.translation.y = (locks & Body2D::TY) ? 0 : transform.translation.y;
	filtered.rotation = (locks & Body2D::RT) ? 0 : transform.rotation;
	filtered.scale.x = (locks & Body2D::SX) ? 0 : transform.scale.x;
	filtered.scale.y = (locks & Body2D::SY) ? 0 : transform.scale.y;
	return filtered;
}

bool EditorWidget(const cstr label, Shape2D& shape) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= ImGui::Combo("Type", (i32*)&shape.type, shape_type_names.data(), shape_type_names.size());
		switch (shape.type) {
		case Shape2D::Circle: changed |= EditorWidget("Radius", shape.circle.radius);break;
		case Shape2D::Polygon: changed |= EditorWidget("Polygon", shape.polygon);break;
		case Shape2D::Line: changed |= EditorWidget("Line", shape.line);break;
		case Shape2D::Point: changed |= EditorWidget("Point", shape.point);break;
		}
	}
	return changed;
}

bool EditorWidget(const cstr label, Collider2D& collider) {
	bool changed = false;

	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("Shape", collider.shape);
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
		changed |= ImGui::bit_flags("Locks", body.locks, {
			"TX", "TY",
			"R",
			"SX", "SY",
			});
		// changed |= EditorWidget("Shape", body.shape);
	}
	return changed;
}

bool EditorWidget(const cstr label, Collision& col) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("Colliders", larray(col.colliders));
		changed |= EditorWidget("Inverse masses", larray(col.inverse_masses));
		changed |= EditorWidget("Penetration", col.penetration);
		changed |= EditorWidget("Elasticity", col.elasticity);
		changed |= EditorWidget("Relative velocity", col.rvel);
		auto sr = should_resolve(col);
		changed |= EditorWidget("Should resolve", sr);
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
