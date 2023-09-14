#ifndef GINPUTS
# define GINPUTS

#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <math.cpp>
#include <blblstd.hpp>

namespace Input {
	namespace Action {
		enum Type : u32 {
			Release = GLFW_RELEASE,
			Press = GLFW_PRESS,
			Repeat = GLFW_REPEAT
		};
	}

	namespace KB {
		enum Key : i32 {
			K_UNKNOWN = GLFW_KEY_UNKNOWN,
			K_SPACE = GLFW_KEY_SPACE,
			K_FIRST = K_SPACE,
			K_APOSTROPHE = GLFW_KEY_APOSTROPHE,
			K_COMMA = GLFW_KEY_COMMA,
			K_MINUS = GLFW_KEY_MINUS,
			K_PERIOD = GLFW_KEY_PERIOD,
			K_SLASH = GLFW_KEY_SLASH,
			K_Key0 = GLFW_KEY_0,
			K_Key1 = GLFW_KEY_1,
			K_Key2 = GLFW_KEY_2,
			K_Key3 = GLFW_KEY_3,
			K_Key4 = GLFW_KEY_4,
			K_Key5 = GLFW_KEY_5,
			K_Key6 = GLFW_KEY_6,
			K_Key7 = GLFW_KEY_7,
			K_Key8 = GLFW_KEY_8,
			K_Key9 = GLFW_KEY_9,
			K_SEMICOLON = GLFW_KEY_SEMICOLON,
			K_EQUAL = GLFW_KEY_EQUAL,
			K_A = GLFW_KEY_A,
			K_B = GLFW_KEY_B,
			K_C = GLFW_KEY_C,
			K_D = GLFW_KEY_D,
			K_E = GLFW_KEY_E,
			K_F = GLFW_KEY_F,
			K_G = GLFW_KEY_G,
			K_H = GLFW_KEY_H,
			K_I = GLFW_KEY_I,
			K_J = GLFW_KEY_J,
			K_K = GLFW_KEY_K,
			K_L = GLFW_KEY_L,
			K_M = GLFW_KEY_M,
			K_N = GLFW_KEY_N,
			K_O = GLFW_KEY_O,
			K_P = GLFW_KEY_P,
			K_Q = GLFW_KEY_Q,
			K_R = GLFW_KEY_R,
			K_S = GLFW_KEY_S,
			K_T = GLFW_KEY_T,
			K_U = GLFW_KEY_U,
			K_V = GLFW_KEY_V,
			K_W = GLFW_KEY_W,
			K_X = GLFW_KEY_X,
			K_Y = GLFW_KEY_Y,
			K_Z = GLFW_KEY_Z,
			K_LEFT_BRACKET = GLFW_KEY_LEFT_BRACKET,
			K_BACKSLASH = GLFW_KEY_BACKSLASH,
			K_RIGHT_BRACKET = GLFW_KEY_RIGHT_BRACKET,
			K_GRAVE_ACCENT = GLFW_KEY_GRAVE_ACCENT,
			K_WORLD_1 = GLFW_KEY_WORLD_1,
			K_WORLD_2 = GLFW_KEY_WORLD_2,
			K_ESCAPE = GLFW_KEY_ESCAPE,
			K_ENTER = GLFW_KEY_ENTER,
			K_TAB = GLFW_KEY_TAB,
			K_BACKSPACE = GLFW_KEY_BACKSPACE,
			K_INSERT = GLFW_KEY_INSERT,
			K_DELETE = GLFW_KEY_DELETE,
			K_RIGHT = GLFW_KEY_RIGHT,
			K_LEFT = GLFW_KEY_LEFT,
			K_DOWN = GLFW_KEY_DOWN,
			K_UP = GLFW_KEY_UP,
			K_PAGE_UP = GLFW_KEY_PAGE_UP,
			K_PAGE_DOWN = GLFW_KEY_PAGE_DOWN,
			K_HOME = GLFW_KEY_HOME,
			K_END = GLFW_KEY_END,
			K_CAPS_LOCK = GLFW_KEY_CAPS_LOCK,
			K_SCROLL_LOCK = GLFW_KEY_SCROLL_LOCK,
			K_NUM_LOCK = GLFW_KEY_NUM_LOCK,
			K_PRINT_SCREEN = GLFW_KEY_PRINT_SCREEN,
			K_PAUSE = GLFW_KEY_PAUSE,
			K_F1 = GLFW_KEY_F1,
			K_F2 = GLFW_KEY_F2,
			K_F3 = GLFW_KEY_F3,
			K_F4 = GLFW_KEY_F4,
			K_F5 = GLFW_KEY_F5,
			K_F6 = GLFW_KEY_F6,
			K_F7 = GLFW_KEY_F7,
			K_F8 = GLFW_KEY_F8,
			K_F9 = GLFW_KEY_F9,
			K_F10 = GLFW_KEY_F10,
			K_F11 = GLFW_KEY_F11,
			K_F12 = GLFW_KEY_F12,
			K_F13 = GLFW_KEY_F13,
			K_F14 = GLFW_KEY_F14,
			K_F15 = GLFW_KEY_F15,
			K_F16 = GLFW_KEY_F16,
			K_F17 = GLFW_KEY_F17,
			K_F18 = GLFW_KEY_F18,
			K_F19 = GLFW_KEY_F19,
			K_F20 = GLFW_KEY_F20,
			K_F21 = GLFW_KEY_F21,
			K_F22 = GLFW_KEY_F22,
			K_F23 = GLFW_KEY_F23,
			K_F24 = GLFW_KEY_F24,
			K_F25 = GLFW_KEY_F25,
			K_KP_0 = GLFW_KEY_KP_0,
			K_KP_1 = GLFW_KEY_KP_1,
			K_KP_2 = GLFW_KEY_KP_2,
			K_KP_3 = GLFW_KEY_KP_3,
			K_KP_4 = GLFW_KEY_KP_4,
			K_KP_5 = GLFW_KEY_KP_5,
			K_KP_6 = GLFW_KEY_KP_6,
			K_KP_7 = GLFW_KEY_KP_7,
			K_KP_8 = GLFW_KEY_KP_8,
			K_KP_9 = GLFW_KEY_KP_9,
			K_KP_DECIMAL = GLFW_KEY_KP_DECIMAL,
			K_KP_DIVIDE = GLFW_KEY_KP_DIVIDE,
			K_KP_MULTIPLY = GLFW_KEY_KP_MULTIPLY,
			K_KP_SUBTRACT = GLFW_KEY_KP_SUBTRACT,
			K_KP_ADD = GLFW_KEY_KP_ADD,
			K_KP_ENTER = GLFW_KEY_KP_ENTER,
			K_KP_EQUAL = GLFW_KEY_KP_EQUAL,
			K_LEFT_SHIFT = GLFW_KEY_LEFT_SHIFT,
			K_LEFT_CONTROL = GLFW_KEY_LEFT_CONTROL,
			K_LEFT_ALT = GLFW_KEY_LEFT_ALT,
			K_LEFT_SUPER = GLFW_KEY_LEFT_SUPER,
			K_RIGHT_SHIFT = GLFW_KEY_RIGHT_SHIFT,
			K_RIGHT_CONTROL = GLFW_KEY_RIGHT_CONTROL,
			K_RIGHT_ALT = GLFW_KEY_RIGHT_ALT,
			K_RIGHT_SUPER = GLFW_KEY_RIGHT_SUPER,
			K_MENU = GLFW_KEY_MENU,
			K_LAST = GLFW_KEY_LAST,
			K_COUNT = K_LAST - K_FIRST + 1
		};

