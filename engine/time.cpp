#ifndef GTIME
# define GTIME

#include <blblstd.hpp>
#include <chrono>
#include <imgui_extension.cpp>

namespace Time {

	struct Timer {
		f32 duration = 0;
		f32 start = xf32::max();
		bool enable(f32 t) { return over(start = t); }
		bool over(f32 t) { return start + duration < t; }
	};

	using namespace std::chrono;
	using moment = time_point<steady_clock>;
	using t64 = duration<f64>;

	moment now() { return steady_clock::now(); }

	struct Clock {
		moment last_advance;
		f64 real_dt;
		f64 runtime;
		f64 dt;
		f64 app_time;
		f64 scale;

		inline static Clock start() {
			return {
				.last_advance = now(),
				.real_dt = 0,
				.runtime = 0,
				.dt = 0,
				.app_time = 0,
				.scale = 1,
			};
		}

		inline Clock& advance_to(moment now) {
			real_dt = duration_cast<t64>(now - last_advance).count();
			runtime += real_dt;
			dt = real_dt * scale;
			app_time += dt;
			last_advance = now;
			return *this;
		}

		inline Clock& update() { return advance_to(now()); }

	};

	inline bool metronome(f32 current, f32 tick_delay, f32& next_tick) {
		assert(tick_delay > 0.0000f);
		bool should_tick = current > next_tick;
		if (should_tick)
			next_tick += tick_delay;
		return should_tick;
	}

} // namespace Time

bool EditorWidget(const cstr label, Time::Clock& clock, bool foldable = true) {
	bool changed = false;
	if (!foldable || ImGui::TreeNode(label)) {
		if (!foldable)
			ImGui::Text("%s", label);
		changed |= EditorWidget("Time", clock.app_time);
		changed |= EditorWidget("Delta time", clock.dt);
		changed |= EditorWidget("Runtime", clock.runtime);
		changed |= EditorWidget("Frame Time", clock.real_dt);
		changed |= EditorWidget("Time Scale", clock.scale);
		if (foldable)
			ImGui::TreePop();
	}
	return changed;
}

bool EditorWidget(const cstr label, Time::Timer& timer, bool foldable = true) {
	bool changed = false;
	if (!foldable || ImGui::TreeNode(label)) {
		if (!foldable)
			ImGui::Text("%s", label);
		changed |= EditorWidget("Duration", timer.duration);
		changed |= EditorWidget("Start", timer.start);
		if (foldable)
			ImGui::TreePop();
	}
	return changed;
}

#endif
