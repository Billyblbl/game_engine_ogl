#ifndef GPHYSICS_2D
# define GPHYSICS_2D

#include <blblstd.hpp>
#include <math.cpp>
#include <imgui_extension.cpp>
#include <stdio.h>
#include <transform.cpp>

static const cstrp shape_type_names_array[] = { "None", "Circle", "Polygon", "Line", "Point", "Composite Hull", "Concave" };
static const auto shape_type_names = larray(shape_type_names_array);

struct Shape2D {
	enum Type : u32 { None, Circle, Polygon, Line, Point, CompositeHull, Concave, TypeCount } type = None;
	union {
		Array<v2f32> polygon;
		Array<Shape2D> composite;
		v3f32 circle;//xy center, z radius
		Segment<v2f32> line;
		v2f32 point;
	};
};

struct Body {
	f32 inverse_mass = 0.f;
	f32 inverse_inertia = 0.f;
	f32 restitution = 0.f;
	f32 friction = 0.f;
};

template<Shape2D::Type type> struct shape_mapping {};
template<> struct shape_mapping<Shape2D::Circle> { static constexpr auto member = &Shape2D::circle; };
template<> struct shape_mapping<Shape2D::Polygon> { static constexpr auto member = &Shape2D::polygon; };
template<> struct shape_mapping<Shape2D::Line> { static constexpr auto member = &Shape2D::line; };
template<> struct shape_mapping<Shape2D::Point> { static constexpr auto member = &Shape2D::point; };
template<> struct shape_mapping<Shape2D::CompositeHull> { static constexpr auto member = &Shape2D::composite; };
template<> struct shape_mapping<Shape2D::Concave> { static constexpr auto member = &Shape2D::composite; };

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
	auto point = center + glm::normalize(direction) * radius;
	return { point, point };
}

Segment<v2f32> support_point_cloud(Array<v2f32> vertices, v2f32 direction) {
	struct {
		f32 dot = std::numeric_limits<f32>::lowest();
		v2f32 point;
	} buff[2];
	auto supports = List{ larray(buff), 0 };
	for (auto v : vertices) {
		auto dot = glm::dot(v, glm::normalize(direction));
		if (dot > supports.allocated()[0].dot) {
			supports.current = 0;
			supports.push({ dot, v });
		} else if (supports.current < supports.capacity.size() && dot == supports.allocated()[0].dot) {//TODO check if this == is enough or if it needs an epsilon comp
			supports.push({ dot, v });
		}
	}
	while (supports.current < supports.capacity.size())
		supports.push(supports.allocated().back());
	return { supports.allocated()[0].point, supports.allocated()[1].point };
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
	case Shape2D::CompositeHull: {
		Segment<v2f32> supports[shape.composite.size()];
		for (auto i : u64xrange{ 0, shape.composite.size() })
			supports[i] = support(shape.composite[i], direction);
		return support_point_cloud(cast<v2f32>(carray(supports, shape.composite.size())), direction);
	};
	default: return fail_ret("Unimplemented support function", Segment<v2f32>{});
	}
	return {};
}

template<typename F> concept support_function = requires(const F & f, v2f32 direction) { { f(direction) } -> std::same_as<Segment<v2f32>>; };

auto support_function_of(const Shape2D& shape, m4x4f32 transform) {
	return [&shape, transform](v2f32 direction) -> Segment<v2f32> {
		auto [lA, lB] = support(shape, glm::transpose(transform) * v4f32(direction, 0, 1));
		return { v2f32(transform * v4f32(lA, 0, 1)), v2f32(transform * v4f32(lB, 0, 1)) };
		};
}

