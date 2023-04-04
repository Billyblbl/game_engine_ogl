#ifndef GINPUTS
# define GINPUTS

#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <math.cpp>
#include <blblstd.hpp>

namespace Input {
	namespace Action {
		enum Type: u32 {
			Release = GLFW_RELEASE,
			Press = GLFW_PRESS,
			Repeat = GLFW_REPEAT
		};
	}

	namespace Keyboard {
		enum Key: i32 {
			UNKNOWN = GLFW_KEY_UNKNOWN,
			SPACE = GLFW_KEY_SPACE,
			FIRST = SPACE,
			APOSTROPHE = GLFW_KEY_APOSTROPHE,
			COMMA = GLFW_KEY_COMMA,
			MINUS = GLFW_KEY_MINUS,
			PERIOD = GLFW_KEY_PERIOD,
			SLASH = GLFW_KEY_SLASH,
			Key0 = GLFW_KEY_0,
			Key1 = GLFW_KEY_1,
			Key2 = GLFW_KEY_2,
			Key3 = GLFW_KEY_3,
			Key4 = GLFW_KEY_4,
			Key5 = GLFW_KEY_5,
			Key6 = GLFW_KEY_6,
			Key7 = GLFW_KEY_7,
			Key8 = GLFW_KEY_8,
			Key9 = GLFW_KEY_9,
			SEMICOLON = GLFW_KEY_SEMICOLON,
			EQUAL = GLFW_KEY_EQUAL,
			A = GLFW_KEY_A,
			B = GLFW_KEY_B,
			C = GLFW_KEY_C,
			D = GLFW_KEY_D,
			E = GLFW_KEY_E,
			F = GLFW_KEY_F,
			G = GLFW_KEY_G,
			H = GLFW_KEY_H,
			I = GLFW_KEY_I,
			J = GLFW_KEY_J,
			K = GLFW_KEY_K,
			L = GLFW_KEY_L,
			M = GLFW_KEY_M,
			N = GLFW_KEY_N,
			O = GLFW_KEY_O,
			P = GLFW_KEY_P,
			Q = GLFW_KEY_Q,
			R = GLFW_KEY_R,
			S = GLFW_KEY_S,
			T = GLFW_KEY_T,
			U = GLFW_KEY_U,
			V = GLFW_KEY_V,
			W = GLFW_KEY_W,
			X = GLFW_KEY_X,
			Y = GLFW_KEY_Y,
			Z = GLFW_KEY_Z,
			LEFT_BRACKET = GLFW_KEY_LEFT_BRACKET,
			BACKSLASH = GLFW_KEY_BACKSLASH,
			RIGHT_BRACKET = GLFW_KEY_RIGHT_BRACKET,
			GRAVE_ACCENT = GLFW_KEY_GRAVE_ACCENT,
			WORLD_1 = GLFW_KEY_WORLD_1,
			WORLD_2 = GLFW_KEY_WORLD_2,
			ESCAPE = GLFW_KEY_ESCAPE,
			ENTER = GLFW_KEY_ENTER,
			TAB = GLFW_KEY_TAB,
			BACKSPACE = GLFW_KEY_BACKSPACE,
			INSERT = GLFW_KEY_INSERT,
			DELETE = GLFW_KEY_DELETE,
			RIGHT = GLFW_KEY_RIGHT,
			LEFT = GLFW_KEY_LEFT,
			DOWN = GLFW_KEY_DOWN,
			UP = GLFW_KEY_UP,
			PAGE_UP = GLFW_KEY_PAGE_UP,
			PAGE_DOWN = GLFW_KEY_PAGE_DOWN,
			HOME = GLFW_KEY_HOME,
			END = GLFW_KEY_END,
			CAPS_LOCK = GLFW_KEY_CAPS_LOCK,
			SCROLL_LOCK = GLFW_KEY_SCROLL_LOCK,
			NUM_LOCK = GLFW_KEY_NUM_LOCK,
			PRINT_SCREEN = GLFW_KEY_PRINT_SCREEN,
			PAUSE = GLFW_KEY_PAUSE,
			F1 = GLFW_KEY_F1,
			F2 = GLFW_KEY_F2,
			F3 = GLFW_KEY_F3,
			F4 = GLFW_KEY_F4,
			F5 = GLFW_KEY_F5,
			F6 = GLFW_KEY_F6,
			F7 = GLFW_KEY_F7,
			F8 = GLFW_KEY_F8,
			F9 = GLFW_KEY_F9,
			F10 = GLFW_KEY_F10,
			F11 = GLFW_KEY_F11,
			F12 = GLFW_KEY_F12,
			F13 = GLFW_KEY_F13,
			F14 = GLFW_KEY_F14,
			F15 = GLFW_KEY_F15,
			F16 = GLFW_KEY_F16,
			F17 = GLFW_KEY_F17,
			F18 = GLFW_KEY_F18,
			F19 = GLFW_KEY_F19,
			F20 = GLFW_KEY_F20,
			F21 = GLFW_KEY_F21,
			F22 = GLFW_KEY_F22,
			F23 = GLFW_KEY_F23,
			F24 = GLFW_KEY_F24,
			F25 = GLFW_KEY_F25,
			KP_0 = GLFW_KEY_KP_0,
			KP_1 = GLFW_KEY_KP_1,
			KP_2 = GLFW_KEY_KP_2,
			KP_3 = GLFW_KEY_KP_3,
			KP_4 = GLFW_KEY_KP_4,
			KP_5 = GLFW_KEY_KP_5,
			KP_6 = GLFW_KEY_KP_6,
			KP_7 = GLFW_KEY_KP_7,
			KP_8 = GLFW_KEY_KP_8,
			KP_9 = GLFW_KEY_KP_9,
			KP_DECIMAL = GLFW_KEY_KP_DECIMAL,
			KP_DIVIDE = GLFW_KEY_KP_DIVIDE,
			KP_MULTIPLY = GLFW_KEY_KP_MULTIPLY,
			KP_SUBTRACT = GLFW_KEY_KP_SUBTRACT,
			KP_ADD = GLFW_KEY_KP_ADD,
			KP_ENTER = GLFW_KEY_KP_ENTER,
			KP_EQUAL = GLFW_KEY_KP_EQUAL,
			LEFT_SHIFT = GLFW_KEY_LEFT_SHIFT,
			LEFT_CONTROL = GLFW_KEY_LEFT_CONTROL,
			LEFT_ALT = GLFW_KEY_LEFT_ALT,
			LEFT_SUPER = GLFW_KEY_LEFT_SUPER,
			RIGHT_SHIFT = GLFW_KEY_RIGHT_SHIFT,
			RIGHT_CONTROL = GLFW_KEY_RIGHT_CONTROL,
			RIGHT_ALT = GLFW_KEY_RIGHT_ALT,
			RIGHT_SUPER = GLFW_KEY_RIGHT_SUPER,
			MENU = GLFW_KEY_MENU,
			LAST = GLFW_KEY_LAST,
			COUNT = LAST - FIRST + 1
		};

