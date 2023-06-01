#ifndef GTOP_DOWN_CONTROLS
# define GTOP_DOWN_CONTROLS

#include <inputs.cpp>
// #include <physics2D.cpp>
#include <physics_2d.cpp>
#include <animation.cpp>
#include <imgui_extension.cpp>

namespace controls {

	// x = time
	// y = angle
	// z = speed
	rtf32 animate_character_spritesheet(Array<const rtf32> keyframes, v3u32 dimensions, v3f32 coordinates) {
		return animate_grid(keyframes, dimensions, coordinates, AnimationConfig<3>(AnimRepeat, AnimRepeat, AnimClamp));
	}

	constexpr auto VECTOR_LENGTH_THRESHOLD = 0.000001f;

	inline auto safe_normalise(auto input) {
		return glm::length2(input) < VECTOR_LENGTH_THRESHOLD ? v2f32(0) : glm::normalize(input);
	}

	using namespace Input;

	inline v2f32 keyboard_plane(KB::Key up, KB::Key left, KB::Key down, KB::Key right) {
		return safe_normalise(key_axis(left, right, down, up));
	}

	struct TopDownControl {
		f32 speed;
		f32 accel;
		f32 walk_cycle_duration;
		v2f32 input;
		f32 look_angle;
	};

	// rtf32 animate(TopDownControl& ctrl, AnimationGrid<rtf32, 3>& animation, f32 time) { //TODO speed parameter
	rtf32 animate(TopDownControl& ctrl, AnimationGrid<rtf32, 2>& animation, f32 time) {
		auto walking = glm::length(ctrl.input) > 0.1f;
		if (walking)
			ctrl.look_angle = glm::orientedAngle(v2f32(0, 1), ctrl.input);
		return animate_character_spritesheet(animation.keyframes, v3u32(animation.dimensions, 1),
			v3f32(
				walking ? time * ctrl.speed / ctrl.walk_cycle_duration : 0,
				ctrl.look_angle / (2 * glm::pi<f32>()),
				0 //TODO speed variation
			)
		);
	}

	void move_top_down(v2f32& velocity, v2f32 input, f32 speed, f32 accel) {
		auto target_velocity = input * speed;
		auto target_accel = target_velocity - velocity;
		auto effective_accel = safe_normalise(target_accel) * min(accel, glm::length(target_accel));
		velocity += effective_accel;
	}

}

bool EditorWidget(const cstr label, controls::TopDownControl& data) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("speed", data.speed);
		changed |= EditorWidget("accel", data.accel);
		changed |= EditorWidget("walk cycke duration", data.walk_cycle_duration);
		changed |= EditorWidget("input", data.input);
		changed |= EditorWidget("look angle", data.look_angle);
		ImGui::TreePop();
	}
	return changed;
}

#endif
