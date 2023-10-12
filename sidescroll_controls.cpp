#ifndef GSIDESCROLL_CONTROLS
# define GSIDESCROLL_CONTROLS

#include <blblstd.hpp>
#include <inputs.cpp>
#include <animation.cpp>
#include <physics_2d.cpp>
#include <imgui_extension.cpp>
#include <spritesheet.cpp>

struct SidescrollControl {
	f32 speed = 10;
	f32 accel = 100;
	f32 jump_force = 10;
	f32 fall_multiplier = 1;
	f32 max_slope = glm::pi<f32>() / 4.f;

	union {
		u8 locomotion;
		struct {
			bool grounded : 1;
			bool look_left : 1;
			bool falling : 1;
		};
	};

	v2f32 movement;
	union {
		struct {
			Input::ButtonState jump;
			Input::ButtonState attack;
			Input::ButtonState heavy;
			Input::ButtonState special;
		};
		Input::CompositeButtonState<4> all;
	} actions;

	struct PlayerBindings {
		u8 device;
		Input::AxisSourceD<2> movement;
		union {
			struct {
				Input::ButtonSource jump;
				Input::ButtonSource attack;
				Input::ButtonSource heavy;
				Input::ButtonSource special;
			};
			Input::ButtonSourceD<4> all;
		} actions;
	};

	static const PlayerBindings& default_bindings() {
		using namespace Input;
		static PlayerBindings bd = (
			[]() {
				PlayerBindings bd;
				bd.device = 0;
				bd.movement = LS;
				bd.actions.all = make_source(GP::A, GP::X, GP::Y, GP::B);
				return bd;
			}
		());
		return bd;
	};
};

bool EditorWidget(const cstr label, SidescrollControl& ctrl) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("speed", ctrl.speed);
		changed |= EditorWidget("accel", ctrl.accel);
		changed |= EditorWidget("jump_force", ctrl.jump_force);
		changed |= EditorWidget("fall_multiplier", ctrl.fall_multiplier);
		{
			auto mxsl = glm::degrees(ctrl.max_slope);
			changed |= EditorWidget("max_slope", mxsl);
			ctrl.max_slope = glm::radians(mxsl);
		}

		{
			bool grounded = ctrl.grounded;
			changed |= EditorWidget("grounded", grounded);
			ctrl.grounded = grounded;
		}

		{
			bool look_left = ctrl.look_left;
			changed |= EditorWidget("look_left", look_left);
			ctrl.look_left = look_left;
		}

		{
			bool falling = ctrl.falling;
			changed |= EditorWidget("falling", falling);
			ctrl.falling = falling;
		}

		changed |= EditorWidget("movement", ctrl.movement);
	}
	return changed;
}

void player_input(SidescrollControl& ctrl, const SidescrollControl::PlayerBindings& bindings = SidescrollControl::default_bindings()) {
	using namespace Input;
	ctrl.movement = poll(bindings.device, bindings.movement);//? inverted Y by default ??? why ? how ? wtf?
	ctrl.actions.all = poll(bindings.device, bindings.actions.all);
}

v2f32 control(SidescrollControl& ctrl, v2f32& velocity, v2f32& scale, v2f32 gravity, f32 time) {
	using namespace glm;
	using namespace Input;
	f32 target_vel = ctrl.movement.x * ctrl.speed;
	f32 target_accel = target_vel - velocity.x;
	velocity.x += clamp(target_accel, -ctrl.accel * time, ctrl.accel * time);
	if (ctrl.grounded && (ctrl.actions.jump & Input::ButtonState::Down))
		velocity.y = ctrl.jump_force;
	else if (!ctrl.grounded && (!(ctrl.actions.jump & ButtonState::Pressed) || velocity.y < 0)) //* assumes gravity down
		ctrl.falling = true;
	if (ctrl.movement.x > +0.1 && ctrl.look_left) {
		ctrl.look_left = false;
		scale.x *= -1;
	} else if (ctrl.movement.x < -0.1 && !ctrl.look_left) {
		ctrl.look_left = true;
		scale.x *= -1;
	}
	return velocity;
}


Array<SpriteAnimation> build_sidescroll_character_animations(Alloc allocator, const SpritesheetLayout& layout, v2u32 texture_dimensions) {
	return build_animations(allocator, layout, texture_dimensions,
		{
		"locomotion",
		"idle",
		"jump",
		"fall",
		}
	);
}

rtu32 animate_sidescroll_character(Animator& animator, Array<SpriteAnimation> animations, const SidescrollControl& ctrl, f32 time) {
	enum {
		LOCOMOTION = 0,
		IDLE,
		JUMP,
		FALL,
		STATE_COUNT
	};
	assert(animations.size() >= STATE_COUNT);

	u32 state = animator.select_state(
		[&]()->u32 {
			if (ctrl.falling) return FALL;
			else if (ctrl.actions.jump & Input::Pressed) return JUMP;
			else if (abs(ctrl.movement.x) > 0.0001) return LOCOMOTION;
			else return IDLE;
		}
	(), time);
	switch (state) {
	case LOCOMOTION: return animate(animations[state], v2f32(animator.state_time(time), abs(ctrl.movement.x)));
	case JUMP: return animate(animations[state], v1f32(animator.state_time(time)));
	case FALL: return animate(animations[state], v1f32(animator.state_time(time)));
	case IDLE: return animate(animations[state], v1f32(animator.state_time(time)));
	default: return (assert(("Unimplemented state type", false)), rtu32{});
	}
}

#endif
