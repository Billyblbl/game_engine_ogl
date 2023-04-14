#ifndef GAUDIO_SOURCE
# define GAUDIO_SOURCE

#include <alutils.cpp>
#include <blblstd.hpp>
#include <math.cpp>
#include <imgui_extension.cpp>
#include <audio_buffer.cpp>

bool EditorWidget(const cstr label, AL_SourceType& type) {
	static const AL_SourceType types[] = { UNDETERMINED, STATIC, STREAMING };
	static const cstrp type_names[] = { "UNDETERMINED", "STATIC", "STREAMING" };
	i32 index;
	for (auto i : i32xrange{ 0, array_size(types) }) if (types[i] == type) {
		index = i;
		break;
	}
	if (ImGui::Combo(label, &index, type_names, array_size(types))) {
		type = types[index];
		return true;
	}
	return false;
}

bool EditorWidget(const cstr label, AL_SourceState& type) {
	static const AL_SourceState states[] = { INITIAL, PLAYING, PAUSED, STOPPED };
	static const cstrp state_names[] = { "INITIAL", "PLAYING", "PAUSED", "STOPPED" };
	i32 index;
	for (auto i : i32xrange{ 0, array_size(states) }) if (states[i] == type) {
		index = i;
		break;
	}
	if (ImGui::Combo(label, &index, state_names, array_size(states))) {
		type = states[index];
		return true;
	}
	return false;
}

template<AL_Property P> using prop_t = prop_type<P>::type;

template<typename T> T get_source_prop(ALuint source, AL_Property property);
template<typename T> void set_source_prop(ALuint source, AL_Property property, T value);

template<> f32 get_source_prop<f32>(ALuint source, AL_Property property) {
	f32 value;
	AL_GUARD(alGetSourcef(source, property, &value));
	return value;
}

template<> void set_source_prop<f32>(ALuint source, AL_Property property, f32 value) {
	AL_GUARD(alSourcef(source, property, value));
}

template<> i32 get_source_prop<i32>(ALuint source, AL_Property property) {
	i32 value;
	AL_GUARD(alGetSourcei(source, property, &value));
	return value;
}

template<> void set_source_prop<i32>(ALuint source, AL_Property property, i32 value) {
	AL_GUARD(alSourcei(source, property, value));
}

template<> v3f32 get_source_prop<v3f32>(ALuint source, AL_Property property) {
	v3f32 value;
	AL_GUARD(alGetSource3f(source, property, &value.x, &value.y, &value.z));
	return value;
}

template<> void set_source_prop<v3f32>(ALuint source, AL_Property property, v3f32 value) {
	AL_GUARD(alSource3f(source, property, value.x, value.y, value.z));
}

template<> AL_SourceType get_source_prop<AL_SourceType>(ALuint source, AL_Property property) {
	i32 value;
	AL_GUARD(alGetSourcei(source, property, &value));
	return (AL_SourceType)value;
}

template<> void set_source_prop<AL_SourceType>(ALuint source, AL_Property property, AL_SourceType value) {
	AL_GUARD(alSourcei(source, property, value));
}

template<> AL_SourceState get_source_prop<AL_SourceState>(ALuint source, AL_Property property) {
	i32 value;
	AL_GUARD(alGetSourcei(source, property, &value));
	return (AL_SourceState)value;
}

template<> void set_source_prop<AL_SourceState>(ALuint source, AL_Property property, AL_SourceState value) {
	AL_GUARD(alSourcei(source, property, value));
}

template<> bool get_source_prop<bool>(ALuint source, AL_Property property) {
	i32 value;
	AL_GUARD(alGetSourcei(source, property, &value));
	return value == AL_TRUE;
}

template<> void set_source_prop<bool>(ALuint source, AL_Property property, bool value) {
	AL_GUARD(alSourcei(source, property, value ? AL_TRUE : AL_FALSE));
}

template<> ALuint get_source_prop<ALuint>(ALuint source, AL_Property property) {
	i32 value;
	AL_GUARD(alGetSourcei(source, property, &value));
	return value;
}

template<> void set_source_prop<ALuint>(ALuint source, AL_Property property, ALuint value) {
	AL_GUARD(alSourcei(source, property, value));
}

struct AudioSource {
	ALuint id;
	template<AL_Property P> inline auto get() { return get_source_prop<prop_t<P>>(id, P); }
	template<AL_Property P> inline AudioSource set(prop_t<P> value) { return (set_source_prop<prop_t<P>>(id, P, value), *this); }
	inline void play() { AL_GUARD(alSourcePlay(id)); }
	inline void pause() { AL_GUARD(alSourcePause(id)); }
	inline void stop() { AL_GUARD(alSourceStop(id)); }
	inline void rewind() { AL_GUARD(alSourceRewind(id)); }
	inline void queue(Array<const AudioBuffer> buffers) { AL_GUARD(alSourceQueueBuffers(id, buffers.size(), (ALuint*)buffers.data())); }
	inline void unqueue(Array<const AudioBuffer> buffers) { AL_GUARD(alSourceUnqueueBuffers(id, buffers.size(), (ALuint*)buffers.data())); }
	inline void queue(LiteralArray<AudioBuffer> buffers) { queue(larray(buffers)); }
	inline void unqueue(LiteralArray<AudioBuffer> buffers) { unqueue(larray(buffers)); }
};

AudioSource create_audio_source() {
	ALuint id;
	AL_GUARD(alGenSources(1, &id));
	return { id };
}

void destroy(AudioSource& source) {
	AL_GUARD(alDeleteSources(1, &source.id));
	source.id = 0;
}

template<AL_Property P> bool AudioSourcePropertyWidget(const cstr label, AudioSource& source) {
	auto data = source.get<P>();
	if (EditorWidget(label, data)) {
		source.set<P>(data);
		return true;
	}
	return false;
}

template<AL_Property... P> bool AudioSourceWidgetHelper(AudioSource& source) {
	auto changed = false;
	((changed |= AudioSourcePropertyWidget<P>(al_to_string(P).data(), source)), ...);
	return changed;
}

bool EditorWidget(const cstr label, AudioSource& source) {
	auto changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		ImGui::Text("Id : %u", source.id); ImGui::SameLine();
		if (ImGui::Button("Play")) source.play(); ImGui::SameLine();
		if (ImGui::Button("Pause")) source.pause(); ImGui::SameLine();
		if (ImGui::Button("Stop")) source.stop(); ImGui::SameLine();
		if (ImGui::Button("Rewind")) source.rewind();
		changed |= AudioSourceWidgetHelper <
			PITCH,
			GAIN,
			MAX_DISTANCE,
			ROLLOFF_FACTOR,
			REFERENCE_DISTANCE,
			MIN_GAIN,
			MAX_GAIN,
			CONE_OUTER_GAIN,
			CONE_INNER_ANGLE,
			CONE_OUTER_ANGLE,
			POSITION,
			VELOCITY,
			DIRECTION,
			SOURCE_RELATIVE,
			SOURCE_TYPE,
			LOOPING,
			BUFFER,
			SOURCE_STATE,
			BUFFERS_QUEUED,
			BUFFERS_PROCESSED,
			SEC_OFFSET,
			SAMPLE_OFFSET,
			BYTE_OFFSET
		> (source);
	}

	return changed;
}

#endif