		int index_of(Key key) { return key - K_FIRST; }

		const Key All[] = {
			K_SPACE,
			K_APOSTROPHE,
			K_COMMA,
			K_MINUS,
			K_PERIOD,
			K_SLASH,
			K_Key0,
			K_Key1,
			K_Key2,
			K_Key3,
			K_Key4,
			K_Key5,
			K_Key6,
			K_Key7,
			K_Key8,
			K_Key9,
			K_SEMICOLON,
			K_EQUAL,
			K_A,
			K_B,
			K_C,
			K_D,
			K_E,
			K_F,
			K_G,
			K_H,
			K_I,
			K_J,
			K_K,
			K_L,
			K_M,
			K_N,
			K_O,
			K_P,
			K_Q,
			K_R,
			K_S,
			K_T,
			K_U,
			K_V,
			K_W,
			K_X,
			K_Y,
			K_Z,
			K_LEFT_BRACKET,
			K_BACKSLASH,
			K_RIGHT_BRACKET,
			K_GRAVE_ACCENT,
			K_WORLD_1,
			K_WORLD_2,
			K_ESCAPE,
			K_ENTER,
			K_TAB,
			K_BACKSPACE,
			K_INSERT,
			K_DELETE,
			K_RIGHT,
			K_LEFT,
			K_DOWN,
			K_UP,
			K_PAGE_UP,
			K_PAGE_DOWN,
			K_HOME,
			K_END,
			K_CAPS_LOCK,
			K_SCROLL_LOCK,
			K_NUM_LOCK,
			K_PRINT_SCREEN,
			K_PAUSE,
			K_F1,
			K_F2,
			K_F3,
			K_F4,
			K_F5,
			K_F6,
			K_F7,
			K_F8,
			K_F9,
			K_F10,
			K_F11,
			K_F12,
			K_F13,
			K_F14,
			K_F15,
			K_F16,
			K_F17,
			K_F18,
			K_F19,
			K_F20,
			K_F21,
			K_F22,
			K_F23,
			K_F24,
			K_F25,
			K_KP_0,
			K_KP_1,
			K_KP_2,
			K_KP_3,
			K_KP_4,
			K_KP_5,
			K_KP_6,
			K_KP_7,
			K_KP_8,
			K_KP_9,
			K_KP_DECIMAL,
			K_KP_DIVIDE,
			K_KP_MULTIPLY,
			K_KP_SUBTRACT,
			K_KP_ADD,
			K_KP_ENTER,
			K_KP_EQUAL,
			K_LEFT_SHIFT,
			K_LEFT_CONTROL,
			K_LEFT_ALT,
			K_LEFT_SUPER,
			K_RIGHT_SHIFT,
			K_RIGHT_CONTROL,
			K_RIGHT_ALT,
			K_RIGHT_SUPER,
			K_MENU,
		};
	}

