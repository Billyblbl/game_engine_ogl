#ifndef GTIME
# define GTIME

#include <blblstd.hpp>
#include <chrono>
#include <imgui_extension.cpp>

namespace Time {

	using namespace std::chrono;
	using moment = time_point<steady_clock>;
	using time = duration<f32>;

	struct Timer {
		f32 duration = 0;
		f32 start = xf32::max();
		bool enable(f32 t) { return over(start = t); }
		bool over(f32 t) { return start + duration < t; }
	};

	struct Clock {
		f32 dt;
		f32 current;
		moment start;
		moment this_frame;
	};

	inline Clock& update(Clock& clock) {
		moment new_frame = steady_clock::now();
		clock.dt = duration_cast<time>(new_frame - clock.this_frame).count();
		clock.current = duration_cast<time>(new_frame - clock.start).count();
		clock.this_frame = new_frame;
		return clock;
	}

	inline Clock start() {
		Clock clock;
		clock.start = (clock.this_frame = steady_clock::now());
		clock.current = 0;
		clock.dt = 0;
		return clock;
	}

	inline f32 timer(f32 start, f32 duration) { return start + duration; }

	inline bool metronome(f32 current, f32 tick_delay, f32& next_tick) {
		assert(tick_delay > 0.0000f);
		bool should_tick = current > next_tick;
		if (should_tick)
			next_tick = timer(next_tick, tick_delay);
		return should_tick;
	}


} // namespace Time

bool EditorWidget(const cstr label, Time::Clock& clock, bool foldable = true) {
	bool changed = false;
	if (!foldable || ImGui::TreeNode(label)) {
		if (!foldable)
			ImGui::Text(label);
		changed |= EditorWidget("Current", clock.current);
		changed |= EditorWidget("Delta time", clock.dt);
		if (foldable)
			ImGui::TreePop();
	}
	return changed;
}

bool EditorWidget(const cstr label, Time::Timer& timer, bool foldable = true) {
	bool changed = false;
	if (!foldable || ImGui::TreeNode(label)) {
		if (!foldable)
			ImGui::Text(label);
		changed |= EditorWidget("Duration", timer.duration);
		changed |= EditorWidget("Start", timer.start);
		if (foldable)
			ImGui::TreePop();
	}
	return changed;
}

#endif