inline rtf32 aabb_circle(v2f32 center, f32 radius, m4x4f32 transform) {
	auto scaling_factors = v2f32(glm::length(v3f32(transform[0])), glm::length(v3f32(transform[1])));
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
	case Shape2D::CompositeHull:
	case Shape2D::Concave: {
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

template<support_function F1, support_function F2> Segment<v2f32> minkowski_diff_support(const F1& f1, const F2& f2, v2f32 direction) {
	auto p = f1(direction).A - f2(-direction).A;
	return { p, p };
}

// https://www.youtube.com/watch?v=ajv46BSqcK4&ab_channel=Reducible
// Gilbert-Johnson-Keerthi
// TODO continuous collision -> support function only selects point, should sample from the convex hull of the shapes at time t & t+dt
template<support_function F1, support_function F2> tuple<bool, tuple<v2f32, v2f32, v2f32>> GJK(const F1& f1, const F2& f2, v2f32 start_direction = glm::normalize(v2f32(1))) {
	constexpr tuple<bool, tuple<v2f32, v2f32, v2f32>> no_collision = { false, {{}, {}, {}} };

	v2f32 triangle_vertices[3];
	auto triangle = List{ larray(triangle_vertices), 0 };
	auto direction = start_direction;

	for (auto i = 0; i < 999;i++) {// inf loop safeguard
		auto new_point = minkowski_diff_support(f1, f2, direction).A;
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
				return { true, to_tuple<3>(triangle.allocated()) };
			}
		}
	}

	return fail_ret("GJK iteration out of bounds", no_collision);
}

// Expanding Polytope algorithm
// https://www.youtube.com/watch?v=0XQ2FSz3EK8&ab_channel=Winterdev
template<support_function F1, support_function F2> v2f32 EPA(const F1& f1, const F2& f2, tuple<v2f32, v2f32, v2f32> triangle, u32 max_iteration = 64, f32 iteration_threshold = 0.05f) {
	auto [A, B, C] = triangle;
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
			auto normal = glm::normalize(orthogonal(vj - vi));
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

		auto support_point = minkowski_diff_support(f1, f2, min_normal).A;
		auto s_distance = glm::dot(min_normal, support_point);

		if (glm::abs(s_distance - min_dist) > iteration_threshold) {
			min_dist = std::numeric_limits<f32>::max();
			points.insert_ordered(min_index, std::move(support_point));
		} else {
			break;
		}

	}
	return min_normal * min_dist;
}

template<support_function F1, support_function F2> inline Segment<v2f32> contacts_segment(const F1& f1, const F2& f2, v2f32 normal) {
	//* assumes normal is in general direction s1 -> s2, aka normal is in direction of the penetration of s1 into s2
	Segment<v2f32> supports[] = { f1(+normal), f2(-normal) };
	for (auto [a, b] : supports) if (a == b)
		return { a, b };

	return Segment<v2f32>{// overlap of segments
		glm::min(glm::max(supports[0].A, supports[0].B), glm::max(supports[1].A, supports[1].B)),
			glm::max(glm::min(supports[0].A, supports[0].B), glm::min(supports[1].A, supports[1].B))
	};
}

template<support_function F1, support_function F2> tuple<bool, v2f32> intersect_convex(const F1& f1, const F2& f2) {
	auto [collided, triangle] = GJK(f1, f2);
	return { collided, (collided ? EPA(f1, f2, triangle) : v2f32(0)) };
}

struct Contact2D {
	v2f32 penetration;
	v2f32 levers[2];
	f32 surface;
};

bool EditorWidget(const cstr label, Contact2D& contact) {
	auto changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("Penetration", contact.penetration);
		changed |= EditorWidget("Levers", larray(contact.levers));
		ImGui::TreePop();
	}
	return changed;
}

Array<Contact2D> intersect_concave(List<Contact2D>& intersections, const Shape2D& s1, const Shape2D& s2, const m4x4f32& t1, const m4x4f32& t2) {

	auto start = intersections.current;

	if (s1.type != Shape2D::Concave && s2.type != Shape2D::Concave) {
		auto [collided, pen] = intersect_convex(support_function_of(s1, t1), support_function_of(s2, t2));

		if (collided) {
			auto offset_t1 = glm::translate(t1, v3f32(-pen, 0));
			auto [ctct1, ctct2] = contacts_segment(
				support_function_of(s1, offset_t1),
				support_function_of(s2, t2),
				glm::normalize(pen)
			);
			auto av_contact = average({ ctct1, ctct2 });
			v2f32 world_cmass[] = { t1 * v4f32(0, 0, 0, 1), t2 * v4f32(0, 0, 0, 1) };

			intersections.push({ pen, { av_contact - world_cmass[0], av_contact - world_cmass[1] }, glm::length(ctct2 - ctct1) });
		}
	} else {
		for (auto& sub_shape1 : (s1.type == Shape2D::Concave) ? s1.composite : carray(&s1, 1))
			for (auto& sub_shape2 : (s2.type == Shape2D::Concave) ? s2.composite : carray(&s2, 1))
				intersect_concave(intersections, sub_shape1, sub_shape2, t1, t2);
	}

	return intersections.allocated().subspan(start, intersections.current - start);
}

