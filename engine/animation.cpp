#ifndef GANIMATION
# define GANIMATION

#include <blblstd.hpp>
#include <math.cpp>
#include <cstdio>

struct AnimationGridHeader {
	u32 dimensions_count;
	usize keyframe_size;
};

template<typename Keyframe> struct AnimationGrid {
	Array<Keyframe> keyframes;
	Array<u32> dimensions;
};

template<typename Keyframe> usize write_animation_grid(FILE* file, Array<const Keyframe> keyframes, Array<u32> dimensions) {
	auto header = AnimationGridHeader{ dimensions.size(), sizeof(Keyframe) };
	auto written = fwrite(&header, sizeof(AnimationGridHeader), 1, file);
	written += fwrite(dimensions.data(), dimensions.size_bytes(), 1, file);
	written += fwrite(keyframes.data(), keyframes.size_bytes(), 1, file);
	return written;
}

tuple<AnimationGridHeader, Buffer, Array<u32>, usize> read_animation_grid(FILE* file, Alloc allocator) {
	AnimationGridHeader header;
	auto read = fread(&header, sizeof(header), 1, file);
	auto dimensions = alloc_array<u32>(allocator, header.dimensions_count);
	read += fread(dimensions.data(), dimensions.size_bytes(), 1, file);
	auto data = allocator.alloc(header.keyframe_size * fold(1, dimensions, [](u32 a, u32 b)->u32 { return b > 0 ? a * b : a; }));
	read += fread(data.data(), data.size_bytes(), 1, file);
	return { header, data, dimensions, read };
}

template<typename K> AnimationGrid<K> parse_animation_grid(FILE* file, Alloc allocator) {
	auto [hd, buffer, dims, _] = read_animation_grid(file, allocator);
	if (expect(hd.keyframe_size == sizeof(K)))
		return { cast<K>(buffer), dims };
	else
		return fail_ret("Invalid animation format", AnimationGrid<K>{});
}

template<typename K> AnimationGrid<K> load_animation_grid(const cstr path, Alloc allocator) {
	printf("Loading animation grid %s\n", path);
	auto file = fopen(path, "r"); defer{ fclose(file); };
	if (!file)
		perror("Failed to open file");
	return parse_animation_grid<K>(file, allocator);
}

template<typename K> void unload(AnimationGrid<K>& animation, Alloc allocator) {
	dealloc_array(allocator, animation.dimensions);
	dealloc_array(allocator, animation.keyframes);
	animation.dimensions = {};
	animation.keyframes = {};
}

// Index = ∑(i=0 to D-1)(coordinates[i] * ∏(j=i+1 to D-1)(dimensions[j]))
template<i32 D> u32 coord_to_index(glm::vec<D, u32> dimensions, glm::vec<D, u32> coord) {
	auto sum = 0;
	auto prod = 1;
	for (auto i : u64xrange{ 0, D }) {
		sum += coord[i] * prod;
		prod *= dimensions[i];
	}
	return sum;
}

u32 coord_to_index(Array<u32> dimensions, Array<u32> coord) {
	auto sum = 0;
	auto prod = 1;
	for (auto i : u64xrange{ 0, min(dimensions.size(), coord.size()) }) {
		sum += coord[i] * prod;
		prod *= dimensions[i];
	}
	return sum;
}

enum AnimationWrap { AnimRepeat, AnimClamp, AnimMirror };
using AnimationConfig = Array<const AnimationWrap>;
using LAnimationConfig = LiteralArray<AnimationWrap>;

f32 wrap(f32 value, AnimationWrap w) {
	using namespace glm;
	switch (w) {
	case AnimRepeat: return mod(value, 1.f);
	case AnimClamp: return clamp(value, 0.f, 1.f);
	case AnimMirror: return mod(value, 2.f) < 1.f ? mod(value, 1.f) : 1.f - mod(value, 1.f);
	}
	return 0;
}

template<i32 D> glm::vec<D, f32> wrap(glm::vec<D, f32> coord, glm::vec<D, AnimationWrap> config) {
	using namespace glm;
	auto res = vec<D, f32>(0);
	for (auto i : u64xrange{ 0, D })
		res[i] = wrap(coord[i], config[i]);
	return res;
}

Array<f32> wrap(Array<f32> buffer, Array<f32> coord, AnimationConfig config) {
	for (auto i : u64xrange{ 0, coord.size() })
		buffer[i] = wrap(coord[i], config[i]);
	return buffer;
}

template<typename Keyframe> Keyframe animate(AnimationGrid<Keyframe> anim, Array<f32> coord, AnimationConfig config) {
	u64 size = coord.size() * sizeof(f32) + coord.size() * sizeof(u32);
	byte buffer[size];
	auto arena = as_arena(carray(buffer, size));
	auto wrapped_coord = wrap(alloc_array<f32>(as_stack(arena), coord.size()), coord, config);
	auto frame_coord = alloc_array<u32>(as_stack(arena), coord.size());
	for (auto i : u64xrange{ 0, anim.dimensions.size() })
		frame_coord[i] = wrapped_coord[i] * anim.dimensions[i];
	auto index = coord_to_index(anim.dimensions, frame_coord);
	return anim.keyframes[index];
}

template<typename Keyframe, i32 D> Keyframe animate(AnimationGrid<Keyframe> anim, glm::vec<D, f32> coord, AnimationConfig config) { return animate(anim, cast<f32>(carray(&coord, 1)), config); }
template<typename Keyframe, i32 D> Keyframe animate(AnimationGrid<Keyframe> anim, glm::vec<D, f32> coord, LAnimationConfig config) { return animate(anim, cast<f32>(carray(&coord, 1)), larray(config)); }

#endif
