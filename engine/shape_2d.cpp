#ifndef GSHAPE_2D
# define GSHAPE_2D

#include <blblstd.hpp>
#include <polygon.cpp>
#include <transform.cpp>

struct Shape2D {
	Array<v2f32> points;
	Array<Shape2D> children;
	f32 radius;
	m3x3f32 transform;
};

Shape2D make_shape_2d(const Transform2D& transform = identity_2d, f32 radius = 0, Array<v2f32> points = {}, Array<Shape2D> children = {}) {
	Shape2D shape = {};
	shape.transform = transform;
	shape.points = points;
	shape.children = children;
	shape.radius = radius;
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

inline rtf32 aabb_point_cloud(Array<v2f32> vertices, const m3x3f32& transform) {
	auto mx = v2f32(std::numeric_limits<f32>::lowest());
	auto mn = v2f32(std::numeric_limits<f32>::max());

	for (auto&& v : vertices) {
		auto tfmd = v2f32(transform * v3f32(v, 1));
		mx = glm::max(mx, tfmd);
		mn = glm::min(mn, tfmd);
	}
	return { mn, mx };
}

inline rtf32 aabb_circle(v2f32 center, f32 radius) {
	return {
		center - v2f32(radius),
		center + v2f32(radius)
	};
}

rtf32 aabb_shape_group(Array<const Shape2D> shapes, const m3x3f32& parent);

inline rtf32 aabb_shape(const Shape2D& shape, const m3x3f32& parent) {
	PROFILE_SCOPE(__FUNCTION__);
	auto world = parent * shape.transform;
	auto box = aabb_point_cloud(shape.points, world);
	box = combined_aabb(box, aabb_shape_group(shape.children, world));
	for (auto& point : shape.points) { //* radius applied approximatively, taking the aabb of the transformed aabb of the circle in its own space, simpler that way
		auto aabb = aabb_circle(point, shape.radius);
		auto approx = aabb_point_cloud(cast<v2f32>(carray(&aabb, 1)), world);
		box = combined_aabb(box, approx);
	}
	return box;
}

rtf32 aabb_shape_group(Array<const Shape2D> shapes, const m3x3f32& parent) {
	PROFILE_SCOPE(__FUNCTION__);
	rtf32 box = { v2f32(std::numeric_limits<f32>::max()), v2f32(std::numeric_limits<f32>::lowest()) };
	for (auto& sub : shapes) box = combined_aabb(box, aabb_shape(sub, parent));
	return box;
}

Array<Shape2D> flatten(Arena& arena, const Shape2D& root) {
	PROFILE_SCOPE(__FUNCTION__);
	auto flattened = List{ arena.push_array<Shape2D>(root.children.size()), 0 };

	auto traverse = (
		[&](auto& recurse, const Shape2D& shape, const m3x3f32& parent) -> void {
			PROFILE_SCOPE("traverse");
			auto world = parent * shape.transform;
			if (shape.points.size() > 0) {
				Shape2D flattened_shape = shape;
				flattened_shape.transform = world;
				flattened_shape.children = {};
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
