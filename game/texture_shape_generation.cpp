#ifndef GTEXTURE_SHAPE_GENERATION
# define GTEXTURE_SHAPE_GENERATION

#include <blblstd.hpp>
#include <polygon.cpp>
#include <sprite.cpp>
#include <image.cpp>

const v2i32 grid_neigbhours[] = {
	v2i32(+0, +1), v2i32(+1, +0), v2i32(+0, -1), v2i32(-1, +0),// cardinal
	v2i32(+1, +1), v2i32(+1, -1), v2i32(-1, -1), v2i32(-1, +1),// corners
};

// Image& flood_fill(
// 	Arena& arena,
// 	const Image& source,
// 	Image& dest,
// 	v2u32 start,
// 	Buffer filler,
// 	auto mask,
// 	bool corner = true
// ) {
// 	List<v2i32> to_fill = { arena.push_array<v2i32>(source.dimensions.x * source.dimensions.y), 0 }; defer{ dearena.push_array(arena, to_fill.capacity); };
// 	to_fill.push(start);

// 	auto inside = [](v2u32 dimensions, v2i32 coord) { return (rti32{ v2i32(0), dimensions }).contain(coord); };
// 	auto should_fill = [&](v2i32 coord) { return inside(source.dimensions, coord) && mask(source[v2u64(coord)]) && linear_search(to_fill.used(), coord) < 0; };

// 	while (to_fill.current > 0) {
// 		auto coord = to_fill.pop();
// 		copy(filler, dest[coord]);
// 		for (auto&& nb : larray(grid_neigbhours).subspan(0, corner ? 8 : 4)) if (should_fill(coord + nb))
// 			to_fill.push(coord + nb);
// 	}
// 	return dest;
// }

Array<v2i32> flood_outline(Arena& arena, auto markings_view, v2i32 start, v2u64 dimensions, auto mask, bool corner = true) {
	auto size = dimensions.x * dimensions.y;
	List<v2i32> outline = { arena.push_array<v2i32>(size), 0 };
	List<v2i32> to_fill = { arena.push_array<v2i32>(size), 0 }; //? maybe should use a scratch instead ?
	to_fill.push(start);
	markings_view(start) = true;

	while (to_fill.current > 0) {
		auto coord = to_fill.pop();
		bool is_outline = false;
		for (v2i32 nb : larray(grid_neigbhours).subspan(0, corner ? 8 : 4)) {
			auto px = coord + nb;
			if (mask(px)) {
				if (!markings_view(px)) {
					to_fill.push(px);
					markings_view(px) = true;
				}
			} else
				is_outline = true;
		}
		if (is_outline)
			outline.push(coord);
	}

	arena.pop_local(to_fill.capacity.size());
	return outline.shrink_to_content(arena);
}

bool inside(v2u32 dimensions, v2i32 coord) { return (rti32{ v2i32(0), dimensions }).contain(coord); }

u8 neigbhour_mask(bool* markings, v2u64 dimensions, v2i32 cell) {
	u8 mask = 0;
	for (auto i : u32xrange{ 0, array_size(grid_neigbhours) }) {
		auto px = cell + grid_neigbhours[i];
		if (inside(dimensions, px) && markings[px.x + px.y * dimensions.x])
			mask |= 1 << i;
	}
	return mask;
}

u8 piece_mask(bool* markings, v2u64 dimensions, v2i32 cell) {
	u8 mask = 0;
	for (auto i : u32xrange{ 0, array_size(grid_neigbhours) }) {
		auto px = cell + grid_neigbhours[i];
		if (inside(dimensions, px) && markings[px.x + px.y * dimensions.x])
			mask |= 1 << i;
	}
	return mask;
}

Array<v2f32> decimate(Arena& arena, Array<v2f32> poly) {
	auto pruned = List{ arena.push_array(poly), poly.size() };
	i64 idx = 0;
	while ((idx = linear_search_idx(pruned.used(), [&](v2f32, i64 i) { return wo_at(pruned.used(), i) == WindingOrder::Unknown; }, idx)) >= 0)
		pruned.remove_ordered(idx);

	return pruned.shrink_to_content(arena);
}

