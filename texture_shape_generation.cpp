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

Image& flood_fill(
	Alloc allocator,
	const Image& source,
	Image& dest,
	v2u32 start,
	Buffer filler,
	auto mask,
	bool corner = true
) {
	List<v2i32> to_fill = { alloc_array<v2i32>(allocator, source.dimensions.x * source.dimensions.y), 0 }; defer{ dealloc_array(allocator, to_fill.capacity); };
	to_fill.push(start);

	auto inside = [](v2u32 dimensions, v2i32 coord) { return (rti32{ v2i32(0), dimensions }).contain(coord); };
	auto should_fill = [&](v2i32 coord) { return inside(source.dimensions, coord) && mask(source[v2u64(coord)]) && linear_search(to_fill.allocated(), coord) < 0; };

	while (to_fill.current > 0) {
		auto coord = to_fill.pop();
		copy(filler, dest[coord]);
		for (auto&& nb : larray(grid_neigbhours).subspan(0, corner ? 8 : 4)) if (should_fill(coord + nb))
			to_fill.push(coord + nb);
	}
	return dest;
}

Array<v2i32> flood_outline(Alloc allocator, auto markings_view, v2i32 start, v2u64 dimensions, auto mask, bool corner = true) {
	auto size = dimensions.x * dimensions.y;
	List<v2i32> outline = { alloc_array<v2i32>(allocator, size), 0 };
	List<v2i32> to_fill = { alloc_array<v2i32>(allocator, size), 0 };
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

	dealloc_array(allocator, to_fill.capacity);
	return outline.shrink_to_content(allocator);
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

u8 piece_mask(bool* markings, v2u64 dimensions, v2i32 cell, u8 nb_mask) {
	u8 mask = 0;
	for (auto i : u32xrange{ 0, array_size(grid_neigbhours) }) {
		auto px = cell + grid_neigbhours[i];
		if (inside(dimensions, px) && markings[px.x + px.y * dimensions.x])
			mask |= 1 << i;
	}
	return mask;
}

Array<v2f32> decimate(Alloc allocator, Array<v2f32> poly) {
	auto pruned = List{ duplicate_array(allocator, poly), poly.size() };
	i64 idx = 0;
	while ((idx = linear_search_idx(pruned.allocated(), [&](v2f32, i64 i) { return wo_at(pruned.allocated(), i) == WindingOrder::Unknown; }, idx)) >= 0)
		pruned.remove_ordered(idx);

	return pruned.shrink_to_content(allocator);
}

//* selective marching squares
Array<Segment<v2f32>> marchsq_contour_segments(Alloc allocator, v2u64 dimensions, Array<v2i32> pixels, auto sampler) {
	auto scratch = create_virtual_arena(5000000); defer{ destroy_virtual_arena(scratch); };//TODO replace with thread local arena scheme

	v2i32 pieces_offsets[] = { v2i32(0), v2i32(1, 0), v2i32(0, 1), v2i32(1) };
	bool secondary_markings[dimensions.y + 1][dimensions.x + 1];
	memset(secondary_markings, 0, (dimensions.y + 1)* (dimensions.x + 1));

	auto segments = List{ alloc_array<Segment<v2f32>>(allocator, pixels.size() * 4 * 2), 0 };

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

			return map(as_v_alloc(scratch), models[samples], [&](Segment<v2f32> md)->Segment<v2f32> { return { v2f32(piece) + md.A, v2f32(piece) + md.B };});
		}
	);

	for (auto px : pixels) for (auto off : pieces_offsets) if (!secondary_markings[px.y + off.y][px.x + off.x]) {
		auto piece = px + off;
		secondary_markings[piece.y][piece.x] = true;
		v2i32 sample_targets[] = { piece - v2i32(1), piece - v2i32(0, 1), piece, piece - v2i32(1, 0) };
		u8 samples = 0;
		for (auto i : u32xrange{ 0, array_size(sample_targets) })
			samples |= sampler(sample_targets[i]) ? 1 << i : 0;
		segments.push_range(create_piece_geometry(piece, samples));
	}
	return segments.shrink_to_content(allocator);
}

