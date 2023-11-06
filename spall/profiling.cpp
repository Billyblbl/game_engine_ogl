#ifndef GPROFILING
# define GPROFILING

#include <spall/spall.h>
#include <blblstd.hpp>

void profile_process_begin(const cstr trace_file = "last_run.spall");
void profile_process_end();
void profile_thread_begin(usize buffer_size = 1024 * 1024);
void profile_thread_end();
void profile_scope_begin(string name);
void profile_scope_end();

#if defined(PROFILING_IMPL)

#if defined(PLATFORM_WINDOWS)
#include <Windows.h>
static f64 get_time_in_micros(void) {
	static f64 invfreq;
	if (!invfreq) {
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		invfreq = 1000000.0 / frequency.QuadPart;
	}
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart * invfreq;
}
#else
#include <unistd.h>
static f64 get_time_in_micros(void) {
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	return (((f64)spec.tv_sec) * 1000000) + (((f64)spec.tv_nsec) / 1000);
}
#endif

static SpallProfile spall_ctx;
static thread_local SpallBuffer  spall_buffer;

void profile_process_begin(const cstr trace_file) { spall_ctx = spall_init_file(trace_file, 1); }
void profile_process_end() { spall_quit(&spall_ctx); }

void profile_thread_begin(usize buffer_size) {
	auto buff = virtual_reserve(buffer_size, true);
	spall_buffer.data = buff.data();
	spall_buffer.length = buff.size_bytes();
	spall_buffer_init(&spall_ctx, &spall_buffer);
}

void profile_thread_end() {
	spall_buffer_quit(&spall_ctx, &spall_buffer);
	virtual_release(carray((byte*)spall_buffer.data, spall_buffer.length));
}

void profile_scope_begin(string name) { spall_buffer_begin(&spall_ctx, &spall_buffer, name.data(), name.size(), get_time_in_micros()); }
void profile_scope_end() { spall_buffer_end(&spall_ctx, &spall_buffer, get_time_in_micros()); }

#endif

#define PROFILE_PROCESS(n) profile_process_begin(n);\
defer { profile_process_end(); };
#define PROFILE_THREAD(s) profile_thread_begin(s);\
defer { profile_thread_end(); };
#define PROFILE_SCOPE(n) profile_scope_begin(n); \
defer { profile_scope_end(); };

#endif
