#ifndef GTIME
# define GTIME

#include <blblstd.hpp>
#include <chrono>

namespace Time {

	using namespace std::chrono;
	using moment = time_point<steady_clock>;
	using time = duration<f32>;

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
		clock.start = clock.this_frame = steady_clock::now();
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

#endif