tuple<bool, rtf32, v2f32> intersect(const Shape2D& s1, const Shape2D& s2, m4x4f32 t1, m4x4f32 t2) {
	auto aabb_intersection = intersect(aabb_shape(s1, t1), aabb_shape(s2, t2));
	if (width(aabb_intersection) < 0 || height(aabb_intersection) < 0)
		return { false, aabb_intersection, v2f32(0) };

	auto [collided, pen] = intersect_convex(support_function_of(s1, t1), support_function_of(s2, t2));
	return { collided, aabb_intersection, pen };
}

bool can_respond(f32 i1, f32 i2) { return i1 > 0 || i2 > 0; }

bool can_resolve_collision(Body* b1, Body* b2) {
	return (b1 && b2) && // has bodies
		can_respond(b1->inverse_mass, b2->inverse_mass) &&
		can_respond(b1->inverse_inertia, b2->inverse_inertia)
		;
}

v2f32 velocity_at_point(const Transform2D& velocity, v2f32 point) {
	return velocity.translation + orthogonal(point) * glm::radians(velocity.rotation);
}

tuple<Transform2D, Transform2D> lever_impulsion(v2f32 impulse, Array<const v2f32> lever, Array<const f32> inverse_masses, Array<const f32> inverse_inertias) {
	using namespace glm;
	if (length(impulse) <= 0) return { null_transform_2d, null_transform_2d };

	auto attenuated = impulse / f32(inverse_masses[0] + inverse_masses[1] +
		pow(dot(orthogonal_axis(lever[0]), normalize(impulse)), 2) * inverse_inertias[0] +
		pow(dot(orthogonal_axis(lever[1]), normalize(impulse)), 2) * inverse_inertias[1]);

	auto push_at_lever = (
		[](v2f32 attenuated_impulse, v2f32 lever, f32 inverse_mass, f32 inverse_inertia) -> Transform2D {
			Transform2D delta_v;
			delta_v.translation = attenuated_impulse * inverse_mass;
			delta_v.rotation = degrees(cross(v3f32(lever, 0), v3f32(attenuated_impulse, 0)).z * inverse_inertia);
			return delta_v;
		}
	);

	return {
		push_at_lever(-attenuated, lever[0], inverse_masses[0], inverse_inertias[0]),
		push_at_lever(+attenuated, lever[1], inverse_masses[1], inverse_inertias[1])
	};
}

tuple<Transform2D, Transform2D> lever_impulsion(v2f32 impulse, LiteralArray<v2f32> lever, LiteralArray<f32> inverse_masses, LiteralArray<f32> inverse_inertias) { return lever_impulsion(impulse, larray(lever), larray(inverse_masses), larray(inverse_inertias)); }

#ifndef DEFAULT_GRAVITY
#define DEFAULT_GRAVITY v2f32(0, -9.807f)
#endif

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
		case Shape2D::CompositeHull: changed |= EditorWidget("Composite Hull", shape.composite);break;
		case Shape2D::Concave: changed |= EditorWidget("Concave Manifold", shape.composite);break;
		}
	}
	return changed;
}

