#ifndef GTILEMAP
# define GTILEMAP

#include <rendering.cpp>
#include <textures.cpp>
#include <atlas.cpp>
#include <tmx_module.h>

struct Tilemap {

	Atlas2D layer_atlas;
	tmx_map* tree;
	Array<rtu32> atlas_views;
	Array<rtu32> tiles;

	static Image layer_gids(Arena& arena, const tmx_layer& layer, v2u32 dimensions) {
		return make_image(map(arena, carray(layer.content.gids, dimensions.x * dimensions.y), [](u32 gid) -> u32 { return gid & TMX_FLIP_BITS_REMOVAL; }), dimensions, 1);
	}

	static Tilemap load(Arena& arena, Atlas2D& texture_atlas, const cstr path, auto parse_object_group) {
		//* https://libtmx.readthedocs.io/en/latest/renderer-from-scratch.html
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		printf("Loading tilemap %s\n", path);
		Tilemap tm;

		auto source = tmx_load(path);
		if (!source)(tmx_perror("Tilemap Loading"), abort());//TODO handle failed load

		auto heuristic_layer_count = count<tmx_layer, &tmx_layer::next>(source->ly_head);
		tm.layer_atlas = Atlas2D::create(v2u32(source->width, source->height * heuristic_layer_count), R32UI);
		tm.tree = source;

		auto [scratch, scope] = scratch_push_scope(1lu << 17, &arena); defer{ scratch_pop_scope(scratch, scope); };
		List<rtu32> views = { cast<rtu32>(scratch.push_bytes(1lu << 16, alignof(rtu32))), 0 };

		{ //* load tilesets
			PROFILE_SCOPE("tilesets images");
			for (auto& entry : traverse_by<tmx_tileset_list, &tmx_tileset_list::next>(source->ts_head)) {
				entry.tileset->user_data.integer = views.current;
				views.push(texture_atlas.load(entry.tileset->image->source));
			}
		}

		num_range<u64> tiles_range = { views.current, views.current };
		{ //* Load tiles
			PROFILE_SCOPE("tiles");
			auto tiles = map(scratch, carray(source->tiles, source->tilecount),
				[&](tmx_tile* tile) {
					if (!tile) return rtu32{};
					tile->user_data.integer = views.current;
					auto tileset_view = views[tile->tileset->user_data.integer];
					auto pos = v2u32(tile->ul_x, tile->ul_y);
					auto dims = v2u32(tile->tileset->tile_width, tile->tileset->tile_height);
					return sub_rect(tileset_view, rtu32{ pos, pos + dims });
				}
			);
			views.push(tiles);
			tiles_range.max = views.current;
			printf("Loaded %llu tiles\n", tiles_range.size());
		}

		{ //* Load layers
			PROFILE_SCOPE("layers");
			auto traverse = (
				[&](auto const& recurse, tmx_layer* list)->void {
					for (auto& layer : traverse_by<tmx_layer, &tmx_layer::next>(list)) if (layer.visible) {
						switch (layer.type) {
						case L_GROUP: { recurse(recurse, layer.content.group_head); } break;
						case L_OBJGR: { parse_object_group(layer.content.objgr, v2u32(tm.tree->tile_width, tm.tree->tile_height)); } break;
						case L_IMAGE: {
							layer.user_data.integer = views.current;
							views.push(texture_atlas.load(layer.content.image->source));
						} break;
						case L_LAYER: {
							layer.user_data.integer = views.current;
							printf("Loading Layer %s\n", layer.name);
							//* Currently ignores all tmx flip bits configs;
							auto layer_size = source->width * source->height;
							auto [layer_scratch, layer_scope] = scratch_push_scope(layer_size * sizeof(u32), { &arena, &scratch }); defer{ scratch_pop_scope(layer_scratch, layer_scope); };
							auto layer_data = map(layer_scratch, carray(layer.content.gids, layer_size), [](u32 gid) -> u32 { return gid & TMX_FLIP_BITS_REMOVAL; });
							views.push(tm.layer_atlas.push(make_image(layer_data, v2u32(source->width, source->height), 1)));
						} break;
						default: break;
						}
					}
				}
			);
			traverse(traverse, source->ly_head);

		}
		tm.atlas_views = arena.push_array(views.used());
		tm.tiles = tm.atlas_views.subspan(tiles_range.min, tiles_range.size());
		return tm;
	}

