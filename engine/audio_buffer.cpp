#ifndef GAUDIO_BUFFER
# define GAUDIO_BUFFER

#include <alutils.cpp>

enum AL_BufferProperty: ALenum {
	FREQUENCY = AL_FREQUENCY,
	BITS = AL_BITS,
	CHANNELS = AL_CHANNELS,
	SIZE = AL_SIZE
};

struct AudioBuffer {
	ALuint id;

	ALint get(AL_BufferProperty property) {
		ALint value;
		AL_GUARD(alGetBufferi(id, property, &value));
		return value;
	}

	void set(AL_BufferProperty property, ALint value) {
		AL_GUARD(alBufferi(id, property, value));
	}

	bool is_valid() { return AL_GUARD(alIsBuffer(id)); }
};

AudioBuffer create_audio_buffer() {
	AudioBuffer buffer;
	AL_GUARD(alGenBuffers(1 , &buffer.id));
	return buffer;
}

void destroy(AudioBuffer buffer) {
	AL_GUARD(alDeleteBuffers(1 , &buffer.id));
}

Array<AudioBuffer> allocate_audio_buffers(Arena& arena, usize count) {
	auto buffers = arena.push_array<AudioBuffer>(count);
	AL_GUARD(alGenBuffers(count, &buffers[0].id));
	return buffers;
}

void deallocate(Array<AudioBuffer> buffers) {
	AL_GUARD(alDeleteBuffers(buffers.size(), &buffers[0].id));
}


#endif
