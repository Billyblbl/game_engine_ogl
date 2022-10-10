#ifndef GTIME
# define GTIME

#include <chrono>

namespace Time {

	struct Clock {
		std::chrono::duration<float> dt;
		std::chrono::duration<float> totalElapsedTime;

		std::chrono::time_point<std::chrono::steady_clock> start;
		std::chrono::time_point<std::chrono::steady_clock> lastFrame;

		Clock& Update() {
			std::chrono::time_point<std::chrono::steady_clock> end = std::chrono::steady_clock::now();
			dt = end - lastFrame;
			totalElapsedTime = end - start;
			lastFrame = end;
			return *this;
		}

	};

	Clock Start() {
		Clock clock;
		clock.start = clock.lastFrame = std::chrono::steady_clock::now();
		clock.totalElapsedTime = std::chrono::duration<float>(0);
		return clock;
	}

} // namespace Time

#endif
