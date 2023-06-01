#ifndef GPHYSICS_2D
# define GPHYSICS_2D

#include <blblstd.hpp>
#include <math.cpp>
#include <imgui_extension.cpp>
#include <stdio.h>
#include <transform.cpp>

static const cstrp shape_type_names_array[] = { "None", "Circle", "Polygon", "Line", "Point", "Composite" };
static const auto shape_type_names = larray(shape_type_names_array);

struct Shape2D {
	enum Type : u32 { None, Circle, Polygon, Line, Point, Composite, TypeCount } type = None;
	union {
		Array<v2f32> polygon = {};
		Array<Shape2D> composite;
		v3f32 circle;//xy center, z radius
		Segment<v2f32> line;
		v2f32 point;
	};
};

template<Shape2D::Type type> struct shape_mapping {};
template<> struct shape_mapping<Shape2D::Circle> { static constexpr auto member = &Shape2D::circle; };
template<> struct shape_mapping<Shape2D::Polygon> { static constexpr auto member = &Shape2D::polygon; };
template<> struct shape_mapping<Shape2D::Line> { static constexpr auto member = &Shape2D::line; };
template<> struct shape_mapping<Shape2D::Point> { static constexpr auto member = &Shape2D::point; };
template<> struct shape_mapping<Shape2D::Composite> { static constexpr auto member = &Shape2D::composite; };

template<Shape2D::Type type> Shape2D make_shape_2d(auto&& init) {
	Shape2D shape = {};
	shape.type = type;
	shape.*shape_mapping<type>::member = init;
	return shape;
}

Array<v2f32> make_box_poly(Array<v2f32> buffer, v2f32 dimensions, v2f32 center) {
	assert(buffer.size() >= 4);
	buffer[0] = center + v2f32(-dimensions.x, dimensions.y) / 2.f;
	buffer[1] = center - dimensions / 2.f;
	buffer[2] = center - v2f32(-dimensions.x, dimensions.y) / 2.f;
	buffer[3] = center + dimensions / 2.f;
	return buffer.subspan(0, 4);
}

Shape2D make_box_shape(Array<v2f32> buffer, v2f32 dimensions, v2f32 center) {
	return make_shape_2d<Shape2D::Polygon>(make_box_poly(buffer, dimensions, center));
}

Segment<v2f32> support_circle(v2f32 center, f32 radius, v2f32 direction) {
	return { center + glm::normalize(direction) * radius, center + glm::normalize(direction) * radius };
}

constexpr auto penetration_threshold = 0.0001f;

Segment<v2f32> support_point_cloud(Array<v2f32> vertices, v2f32 direction) {
	f32 support_dot[2] = {
		std::numeric_limits<f32>::lowest(),
		std::numeric_limits<f32>::lowest()
	};
	v2f32 support[2];

	for (auto v : vertices) {
		auto dot = glm::dot(v, glm::normalize(direction));
		if (dot > support_dot[0]) {
			support_dot[1] = support_dot[0];
			support[1] = support[0];
			support_dot[0] = dot;
			support[0] = v;
		} else if (dot > support_dot[1]) {
			support_dot[1] = dot;
			support[1] = v;
		}
	}
	if (support_dot[0] - support_dot[1] < penetration_threshold)
		return { support[0], support[1] };
	else
		return { support[0], support[0] };
}

Segment<v2f32> support_polygon(Array<v2f32> vertices, v2f32 direction) {
	return support_point_cloud(vertices, direction);
}

Segment<v2f32> support_line(Segment<v2f32> line, v2f32 direction) {
	return support_point_cloud(cast<v2f32>(carray(&line, 1)), direction);
}