	namespace Mouse {
		enum Button : u32 {
			LAST = GLFW_MOUSE_BUTTON_LAST,
			LEFT = GLFW_MOUSE_BUTTON_LEFT,
			RIGHT = GLFW_MOUSE_BUTTON_RIGHT,
			MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE,
			COUNT = LAST
		};
	}

	enum ButtonState : u8 {
		None = 0,
		Pressed = 1,
		Down = 2,
		Up = 4
	};

	ButtonState operator|=(ButtonState& a, ButtonState b) {
		return (a = (ButtonState)(a | b));
	}

	inline ButtonState manual_update(ButtonState current, ButtonState polled) {
		ButtonState pressed = polled;
		auto down = ((polled & ButtonState::Pressed) & (current ^ ButtonState::Pressed)) << 1;
		auto up = ((polled ^ ButtonState::Pressed) & (current & ButtonState::Pressed)) << 2;
		return static_cast<ButtonState>(pressed | down | up);
	}

	namespace GP {
		enum Button : u32 {
			A = GLFW_GAMEPAD_BUTTON_A,
			B = GLFW_GAMEPAD_BUTTON_B,
			X = GLFW_GAMEPAD_BUTTON_X,
			Y = GLFW_GAMEPAD_BUTTON_Y,
			LEFT_BUMPER = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,
			RIGHT_BUMPER = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
			BACK = GLFW_GAMEPAD_BUTTON_BACK,
			START = GLFW_GAMEPAD_BUTTON_START,
			GUIDE = GLFW_GAMEPAD_BUTTON_GUIDE,
			LEFT_THUMB = GLFW_GAMEPAD_BUTTON_LEFT_THUMB,
			RIGHT_THUMB = GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,
			DPAD_UP = GLFW_GAMEPAD_BUTTON_DPAD_UP,
			DPAD_RIGHT = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
			DPAD_DOWN = GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
			DPAD_LEFT = GLFW_GAMEPAD_BUTTON_DPAD_LEFT,

