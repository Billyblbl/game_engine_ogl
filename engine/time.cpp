#ifndef GTIME
# define GTIME

#include <blblstd.hpp>
#include <chrono>

namespace Time {

	template<typename T> using time = std::chrono::duration<T>;
	template<typename T> using moment = std::chrono::time_point<T>;
	using precision = std::chrono::steady_clock;

	struct Clock {
		time<f32> dt;
		time<f32> total;

		moment<precision> start;
		moment<precision> last_frame;

		Clock& update() {
			moment<precision> end = precision::now();
			dt = end - last_frame;
			total = end - start;
			last_frame = end;
			return *this;
		}

	};

	Clock start() {
		Clock clock;
		clock.start = clock.last_frame = precision::now();
		clock.total = time<f32>(0);
		return clock;
	}

	template<typename T> bool metronome(time<T> current, T tick_delay, T& next_tick) {
		assert(tick_delay > 0.0000f);
		bool should_tick = current.count() - next_tick >= 0.f;
		if (should_tick)
			next_tick += tick_delay;
		return should_tick;
	}

} // namespace Time

#endif
