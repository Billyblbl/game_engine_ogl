#ifndef GANIMATION
# define GANIMATION

#include <blblstd.hpp>
#include <math.cpp>
#include <cstdio>

struct AnimationGridHeader {
	u32 dimensions_count;
	usize keyframe_size;
};

template<typename Keyframe, i32 D> struct AnimationGrid {
	Array<Keyframe> keyframes;
	glm::vec<D, u32> dimensions;
};

template<typename Keyframe, i32 Dims> usize write_animation_grid(FILE* file, Array<const Keyframe> keyframes, glm::vec<Dims, u32> dimensions) {
	auto header = AnimationGridHeader{ Dims, sizeof(Keyframe) };
	auto written = fwrite(&header, sizeof(AnimationGridHeader), 1, file);
	written += fwrite(&dimensions, sizeof(u32) * Dims, 1, file);
	written += fwrite(keyframes.data(), keyframes.size_bytes(), 1, file);
	return written;
}

tuple<AnimationGridHeader, Buffer, v4u32, usize> read_animation_grid(FILE* file, Alloc allocator) {
	AnimationGridHeader header;
	auto read = fread(&header, sizeof(header), 1, file);
	auto dimensions = v4u32(0);
	read += fread(&dimensions, sizeof(u32) * header.dimensions_count, 1, file);
	auto data = allocator.alloc(header.keyframe_size * dimensions.x * dimensions.y);
	read += fread(data.data(), data.size_bytes(), 1, file);
	return { header, data, dimensions, read };
}

template<typename K, i32 D> AnimationGrid<K, D> parse_animation_grid(FILE* file, Alloc allocator) {
	using GridType = AnimationGrid<K, D>;
	auto [hd, buffer, dims, _] = read_animation_grid(file, allocator);
	if (expect(hd.keyframe_size == sizeof(K)) && expect(hd.dimensions_count == D))
		return { cast<K>(buffer), dims };
	else
		return fail_ret("Invalid animation format", GridType{});
}

template<typename Keyframe, i32 D> AnimationGrid<Keyframe, D> load_animation_grid(const cstr path, Alloc allocator) {
	printf("Loading animation grid %s\n", path);
	auto file = fopen(path, "r"); defer{ fclose(file); };
	if (!file)
		perror("Failed to open file");
	return parse_animation_grid<Keyframe, D>(file, allocator);
}

template<typename Keyframe, i32 D> void unload(AnimationGrid<Keyframe, D>& animation, Alloc allocator) {
	dealloc_array(allocator, animation.keyframes);
	animation.keyframes = {};
	animation.dimensions = glm::vec<D, u32>(0);
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

enum AnimationWrap { AnimRepeat, AnimClamp, AnimMirror };
template<i32 D> using AnimationConfig = glm::vec<D, AnimationWrap>;

f32 wrap(f32 value, AnimationWrap w) {
	using namespace glm;
	switch (w) {
	case AnimRepeat: return mod(value, 1.f);
	case AnimClamp: return clamp(value, 0.f, 1.f);
	case AnimMirror: return mod(value, 2.f) < 1.f ? mod(value, 1.f) : 1.f - mod(value, 1.f);
	}
	return 0;
}

template<i32 D> glm::vec<D, f32> wrap(glm::vec<D, f32> coord, AnimationConfig<D> config) {
	using namespace glm;
	auto res = vec<D, f32>(0);
	for (auto i : u64xrange{ 0, D })
		res[i] = wrap(coord[i], config[i]);
	return res;
}

template<typename Keyframe, i32 D> Keyframe animate(AnimationGrid<Keyframe, D> anim, glm::vec<D, f32> coord, AnimationConfig<D> config) {
	return anim.keyframes[coord_to_index(anim.dimensions, glm::vec<D, u32>(glm::vec<D, f32>(anim.dimensions) * wrap(coord, config)))];
}

#endif
