#ifndef GAUDIO
# define GAUDIO

#include <alutils.cpp>
#include <audio_source.cpp>
#include <audio_buffer.cpp>
#define STB_VORBIS_HEADER_ONLY
// #define STB_VORBIS_NO_INTEGER_CONVERSION
#include <stb_vorbis.c>

#include <blblstd.hpp>

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

struct AudioData {
	ALCdevice* device;
	ALCcontext* context;
	Array<const string> extensions;
	string device_name;
};

void print_AL_info(ALCdevice* device) {
	printf("OpenAL vendor string: %s\n", AL_GUARD(alGetString(AL_VENDOR)));
	printf("OpenAL renderer string: %s\n", AL_GUARD(alGetString(AL_RENDERER)));
	printf("OpenAL version string: %s\n", AL_GUARD(alGetString(AL_VERSION)));
	// printf("OpenAL extensions string: %s\n", AL_GUARD(alGetString(AL_EXTENSIONS)));
	printf("OpenAL playback device: %s\n", AL_GUARD(alcGetString(device, ALC_ALL_DEVICES_SPECIFIER)));
	ALCint major, minor;
	AL_GUARD(alcGetIntegerv(device, ALC_MAJOR_VERSION, 1, &major));
	AL_GUARD(alcGetIntegerv(device, ALC_MINOR_VERSION, 1, &minor));
	printf("ALC version: %d.%d\n", major, minor);
}

AudioData reinit(AudioData& audio);

AudioData init_audio(Array<const string> extensions = {}) {
	AudioData audio;
	//TODO device selection
	printf("Initialising audio context\n");
	audio.device = alcOpenDevice(null);
	audio.context = alcCreateContext(audio.device, null);
	AL_GUARD(alcMakeContextCurrent(audio.context));
	audio.extensions = extensions;
	audio.device_name = AL_GUARD(alcGetString(audio.device, ALC_ALL_DEVICES_SPECIFIER));
	for (auto ext : audio.extensions) {
		printf("Requiring %s\n", ext.data());
		assert(AL_GUARD(alcIsExtensionPresent(audio.device, ext.data())));
	}
	print_AL_info(audio.device);
	return audio;
};

bool changed_audio(const AudioData& audio) {
	ALCint connected = false;
	AL_GUARD(alcGetIntegerv(audio.device, ALC_CONNECTED, 1, &connected));
	//* using null here in order to actually fetch the device we SHOULD be writing to
	return !connected || audio.device_name != AL_GUARD(alcGetString(null, ALC_ALL_DEVICES_SPECIFIER));
}

AudioData reinit(AudioData& audio) {
	printf("Changing Audio device\n");
	if (auto reopen = (ALCboolean(*)(ALCdevice*, const ALCchar*, const ALCint*))AL_GUARD(alcGetProcAddress(audio.device, "alcReopenDeviceSOFT")))
		AL_GUARD(reopen(audio.device, null, null));
	audio.device_name = AL_GUARD(alcGetString(audio.device, ALC_ALL_DEVICES_SPECIFIER));
	print_AL_info(audio.device);
	return audio;
}

void deinit_audio(AudioData& audio) {
	alcMakeContextCurrent(NULL);
	alcDestroyContext(audio.context);
	alcCloseDevice(audio.device);
	audio.context = null;
	audio.device = null;
	audio.extensions = {};
	audio.device_name = "";
}

AudioClip load_clip_file(const cstr path) {
	i32 channels, freq;
	i16* output;
	auto samples = stb_vorbis_decode_filename(path, &channels, &freq, &output);
	printf("Loading audio file %s:%i-%i\n", path, freq, channels);
	return { carray(output, samples * channels), (channels == 2) ? F2i16 : F1i16, freq };
}

void write_audio_clip(AudioBuffer buffer, AudioClip clip) {
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
#include <inputs.cpp>
#include <system_editor.cpp>

#include <spall/profiling.cpp>

struct Sound {
	AudioSource* source;
	Spacial2D* space;
};

struct Audio {
	static constexpr auto MAX_AUDIO_BUFFER_COUNT = 10;
	static constexpr string exts[] = { "ALC_SOFT_reopen_device" };
	AudioData data = init_audio(larray(exts));
	List<AudioBuffer> buffers = { alloc_array<AudioBuffer>(std_allocator, MAX_AUDIO_BUFFER_COUNT), 0 };

	~Audio() {
		deinit_audio(data);
	}

	void migrate_audio(Array<Sound> entities) {
		data = reinit(data);
		for (auto [source, _] : entities) if (source)
			source->cache_pop();
	}

	void operator()(Array<Sound> entities, Spacial2D* poh = nullptr) {
		PROFILE_SCOPE("Audio");
		if (changed_audio(data))
			migrate_audio(entities);
		for (auto& [source, spacial] : entities) {
			source->set<POSITION>(v3f32(spacial->transform.translation, 0));
			source->set<VELOCITY>(v3f32(spacial->velocity.translation, 0));
			source->cache_push();
		}
		if (poh) {
			ALListener::set<POSITION>(v3f32(poh->transform.translation, 0));
			ALListener::set<VELOCITY>(v3f32(poh->velocity.translation, 0));
		}
	}

	static auto default_editor() { return SystemEditor("Audio", "Alt+A", { Input::KB::K_LEFT_ALT,Input::KB::K_O }); }

	void editor_window() {
		ImGui::Text("Device : %p:%s", data.device, data.device_name.data());
		ImGui::Text("ALC_DEVICES_SPECIFIER : %s\n", alcGetString(data.device, ALC_DEVICE_SPECIFIER));
		ImGui::Text("ALC_DEFAULT_DEVICE_SPECIFIER : %s\n", alcGetString(data.device, ALC_DEFAULT_DEVICE_SPECIFIER));
		ImGui::Text("ALC_ALL_DEVICES_SPECIFIER : %s\n", alcGetString(data.device, ALC_ALL_DEVICES_SPECIFIER));
		ImGui::Text("ALC_DEFAULT_ALL_DEVICES_SPECIFIER : %s\n", alcGetString(data.device, ALC_DEFAULT_ALL_DEVICES_SPECIFIER));

		if (data.extensions.size() > 0) {
			ImGui::Text("%u", data.extensions.size());
			ImGui::SameLine();
			if (ImGui::TreeNode("Extensions")) {
				for (auto ext : data.extensions)
					ImGui::Text("%s\n", ext.data());
				ImGui::TreePop();
			}
		} else {
			ImGui::Text("No extensions");
		}
		ALListener::EditorWidget("Listener");
	};
};

#endif
