#ifndef GTRANSFORM
# define GTRANSFORM

#include <math.cpp>
#include <imgui_extension.cpp>

m4x4f32 trs_2d(v2f32 translation, f32 rotation, v2f32 scales) {
	using namespace glm;
	return translate(m4x4f32(1), v3f32(translation, 0)) *
		rotate(m4x4f32(1), radians(rotation), v3f32(0, 0, -1)) *
		scale(m4x4f32(1), v3f32(scales, 1));
}

m4x4f32 view_project(m4x4f32 projection, m4x4f32 view) {
	return projection * glm::inverse(view);
}

m4x4f32 ortho_project(v3f32	dimensions, v3f32 center) {
	auto min = -dimensions / 2.f - center;
	auto max = dimensions / 2.f - center;
	return glm::ortho(min.x, max.x, min.y, max.y, min.z, max.z);
}

struct Transform2D {
	v2f32 translation = v2f32(0);
	v2f32 scale = v2f32(1);
	f32 rotation = .0f;
};

struct OrthoCamera {
	v3f32	dimensions = v3f32(16, 9, 1);
	v3f32 center = v3f32(0);
};

m4x4f32 trs_2d(const Transform2D& transform) {
	return trs_2d(transform.translation, transform.rotation, transform.scale);
}

m4x4f32 project(const OrthoCamera& camera) {
	return ortho_project(camera.dimensions, camera.center);
}

bool EditorWidget(const char* label, Transform2D& data) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("Position", data.translation);
		changed |= EditorWidget("Rotation", data.rotation);
		changed |= EditorWidget("Scale", data.scale);
		ImGui::TreePop();
	}
	return changed;
}

bool EditorWidget(const char* label, OrthoCamera& data) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("Dimensions", data.dimensions);
		changed |= EditorWidget("Center", data.center);
		ImGui::TreePop();
	}
	return changed;
}

#endif
