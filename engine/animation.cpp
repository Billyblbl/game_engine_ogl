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

template<typename Keyframe, i32 D> AnimationGrid<Keyframe, D> parse_animation_grid(FILE* file, Alloc allocator) {
	using GridType = AnimationGrid<Keyframe, D>;
	auto [hd, buffer, dims, _] = read_animation_grid(file, allocator);
	if (expect(hd.keyframe_size == sizeof(Keyframe)) && expect(hd.dimensions_count == D))
		return { cast<Keyframe>(buffer), dims };
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

enum AnimationWrap {
	AnimRepeat,
	AnimClamp,
	AnimMirror
};

template<i32 D> using AnimationConfig = glm::vec<D, AnimationWrap>;

template<i32 D> glm::vec<D, f32> wrap(glm::vec<D, f32> coord, AnimationConfig<D> config) {
	auto res = glm::vec<D, f32>(0);
	for (auto i : u64xrange{ 0, D }) switch (config[i]) {
	case AnimRepeat: res[i] = glm::mod(coord[i], 1.f); break;
	case AnimClamp: res[i] = glm::clamp(coord[i], 0.f, 1.f); break;
	case AnimMirror: break; //TODO implement mirror
	}
	return res;
}

template<typename Keyframe, i32 D> Keyframe animate_grid(Array<Keyframe> keyframes, glm::vec<D, u32> dimensions, glm::vec<D, f32> coord, AnimationConfig<D> config) {
	return keyframes[coord_to_index(dimensions, glm::vec<D, u32>(glm::vec<D, f32>(dimensions) * wrap(coord, config)))];
}


#endif