			LAST = GLFW_GAMEPAD_BUTTON_LAST,
			B_COUNT = LAST,

			CROSS = GLFW_GAMEPAD_BUTTON_CROSS,
			CIRCLE = GLFW_GAMEPAD_BUTTON_CIRCLE,
			SQUARE = GLFW_GAMEPAD_BUTTON_SQUARE,
			TRIANGLE = GLFW_GAMEPAD_BUTTON_TRIANGLE
		};

		enum Axis : u32 {
			LEFT_X = GLFW_GAMEPAD_AXIS_LEFT_X,
			LEFT_Y = GLFW_GAMEPAD_AXIS_LEFT_Y,
			RIGHT_X = GLFW_GAMEPAD_AXIS_RIGHT_X,
			RIGHT_Y = GLFW_GAMEPAD_AXIS_RIGHT_Y,
			LEFT_TRIGGER = GLFW_GAMEPAD_AXIS_LEFT_TRIGGER,
			RIGHT_TRIGGER = GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER,
			A_COUNT = GLFW_GAMEPAD_AXIS_LAST
		};

		const Button All[] = {
			A,
			B,
			X,
			Y,
			LEFT_BUMPER,
			RIGHT_BUMPER,
			BACK,
			START,
			GUIDE,
			LEFT_THUMB,
			RIGHT_THUMB,
			DPAD_UP,
			DPAD_RIGHT,
			DPAD_DOWN,
			DPAD_LEFT
		};

		constexpr u8 MaxGamepadCount = GLFW_JOYSTICK_LAST;

		struct State {
			ButtonState buttons[array_size(GLFWgamepadstate{}.buttons)];
			f32 axises[array_size(GLFWgamepadstate{}.axes)];
		};

		void poll(u8 id, State& state) {
			State new_state;
			glfwGetGamepadState(id, (GLFWgamepadstate*)&new_state);
			for (auto i : u16xrange{ 0, array_size(new_state.buttons) })
				new_state.buttons[i] = manual_update(state.buttons[i], new_state.buttons[i]);
			state = new_state;
		}

		static_assert(sizeof(State) == sizeof(GLFWgamepadstate));
	}

	struct Context {
		GLFWwindow* window = nullptr;
		ButtonState key_states[KB::K_COUNT];
		ButtonState mouse_button_states[Mouse::COUNT];
		v2f64 mouse_pos = v2f64(0);
		v2f64 mouse_delta = v2f64(0);
		v2f64 scroll_delta = v2f64(0);
		struct {
			Array<u8> indices;
			GP::State states[GP::MaxGamepadCount];
		} gamepads;
	};

	void poll(Context& context) {
		//Reset states
		for (auto key : KB::All)
			context.key_states[KB::index_of(key)] = ButtonState::None;
		for (auto i : u32xrange{ 0, Mouse::COUNT })
			context.key_states[i] = ButtonState::None;
		context.mouse_delta = v2f64(0);
		context.scroll_delta = v2f64(0);

		glfwPollEvents();

		// Read pressed button states
		for (auto key : KB::All) if (glfwGetKey(context.window, key) == Action::Press)
			context.key_states[KB::index_of(key)] |= ButtonState::Pressed;
		for (auto i : u32xrange{ 0, Mouse::COUNT }) if (glfwGetMouseButton(context.window, i))
			context.mouse_button_states[i] |= ButtonState::Pressed;
		for (auto id : context.gamepads.indices)
			GP::poll(id, context.gamepads.states[id]);
	}

