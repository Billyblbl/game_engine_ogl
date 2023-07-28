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
};

AudioData init_audio(LiteralArray<string> extensions = {}) {
	AudioData audio;
	//TODO device selection
	//TODO detect device change and somehow invalidate and reconstruct every piece of data that depends on it
	printf("Initialising audio context\n");
	audio.device = alcOpenDevice(null);
	audio.context = alcCreateContext(audio.device, null);
	audio.extensions = larray(extensions);
	AL_GUARD(alcMakeContextCurrent(audio.context));
	for (auto ext : audio.extensions)
		assert(AL_GUARD(alcIsExtensionPresent(audio.device, ext.data())));
	return audio;
};

void deinit_audio(AudioData& audio) {
	alcDestroyContext(audio.context);
	alcCloseDevice(audio.device);
	audio.context = null;
	audio.device = null;
	audio.extensions = {};
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

struct Audio {
	static constexpr auto MAX_AUDIO_BUFFER_COUNT = 10;
	AudioData data = init_audio();
	List<AudioBuffer> buffers = { alloc_array<AudioBuffer>(std_allocator, MAX_AUDIO_BUFFER_COUNT), 0 };

	~Audio() {
		for (auto&& buffer : buffers.allocated())
			destroy(buffer);
		deinit_audio(data);
	}

	void operator()(Array<tuple<AudioSource*, const Spacial2D*>> entities, Spacial2D* poh = nullptr) {
		for (auto& [source, spacial] : entities) {
			source->set<POSITION>(v3f32(spacial->transform.translation, 0));
			source->set<VELOCITY>(v3f32(spacial->velocity.translation, 0));
		}
		if (poh) {
			ALListener::set<POSITION>(v3f32(poh->transform.translation, 0));
			ALListener::set<VELOCITY>(v3f32(poh->velocity.translation, 0));
		}
	}

	static auto default_editor() { return SystemEditor("Audio", "Alt+A", { Input::KB::K_LEFT_ALT,Input::KB::K_O }); }

	void editor_window() {
		ImGui::Text("Device : %p", data.device);
		if (data.extensions.size() > 0) {
			ImGui::Text("%u", data.extensions.size());
			ImGui::SameLine();
			EditorWidget("Extensions", data.extensions);
		} else {
			ImGui::Text("No extensions");
		}
		ALListener::EditorWidget("Listener");
	};
};

#endif