		int index_of(Key key) { return key - FIRST; }

		const Key All[] = {
			SPACE,
			APOSTROPHE,
			COMMA,
			MINUS,
			PERIOD,
			SLASH,
			Key0,
			Key1,
			Key2,
			Key3,
			Key4,
			Key5,
			Key6,
			Key7,
			Key8,
			Key9,
			SEMICOLON,
			EQUAL,
			A,
			B,
			C,
			D,
			E,
			F,
			G,
			H,
			I,
			J,
			K,
			L,
			M,
			N,
			O,
			P,
			Q,
			R,
			S,
			T,
			U,
			V,
			W,
			X,
			Y,
			Z,
			LEFT_BRACKET,
			BACKSLASH,
			RIGHT_BRACKET,
			GRAVE_ACCENT,
			WORLD_1,
			WORLD_2,
			ESCAPE,
			ENTER,
			TAB,
			BACKSPACE,
			INSERT,
			DELETE,
			RIGHT,
			LEFT,
			DOWN,
			UP,
			PAGE_UP,
			PAGE_DOWN,
			HOME,
			END,
			CAPS_LOCK,
			SCROLL_LOCK,
			NUM_LOCK,
			PRINT_SCREEN,
			PAUSE,
			F1,
			F2,
			F3,
			F4,
			F5,
			F6,
			F7,
			F8,
			F9,
			F10,
			F11,
			F12,
			F13,
			F14,
			F15,
			F16,
			F17,
			F18,
			F19,
			F20,
			F21,
			F22,
			F23,
			F24,
			F25,
			KP_0,
			KP_1,
			KP_2,
			KP_3,
			KP_4,
			KP_5,
			KP_6,
			KP_7,
			KP_8,
			KP_9,
			KP_DECIMAL,
			KP_DIVIDE,
			KP_MULTIPLY,
			KP_SUBTRACT,
			KP_ADD,
			KP_ENTER,
			KP_EQUAL,
			LEFT_SHIFT,
			LEFT_CONTROL,
			LEFT_ALT,
			LEFT_SUPER,
			RIGHT_SHIFT,
			RIGHT_CONTROL,
			RIGHT_ALT,
			RIGHT_SUPER,
			MENU,
		};
	}

