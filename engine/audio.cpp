#ifndef GAUDIO
# define GAUDIO

#include <alutils.cpp>
#include <audio_source.cpp>
#include <audio_buffer.cpp>
#define STB_VORBIS_HEADER_ONLY
// #define STB_VORBIS_NO_INTEGER_CONVERSION
#include <stb_vorbis.c>

#include <blblstd.hpp>
#include <spall/profiling.cpp>

enum AL_Format : ALuint {
	AL_NoFormat = 0,
	F1u8 = AL_FORMAT_MONO8,
	F2u8 = AL_FORMAT_STEREO8,
	F1i16 = AL_FORMAT_MONO16,
	F2i16 = AL_FORMAT_STEREO16
};

struct AudioClip {
	Array<i16> samples;
	AL_Format format;
	i32 freq;
};

void print_AL_info(ALCdevice* device) {
	printf("OpenAL vendor string: %s\n", AL_GUARD(alGetString(AL_VENDOR)));
	printf("OpenAL renderer string: %s\n", AL_GUARD(alGetString(AL_RENDERER)));
	printf("OpenAL version string: %s\n", AL_GUARD(alGetString(AL_VERSION)));
	printf("OpenAL playback device: %s\n", AL_GUARD(alcGetString(device, ALC_ALL_DEVICES_SPECIFIER)));
	ALCint major, minor;
	AL_GUARD(alcGetIntegerv(device, ALC_MAJOR_VERSION, 1, &major));
	AL_GUARD(alcGetIntegerv(device, ALC_MINOR_VERSION, 1, &minor));
	printf("ALC version: %d.%d\n", major, minor);
}

struct AudioDevice {
	ALCdevice* handle;
	ALCcontext* context;
	Array<const string> extensions;
	string name;

	static constexpr string default_exts[] = { "ALC_SOFT_reopen_device" };
	static AudioDevice init(Array<const string> extensions = default_exts) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		AudioDevice audio;
		//TODO device selection
		printf("Initialising audio context\n");
		audio.handle = alcOpenDevice(null);
		audio.context = alcCreateContext(audio.handle, null);
		AL_GUARD(alcMakeContextCurrent(audio.context));
		audio.extensions = extensions;
		audio.name = AL_GUARD(alcGetString(audio.handle, ALC_ALL_DEVICES_SPECIFIER));
		for (auto ext : audio.extensions) {
			printf("Requiring %s\n", ext.data());
			assert(AL_GUARD(alcIsExtensionPresent(audio.handle, ext.data())));
		}
		print_AL_info(audio.handle);
		return audio;
	}

	AudioDevice& reinit(Array<AudioSource*> sounds = {}) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		printf("Changing Audio device\n");
		if (auto reopen = (ALCboolean(*)(ALCdevice*, const ALCchar*, const ALCint*))AL_GUARD(alcGetProcAddress(handle, "alcReopenDeviceSOFT")))
			AL_GUARD(reopen(handle, null, null));
		name = AL_GUARD(alcGetString(handle, ALC_ALL_DEVICES_SPECIFIER));
		print_AL_info(handle);
		for (auto source : sounds) if (source)
			source->cache_pop();
		return *this;
	}

	void release() {
		alcMakeContextCurrent(NULL);
		alcDestroyContext(context);
		alcCloseDevice(handle);
		context = null;
		handle = null;
		extensions = {};
		name = "";
	}

	bool changed() {
		ALCint connected = false;
		AL_GUARD(alcGetIntegerv(handle, ALC_CONNECTED, 1, &connected));
		//* using null here in order to actually fetch the device we SHOULD be writing to
		return !connected || name != AL_GUARD(alcGetString(null, ALC_ALL_DEVICES_SPECIFIER));
	}

};

AudioClip load_clip_file(const cstr path) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	i32 channels, freq;
	i16* output;
	auto samples = stb_vorbis_decode_filename(path, &channels, &freq, &output);
	printf("Loading audio file %s:%i-%i\n", path, freq, channels);
	return { carray(output, samples * channels), (channels == 2) ? F2i16 : F1i16, freq };
}

void write_audio_clip(AudioBuffer buffer, AudioClip clip) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	AL_GUARD(alBufferData(buffer.id, clip.format, clip.samples.data(), clip.samples.size_bytes(), clip.freq));
}

void unload_clip(AudioClip& clip) {
	free(clip.samples.data());
	clip.samples = {};
	clip.format = AL_NoFormat;
	clip.freq = 0;
}

namespace ALListener {

	template<typename T> T get_prop(AL_Property property);
	template<typename T> void set_prop(AL_Property property, T value);

	template<> f32 get_prop<f32>(AL_Property property) {
		f32 value;
		AL_GUARD(alGetListenerf(property, &value));
		return value;
	}

	template<> void set_prop<f32>(AL_Property property, f32 value) {
		AL_GUARD(alListenerf(property, value));
	}

	template<> v3f32 get_prop<v3f32>(AL_Property property) {
		v3f32 value;
		AL_GUARD(alGetListener3f(property, &value.x, &value.y, &value.z));
		return value;
	}

	template<> void set_prop<v3f32>(AL_Property property, v3f32 value) {
		AL_GUARD(alListener3f(property, value.x, value.y, value.z));
	}

	template<AL_Property P> inline auto get() { return get_prop<prop_t<P>>(P); }
	template<AL_Property P> inline void set(prop_t<P> value) { set_prop<prop_t<P>>(P, value); }

	template<AL_Property P> bool AudioListenerPropertyWidget(const cstr label) {
		auto data = get<P>();
		if (EditorWidget(label, data)) {
			set<P>(data);
			return true;
		}
		return false;
	}

	template<AL_Property... P> bool AudioListenerWidgetHelper() {
		auto changed = false;
		((changed |= AudioListenerPropertyWidget<P>(al_to_string(P).data())), ...);
		return changed;
	}

	bool EditorWidget(const cstr label) {
		auto changed = false;
		if (ImGui::TreeNode(label)) {
			changed |= AudioListenerWidgetHelper<GAIN, POSITION, VELOCITY/*, ORIENTATION*/>();
			ImGui::TreePop();
		}
		return changed;
	}

}

#include <transform.cpp>

struct Sound {
	AudioSource* source;
	Spacial2D* space;
};

void update_audio(AudioDevice& device, Array<Sound> sounds, const Spacial2D& poh = { identity_2d, null_transform_2d, null_transform_2d }) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	if (device.changed()) {
		AudioSource* buff[sounds.size()];
		auto scratch = Arena::from_array(carray(buff, sounds.size()));
		device.reinit(map(scratch, sounds, [](Sound& s) { return s.source; }));
	}
	for (auto& [source, spacial] : sounds) {
		source->set<POSITION>(v3f32(spacial->transform.translation, 0));
		source->set<VELOCITY>(v3f32(spacial->velocity.translation, 0));
		source->cache_push();//TODO check if we can move this to start of reinit instead
	}
	ALListener::set<POSITION>(v3f32(poh.transform.translation, 0));
	ALListener::set<VELOCITY>(v3f32(poh.velocity.translation, 0));
}

#endif
