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
	// Shape2D shapes;

	static Tilemap load(Arena& arena, Atlas2D& texture_atlas, const cstr path) {
		//https://libtmx.readthedocs.io/en/latest/renderer-from-scratch.html
		PROFILE_SCOPE(__FUNCTION__);
		printf("Loading tilemap %s\n", path);
		Tilemap tm;

		profile_scope_begin("tmx_load");
		auto source = tmx_load(path);
		profile_scope_end();
		auto heuristic_layer_count = count<tmx_layer, &tmx_layer::next>(source->ly_head);

		tm.layer_atlas = Atlas2D::create(v2u32(source->width, source->height * heuristic_layer_count), R32UI);
		tm.tree = source;

		List<rtu32> views = { arena.push_array<rtu32>(heuristic_layer_count * source->tilecount), 0 };

		//* load tilesets
		{
			PROFILE_SCOPE("tilesets");
			for (auto& entry : traverse_by<tmx_tileset_list, &tmx_tileset_list::next>(source->ts_head)) {
				//TODO check entry.tileset validity, might need to be manually loaded
				entry.tileset->user_data.integer = views.current;
				views.push_growing(arena, texture_atlas.load(entry.tileset->image->source));
			}
		}

		//* Load tiles
		{
			PROFILE_SCOPE("tiles");
			auto tiles_start = views.current;
			for (auto* tile : carray(source->tiles, source->tilecount)) if (tile) {
				tile->user_data.integer = views.current;
				auto tileset_view = views[tile->tileset->user_data.integer];
				auto pos = v2u32(tile->ul_x, tile->ul_y);
				auto dims = v2u32(tile->tileset->tile_width, tile->tileset->tile_height);
				views.push_growing(arena, sub_rect(tileset_view, rtu32{ pos, pos + dims }));
			} else {
				views.push_growing(arena, rtu32{});
			}
			tm.tiles = views.used().subspan(tiles_start, views.current - tiles_start);
			printf("Loaded %lu tiles\n", tm.tiles.size());
		}

		{ //* Load layers
			PROFILE_SCOPE("layers");
			auto traverse = [&](auto const& recurse, tmx_layer* list)->void {
				for (auto& layer : traverse_by<tmx_layer, &tmx_layer::next>(list)) if (layer.visible) {
					switch (layer.type) {
					case L_GROUP: { recurse(recurse, layer.content.group_head); } break;
					case L_OBJGR: {
						//TODO accumulate shapes
					} break;
					case L_IMAGE: {
						layer.user_data.integer = views.current;
						views.push_growing(arena, texture_atlas.load(layer.content.image->source));
					} break;
					case L_LAYER: {
						layer.user_data.integer = views.current;
						printf("Loading Layer %s\n", layer.name);
						//* Currently ignores all tmx flip bits configs;
						auto layer_size = source->width * source->height;
						auto [scratch, scope] = scratch_push_scope(layer_size * sizeof(u32)); defer{ scratch_pop_scope(scratch, scope); };
						auto layer_data = map(scratch, carray(layer.content.gids, layer_size), [](u32 gid) -> u32 { return gid & TMX_FLIP_BITS_REMOVAL; });
						views.push_growing(arena, tm.layer_atlas.push(make_image(layer_data, v2u32(source->width, source->height), 1)));
					} break;
					}
				}
				};
			traverse(traverse, source->ly_head);
		}

		//* requires "deducing this" c++23 feature, missing in GCC
		//* https://en.cppreference.com/w/cpp/compiler_support
		// //* Load layers
		// [&](this auto const& recurse, tmx_layer* list)->void {
		// 	for (auto& layer : traverse_by<tmx_layer, &tmx_layer::next>(list)) if (layer.visible) {
		// 		switch (layer.type) {
		// 		case L_GROUP: { recurse(layer.content.group_head); } break;
		// 		case L_OBJGR: {
		// 			//TODO accumulate shapes
		// 		} break;
		// 		case L_IMAGE: {
		// 			layer.user_data.integer = views.current;
		// 			views.push_growing(arena, texture_atlas.load(layer.content.image->source));
		// 		} break;
		// 		case L_LAYER: {
		// 			layer.user_data.integer = views.current;
		// 			views.push_growing(arena, tm.layer_atlas.push(make_image(carray(layer.content.gids, source->width * source->height), v2u32(source->width, source->height), 1)));
		// 		} break;
		// 		}
		// 	}
		// 	}
		// (source->ly_head);

		tm.atlas_views = views.shrink_to_content(arena);
		return tm;
	}

	void release() {
		tmx_map_free(tree);
		layer_atlas.release();
	}

};