	namespace Mouse {
		enum Button: u32 {
			LAST = GLFW_MOUSE_BUTTON_LAST,
			LEFT = GLFW_MOUSE_BUTTON_LEFT,
			RIGHT = GLFW_MOUSE_BUTTON_RIGHT,
			MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE,
			COUNT = LAST
		};
	}

	enum ButtonState: u8 {
		None = 0,
		Pressed = 1,
		Down = 2,
		Up = 4
	};

	ButtonState operator|=(ButtonState& a, ButtonState b) {
		return a = (ButtonState)(a | b);
	}

	inline ButtonState manual_update(ButtonState current, ButtonState polled) {
		ButtonState pressed = polled;
		auto down = ((polled & ButtonState::Pressed) & (current ^ ButtonState::Pressed)) << 1;
		auto up = ((polled ^ ButtonState::Pressed) & (current & ButtonState::Pressed)) << 2;
		return static_cast<ButtonState>(pressed | down | up);
	}

	namespace Gamepad {
		enum Button: u32 {
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

		enum Axis: u32 {
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
		ButtonState keyStates[Keyboard::COUNT];
		ButtonState mouseButtonStates[Mouse::COUNT];
		v2f64 mousePos = v2f64(0);
		v2f64 mouseDelta = v2f64(0);
		v2f64 scrollDelta = v2f64(0);
		struct {
			Array<u8> indices;
			Gamepad::State states[Gamepad::MaxGamepadCount];
		} gamepads;
	};

	void poll(Context& context) {
		//Reset states
		for (auto key : Keyboard::All)
			context.keyStates[Keyboard::index_of(key)] = ButtonState::None;
		for (auto i : u32xrange{ 0, Mouse::COUNT })
			context.keyStates[i] = ButtonState::None;
		context.mouseDelta = v2f64(0);
		context.scrollDelta = v2f64(0);

		glfwPollEvents();

		// Read pressed button states
		for (auto key : Keyboard::All) if (glfwGetKey(context.window, key) == Action::Press)
			context.keyStates[Keyboard::index_of(key)] |= ButtonState::Pressed;
		for (auto i : u32xrange{ 0, Mouse::COUNT }) if (glfwGetMouseButton(context.window, i))
			context.mouseButtonStates[i] |= ButtonState::Pressed;
		for (auto id : context.gamepads.indices)
			Gamepad::poll(id, context.gamepads.states[id]);
	}

	inline auto& get_context() {
		static Context context;
		return context;
	}

	inline f32 composite(ButtonState neg, ButtonState pos) {
		f32 value = 0.f;
		if (neg & ButtonState::Pressed)
			value -= 1.f;
		if (pos & ButtonState::Pressed)
			value += 1.f;
		return value;
	}