//* selective marching squares
Array<Segment<v2f32>> marchsq_contour_segments(Arena& arena, v2u64 dimensions, Array<v2i32> pixels, auto sampler) {

	auto [scratch, scope] = scratch_push_scope(0, &arena); defer { scratch_pop_scope(scratch, scope); };

	v2i32 pieces_offsets[] = { v2i32(0), v2i32(1, 0), v2i32(0, 1), v2i32(1) };
	bool secondary_markings[dimensions.y + 1][dimensions.x + 1];
	memset(secondary_markings, 0, (dimensions.y + 1)* (dimensions.x + 1));

	auto segments = List{ arena.push_array<Segment<v2f32>>(pixels.size() * 4 * 2), 0 };

	auto create_piece_geometry = (
		[&](v2i32 piece, u8 samples) -> Array<const Segment<v2f32>> {
			static const Segment<v2f32> sgmts[] = {
				//0
				{ v2f32(0, .5f), v2f32(.5f, 0) }, //1
				{ v2f32(.5f, 0), v2f32(1, .5f) }, //2
				{ v2f32(0, .5f), v2f32(1, .5f) }, //3

				{ v2f32(1, .5f), v2f32(.5f, 1) }, //4
				{ v2f32(0, .5f), v2f32(.5f, 1) }, { v2f32(1, .5f), v2f32(.5f, 0) }, //5
				{ v2f32(.5f, 0), v2f32(.5f, 1) }, //6
				{ v2f32(0, .5f), v2f32(.5f, 1) }, //7

				{ v2f32(.5f, 1), v2f32(0, .5f) }, //8
				{ v2f32(.5f, 1), v2f32(.5f, 0) }, //9
				{ v2f32(.5f, 1), v2f32(1, .5f) }, { v2f32(.5f, 0), v2f32(0, .5f) }, //10
				{ v2f32(.5f, 1), v2f32(1, .5f) }, //11

				{ v2f32(1, .5f), v2f32(0, .5f) }, //12
				{ v2f32(1, .5f), v2f32(.5f, 0) }, //13
				{ v2f32(.5f, 0), v2f32(0, .5f) }, //14
				//14
			};

			static const Array<const Segment<v2f32>> models[] = {
				{}, larray(sgmts).subspan(0,1), larray(sgmts).subspan(1,1), larray(sgmts).subspan(2,1),
				larray(sgmts).subspan(3,1), larray(sgmts).subspan(4,2), larray(sgmts).subspan(6,1), larray(sgmts).subspan(7,1),
				larray(sgmts).subspan(8,1), larray(sgmts).subspan(9,1), larray(sgmts).subspan(10,2), larray(sgmts).subspan(12,1),
				larray(sgmts).subspan(13,1), larray(sgmts).subspan(14,1), larray(sgmts).subspan(15,1), {}
			};

			return map((scratch), models[samples], [&](Segment<v2f32> md)->Segment<v2f32> { return { v2f32(piece) + md.A, v2f32(piece) + md.B };});
		}
	);

	for (auto px : pixels) for (auto off : pieces_offsets) if (!secondary_markings[px.y + off.y][px.x + off.x]) {
		auto piece = px + off;
		secondary_markings[piece.y][piece.x] = true;
		v2i32 sample_targets[] = { piece - v2i32(1), piece - v2i32(0, 1), piece, piece - v2i32(1, 0) };
		u8 samples = 0;
		for (auto i : u32xrange{ 0, array_size(sample_targets) })
			samples |= sampler(sample_targets[i]) ? 1 << i : 0;
		segments.push(create_piece_geometry(piece, samples));
	}
	return segments.shrink_to_content(arena);
}

Array<v2f32> weld_segments(Arena& arena, Array<Segment<v2f32>> pieces) {
	auto input = List{ pieces, pieces.size() };
	auto outline = List{ arena.push_array<v2f32>(pieces.size()), 0 };

	for (auto i : u64xrange{ 0, pieces.size() }) {
		i64 index = (i > 0) ? best_fit_search(input.used(), [&](const Segment<v2f32>& piece) { return -glm::distance(piece.A, outline.used().back()); }) : 0;
#if 1
		assert(index >= 0);
#else
		if (index < 0)
			return outline.used();
#endif
		outline.push(input.used()[index].B);
		input.swap_out(index);
	}
	return outline.used();
}