Array<v2f32> weld_segments(Alloc allocator, Array<Segment<v2f32>> pieces) {
	auto input = List{ pieces, pieces.size() };
	auto outline = List{ alloc_array<v2f32>(allocator, pieces.size()), 0 };

	for (auto i : u64xrange{ 0, pieces.size() }) {
		i64 index = (i > 0) ? best_fit_search(input.allocated(), [&](const Segment<v2f32>& piece) { return -glm::distance(piece.A, outline.allocated().back()); }) : 0;
#if 1
		assert(index >= 0);
#else
		if (index < 0)
			return outline.allocated();
#endif
		outline.push(input.allocated()[index].B);
		input.swap_out(index);
	}
	return outline.allocated();
}

#define for2(i, r) for (auto y : u32xrange{ r.min.y, r.max.y } ) for (auto x : u32xrange { r.min.x, r.max.x}) if (i = v2u32(x, y); true)
// for2(v2u32 pixel, (rtu32{ v2u32(0), source.dimensions })) if (free_spot(source[v2i32(x, y)]) && is_collider(source[v2i32(x, y)])) {

Array<Array<v2f32>> outline_polygons(Alloc allocator, const Image& source, rtu64 clip, const m3x3f32& transform, auto is_collider, u64 expected_poly_count = 8) {
	auto clip_dims = dims_p2(clip);
	bool markings[clip_dims.y][clip_dims.x];
	memset(markings, 0, clip_dims.y * clip_dims.x);
	auto sample_markings = [marks = &markings[0][0], &clip_dims](v2i32 coord)->bool& { return marks[coord.y * clip_dims.x + coord.x]; };
	auto flood_filter = [&](v2i64 coord) { return clip.contain(v2i64(clip.min) + coord) && is_collider(source[v2i64(clip.min) + coord]); };

	auto scratch = create_virtual_arena(5000000); defer{ destroy_virtual_arena(scratch); }; //TODO replace with thread local storage scratch
	auto polygons = List{ alloc_array<Polygon>(allocator, expected_poly_count), 0 };

	for (auto view_px : range2(clip_dims)) if (!sample_markings(view_px) && is_collider(source[clip.min + view_px])) {
		auto outline_pixels = flood_outline(as_v_alloc(scratch), sample_markings, view_px, clip_dims, flood_filter);
		for (auto coord : outline_pixels) markings[coord.y][coord.x] = true;
		auto segments = marchsq_contour_segments(as_v_alloc(scratch), clip_dims, outline_pixels, sample_markings);
		auto contour = weld_segments(as_v_alloc(scratch), segments);
		auto pruned = decimate(as_v_alloc(scratch), contour);
		auto transformed = map(allocator, pruned, [&](v2f32 p)->v2f32 { return transform * v3f32(p, 1); });
		polygons.push_growing(allocator, transformed);
	}
	return polygons.allocated();
}

#include <shape_2d.cpp>
#include <animation.cpp>

Shape2D create_frame_shape(Alloc allocator, const Image& source, rtf32 clip, auto is_collider) {
	rtu64 pixel_clip = {
		v2u64(clip.min.x * source.dimensions.x, clip.min.y * source.dimensions.y),
		v2u64(clip.max.x * source.dimensions.x, clip.max.y * source.dimensions.y)
	};
	auto dims = dims_p2(pixel_clip);
	// auto transform = glm::translate(m3x3f32(1), -v2f32(.5f)) * glm::scale(m3x3f32(1), v2f32(1.f / dims.x, -1.f / dims.y));//TODO parameterises this, currently uses assumed render rect of 1x1 with origin at its center
	// auto transform = glm::translate(glm::scale(m3x3f32(1), v2f32(1.f / dims.x, -1.f / dims.y)), -v2f32(10));//TODO parameterises this, currently uses assumed render rect of 1x1 with origin at its center
	auto transform = glm::scale(glm::translate(m3x3f32(1), v2f32(-.5f, .5f)), v2f32(1.f / dims.x, -1.f / dims.y));//TODO parameterises this, currently uses assumed render rect of 1x1 with origin at its center
	auto outlines = outline_polygons(allocator, source, pixel_clip, transform, is_collider);
	return create_polyshape(allocator, outlines);
}

template<i32 D> AnimationGrid<Shape2D, D> create_animated_shape(Alloc allocator, const Image& source, AnimationGrid<rtf32, D> animation, auto is_collider) {
	return { map(allocator, animation.keyframes, [&](rtf32 clip) { return create_frame_shape(allocator, source, clip, is_collider); }), animation.dimensions };
}

#endif
