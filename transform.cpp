#ifndef GTRANSFORM
# define GTRANSFORM

#include <math.cpp>
#include <imgui_extension.cpp>

struct Transform2D {
	v2f32 translation = v2f32(0);
	v2f32 scale = v2f32(1);
	f32 rotation = .0f;

	m4x4f32 matrix() const {
		return (
			glm::translate(m4x4f32(1), v3f32(translation, 0)) * // Translation
			glm::rotate(m4x4f32(1), glm::radians(rotation), v3f32(0, 0, -1)) * // Rotation
			glm::scale(m4x4f32(1), v3f32(scale, 1)) // Scale;
		);
	}

};

struct OrthoCamera {
	v3f32	dimensions = v3f32(600, 400, 1);
	v3f32 center = v3f32(0);

	m4x4f32 matrix() const {
		auto min = -dimensions / 2.f - center;
		auto max = dimensions / 2.f - center;
		return glm::ortho(min.x, max.x, min.y, max.y, min.z, max.z);
	}
};

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