	inline auto& get_context() {
		static Context context;
		return context;
	}

	template<i32 D> using CompositeButtonState = glm::vec<D, ButtonState>;

	inline f32 composite(ButtonState neg, ButtonState pos) {
		f32 value = 0.f;
		if (neg & ButtonState::Pressed)
			value -= 1.f;
		if (pos & ButtonState::Pressed)
			value += 1.f;
		return value;
	}

	template<i32 D> inline glm::vec<D, f32> composite(CompositeButtonState<D> state) {
		glm::vec<D, f32> input;
		for (auto i : i32xrange{ 0, D })
			input[i] = composite(state[i].neg, state[i].pos);
		return input;
	}


	void context_key_callback(
		GLFWwindow* window,
		KB::Key key,
		int scancode,
		Action::Type action,
		int mods
	) {
		if (action != Action::Press && action != Action::Release) return;
		auto& context = get_context();
		if (action == Action::Press) {
			context.key_states[KB::index_of(key)] |= ButtonState::Down;
		} else if (action == Action::Release) {
			context.key_states[KB::index_of(key)] |= ButtonState::Up;
		}
	}

	void context_mouse_button_callback(
		GLFWwindow* window,
		Mouse::Button button,
		Action::Type action,
		int mods
	) {
		if (action != Action::Press && action != Action::Release) return;
		auto& context = get_context();
		if (action != Action::Press)
			context.mouse_button_states[button] |= ButtonState::Down;
		else if (action != Action::Release)
			context.mouse_button_states[button] |= ButtonState::Up;
	}

	void context_mouse_pos_callback(GLFWwindow* window, double x, double y) {
		auto& context = get_context();
		auto newPos = v2f64(x, y);
		context.mouse_delta += newPos - context.mouse_pos;
		context.mouse_pos = newPos;
	}

	void context_scroll_callback(GLFWwindow* window, double x, double y) {
		get_context().scroll_delta += glm::dvec2(x, y);
	}

	Array<u8>	get_gamepads() {
		static u8 dest[GP::MaxGamepadCount];
		auto list = List{ larray(dest), 0 };
		for (u8 i : u8xrange{ 0, GP::MaxGamepadCount }) {
			if (glfwJoystickIsGamepad(i)) {
				list.push(i);
			}
		}
		return list.allocated();
	}

	Context& init_context(GLFWwindow* window) {
		auto& newContext = get_context();
		newContext.window = window;
		memset(newContext.key_states, 0, sizeof(newContext.key_states));
		memset(newContext.mouse_button_states, 0, sizeof(newContext.mouse_button_states));

		glfwSetKeyCallback(window, (GLFWkeyfun)&context_key_callback);
		glfwSetMouseButtonCallback(window, (GLFWmousebuttonfun)&context_mouse_button_callback);
		glfwGetCursorPos(window, &newContext.mouse_pos.x, &newContext.mouse_pos.y);
		glfwSetCursorPosCallback(window, (GLFWcursorposfun)&context_mouse_pos_callback);
		glfwSetScrollCallback(window, (GLFWscrollfun)&context_scroll_callback);

		newContext.gamepads.indices = get_gamepads();
		for (auto id : newContext.gamepads.indices)
			glfwGetGamepadState(id, (GLFWgamepadstate*)&newContext.gamepads.states[id]);
		return newContext;
	}

	namespace KB {
		bool shortcut(Array<const Key> keys, bool* selected = null) {
			auto downed = false;
			for (auto key : keys) {
				auto state = get_context().key_states[index_of(key)];
				if ((state & Pressed) == 0)
					return false;
				if ((state & Down))
					downed = true;
			}
			if (downed && selected)
				*selected = !(*selected);
			return downed;
		}

		bool shortcut(LiteralArray<Key> keys, bool* selected = null) { return shortcut(larray(keys), selected); }

	}