#define for2(i, r) for (auto y : u32xrange{ r.min.y, r.max.y } ) for (auto x : u32xrange { r.min.x, r.max.x}) if (i = v2u32(x, y); true)
// for2(v2u32 pixel, (rtu32{ v2u32(0), source.dimensions })) if (free_spot(source[v2i32(x, y)]) && is_collider(source[v2i32(x, y)])) {

Array<Polygon> outline_polygons(Arena& arena, const Image& source, rtu64 clip, const m3x3f32& transform, auto is_collider, u64 expected_poly_count = 8) {
	auto clip_dims = dims_p2(clip);
	bool markings[clip_dims.y][clip_dims.x];
	memset(markings, 0, clip_dims.y * clip_dims.x);
	auto sample_markings = [marks = &markings[0][0], &clip_dims](v2i32 coord)->bool& { return marks[coord.y * clip_dims.x + coord.x]; };
	auto flood_filter = [&](v2i64 coord) { return clip.contain(v2i64(clip.min) + coord) && is_collider(source[v2i64(clip.min) + coord]); };


	auto [scratch, scope] = scratch_push_scope(0, &arena); defer{ scratch_pop_scope(scratch, scope); };
	auto polygons = List{ arena.push_array<Polygon>(expected_poly_count), 0 };

	for (auto view_px : range2(clip_dims)) if (!sample_markings(view_px) && is_collider(source[clip.min + view_px])) {
		auto outline_pixels = flood_outline((scratch), sample_markings, view_px, clip_dims, flood_filter);
		for (auto coord : outline_pixels) markings[coord.y][coord.x] = true;
		auto segments = marchsq_contour_segments((scratch), clip_dims, outline_pixels, sample_markings);
		auto contour = weld_segments((scratch), segments);
		auto pruned = decimate((scratch), contour);
		auto transformed = map(arena, pruned, [&](v2f32 p)->v2f32 { return transform * v3f32(p, 1); });
		polygons.push_growing(arena, transformed);
	}
	return polygons.used();
}

#include <shape_2d.cpp>
#include <animation.cpp>

// Shape2D create_frame_shape(Arena& arena, const Image& source, rtu32 clip, auto is_collider) {
// 	auto dims = dims_p2(clip);
// 	auto transform = glm::scale(glm::translate(m3x3f32(1), v2f32(-.5f, .5f)), v2f32(1.f / dims.x, -1.f / dims.y));//TODO parameterises this, currently uses assumed render rect of 1x1 with origin at its center
// 	auto outlines = outline_polygons(arena, source, clip, transform, is_collider);
// 	return create_polyshape(arena, outlines);
// }

// AnimationGrid<Shape2D> create_animated_shape(Arena& arena, const Image& source, const AnimationGrid<rtu32>& animation, auto is_collider) {
// 	return { map(arena, animation.keyframes, [&](rtu32 clip) { return create_frame_shape(arena, source, clip, is_collider); }), animation.dimensions };
// }

#pragma region standard filters

constexpr v4u8 RGBA_idx = v4u8(0, 1, 2, 3);
inline auto channel_filter(u8 channel, u8 threshold = 128) { return [=](Array<const byte> pixel)->bool { return cast<u8>(pixel)[channel] > threshold; }; }
inline auto alpha_filter(u8 threshold = 128) { return channel_filter(RGBA_idx.a, threshold); }
inline auto bit_filter(u8 bit_index = 0) { return [=](Array<const byte> pixel) { return cast<u8>(pixel)[bit_index / 8] & (1 << (bit_index % 8)); }; }
template<typename U> inline auto mask_filter(U mask) { return [=](Array<const byte> pixel){  return (cast<U>(pixel)[0] & mask) == mask; }; }

#pragma endregion standard filters

#endif
