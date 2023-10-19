#ifndef GANIMATION
# define GANIMATION

#include <blblstd.hpp>
#include <math.cpp>
#include <cstdio>

struct AnimationGridHeader {
	u32 dimensions_count;
	usize keyframe_size;
};

enum AnimationWrap { AnimRepeat, AnimClamp, AnimMirror };
using AnimationConfig = Array<const AnimationWrap>;
using LAnimationConfig = LiteralArray<AnimationWrap>;

template<typename Keyframe> struct AnimationGrid {
	Array<Keyframe> keyframes;
	Array<u32> dimensions;
	Array<f32> extents;
	Array<AnimationWrap> config;
};

template<typename Keyframe> usize write_animation_grid(FILE* file, const AnimationGrid<Keyframe>& anim) {
	auto header = AnimationGridHeader{ u32(anim.dimensions.size()), sizeof(Keyframe) };
	auto written = fwrite(&header, sizeof(AnimationGridHeader), 1, file);
	written += fwrite(anim.dimensions.data(), anim.dimensions.size_bytes(), 1, file);
	written += fwrite(anim.extents.data(), anim.extents.size_bytes(), 1, file);
	written += fwrite(anim.config.data(), anim.config.size_bytes(), 1, file);
	written += fwrite(anim.keyframes.data(), anim.keyframes.size_bytes(), 1, file);
	return written;
}

template<typename K> AnimationGrid<K> parse_animation_grid(FILE* file, Arena& arena) {
	AnimationGridHeader header;
	auto read = fread(&header, sizeof(header), 1, file);
	assert(header.keyframe_size == sizeof(K));

	auto dimensions = arena.push_array<u32>(header.dimensions_count);
	auto extents = arena.push_array<f32>(header.dimensions_count);
	auto config = arena.push_array<AnimationWrap>(header.dimensions_count);

	read += fread(dimensions.data(), dimensions.size_bytes(), 1, file);
	read += fread(extents.data(), extents.size_bytes(), 1, file);
	read += fread(config.data(), config.size_bytes(), 1, file);

	auto keyframes = arena.push_array<K>(fold(1, dimensions, [](u32 a, u32 b)->u32 { return b > 0 ? a * b : a; }));
	read += fread(keyframes.data(), keyframes.size_bytes(), 1, file);

	AnimationGrid<K> grid;
	grid.dimensions = dimensions;
	grid.extents = extents;
	grid.config = config;
	grid.keyframes = keyframes;

	return grid;
}

template<typename K> AnimationGrid<K> load_animation_grid(const cstr path, Arena& arena) {
	printf("Loading animation grid %s\n", path);
	auto file = fopen(path, "r"); defer{ fclose(file); };
	if (!file)
		perror("Failed to open file");
	return parse_animation_grid<K>(file, arena);
}

//TODO maybe? remove
template<typename K> void unload(AnimationGrid<K>& animation, Arena& arena) {
	// dealloc_array(arena, animation.keyframes);
	// dealloc_array(arena, animation.config);
	// dealloc_array(arena, animation.extents);
	// dealloc_array(arena, animation.dimensions);
	animation.dimensions = {};
	animation.extents = {};
	animation.config = {};
	animation.keyframes = {};
}

// Index = ∑(i=0 to D-1)(coordinates[i] * ∏(j=i+1 to D-1)(dimensions[j]))
u32 coord_to_index(Array<u32> dimensions, Array<u32> coord) {
	auto sum = 0;
	auto prod = 1;
	for (auto i : u64xrange{ 0, min(dimensions.size(), coord.size()) }) {
		sum += coord[i] * prod;
		prod *= dimensions[i];
	}
	return sum;
}

template<i32 D> u32 coord_to_index(glm::vec<D, u32> dimensions, glm::vec<D, u32> coord) {
	auto sum = 0;
	auto prod = 1;
	for (auto i : u64xrange{ 0, D }) {
		sum += coord[i] * prod;
		prod *= dimensions[i];
	}
	return sum;
}

f32 wrap_one(f32 value, AnimationWrap w) {
	using namespace glm;
	
	switch (w) {
	case AnimRepeat: return mod(value, 1.f);
	case AnimClamp: return clamp(value, std::nexttowardf(0.f, 1.f), std::nexttowardf(1.f, -1.f));
	case AnimMirror: return mod(value, 2.f) < 1.f ? mod(value, 1.f) : 1.f - mod(value, 1.f);
	}
	return 0;
}

template<i32 D> glm::vec<D, f32> wrap_one(glm::vec<D, f32> coord, glm::vec<D, AnimationWrap> config) {
	using namespace glm;
	auto res = vec<D, f32>(0);
	for (auto i : u64xrange{ 0, D })
		res[i] = wrap_one(coord[i], config[i]);
	return res;
}

Array<f32> wrap_one(Arena& arena, Array<const f32> coord, AnimationConfig config) {
	auto buffer = arena.push_array<f32>(coord.size());
	for (auto i : u64xrange{ 0, coord.size() })
		buffer[i] = wrap_one(coord[i], config[i]);
	return buffer;
}

Array<f32> scale(Arena& arena, Array<const f32> coord, Array<const f32> extents) {
	auto buffer = arena.push_array<f32>(coord.size());
	for (auto i : u64xrange{ 0, coord.size() })
		buffer[i] = coord[i] / extents[i];
	return buffer;
}

template<typename Keyframe> Keyframe& get_keyframe(AnimationGrid<Keyframe> anim, Array<u32> coord) {
	return anim.keyframes[coord_to_index(anim.dimensions, coord)];
}

template<typename Keyframe, i32 D> Keyframe& get_keyframe(AnimationGrid<Keyframe> anim, glm::vec<D, u32> coord) {
	return anim.keyframes[coord_to_index(anim.dimensions, cast<u32>(carray(&coord, 1)))];
}

template<typename Keyframe> const Keyframe& animate(AnimationGrid<Keyframe> anim, Array<const f32> coord) {
	u64 size = coord.size() * sizeof(f32) + coord.size() * sizeof(u32) + coord.size() * sizeof(f32);
	byte buffer[size];
	Arena arena = Arena::from_buffer(carray(buffer, size));
	auto scaled_coord = scale(arena, coord, anim.extents);
	auto wrapped_coord = wrap_one(arena, coord, anim.config);
	//TODO replace with a "map" call that takes index into account for wrapped_coord[i] access
	auto frame_coord = arena.push_array<u32>(coord.size());
	for (auto i : u64xrange{ 0, min(anim.dimensions.size(), coord.size()) })
		frame_coord[i] = wrapped_coord[i] * anim.dimensions[i];
	return get_keyframe(anim, frame_coord);
}

template<typename Keyframe, i32 D> Keyframe animate(AnimationGrid<Keyframe> anim, LiteralArray<f32> coord) { return animate(anim, larray(coord)); }
template<typename Keyframe, i32 D> Keyframe animate(AnimationGrid<Keyframe> anim, glm::vec<D, f32> coord) { return animate(anim, cast<f32>(carray(&coord, 1))); }

struct Animator {
	u32 current = 0;
	f32 state_start = 0;

	f32 state_time(f32 time) const { return time - state_start; }
	u32 select_state(u32 state, f32 time) {
		if (current != state) {
			state_start = time;
			current = state;
		}
		return state;
	}
};

#include <imgui_extension.cpp>

bool EditorWidget(const cstr label, Animator& anim) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("Current", anim.current);
		changed |= EditorWidget("State start", anim.state_start);
	}
	return changed;

}

#endif
