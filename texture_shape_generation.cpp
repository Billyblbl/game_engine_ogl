#ifndef GTEXTURE_SHAPE_GENERATION
# define GTEXTURE_SHAPE_GENERATION

#include <blblstd.hpp>
#include <physics_2d.cpp>
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

Array<v2i32> flood_outline(Alloc allocator, bool* markings, v2i32 start, v2u64 dimensions, auto mask, bool corner = true) {
	auto size = dimensions.x * dimensions.y;
	List<v2i32> outline = { alloc_array<v2i32>(allocator, size), 0 };
	List<v2i32> to_fill = { alloc_array<v2i32>(allocator, size), 0 };
	to_fill.push(start);
	markings[start.x + start.y * dimensions.x] = true;

	while (to_fill.current > 0) {
		auto coord = to_fill.pop();
		bool is_outline = false;
		for (v2i32 nb : larray(grid_neigbhours).subspan(0, corner ? 8 : 4)) {
			auto px = coord + nb;
			if (mask(px)) {
				if (!markings[px.x + px.y * dimensions.x]) {
					to_fill.push(px);
					markings[px.x + px.y * dimensions.x] = true;
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

Array<Segment<v2f32>> _outline_segments;

Array<v2f32> decimate(Alloc allocator, Array<v2f32> poly) {
	auto pruned = List{ duplicate_array(allocator, poly), poly.size() };
	i64 idx = 0;
	while ((idx = linear_search_idx(pruned.allocated(), [&](v2f32, i64 i) { return wo_at(pruned.allocated(), i) == WindingOrder::Unknown; }, idx)) >= 0)
		pruned.remove_ordered(idx);

	return pruned.shrink_to_content(allocator);
}

Array<Segment<v2f32>> create_pieces_ms(Alloc allocator, bool* markings, v2u64 dimensions, Array<v2i32> pixels) {

	auto scratch = create_virtual_arena(5000000); defer{ destroy_virtual_arena(scratch); };//TODO replace with thread local arena scheme

	v2i32 pieces_offsets[] = { v2i32(0), v2i32(1, 0), v2i32(0, 1), v2i32(1) };

	bool secondary_markings[dimensions.y + 1][dimensions.x + 1];
	auto segments = List{ alloc_array<Segment<v2f32>>(allocator, pixels.size() * 4), 0 };

	// TODO test that ALL pieces are properly oriented
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

				{ v2f32(0, .5f), v2f32(1, .5f) }, //12
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
			samples |= markings[sample_targets[i].x + sample_targets[i].y * dimensions.x] ? 1 << i : 0;
		segments.push_range(create_piece_geometry(piece, samples));
	}

	_outline_segments = duplicate_array(std_allocator, segments.allocated());
	return segments.shrink_to_content(allocator);
}

Array<v2f32> weld_segments(Alloc allocator, Array<Segment<v2f32>> pieces) {
	auto input = List{ pieces, pieces.size() };
	auto outline = List{ alloc_array<v2f32>(allocator, pieces.size()), 0 };

	for (auto i : u64xrange{ 0, pieces.size() }) {
		i64 index = 0;
		if (i > 0)
			index = best_fit_search(input.allocated(), [&](const Segment<v2f32>& piece) { return -glm::distance(piece.A, outline.allocated().back()); });
		if (index < 0)
			return outline.allocated();
		outline.push(input.allocated()[index].B);
		input.swap_out(index);
	}
	return outline.allocated();
}

#define for2(i, r) for (auto y : u32xrange{ r.min.y, r.max.y } ) for (auto x : u32xrange { r.min.x, r.max.x}) if (i = v2u32(x, y); true)
// for2(v2u32 pixel, (rtu32{ v2u32(0), source.dimensions })) if (free_spot(source[v2i32(x, y)]) && is_collider(source[v2i32(x, y)])) {

Polygon _test_outline;
Polygon _contour_welded;

Polygon _test_outlines_decomposed_buff[100000];
List<Polygon> _test_outlines_decomposed = { larray(_test_outlines_decomposed_buff), 0 };

//* completely untested
//TODO test
Shape2D generate_shape(Alloc allocator, const Image& source, auto is_collider) {
	printf("Generating shape from image\n");
	printf("Format : { type : %s, channels %s, channel count %u, channel size %u }\n", GLtoString(source.format.type).data(), GLtoString(source.format.type).data(), source.format.channel_count, source.format.channel_size);
	printf("Dimensions : (%f,%f)\n", source.dimensions.x, source.dimensions.y);
	printf("Pixel size : %lu\n", source.pixel_size);

	//TODO replace with thread local storage scratch
	auto scratch = create_virtual_arena(5000000); defer{ destroy_virtual_arena(scratch); };

	bool markings[source.dimensions.x][source.dimensions.y];
	auto size = source.dimensions.x * source.dimensions.y;
	memset(markings, 0, size);

	auto polygons = List{ alloc_array<Polygon>(as_v_alloc(scratch), 100), 0 };
	u64 vertex_count = 0;

	auto flood_filter = [&](v2i32 coord) { return inside(source.dimensions, coord) && is_collider(source[v2u64(coord)]); };

	for (auto pixel : range2(source.dimensions)) {
		if (!markings[pixel.y][pixel.x] && is_collider(source[v2u64(pixel)])) {
			auto outline_pixels = flood_outline(as_v_alloc(scratch), &markings[0][0], pixel, source.dimensions, flood_filter, false);
			printf("Marking used pixels\n");
			for (auto coord : outline_pixels)
				markings[coord.y][coord.x] = true;

			printf("Creating marching squares contour pieces\n");
			//TODO add target rect sourced transformation
			auto pieces = create_pieces_ms(as_v_alloc(scratch), &markings[0][0], source.dimensions, outline_pixels);
			printf("Welding contour pieces\n");
			auto contour = weld_segments(as_v_alloc(scratch), pieces);
			_contour_welded = duplicate_array(std_allocator, contour);
			printf("Pruning contour vertices (%lu)\n", contour.size());
			auto pruned = decimate(as_v_alloc(scratch), contour);
			printf("Ear clipping polygon (%lu)\n", pruned.size());
			auto [sub_polys, vertices] = ear_clip(pruned, as_v_alloc(scratch));
			printf("Ear clipping complete : %lu polygons, %lu vertices\n", sub_polys.size(), vertices.size());

			_test_outlines_decomposed.push_range(map(as_v_alloc(scratch), sub_polys, [](Polygon poly) { return duplicate_array(std_allocator, poly); }));

			vertex_count += vertices.size();
			polygons.push_range_growing(as_v_alloc(scratch), sub_polys);
		}
	}

	if (polygons.current == 0)
		return null_shape;

	if (polygons.current == 1)
		return make_shape_2d<Shape2D::Polygon>(duplicate_array(allocator, polygons.allocated()[0]));

	auto vertices = List{ alloc_array<v2f32>(allocator, vertex_count), 0 };
	auto shapes = map(allocator, polygons.allocated(), [&](Array<v2f32> poly) -> Shape2D { return make_shape_2d<Shape2D::Polygon>(vertices.push_range(poly)); });
	return make_shape_2d<Shape2D::Concave>(shapes);
}


#endif