Segment<v2f32> support(const Shape2D& shape, v2f32 direction) {
	switch (shape.type) {
	case Shape2D::Circle: return support_circle(v2f32(shape.circle), shape.circle.z, direction);
	case Shape2D::Polygon: return support_polygon(shape.polygon, direction);
	case Shape2D::Line: return support_line(shape.line, direction);
	case Shape2D::Point: return { shape.point, shape.point };
	case Shape2D::Composite: {
		Segment<v2f32> supports[shape.composite.size()];
		for (auto i : u64xrange{ 0, shape.composite.size() })
			supports[i] = support(shape.composite[i], direction);
		return support_point_cloud(cast<v2f32>(carray(supports, shape.composite.size())), direction);
	};
	default: return fail_ret("Unimplemented support function", Segment<v2f32>{});
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

inline rtf32 aabb_line(Segment<v2f32> line, m4x4f32 transform) {
	auto transformed = rtf32{ v2f32(transform * v4f32(line.A, 0, 1)), v2f32(transform * v4f32(line.B, 0, 1)) };
	return { glm::min(transformed.min, transformed.max), glm::max(transformed.min, transformed.max) };
}

inline rtf32 aabb_point(v2f32 point, m4x4f32 transform) {
	auto transformed = v2f32(transform * v4f32(point, 0, 1));
	return { transformed, transformed };
}

inline rtf32 aabb_shape(const Shape2D& shape, m4x4f32 transform) {
	switch (shape.type) {
	case Shape2D::Circle: return aabb_circle(v2f32(shape.circle), shape.circle.z, transform);
	case Shape2D::Polygon: return aabb_polygon(shape.polygon, transform);
	case Shape2D::Line: return aabb_line(shape.line, transform);
	case Shape2D::Point: return aabb_point(shape.point, transform);
	case Shape2D::Composite: {
		rtf32 complete = { v2f32(std::numeric_limits<f32>::max()), v2f32(std::numeric_limits<f32>::lowest()) };
		for (auto&& s : shape.composite) {
			auto aabb = aabb_shape(s, transform);
			complete.min = glm::min(complete.min, aabb.min);
			complete.max = glm::max(complete.max, aabb.max);
		}
		return complete;
	};
	default: return { v2f32(1), v2f32(-1) };
	}
}

using Triangle = tuple<v2f32, v2f32, v2f32>;

v2f32 minkowski_diff_support(const Shape2D& s1, const Shape2D& s2, m4x4f32 m1, m4x4f32 m2, v2f32 direction) {
	return v2f32(
		m1 * v4f32(support(s1, glm::transpose(m1) * v4f32(+direction, 0, 1)).A, 0, 1) -
		m2 * v4f32(support(s2, glm::transpose(m2) * v4f32(-direction, 0, 1)).A, 0, 1)
	);
}

// https://www.youtube.com/watch?v=ajv46BSqcK4&ab_channel=Reducible
// Gilbert-Johnson-Keerthi
// TODO continuous collision -> support function only selects point, should sample from the convex hull of the shapes at time t & t+dt
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
	// return min_normal * (min_dist + iteration_threshold);
	return min_normal * min_dist;
}

tuple<bool, rtf32, v2f32> intersect(const Shape2D& s1, const Shape2D& s2, m4x4f32 t1, m4x4f32 t2) {
	auto aabb_intersection = intersect(aabb_shape(s1, t1), aabb_shape(s2, t2));
	if (width(aabb_intersection) < 0 || height(aabb_intersection) < 0)
		return { false, aabb_intersection, v2f32(0) };
	auto [collided, triangle] = GJK(s1, s2, t1, t2);
	if (!collided)
		return { false, aabb_intersection, v2f32(0) };
	auto penetration = EPA(s1, s2, t1, t2, triangle);
	return { true, aabb_intersection, penetration };
}

constexpr auto max_contact_points_convex_2d = 2;

bool can_resolve_collision(f32 im1, f32 im2, f32 ii1, f32 ii2) {
	return
		(im1 > 0 || im2 > 0) && // can turn
		(ii1 > 0 || ii2 > 0);// can move
}

struct PhysicalDescriptor {
	Transform2D velocity;
	v2f32 world_cmass;
	f32 inverse_mass;
	f32 inverse_inertia;
};

//linear + angular momentum -> https://www.youtube.com/watch?v=VbvdoLQQUPs&ab_channel=Two-BitCoding
tuple<Transform2D, Transform2D> bounce_contacts(
	PhysicalDescriptor ent1, PhysicalDescriptor ent2,
	Array<const v2f32> contacts,
	v2f32 normal,
	f32 elasticity
) {
	auto delta_vel_1 = Transform2D{ v2f32(0), v2f32(0), 0.f };
	auto delta_vel_2 = Transform2D{ v2f32(0), v2f32(0), 0.f };

	for (auto contact : contacts) {
		auto lever1 = contact - ent1.world_cmass;
		auto lever2 = contact - ent2.world_cmass;
		//TODO maybe? "scale" velocity

		auto rvel =
			(ent2.velocity.translation + perpendicular(lever2) * ent2.velocity.rotation) -
			(ent1.velocity.translation + perpendicular(lever1) * ent1.velocity.rotation);

		auto impulse =
			normal *
			-(1.f + elasticity) * glm::dot(rvel, normal) /
			f32(ent1.inverse_mass + ent2.inverse_mass +
				glm::pow(glm::dot(perpendicular(lever1), normal), 2) * ent1.inverse_inertia +
				glm::pow(glm::dot(perpendicular(lever2), normal), 2) * ent2.inverse_inertia);
		delta_vel_1 = delta_vel_1 + Transform2D{
			-impulse * ent1.inverse_mass,
				v2f32(0),
				// glm::degrees(-glm::cross(v3f32(lever1, 0), v3f32(impulse, 0)).z * ent1.inverse_inertia)
				// glm::radians(-glm::cross(v3f32(lever1, 0), v3f32(impulse, 0)).z * ent1.inverse_inertia)
				-glm::cross(v3f32(lever1, 0), v3f32(impulse, 0)).z * ent1.inverse_inertia
		};
		delta_vel_2 = delta_vel_2 + Transform2D{
			+impulse * ent2.inverse_mass,
				v2f32(0),
				// glm::degrees(+glm::cross(v3f32(lever2, 0), v3f32(impulse, 0)).z * ent2.inverse_inertia)
				// glm::radians(+glm::cross(v3f32(lever2, 0), v3f32(impulse, 0)).z * ent2.inverse_inertia)
				+glm::cross(v3f32(lever2, 0), v3f32(impulse, 0)).z * ent2.inverse_inertia
		};
	}

	auto contact_count_inv = 1 / f32(contacts.size());
	delta_vel_1 = delta_vel_1 * contact_count_inv;
	delta_vel_2 = delta_vel_2 * contact_count_inv;
	return { delta_vel_1, delta_vel_2 };
}

inline tuple<v2f32, v2f32> contacts_points(const Shape2D& s1, const Shape2D& s2, m4x4f32 t1, m4x4f32 t2, v2f32 normal) {

	//Local support edges
	auto [lA1, lB1] = support(s1, glm::transpose(t1) * v4f32(+normal, 0, 1));
	auto [lA2, lB2] = support(s2, glm::transpose(t2) * v4f32(-normal, 0, 1));

	// global support edges
	auto A1 = v2f32(t1 * v4f32(lA1, 0, 1));
	auto B1 = v2f32(t1 * v4f32(lB1, 0, 1));

	auto A2 = v2f32(t2 * v4f32(lA2, 0, 1));
	auto B2 = v2f32(t2 * v4f32(lB2, 0, 1));

	auto pen_perp = v2f32(-normal.y, normal.x);

	// Shadow intersection
	return {
		support_line({support_line({ A1, B1 }, -pen_perp).A, support_line({ A2, B2 }, -pen_perp).A}, +pen_perp).A,
		support_line({support_line({ A1, B1 }, +pen_perp).A, support_line({ A2, B2 }, +pen_perp).A}, -pen_perp).A
	};

}

inline tuple<v2f32, v2f32> correction_offset(v2f32 penetration, f32 im1, f32 im2) {
	return {
		-penetration * inv_lerp(0.f, im1 + im2, im1),
		+penetration * inv_lerp(0.f, im1 + im2, im2),
	};
}

#ifndef DEFAULT_GRAVITY
#define DEFAULT_GRAVITY v2f32(0, -9.81f)
#endif

constexpr auto GRAVITY = DEFAULT_GRAVITY;

bool EditorWidget(const cstr label, Shape2D& shape) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= ImGui::Combo("Type", (i32*)&shape.type, shape_type_names.data(), shape_type_names.size());
		switch (shape.type) {
		case Shape2D::Circle: changed |= EditorWidget("Radius", shape.circle.z);break;
		case Shape2D::Polygon: changed |= EditorWidget("Polygon", shape.polygon);break;
		case Shape2D::Line: changed |= EditorWidget("Line", shape.line);break;
		case Shape2D::Point: changed |= EditorWidget("Point", shape.point);break;
		}
	}
	return changed;
}

struct Collider2D {
	struct {
		f32 inverse_mass = 0.f;
		f32 inverse_inertia = 0.f;
		f32 restitution = 0.f;
		f32 friction = 0.f;
	} material;
	Shape2D shape;
};

bool EditorWidget(const cstr label, Collider2D& collider) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("Shape", collider.shape);
		if (ImGui::TreeNode("Material")) {
			defer{ ImGui::TreePop(); };
			changed |= EditorWidget("Restitution", collider.material.restitution);
			changed |= EditorWidget("Friction", collider.material.friction);
			f32 mass = (collider.material.inverse_mass != 0) ? 1 / collider.material.inverse_mass : 0;
			if (EditorWidget("Mass", mass)) {
				collider.material.inverse_mass = mass != 0 ? 1 / mass : 0;
				changed = true;
			}
			f32 inertia = (collider.material.inverse_inertia != 0) ? 1 / collider.material.inverse_inertia : 0;
			if (EditorWidget("Inertia", inertia)) {
				collider.material.inverse_inertia = inertia != 0 ? 1 / inertia : 0;
				changed = true;
			}
		}
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