	static Tilemap load(Arena& arena, Atlas2D& texture_atlas, const cstr path) { return load(arena, texture_atlas, path, [](tmx_object_group*, v2u32) {}); }

	void release() {
		tmx_map_free(tree);
		layer_atlas.release();
	}

};

bool EditorWidget(const cstr label, Tilemap& tm) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("Layer Atlas", tm.layer_atlas);
		changed |= EditorWidget("Tree", tm.tree);
		changed |= EditorWidget("Atlas Views", tm.atlas_views);
		changed |= EditorWidget("Tiles", tm.tiles);
	}
	return changed;
}

struct TilemapRenderer {

	struct Instance {
		m4x4f32 matrix;
		rtu32 rect;
	};

	struct Scene {
		m4x4f32 view_matrix;
		v2u32 tilemap_atlas_size;
		v2u32 texture_atlas_size;
		f32 alpha_discard;
	};

	struct {
		ShaderInput instances;
		ShaderInput tiles;
		ShaderInput scene;
		ShaderInput tilemap;
		ShaderInput atlas;
	} inputs;

	GLuint pipeline;
	RenderMesh rect;

	void operator()(const Tilemap& tilemap, const m4x4f32& tm_transform, const m4x4f32& vp, const TexBuffer& texture_atlas) {
		//* https://libtmx.readthedocs.io/en/latest/renderer-from-scratch.html

		auto instances = List{ cast<Instance>(inputs.instances.backing_buffer.map()), 0 };
		auto traverse = (
			[&](const auto& recurse, tmx_layer* list)->u64 {
				for (auto& layer : traverse_by<tmx_layer, &tmx_layer::next>(list)) if (layer.visible) {
					auto layer_transform = m4x4f32(1);//TODO per layer transformations
					switch (layer.type) {
					case L_NONE: {} break;
					case L_LAYER: {
						auto rect_transform =
							glm::scale(v3f32(tilemap.tree->width, tilemap.tree->height, 1)) *
							glm::translate(v3f32(.5f, -.5f, 0));
						instances.push(
							{
								tm_transform *
								layer_transform *
								rect_transform,
								tilemap.atlas_views[layer.user_data.integer]
							}
						);
					} break;
					case L_IMAGE: {} break;//TODO
					case L_OBJGR: {} break;
					case L_GROUP: { recurse(recurse, layer.content.group_head); } break;
					}
				}
				return instances.current;
			}
		);
		auto instance_count = traverse(traverse, tilemap.tree->ly_head);
		// inputs.instances.backing_buffer.flush({0, instance_count * sizeof(Instance)});
		inputs.instances.backing_buffer.unmap();

		GL_GUARD(glUseProgram(pipeline)); defer{ GL_GUARD(glUseProgram(0)); };
		GL_GUARD(glBindVertexArray(rect.vao.id)); defer{ GL_GUARD(glBindVertexArray(0)); };

		inputs.instances.bind({ 0, instance_count * sizeof(Instance) }).size(); defer{ inputs.instances.unbind(); };
		inputs.tiles.bind_content(tilemap.tiles); defer{ inputs.tiles.unbind(); };
		inputs.scene.bind_object(Scene{ vp, tilemap.layer_atlas.texture.dimensions, texture_atlas.dimensions, 0.001f }); defer{ inputs.scene.unbind(); };
		inputs.tilemap.bind_texture(tilemap.layer_atlas.texture.id); defer{ inputs.tilemap.unbind(); };
		inputs.atlas.bind_texture(texture_atlas.id); defer{ inputs.atlas.unbind(); };

		GL_GUARD(glDrawElementsInstanced(rect.vao.draw_mode, rect.vao.element_count, rect.vao.index_type, null, instance_count));
	}

	static TilemapRenderer load(const cstr pipeline_path, u64 max_draw_batch = 256, u64 max_tiles = 1000, const RenderMesh* mesh = null) {
		TilemapRenderer rd;
		rd.pipeline = load_pipeline(pipeline_path);
		rd.rect = mesh ? *mesh : create_rect_mesh(v2f32(1));
		rd.inputs.atlas = ShaderInput::create_slot(rd.pipeline, ShaderInput::Texture, "atlas");
		rd.inputs.tilemap = ShaderInput::create_slot(rd.pipeline, ShaderInput::Texture, "tilemap_atlas");
		rd.inputs.instances = ShaderInput::create_slot(rd.pipeline, ShaderInput::SSBO, "Entities", sizeof(Instance) * max_draw_batch);
		rd.inputs.tiles = ShaderInput::create_slot(rd.pipeline, ShaderInput::SSBO, "Tiles", sizeof(rtu32) * max_tiles);
		rd.inputs.scene = ShaderInput::create_slot(rd.pipeline, ShaderInput::UBO, "Scene", sizeof(Scene));
		return rd;
	}