	inline v2f32 composite(ButtonState negH, ButtonState posH, ButtonState negV, ButtonState posV) {
		return v2f32(composite(negH, posH), composite(negV, posV));
	}

	inline v3f32 composite(ButtonState negH, ButtonState posH, ButtonState negV, ButtonState posV, ButtonState negD, ButtonState posD) {
		return v3f32(composite(negH, posH), composite(negV, posV), composite(negD, posD));
	}

	inline ButtonState get_key(Keyboard::Key key) { return get_context().keyStates[Keyboard::index_of(key)]; }

	inline f32 key_axis(Keyboard::Key neg, Keyboard::Key pos) {
		return composite(get_key(neg), get_key(pos));
	}

	inline v2f32 key_axis(Keyboard::Key negH, Keyboard::Key posH, Keyboard::Key negV, Keyboard::Key posV) {
		return composite(
			get_key(negH), get_key(posH),
			get_key(negV), get_key(posV)
		);
	}

	inline v3f32 key_axis(Keyboard::Key negH, Keyboard::Key posH, Keyboard::Key negV, Keyboard::Key posV, Keyboard::Key negD, Keyboard::Key posD) {
		return composite(
			get_key(negH), get_key(posH),
			get_key(negV), get_key(posV),
			get_key(negD), get_key(posD)
		);
	}

	void context_key_callback(
		GLFWwindow* window,
		Keyboard::Key key,
		int scancode,
		Action::Type action,
		int mods
	) {
		if (action != Action::Press && action != Action::Release) return;
		auto context = get_context();
		if (action != Action::Press)
			context.keyStates[Keyboard::index_of(key)] |= ButtonState::Down;
		else if (action != Action::Release)
			context.keyStates[Keyboard::index_of(key)] |= ButtonState::Up;
	}

	void context_mouse_button_callback(
		GLFWwindow* window,
		Mouse::Button button,
		Action::Type action,
		int mods
	) {
		if (action != Action::Press && action != Action::Release) return;
		auto context = get_context();
		if (action != Action::Press)
			context.mouseButtonStates[button] |= ButtonState::Down;
		else if (action != Action::Release)
			context.mouseButtonStates[button] |= ButtonState::Up;
	}

	void context_mouse_pos_callback(GLFWwindow* window, double x, double y) {
		auto context = get_context();
		auto newPos = v2f64(x, y);
		context.mouseDelta += newPos - context.mousePos;
		context.mousePos = newPos;
	}

	void context_scroll_callback(GLFWwindow* window, double x, double y) {
		get_context().scrollDelta += glm::dvec2(x, y);
	}

	Array<u8>	get_gamepads() {
		static u8 dest[Gamepad::MaxGamepadCount];
		auto list = List{ larray(dest), 0 };
		for (u8 i : u8xrange{ 0, Gamepad::MaxGamepadCount }) {
			if (glfwJoystickIsGamepad(i)) {
				list.push(i);
			}
		}
		return list.allocated();
	}

	Context& init_context(GLFWwindow* window) {
		auto& newContext = get_context();
		newContext.window = window;
		memset(newContext.keyStates, 0, sizeof(newContext.keyStates));
		memset(newContext.mouseButtonStates, 0, sizeof(newContext.mouseButtonStates));

		glfwSetKeyCallback(window, (GLFWkeyfun)&context_key_callback);
		glfwSetMouseButtonCallback(window, (GLFWmousebuttonfun)&context_mouse_button_callback);
		glfwGetCursorPos(window, &newContext.mousePos.x, &newContext.mousePos.y);
		glfwSetCursorPosCallback(window, (GLFWcursorposfun)&context_mouse_pos_callback);
		glfwSetScrollCallback(window, (GLFWscrollfun)&context_scroll_callback);

		newContext.gamepads.indices = get_gamepads();
		for (auto id : newContext.gamepads.indices)
			glfwGetGamepadState(id, (GLFWgamepadstate*)&newContext.gamepads.states[id]);
		return newContext;
	}
}

#endif
