#ifndef GSHAPE_2D
# define GSHAPE_2D

#include <blblstd.hpp>
#include <polygon.cpp>
#include <transform.cpp>

template<typename T, i32 Row, i32 Col, glm::qualifier Q> bool EditorWidget(const cstr label, glm::mat<Row, Col, T, Q>& m) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		for (auto c : u64xrange{ 0, Col }) {
			char buff[10] = "";
			snprintf(buff, sizeof(buff), "%u", c);
			changed |= EditorWidget(buff, m[c]);
		}
	}
	return changed;
}

inline rtf32 aabb_point_cloud(Array<v2f32> vertices, const m3x3f32& transform = m3x3f32(1)) {
	PROFILE_SCOPE(__FUNCTION__);
	auto mx = v2f32(std::numeric_limits<f32>::lowest());
	auto mn = v2f32(std::numeric_limits<f32>::max());

	for (auto&& v : vertices) {
		auto tfmd = v2f32(transform * v3f32(v, 1));
		mx = glm::max(mx, tfmd);
		mn = glm::min(mn, tfmd);
	}
	return { mn, mx };
}

inline rtf32 aabb_transformed_aabb(rtf32 aabb, const m3x3f32& transform = m3x3f32(1)) {
	PROFILE_SCOPE(__FUNCTION__);
	v2f32 corners[] = { aabb.min, v2f32(aabb.min.x, aabb.max.y), aabb.max, v2f32(aabb.max.x, aabb.min.y) };
	return aabb_point_cloud(larray(corners), transform);
}

//* approximative aabb, taking the aabb of the transformed aabb of the circle in its own space, simpler that way
inline rtf32 aabb_circle(v2f32 center, f32 radius, const m3x3f32& transform = m3x3f32(1)) {
	PROFILE_SCOPE(__FUNCTION__);
	return aabb_transformed_aabb({ center - v2f32(radius), center + v2f32(radius) }, transform);
}

struct Shape2D;
rtf32 aabb_shape_group(Array<const Shape2D> shapes, const m3x3f32& parent = m3x3f32(1));
using xf32 = std::numeric_limits<f32>;

struct Shape2D {
	m3x3f32 transform;
	Array<v2f32> points;
	Array<Shape2D> children;
	rtf32 aabb;
	f32 radius;
	inline rtf32 compute_aabb() {
		PROFILE_SCOPE(__FUNCTION__);
		auto points_aabb = aabb_point_cloud(points, transform);
		auto children_aabb = aabb_shape_group(children, transform);
		auto radius_aabb = fold(rtf32{ v2f32(xf32::max()), v2f32(xf32::lowest()) }, points, [&](rtf32 acc, v2f32 point) { return combined_aabb(acc, aabb_circle(point, radius, transform)); });
		return combined_aabb(points_aabb, combined_aabb(children_aabb, radius_aabb));
	}
};

rtf32 aabb_shape_group(Array<const Shape2D> shapes, const m3x3f32& parent) {
	PROFILE_SCOPE(__FUNCTION__);
	auto box = rtf32{ v2f32(xf32::max()), v2f32(xf32::lowest()) };
	return fold(box, shapes, [](rtf32 a, const Shape2D& b) { return combined_aabb(a, b.aabb); });
}

Shape2D make_shape_2d(const m3x3f32& transform = identity_2d, f32 radius = 0, Array<v2f32> points = {}, Array<Shape2D> children = {}) {
	PROFILE_SCOPE(__FUNCTION__);
	Shape2D shape = {};
	shape.transform = transform;
	shape.points = points;
	shape.children = children;
	shape.radius = radius;
	shape.aabb = shape.compute_aabb();
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

Shape2D make_box_shape(Array<v2f32> buffer, v2f32 dimensions, v2f32 center, const Transform2D& transform = identity_2d) {
	return make_shape_2d(transform, 0, make_box_poly(buffer, dimensions, center));
}

Shape2D make_capsule_shape(Array<v2f32> buffer, f32 height, f32 radius, const Transform2D& transform = identity_2d) {
	assert(buffer.size() > 0);
	buffer[0] = v2f32(0, +height / 2.f);
	buffer[1] = v2f32(0, -height / 2.f);
	return make_shape_2d(transform, radius, buffer.subspan(0, 2));
}

Shape2D make_circle_shape(f32 radius, const Transform2D& transform = identity_2d) {
	static v2f32 orig[1] = { v2f32(0) };
	return make_shape_2d(transform, radius, larray(orig));
}

Array<Shape2D> flatten(Arena& arena, const Shape2D& root) {
	PROFILE_SCOPE(__FUNCTION__);
	auto flattened = List{ arena.push_array<Shape2D>(root.children.size()), 0 };
	auto traverse = (
		[&](auto& recurse, const Shape2D& shape, const m3x3f32& parent) -> void {
			auto world = parent * shape.transform;
			if (shape.points.size() > 0) {
				Shape2D flattened_shape = shape;
				flattened_shape.transform = world;
				flattened_shape.children = {};
				flattened_shape.aabb = flattened_shape.compute_aabb();
				flattened.push_growing(arena, flattened_shape);
			}
			for (auto& child_shape : shape.children) recurse(recurse, child_shape, world);
		}
	);
	traverse(traverse, root, m3x3f32(1));
	return flattened.shrink_to_content(arena);
}

Shape2D create_concave_poly_shape(Arena& arena, Polygon poly) {
	auto [sub_polys, vertices] = ear_clip(arena, poly);
	if (sub_polys.size() == 0)
		return {};
	else if (sub_polys.size() == 1)
		return make_shape_2d(identity_2d, 0, sub_polys.front());
	else
		return make_shape_2d(identity_2d, 0, {}, map(arena, sub_polys, [](Array<v2f32> poly) -> Shape2D { return make_shape_2d(identity_2d, 0, poly); }));
}

Shape2D create_polyshape(Arena& arena, Array<Polygon> polygons) {
	return make_shape_2d(identity_2d, 0, {}, map(arena, polygons, [&](Array<v2f32> poly) -> Shape2D { return create_concave_poly_shape(arena, poly); }));
}

#include <imgui_extension.cpp>

bool EditorWidget(const cstr label, Shape2D& shape) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("Points", shape.points);
		changed |= EditorWidget("Children", shape.children);
		changed |= EditorWidget("Radius", shape.radius);
		changed |= EditorWidget("Transform", shape.transform);
	}
	return changed;
}

#endif