bool EditorWidget(const cstr label, Body& body) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("Restitution", body.restitution);
		changed |= EditorWidget("Friction", body.friction);
		f32 mass = (body.inverse_mass != 0) ? 1 / body.inverse_mass : 0;
		if (EditorWidget("Mass", mass)) {
			body.inverse_mass = mass != 0 ? 1 / mass : 0;
			changed = true;
		}
		f32 inertia = (body.inverse_inertia != 0) ? 1 / body.inverse_inertia : 0;
		if (EditorWidget("Inertia", inertia)) {
			body.inverse_inertia = inertia != 0 ? 1 / inertia : 0;
			changed = true;
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

using Polygon = Array<v2f32>;

enum WindingOrder : i8 {
	Clockwise = 1,
	AntiClockwise = -1
};

bool is_convex(Polygon poly) {
	if (poly.size() < 3)
		return false;

	bool clockwise = false;
	bool anti_clockwise = false;

	for (auto i : u64xrange{ 0, poly.size() }) {
		v2f32 verts[] = { poly[i], poly[(i + 1) % poly.size()], poly[(i + 2) % poly.size()] };
		v2f32 edges[] = { verts[1] - verts[0], verts[2] - verts[1] };
		auto cross = glm::cross(v3f32(edges[0], 0), v3f32(edges[1], 0)).z;

		if (cross < 0)
			clockwise = true;
		else if (cross > 0)
			anti_clockwise = true;

		if (clockwise && anti_clockwise)
			return false;
	}
	return true;

}

bool intersect_poly_poly(Array<v2f32> p1, Array<v2f32> p2) {
	auto s1 = make_shape_2d<Shape2D::Polygon>(p1);
	auto s2 = make_shape_2d<Shape2D::Polygon>(p2);
	auto f1 = support_function_of(s1, m4x4f32(1));
	auto f2 = support_function_of(s2, m4x4f32(1));
	auto [i, _] = intersect_convex(f1, f2);
	return i;
}

tuple<Array<Polygon>, Array<v2f32>> decompose_concave_poly(Polygon polygon, Alloc allocator = std_allocator) {

	if (polygon.size() < 4) {
		auto p = duplicate_array(allocator, polygon);
		auto dec = alloc_array<Polygon>(allocator, 1);
		dec[0] = p;
		return { dec, p };
	}

	List<v2f32> vertices = { alloc_array<v2f32>(allocator, polygon.size()), 0 };
	List<u64range> sub_poly_indices = { alloc_array<u64range>(allocator, polygon.size() - 2), 0 }; defer{ dealloc_array(allocator, sub_poly_indices.capacity); }; // exclusive ranges
	u64 poly_start = 0;

	auto to_decompose = List{ alloc_array<u64range>(allocator, polygon.size()), 0 }; // inclusive ranges
	to_decompose.push(array_indices(polygon));

	while (to_decompose.current > 0) {
		assert(sub_poly_indices.current < polygon.size());
		auto poly = to_decompose.pop();
		u64range ear_poly = { 0, 0 }; // inclusive range
		for (auto i : u64xrange{ poly.min, poly.max + 1 }) {
			auto index = i % polygon.size();
			vertices.push_growing(allocator, polygon[index]);
			u64range leftover = { i + 1, poly.max };

			//* Need to copy from polygon because the indices we have need to be wrapped around with % size, and we need an actual Array<v2f32> out of it
			v2f32 leftover_vertices[leftover.size() + 1];
			auto leftover_poly = List{ carray(leftover_vertices, leftover.size() + 1), 0 };
			for (auto j : u64xrange{ leftover.min, leftover.max + 1 })
				leftover_poly.push(polygon[j % polygon.size()]);

			auto current_poly = vertices.allocated().subspan(poly_start, vertices.current - poly_start);
			if (current_poly.size() < 3) continue;
			if (!is_convex(current_poly) || intersect_poly_poly(current_poly, leftover_poly.allocated())) {//* breaks convex -> start ear poly
				vertices.pop();
				if (ear_poly.min == ear_poly.max)
					ear_poly.min = i - 1;
				ear_poly.max = i + 1;
			} else if (ear_poly.min != ear_poly.max) {// *finished ear polygon
				to_decompose.push_growing(allocator, ear_poly);
				assert(to_decompose.allocated().back().size() + 1 > 2);
				ear_poly = { 0, 0 };
			}
		}
		if (ear_poly.min != ear_poly.max)// *finished ear polygon
			to_decompose.push_growing(allocator, ear_poly);
		sub_poly_indices.push_growing(allocator, { poly_start, vertices.current });
		poly_start = vertices.current;
	}

	//* Build the polygon array from the indices ranges
	//* Need this index & rebuild system because when vertices grows it would invalidate Array<v2f32>s referencing its content
	vertices.shrink_to_content(allocator);
	auto sub_polys = map(allocator, sub_poly_indices.allocated(), [&](u64range r) { return vertices.allocated().subspan(r.min, r.max - r.min); });
	for (auto p : sub_polys)
		assert(is_convex(p));
	return { sub_polys, vertices.allocated() };
}

#include <system_editor.cpp>
#include <entity.cpp>
#include <physics_2d_debug.cpp>

struct Collision2D {
	enum : u8 {
		Physical = 1 << 0,
		Linear = 1 << 1,
		Angular = 1 << 2,
	};
	Array<Contact2D> contacts;
	EntityHandle entities[2];
	u8 flags;
};

u8 collision_type(Body* b1, Body* b2) {
	u8 flags = 0;
	if (b1 && b2)
		flags |= Collision2D::Physical;
	else
		return 0;
	if (can_respond(b1->inverse_mass, b2->inverse_mass))
		flags |= Collision2D::Linear;
	if (can_respond(b1->inverse_inertia, b2->inverse_inertia))
		flags |= Collision2D::Angular;
	return flags;
}

struct RigidBody {
	EntityHandle handle;
	Shape2D* shape;
	Spacial2D* spacial;
	Body* body;
};

using RigidBodyComp = tuple<EntityHandle, Shape2D*, Spacial2D*, Body*>;

constexpr auto MaxContactPerCollision = 10u;

//* linear + angular momentum deltas -> https://www.youtube.com/watch?v=VbvdoLQQUPs&ab_channel=Two-BitCoding
//* heavily modified since first written from this video
//* static friction taken from : https://gamedevelopment.tutsplus.com/how-to-create-a-custom-2d-physics-engine-friction-scene-and-jump-table--gamedev-7756t
tuple<Transform2D, Transform2D> contact_response(
	Array<RigidBody> ents,
	const Contact2D& contact,
	f32 elasticity,
	f32 kinetic_friction,
	f32 static_friction
) {
	using namespace glm;
	auto normal = normalize(contact.penetration);
	auto ctc_rvel = velocity_at_point(ents[1].spacial->velocity, contact.levers[1]) - velocity_at_point(ents[0].spacial->velocity, contact.levers[0]);
	auto tangent = orthogonal_axis(normal) * sign(dot(orthogonal_axis(normal), ctc_rvel));

	auto cancel = [&](v2f32 v, v2f32 d)->f32 { return length(d) > 0 && length(v) > 0 ? -dot(v, d) : 0; };
	f32 normal_impulse = cancel(ctc_rvel, normal);//cancel(ctc_rvel, normal);
	f32 friction_impulse = cancel(ctc_rvel, tangent);//cancel(ctc_rvel, tangent);

	auto overcome_static = abs(friction_impulse) > abs(normal_impulse * static_friction);

	v2f32 impulse =
		normal * (1.f + elasticity) * normal_impulse + //contact elastic bounce
		tangent * (overcome_static ? kinetic_friction : 1.f) * friction_impulse; // static friction resistance | kinetic friction dragging

	return lever_impulsion(
		impulse,
		{ contact.levers[0], contact.levers[1] },
		{ ents[0].body->inverse_mass, ents[1].body->inverse_mass },
		{ ents[0].body->inverse_inertia, ents[1].body->inverse_inertia }
	);
}

Array<Collision2D> simulate_collisions(Alloc allocator, Array<RigidBody> entities) {
	if (entities.size() == 0) return {};
	auto contact_pool = List{ alloc_array<Contact2D>(allocator, entities.size() * entities.size() * MaxContactPerCollision), 0 };
	auto collisions = List{ alloc_array<Collision2D>(allocator, entities.size() * entities.size()), 0 };
	for (auto i : u64xrange{ 0, entities.size() - 1 }) {
		for (auto j : u64xrange{ i + 1, entities.size() }) {
			RigidBody ents[] = { entities[i], entities[j] };

			auto contacts = intersect_concave(contact_pool, *ents[0].shape, *ents[1].shape, trs_2d(ents[0].spacial->transform), trs_2d(ents[1].spacial->transform));
			if (contacts.size() == 0)
				continue;

			auto type = collision_type(ents[0].body, ents[1].body);
			collisions.push_growing(allocator, { contacts, { ents[0].handle, ents[1].handle },  type });
			if (!has_all(type, Collision2D::Physical))
				continue;

			// Correct penetration
			auto pen = average(contacts, &Contact2D::penetration);
			ents[0].spacial->transform.translation += pen * -inv_lerp(0.f, ents[0].body->inverse_mass + ents[1].body->inverse_mass, ents[0].body->inverse_mass);
			ents[1].spacial->transform.translation += pen * +inv_lerp(0.f, ents[0].body->inverse_mass + ents[1].body->inverse_mass, ents[1].body->inverse_mass);
			if (!has_one(type, Collision2D::Linear | Collision2D::Angular))
				continue;

			// Velocity Impulse
			f32 elasticity = min(ents[0].body->restitution, ents[1].body->restitution);
			f32 kinetic_friction = average({ ents[0].body->friction, ents[1].body->friction });
			f32 static_friction = ents[0].body->friction * ents[1].body->friction;
			auto [d1, d2] = fold(tuple<Transform2D, Transform2D>{}, contacts,
				[&](const tuple<Transform2D, Transform2D>& acc, const Contact2D& contact) -> tuple<Transform2D, Transform2D> {
					auto [acc1, acc2] = acc;
					auto [r1, r2] = contact_response(ents, contact, elasticity, kinetic_friction, static_friction);
					return { acc1 + r1, acc2 + r2 };
				}
			);
			ents[0].spacial->velocity = ents[0].spacial->velocity + d1;
			ents[1].spacial->velocity = ents[1].spacial->velocity + d2;
		}
	}
	collisions.shrink_to_content(allocator);
	contact_pool.shrink_to_content(allocator);
	return collisions.allocated();
}

struct Physics2D {

	static constexpr u64 PhysicsMemory = (MAX_ENTITIES * MAX_ENTITIES * sizeof(Collision2D) + MAX_ENTITIES * MAX_ENTITIES * sizeof(Contact2D) * MaxContactPerCollision);
	Array<Collision2D> collisions = {};
	Arena physics_scratch = create_virtual_arena(PhysicsMemory);

	void operator()(Array<RigidBody> entities) {
		collisions = {};
		reset_virtual_arena(physics_scratch);
		collisions = simulate_collisions(as_v_alloc(physics_scratch), entities);
	}

	struct Editor : public SystemEditor {
		ShapeRenderer debug_draw = load_shape_renderer();
		bool debug = false;
		bool colliders = true;
		bool wireframe = true;
		bool collisions = true;
		MappedObject<m4x4f32> view_projection_matrix;

		Editor() : SystemEditor("Physics2D", "Alt+P", { Input::KB::K_LEFT_ALT, Input::KB::K_P }) {
			view_projection_matrix = map_object<m4x4f32>(m4x4f32(1));
		}

		void draw_shapes(Array<tuple<Shape2D*, Spacial2D*>> shapes, const m4x4f32& matrix) {
			sync(view_projection_matrix, matrix);
			for (auto [shape, space] : shapes) {
				auto mat = trs_2d(space->transform);
				debug_draw(*shape, mat, view_projection_matrix, v4f32(1, 0, 0, 1), wireframe);
				v2f32 local_vel = glm::transpose(mat) * v4f32(space->velocity.translation, 0, 1);
				sync(debug_draw.render_info, { mat, v4f32(1, 1, 0, 1) });
				debug_draw.draw_line(Segment<v2f32>{v2f32(0), local_vel}, view_projection_matrix, wireframe);
			}
		}

		template<typename T> void draw_collisions(Array<Collision2D> collisions, const m4x4f32& matrix, Spacial2D T::* sp_member) {
			sync(view_projection_matrix, matrix);
			for (auto [contacts, entities, physical] : collisions) {
				Spacial2D* sp[] = { &(entities[0]->template content<T>().*sp_member), &(entities[1]->template content<T>().*sp_member) };
				sync(debug_draw.render_info, { m4x4f32(1), v4f32(1, 0, 1, 1) });
				debug_draw.draw_line({ sp[0]->transform.translation , sp[1]->transform.translation }, view_projection_matrix, wireframe);
				if (physical) for (auto& contact : contacts) {
					sync(debug_draw.render_info, { m4x4f32(1), v4f32(0, 1, 1, 1) });
					for (auto i : u64xrange{ 0, 2 })
						debug_draw.draw_line({ sp[i]->transform.translation, sp[i]->transform.translation + contact.levers[i] }, view_projection_matrix, wireframe);
				}
			}
		}

		void editor_window(Physics2D& system) {
			EditorWidget("Draw debug", debug);
			if (debug) {
				EditorWidget("Wireframe", wireframe);
				EditorWidget("Colliders", colliders);
				EditorWidget("Collisions marks", collisions);
			}
			if (ImGui::TreeNode("Collisions")) {
				defer{ ImGui::TreePop(); };
				auto id = 0;
				for (auto [contacts, entities, physical] : system.collisions) {
					char buffer[999];
					snprintf(buffer, sizeof(buffer), "%u:%s:%s", id, entities[0]->name.data(), entities[1]->name.data());
					ImGui::PushID(id++);
					if (ImGui::TreeNode(buffer)) {
						EditorWidget("Contacts", contacts);
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}
		}
	};

	static auto default_editor() { return Editor(); }

};


#endif
