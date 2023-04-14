#ifndef GALUTILS
# define GALUTILS

#include <blblstd.hpp>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <stdio.h>

#define serialise_macro(d) lstr(#d)

const string al_to_string(ALenum value) {
	switch (value) {
	case AL_NO_ERROR: return serialise_macro(AL_NO_ERROR);
	case AL_INVALID_NAME: return serialise_macro(AL_INVALID_NAME);
	case AL_INVALID_ENUM: return serialise_macro(AL_INVALID_ENUM);
	case AL_INVALID_VALUE: return serialise_macro(AL_INVALID_VALUE);
	case AL_INVALID_OPERATION: return serialise_macro(AL_INVALID_OPERATION);
	case AL_OUT_OF_MEMORY: return serialise_macro(AL_OUT_OF_MEMORY);
	case AL_PITCH: return serialise_macro(AL_PITCH);
	case AL_GAIN: return serialise_macro(AL_GAIN);
	case AL_MAX_DISTANCE: return serialise_macro(AL_MAX_DISTANCE);
	case AL_ROLLOFF_FACTOR: return serialise_macro(AL_ROLLOFF_FACTOR);
	case AL_REFERENCE_DISTANCE: return serialise_macro(AL_REFERENCE_DISTANCE);
	case AL_MIN_GAIN: return serialise_macro(AL_MIN_GAIN);
	case AL_MAX_GAIN: return serialise_macro(AL_MAX_GAIN);
	case AL_CONE_OUTER_GAIN: return serialise_macro(AL_CONE_OUTER_GAIN);
	case AL_CONE_INNER_ANGLE: return serialise_macro(AL_CONE_INNER_ANGLE);
	case AL_CONE_OUTER_ANGLE: return serialise_macro(AL_CONE_OUTER_ANGLE);
	case AL_POSITION: return serialise_macro(AL_POSITION);
	case AL_VELOCITY: return serialise_macro(AL_VELOCITY);
	case AL_DIRECTION: return serialise_macro(AL_DIRECTION);
	case AL_SOURCE_RELATIVE: return serialise_macro(AL_SOURCE_RELATIVE);
	case AL_SOURCE_TYPE: return serialise_macro(AL_SOURCE_TYPE);
	case AL_LOOPING: return serialise_macro(AL_LOOPING);
	case AL_BUFFER: return serialise_macro(AL_BUFFER);
	case AL_SOURCE_STATE: return serialise_macro(AL_SOURCE_STATE);
	case AL_BUFFERS_QUEUED: return serialise_macro(AL_BUFFERS_QUEUED);
	case AL_BUFFERS_PROCESSED: return serialise_macro(AL_BUFFERS_PROCESSED);
	case AL_SEC_OFFSET: return serialise_macro(AL_SEC_OFFSET);
	case AL_SAMPLE_OFFSET: return serialise_macro(AL_SAMPLE_OFFSET);
	case AL_BYTE_OFFSET: return serialise_macro(AL_BYTE_OFFSET);
	case AL_INITIAL: return serialise_macro(AL_INITIAL);
	case AL_PLAYING: return serialise_macro(AL_PLAYING);
	case AL_PAUSED: return serialise_macro(AL_PAUSED);
	case AL_STOPPED: return serialise_macro(AL_STOPPED);
	case AL_UNDETERMINED: return serialise_macro(AL_UNDETERMINED);
	case AL_STATIC: return serialise_macro(AL_STATIC);
	case AL_STREAMING: return serialise_macro(AL_STREAMING);
	default: return "Unknown openAL value";
	}
}

void check_al_error(string expression, string file_name, u32 line_number) {
	for (auto err = alGetError(); err != AL_NO_ERROR; err = alGetError())
		fprintf(stderr, "Error in file %s:%d, when executing %s : %u %s\n", file_name.data(), line_number, expression.data(), err, al_to_string(err).data());
}

#define DEBUG_AL true
// #define DEBUG_AL false


