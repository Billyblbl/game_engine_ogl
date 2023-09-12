#ifndef GTOP_DOWN_CONTROLS
# define GTOP_DOWN_CONTROLS

#include <inputs.cpp>
#include <physics_2d.cpp>
#include <animation.cpp>
#include <imgui_extension.cpp>

namespace controls {
	using namespace Input;
	using namespace glm;

	// x = time
	// y = angle
	// z = speed

	constexpr auto VECTOR_LENGTH_THRESHOLD = 0.000001f;

	inline auto safe_normalise(auto input) { return length2(input) < VECTOR_LENGTH_THRESHOLD ? decltype(input)(0) : normalize(input); }
	inline v2f32 keyboard_plane(CompositeKeybind<2> keybinds = WASD) { return safe_normalise(key_axis(keybinds)); }

	struct TopDownControl {
		//stats
		f32 speed;
		f32 accel;
		f32 walk_cycle_duration; // should this be here ?

		//states
		v2f32 input;
		f32 look_angle;
		f32 velocity_mag;
	};

	v3f32 locomotion(TopDownControl& ctrl, f32 time) {
		auto walking = length(ctrl.input) > 0.01f;
		if (walking)
			ctrl.look_angle = orientedAngle(v2f32(0, 1), ctrl.input);
		return v3f32(
			walking ? time * ctrl.speed / ctrl.walk_cycle_duration : 0,
			ctrl.look_angle / (2 * pi<f32>()),
			ctrl.speed > 0 ? ctrl.velocity_mag / ctrl.speed : 0
		);
	}

	void animate_character(TopDownControl& ctrl, SpriteCursor* sprite, Shape2D* shape, SpriteCursor spritesheet, AnimationGrid<rtu32>* animation, AnimationGrid<Shape2D>* shape_animation, f32 time) {
		auto coord = locomotion(ctrl, time);
		auto config = LAnimationConfig{ AnimRepeat, AnimRepeat, AnimClamp };
		if (sprite) *sprite = sub_sprite(spritesheet, animate(*animation, coord, larray(config)));
		if (shape) *shape = animate(*shape_animation, coord, larray(config));
	}

	v2f32 move_top_down(v2f32 velocity, v2f32 input, f32 speed, f32 accel, f32 time) {
		auto target_velocity = input * speed;
		auto target_accel = target_velocity - velocity;
		auto effective_accel = safe_normalise(target_accel) * min(accel * time, length(target_accel));
		return velocity += effective_accel;
	}

	struct CharacterLocomotionAnimationData {
		AnimationGrid<rtu32> frames;
		AnimationGrid<Shape2D> shapes;
		SpriteCursor spritesheet;
	};

}

bool EditorWidget(const cstr label, controls::TopDownControl& data) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("speed", data.speed);
		changed |= EditorWidget("accel", data.accel);
		changed |= EditorWidget("walk cycke duration", data.walk_cycle_duration);
		changed |= EditorWidget("current velocity", data.velocity_mag);
		changed |= EditorWidget("input", data.input);
		changed |= EditorWidget("look angle", data.look_angle);
		ImGui::TreePop();
	}
	return changed;
}

#endif
