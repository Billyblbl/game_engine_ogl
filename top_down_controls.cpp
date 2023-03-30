#ifndef GTOP_DOWN_CONTROLS
# define GTOP_DOWN_CONTROLS

#include <inputs.cpp>
#include <physics2D.cpp>

namespace controls {

	constexpr auto VECTOR_LENGTH_THRESHOLD = 0.000001f;

	inline auto safe_normalise(auto input) {
		return glm::length(input) < VECTOR_LENGTH_THRESHOLD ? v2f32(0) : glm::normalize(input);
	}

	using namespace Input;

	inline v2f32 keyboard_plane(
		Keyboard::Key up,
		Keyboard::Key left,
		Keyboard::Key down,
		Keyboard::Key right
	) {
		return safe_normalise(key_axis(left, right, down, up));
	}

	void move_top_down(b2Body* body, v2f32 input, f32 speed, f32 accel) {
		auto velocity = b2d_to_glm(body->GetLinearVelocity());
		auto target_velocity = input * speed;
		auto target_accel = target_velocity - velocity;
		auto effective_accel = safe_normalise(target_accel) * min(accel, glm::length(target_accel));
		body->SetLinearVelocity(glm_to_b2d(velocity + effective_accel));
	}

}

#endif