	void release() {
		inputs.atlas.release();
		inputs.tilemap.release();
		inputs.instances.release();
		inputs.tiles.release();
		inputs.scene.release();
		rect.release();
		destroy_pipeline(pipeline);
	}
};

#include <shape_2d.cpp>


Array<Shape2D> object_shape(Arena& arena, tmx_object* obj_head) {
	static v2f32 g_origin[] = { v2f32(0) };
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	auto surface_count = count<tmx_object, &tmx_object::next>(obj_head);
	auto [scratch, scope] = scratch_push_scope(max(1lu << 22, surface_count * 2), &arena); defer{ scratch_pop_scope(scratch, scope); };
	auto objs = List{ scratch.push_array<Shape2D>(surface_count * 2), 0 };
	for (auto& obj : traverse_by<tmx_object, &tmx_object::next>(obj_head)) {
		auto transform = identity_2d;
		transform.translation = v2f32(obj.x, obj.y);
		transform.rotation = glm::radians(obj.rotation);
		auto push_content_points = [&](Arena& arena, tmx_object& obj) { return map(arena, carray(obj.content.shape->points, obj.content.shape->points_len), [&](f64* pt) -> v2f32 { return v2f32(pt[0], pt[1]);}); };
		switch (obj.obj_type) {
		case OT_POLYLINE: {
			auto points = push_content_points(arena, obj);
			for (auto i : u64xrange{ 0, points.size() - 1 })
				objs.push_growing(scratch, make_shape_2d(transform, 0, points.subspan(i, 2)));
		} break;
		case OT_POLYGON: {
			auto local_scope = scratch.current;
			auto concave = push_content_points(scratch, obj);
			auto [polys, verts] = ear_clip(arena, concave);
			scratch_pop_scope(scratch, local_scope);
			for (auto poly : polys)
				objs.push_growing(scratch, make_shape_2d(transform, 0, poly));
		} break;
		case OT_POINT: { objs.push_growing(scratch, make_shape_2d(transform, 0, larray(g_origin))); } break;
		case OT_SQUARE: {
			objs.push_growing(scratch, make_box_shape(arena.push_array<v2f32>(4), v2f32(obj.width, obj.height), v2f32(obj.width, obj.height) / 2.f, transform));
		} break;
		case OT_ELLIPSE: {
			auto dims = v2f32(obj.width, obj.height);
			transform.translation += dims / 2.f;
			transform.scale *= dims / v2f32(max(dims.x, dims.y));
			objs.push_growing(scratch, make_circle_shape(max(dims.x, dims.y) / 2.f, transform));
		} break;
		default: break;;
		}
	}
	return arena.push_array(objs.used());
}

Array<Shape2D> tile_shapeset(Arena& arena, Array<tmx_tile*> tiles) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	return map(arena, tiles, [&](tmx_tile* tile) -> Shape2D { return make_shape_2d(identity_2d, 0, {}, tile ? object_shape(arena, tile->collision) : Array<Shape2D>{}); });
}