#if DEBUG_AL
#define AL_GUARD(x) [&]() -> auto { defer {check_al_error(#x, __FILE__, __LINE__);}; return x;}()
#else
#define AL_GUARD(x) x
#endif

enum AL_Property: ALenum {
	PITCH = AL_PITCH,
	GAIN = AL_GAIN,
	MAX_DISTANCE = AL_MAX_DISTANCE,
	ROLLOFF_FACTOR = AL_ROLLOFF_FACTOR,
	REFERENCE_DISTANCE = AL_REFERENCE_DISTANCE,
	MIN_GAIN = AL_MIN_GAIN,
	MAX_GAIN = AL_MAX_GAIN,
	CONE_OUTER_GAIN = AL_CONE_OUTER_GAIN,
	CONE_INNER_ANGLE = AL_CONE_INNER_ANGLE,
	CONE_OUTER_ANGLE = AL_CONE_OUTER_ANGLE,
	POSITION = AL_POSITION,
	VELOCITY = AL_VELOCITY,
	DIRECTION = AL_DIRECTION,
	SOURCE_RELATIVE = AL_SOURCE_RELATIVE,
	SOURCE_TYPE = AL_SOURCE_TYPE,
	LOOPING = AL_LOOPING,
	BUFFER = AL_BUFFER,
	SOURCE_STATE = AL_SOURCE_STATE,
	BUFFERS_QUEUED = AL_BUFFERS_QUEUED,
	BUFFERS_PROCESSED = AL_BUFFERS_PROCESSED,
	SEC_OFFSET = AL_SEC_OFFSET,
	SAMPLE_OFFSET = AL_SAMPLE_OFFSET,
	BYTE_OFFSET = AL_BYTE_OFFSET,
	ORIENTATION = AL_ORIENTATION
};

enum AL_SourceType: ALint {
	UNDETERMINED = AL_UNDETERMINED,
	STATIC = AL_STATIC,
	STREAMING = AL_STREAMING
};

enum AL_SourceState: ALint {
	INITIAL = AL_INITIAL,
	PLAYING = AL_PLAYING,
	PAUSED = AL_PAUSED,
	STOPPED = AL_STOPPED,
};

template<AL_Property P> struct prop_type { using type = void; };
template<> struct prop_type<PITCH> { using type = f32; };
template<> struct prop_type<GAIN> { using type = f32; };
template<> struct prop_type<MAX_DISTANCE> { using type = f32; };
template<> struct prop_type<ROLLOFF_FACTOR> { using type = f32; };
template<> struct prop_type<REFERENCE_DISTANCE> { using type = f32; };
template<> struct prop_type<MIN_GAIN> { using type = f32; };
template<> struct prop_type<MAX_GAIN> { using type = f32; };
template<> struct prop_type<CONE_OUTER_GAIN> { using type = f32; };
template<> struct prop_type<CONE_INNER_ANGLE> { using type = f32; };
template<> struct prop_type<CONE_OUTER_ANGLE> { using type = f32; };
template<> struct prop_type<POSITION> { using type = v3f32; };
template<> struct prop_type<VELOCITY> { using type = v3f32; };
template<> struct prop_type<DIRECTION> { using type = v3f32; };
template<> struct prop_type<SOURCE_RELATIVE> { using type = bool; };
template<> struct prop_type<SOURCE_TYPE> { using type = AL_SourceType; };
template<> struct prop_type<LOOPING> { using type = bool; };
template<> struct prop_type<BUFFER> { using type = ALuint; };
template<> struct prop_type<SOURCE_STATE> { using type = AL_SourceState; };
template<> struct prop_type<BUFFERS_QUEUED> { using type = i32; };
template<> struct prop_type<BUFFERS_PROCESSED> { using type = i32; };
template<> struct prop_type<SEC_OFFSET> { using type = f32; };
template<> struct prop_type<SAMPLE_OFFSET> { using type = f32; };
template<> struct prop_type<BYTE_OFFSET> { using type = i32; };
// template<> struct prop_type<ORIENTATION> { using type = Array<f32>; };

#endif
