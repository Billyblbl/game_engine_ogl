#ifndef GTRANSFORM
# define GTRANSFORM

#include <math.cpp>
#include <imgui_extension.cpp>
#include <spall/profiling.cpp>

struct Transform2D {
	v2f32 translation = v2f32(0);
	v2f32 scale = v2f32(0);
	f32 rotation = .0f;//* degrees

	operator m3x3f32() const {
		using namespace glm;
		PROFILE_SCOPE(__FUNCTION__);
		return
			translate(m3x3f32(1), translation) *
			rotate(m3x3f32(1), radians(rotation)) *
			glm::scale(m3x3f32(1), scale);
	}

	operator m4x4f32() const {
		using namespace glm;
		PROFILE_SCOPE(__FUNCTION__);
		return
			translate(v3f32(translation, 0)) *
			rotate(radians(rotation), v3f32(0, 0, +1)) *
			// rotate(radians(rotation), v3f32(0, 0, -1)) *
			glm::scale(v3f32(scale, 1));
	}
};

Transform2D make_transform(v2f32 translate = v2f32(0), f32 rotate = 0, v2f32 scale = v2f32(1)) {
	Transform2D t;
	t.translation = translate;
	t.rotation = rotate;
	t.scale = scale;
	return t;
}


constexpr Transform2D identity_2d = { v2f32(0), v2f32(1), 0 };
constexpr Transform2D null_transform_2d = { v2f32(0), v2f32(0), 0 };

inline Transform2D operator+(const Transform2D& lhs, const Transform2D& rhs) {
	return {
		lhs.translation + rhs.translation,
		lhs.scale + rhs.scale,
		lhs.rotation + rhs.rotation
	};
}

inline Transform2D operator*(const Transform2D& lhs, f32 rhs) {
	return {
		lhs.translation * rhs,
		lhs.scale * rhs,
		lhs.rotation * rhs
	};
}

Transform2D lerp(const Transform2D& a, const Transform2D& b, f32 t) {
	Transform2D res;
	res.translation = lerp(a.translation, b.translation, t);
	//TODO verify that works correctly
	res.rotation = lerp(a.rotation, b.rotation, t);
	res.scale = lerp(a.scale, b.scale, t);
	return res;
}

struct Spacial2D {
	Transform2D transform = identity_2d;
	Transform2D velocity = null_transform_2d;
	Transform2D accel = null_transform_2d;
};

inline Spacial2D& euler_integrate(Spacial2D& spacial, f32 dt) {
	spacial.velocity = spacial.velocity + (spacial.accel * dt);
	spacial.transform = spacial.transform + (spacial.velocity * dt);
	return spacial;
}

m4x4f32 ortho_project(v3f32	dimensions, v3f32 center) {
	auto min = -dimensions / 2.f - center;
	auto max = +dimensions / 2.f - center;
	return glm::ortho(min.x, max.x, min.y, max.y, min.z, max.z);
}

struct OrthoCamera {
	v3f32	dimensions = v3f32(16, 9, 1000);
	v3f32 center = v3f32(0);
	operator m4x4f32() { return ortho_project(dimensions, center); }
};

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

bool EditorWidget(const char* label, Spacial2D& data) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("Transform", data.transform);
		changed |= EditorWidget("Velocity", data.velocity);
		changed |= EditorWidget("Acceleration", data.accel);
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