Shape2D tilemap_layer_shape(Arena& arena, const tmx_layer& layer, v2u32 dimensions, Array<Shape2D> shapeset, v2u32 tile_dimensions) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	auto [scratch, scope] = scratch_push_scope(1ul << 19, &arena); defer{ scratch_pop_scope(scratch, scope); };

	Transform2D transform = identity_2d;
	transform.translation = v2f32(layer.offsetx, -layer.offsety) / v2f32(tile_dimensions);
	transform.scale.y = -1;

	v2u32 chunk_dimensions = dimensions;
	if (auto x_prop = tmx_get_property(layer.properties, "chunk_optimize_x"); x_prop && x_prop->type == PT_INT) chunk_dimensions.x = x_prop->value.integer;
	if (auto y_prop = tmx_get_property(layer.properties, "chunk_optimize_y"); y_prop && y_prop->type == PT_INT) chunk_dimensions.y = y_prop->value.integer;

	auto gids = Tilemap::layer_gids(scratch, layer, dimensions);
	auto chunk_count = chunk_dimensions.x * chunk_dimensions.y;

	auto leaf_chunk = (
		[&](rtu32 tilemap_rect) -> Array<Shape2D> {
			auto rect_dims = dim_vec(tilemap_rect);
			auto shapes = List{ arena.push_array<Shape2D>(rect_dims.x * rect_dims.y), 0 };
			for (auto y : u64xrange{ tilemap_rect.min.y, tilemap_rect.max.y }) for (auto x : u64xrange{ tilemap_rect.min.x, tilemap_rect.max.x }) {
				auto gid = cast<u32>(gids[v2f32(x, y)])[0];
				if (gid == 0) continue;
				Transform2D cell_transform = identity_2d;
				cell_transform.translation = v2f32(x, y);
				cell_transform.scale = v2f32(1) / v2f32(tile_dimensions);
				shapes.push(transform_shape(shapeset[gid], cell_transform));
			}
			return shapes.shrink_to_content(arena);
		}
	);

	auto sub_rect = (
		[&](rtu32 tilemap_rect, v2u32 coord) -> rtu32 {
			auto rect_dims = dim_vec(tilemap_rect);
			auto cell_size = rect_dims / chunk_dimensions;
			auto res = rtu32{ tilemap_rect.min + coord * cell_size, tilemap_rect.min + (coord + v2u32(1)) * cell_size };
			//* the ends of the rect gets extended to account for the tiles that might have been lost in the remainder of cell_size computation
			auto ends = glm::equal(coord, chunk_dimensions - v2u32(1));
			for (auto i : u64xrange{ 0, 2 }) if (ends[i])
				res.max[i] = tilemap_rect.max[i];
			return res;
		}
	);

	auto generate = (
		[&](auto& recurse, rtu32 tilemap_rect) -> Array<Shape2D> {
			auto rect_dims = dim_vec(tilemap_rect);
			auto tile_count = rect_dims.x * rect_dims.y;
			if (chunk_count < tile_count) {
				auto shapes = List{ scratch.push_array<Shape2D>(chunk_dimensions.x * chunk_dimensions.y), 0 };
				for (auto y : u64xrange{ 0, chunk_dimensions.y }) for (auto x : u64xrange{ 0, chunk_dimensions.x })
					if (auto sub_shapes = recurse(recurse, sub_rect(tilemap_rect, v2u32(x, y))); sub_shapes.size() > 0)
						shapes.push(make_shape_2d(m3x3f32(1), 0, {}, sub_shapes));
				return arena.push_array(shapes.shrink_to_content(scratch));
			} else return leaf_chunk(tilemap_rect);
		}
	);

	return make_shape_2d(transform, 0, {}, generate(generate, { v2u32(0), dimensions }));
}

Array<Shape2D> tilemap_shapes(Arena& arena, const tmx_map& tree, Array<Shape2D> shapeset) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	auto [scratch, scope] = scratch_push_scope(1lu << 17, &arena); defer{ scratch_pop_scope(scratch, scope); };
	auto traverse = (
		[&](auto const& recurse, tmx_layer* list)->Array<Shape2D> {
			auto local_scope = scratch.current; defer{ scratch_pop_scope(scratch, local_scope); };
			auto shapes = List{ scratch.push_array<Shape2D>(1000), 0 };
			for (auto& layer : traverse_by<tmx_layer, &tmx_layer::next>(list)) if (layer.visible) {
				switch (layer.type) {
				case L_NONE: {} break;
				case L_LAYER: {
					shapes.push(tilemap_layer_shape(arena, layer, v2u32(tree.width, tree.height), shapeset, v2f32(tree.tile_width, tree.tile_width)));
				} break;
				case L_OBJGR: {} break;
				case L_IMAGE: {} break;
				case L_GROUP: {
					Transform2D transform = identity_2d;
					transform.translation = v2f32(layer.offsetx, layer.offsety) / v2f32(tree.tile_width, tree.tile_width);
					shapes.push(make_shape_2d(transform, 0, {}, recurse(recurse, layer.content.group_head)));
				} break;
				}
			}
			return arena.push_array(shapes.used());
		}
	);
	return traverse(traverse, tree.ly_head);
}

#endif