	struct ButtonSource {
		enum Type { None, KB_Key, GP_Button } type;
		union { KB::Key key; GP::Button button; };
	};
	template<i32 D> using ButtonSourceD = glm::vec<D, ButtonSource>;

	struct AxisSource {
		enum Type { None, GP_Axis, Button_Axis } type;
		union { GP::Axis axis; ButtonSourceD<2> button_axis; };
	};
	template<i32 D> using AxisSourceD = glm::vec<D, AxisSource>;

	GP::State& get_gamepad(u8 device) {
		if (i32 index = linear_search(get_context().gamepads.indices, device); index < 0)
			return fail_ret("Invalid input device", get_context().gamepads.states[0]);
		else
			return get_context().gamepads.states[index];
	}

	ButtonState poll(u8 device, const ButtonSource& source) {
		auto key = [&]() {return get_context().key_states[KB::index_of(source.key)]; };
		auto button = [&]() {return get_gamepad(device).buttons[source.button];};

		switch (source.type) {
		case ButtonSource::KB_Key: return key();
		case ButtonSource::GP_Button: return button();
		default: return fail_ret("Invalid input source for button, only accepts KB_key or GP_button", ButtonState::None);
		}
	}

	f32 poll(u8 device, const AxisSource& source) {
		auto comp = [&]() { return composite(poll(device, source.button_axis[0]), poll(device, source.button_axis[1])); };
		auto axis = [&]() { return get_gamepad(device).axises[source.axis]; };

		switch (source.type) {
		case AxisSource::Button_Axis: return comp();
		case AxisSource::GP_Axis: return axis();
		default: return fail_ret("Invalid input source for single axis, only accpts KB_C_Key, GP_Axis, GP_C_Button", ButtonState::None);
		}
	}

	template<i32 D> CompositeButtonState<D> poll(u8 device, const ButtonSourceD<D>& bindings) {
		CompositeButtonState<D> res;
		for (auto i : u64xrange{ 0, D })
			res[i] = poll(device, bindings[i]);
		return res;
	}

	template<i32 D> glm::vec<D, f32> poll(u8 device, const AxisSourceD<D>& bindings) {
		glm::vec<D, f32> res;
		for (auto i : u64xrange{ 0, D })
			res[i] = poll(device, bindings[i]);
		return res;
	}

	ButtonSource make_source(GP::Button button) {
		ButtonSource source;
		source.type = ButtonSource::GP_Button;
		source.button = button;
		return source;
	}

	ButtonSource make_source(KB::Key key) {
		ButtonSource source;
		source.type = ButtonSource::KB_Key;
		source.key = key;
		return source;
	}

	AxisSource make_source(GP::Axis axis) {
		AxisSource source;
		source.type = AxisSource::GP_Axis;
		source.axis = axis;
		return source;
	}

	AxisSource make_source(ButtonSourceD<2> button_axis) {
		AxisSource source;
		source.type = AxisSource::Button_Axis;
		source.button_axis = button_axis;
		return source;
	}

	template<typename... T> auto make_source(const T&&... sources) {
		constexpr usize size = sizeof...(sources);
		using tuple_type = tuple<T...>;
		using res_type = decltype(make_source(std::get<0>(tuple_type{})));
		using vec = glm::vec<size, res_type>;
		return vec(make_source(sources)...);
	}

	const AxisSourceD<2> LS = make_source(GP::LEFT_X, GP::LEFT_Y);
	const AxisSourceD<2> RS = make_source(GP::RIGHT_X, GP::RIGHT_Y);
	const AxisSourceD<2> WASD = make_source(make_source(KB::K_A, KB::K_D), make_source(KB::K_S, KB::K_W));
	const AxisSourceD<2> ZQSD = make_source(make_source(KB::K_Q, KB::K_D), make_source(KB::K_S, KB::K_Z));
	const AxisSourceD<2> Arrows = make_source(make_source(KB::K_LEFT, KB::K_RIGHT), make_source(KB::K_DOWN, KB::K_UP));
}

#endif
