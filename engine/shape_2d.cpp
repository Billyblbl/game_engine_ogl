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
			snprintf(buff, sizeof(buff), "%llu", c);
			changed |= EditorWidget(buff, m[c]);
		}
	}
	return changed;
}

inline rtf32 aabb_point_cloud(Array<v2f32> vertices, const m3x3f32& transform = m3x3f32(1)) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	return fold(rtf32{ v2f32(xf32::max()), v2f32(xf32::lowest()) }, vertices,
		[&](rtf32 a, v2f32 v) {
			auto world = v2f32(transform * v3f32(v, 1));
			return combined_aabb(a, rtf32{ world, world });
		}
	);
}

//* approximative aabb, taking the aabb of the transformed aabb of the circle in its own space, simpler that way
inline rtf32 aabb_circle(v2f32 center, f32 radius) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	return { center - v2f32(radius), center + v2f32(radius) };
}

inline rtf32 aabb_transformed_rect(rtf32 rect, const m3x3f32& transform) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	if (glm::any(negative(rect)))
		return { v2f32(xf32::max()), v2f32(xf32::lowest()) };
	v2f32 corners[] = { rect.min, v2f32(rect.min.x, rect.max.y), rect.max, v2f32(rect.max.x, rect.min.y) };
	return aabb_point_cloud(larray(corners), transform);
}

inline rtf32 aabb_fat_point_cloud(Array<const v2f32> vertices, f32 radius, const m3x3f32& transform = m3x3f32(1)) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	return fold(rtf32{ v2f32(xf32::max()), v2f32(xf32::lowest()) }, vertices, [&](rtf32 a, v2f32 v) { return combined_aabb(a, aabb_transformed_rect(aabb_circle(v, radius), transform)); });
}

inline rtf32 aabb_mesh(PolygonMesh mesh, f32 radius, const m3x3f32& transform = m3x3f32(1)) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	return fold(rtf32{ v2f32(xf32::max()), v2f32(xf32::lowest()) }, mesh, [&](rtf32 a, Polygon v) { return combined_aabb(a, aabb_transformed_rect(aabb_fat_point_cloud(v, radius), transform)); });
}



#endif