void EditorWidget(const cstr label, Tilemap& tm) {
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
	}
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

	Pipeline pipeline;
	MappedBuffer<Instance> instances_buffer;
	MappedBuffer<rtu32> tile_buffer;
	MappedObject<Scene> scene;
	RenderMesh rect;

	void operator()(const Tilemap& tilemap, const m4x4f32& tm_transform, const m4x4f32& vp, const TexBuffer& texture_atlas) {
		//* https://libtmx.readthedocs.io/en/latest/renderer-from-scratch.html
		List<Instance> instances = { instances_buffer.content, 0 };

		auto traverse = [&](const auto& recurse, tmx_layer* list)->void {
			for (auto& layer : traverse_by<tmx_layer, &tmx_layer::next>(list)) if (layer.visible) {
				auto layer_transform = m4x4f32(1);//TODO per layer transformations
				switch (layer.type) {
				case L_GROUP: { recurse(recurse, layer.content.group_head); } break;
				case L_IMAGE: {} break;//TODO
				case L_LAYER: { instances.push({ tm_transform * glm::scale(v3f32(tilemap.tree->width, tilemap.tree->height, 1)) * layer_transform, tilemap.atlas_views[layer.user_data.integer] }); } break;
				}
			}
			};
		traverse(traverse, tilemap.tree->ly_head);

		//* requires "deducing this" c++23 feature, missing in GCC
		// [&](this const auto& recurse, tmx_layer* list)->void {
		// 	for (auto& layer : traverse_by<tmx_layer, &tmx_layer::next>(list)) if (layer.visible) {
		// 		switch (layer.type) {
		// 		case L_GROUP: { recurse(layer.content.group_head); } break;
		// 		case L_IMAGE: {} break;//TODO
		// 		case L_LAYER: { instances.push({ m4x4f32(1), tilemap.atlas_views[layer.user_data.integer] }); } break;
		// 		}
		// 	}
		// 	}
		// (tilemap.tree->ly_head);

		copy(instances.used(), instances_buffer.content);
		copy(tilemap.tiles, tile_buffer.content);
		sync(instances_buffer);
		sync(tile_buffer);
		sync(scene, { vp, tilemap.layer_atlas.texture.dimensions, texture_atlas.dimensions, 0.001f });
		pipeline(rect, instances.current,
			{
				bind_to(texture_atlas, 0),
				bind_to(tilemap.layer_atlas.texture, 1),
				bind_to(instances_buffer, 0),
				bind_to(tile_buffer, 1),
				bind_to(scene, 0),
			}
		);
	}

	static TilemapRenderer load(const cstr pipeline_path, u64 max_draw_batch = 256, u64 max_tiles = 1920 * 1080, const RenderMesh* mesh = null) {
		TilemapRenderer rd;
		rd.pipeline = load_pipeline(pipeline_path);
		rd.instances_buffer = map_buffer<TilemapRenderer::Instance>(max_draw_batch);
		rd.tile_buffer = map_buffer<rtu32>(max_tiles);
		rd.scene = map_object<TilemapRenderer::Scene>({});
		rd.rect = mesh ? *mesh : create_rect_mesh(v2f32(1));
		return rd;
	}

	void unload() {
		destroy_pipeline(pipeline);
		unmap(instances_buffer);
		unmap(tile_buffer);
		unmap(scene);
		delete_mesh(rect);
	}
};



#endif
