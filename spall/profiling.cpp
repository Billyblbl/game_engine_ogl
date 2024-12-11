#ifndef GPROFILING
# define GPROFILING

//* https://stackoverflow.com/questions/73631926/how-to-avoid-apply-compilation-flag-for-third-party-header
//* prevents compilation flags errors in third party headers

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <spall/spall.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <blblstd.hpp>

extern "C" {
	void profile_process_begin(const cstr trace_file = "last_run.spall") __attribute__((no_instrument_function));
	void profile_process_end() __attribute__((no_instrument_function));
	void profile_thread_begin(usize buffer_size = 1024 * 1024) __attribute__((no_instrument_function));
	void profile_thread_end() __attribute__((no_instrument_function));
	void profile_scope_begin(string name) __attribute__((no_instrument_function));
	void profile_scope_end() __attribute__((no_instrument_function));
	void profile_scope_restart(string name) __attribute__((no_instrument_function));
}

#if defined(PROFILING_IMPL)

extern "C" {
	f64 get_time_in_micros(void) __attribute__((no_instrument_function));
}

#if defined(PLATFORM_WINDOWS)
#include <Windows.h>
f64 get_time_in_micros(void) {
	static f64 invfreq = 0;
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
f64 get_time_in_micros(void) {
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	return (((f64)spec.tv_sec) * 1000000) + (((f64)spec.tv_nsec) / 1000);
}
#endif

static SpallProfile spall_ctx;
static thread_local SpallBuffer spall_buffer;

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
void profile_scope_restart(string name) {
	profile_scope_end();
	profile_scope_begin(name);
}

// #define _GNU_SOURCE
#include <dlfcn.h>
//__attribute__((no_instrument_function))

extern "C" {
	void __cyg_profile_func_enter(void* this_fn, void* call_site) __attribute__((no_instrument_function));
	void __cyg_profile_func_exit(void* this_fn, void* call_site) __attribute__((no_instrument_function));

	void __cyg_profile_func_enter(void* this_fn, void* ) {
		Dl_info info;
		char unknown[] = "???";
		dladdr(this_fn, &info);
		if (info.dli_sname)
			spall_buffer_begin(&spall_ctx, &spall_buffer, info.dli_sname, strlen(info.dli_sname), get_time_in_micros());
		else
			spall_buffer_begin(&spall_ctx, &spall_buffer, unknown, sizeof(unknown), get_time_in_micros());
	}

	void __cyg_profile_func_exit(void* , void* ) {
		spall_buffer_end(&spall_ctx, &spall_buffer, get_time_in_micros());
	}
}


#endif

#ifdef PROFILE_TRACE_ON
#define PROFILE_PROCESS(n) profile_process_begin(n);\
defer { profile_process_end(); };
#define PROFILE_THREAD(s) profile_thread_begin(s);\
defer { profile_thread_end(); };
#define PROFILE_SCOPE(n) profile_scope_begin(n); \
defer { profile_scope_end(); };
#else
#define PROFILE_PROCESS(n)
#define PROFILE_THREAD(s)
#define PROFILE_SCOPE(n)
#endif

#endif
