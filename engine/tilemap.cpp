#ifndef GTILEMAP
# define GTILEMAP

#include <rendering.cpp>
#include <textures.cpp>
#include <atlas.cpp>
#include <sprite.cpp>
#include <tmx_module.h>
#include <spall/profiling.cpp>

#include <physics_2d.cpp>

namespace Tilemap {

	static tmx_map* load_source(const cstr path) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		printf("Loading tilemap %s\n", path);
		auto source = tmx_load(path);
		if (!source)
			tmx_perror("Tilemap");
		else
			source->user_data.pointer = (void*)path;
		return source;
	}

	static auto load_proc(const cstr path, auto proc) {
		auto source = load_source(path); defer{ tmx_map_free(source); };
		return proc(*source);
	}

	tmx_property* expect_property(tmx_properties* props, tmx_property_type type, const cstr name) {
		auto prop = tmx_get_property(props, name);
		return prop && prop->type == type ? prop : null;
	}

	template<typename T> struct Array2D {
		Array<T> data;
		v2u32 dimensions;

		u64 index(v2u32 coord) const {
			assert(glm::all(glm::lessThan(coord, dimensions)));
			return coord.x + coord.y * dimensions.x;
		}

		T& operator[](v2u64 coord) { return data[index(coord)]; };
		const T& operator[](v2u64 coord) const { return data[index(coord)]; };
	};


	Array2D<u32> get_layer_tiles(Arena& arena, const tmx_layer& layer, v2u32 dimensions, bool remove_flip_bits = true) {
		auto content = carray(layer.content.gids, dimensions.x  * dimensions.y);
		if (remove_flip_bits)
			content = map(arena, content, [&](auto gid){ return gid & TMX_FLIP_BITS_REMOVAL; });
		else
			content = arena.push_array(content);
		return { .data = content, .dimensions = dimensions };
	}

	//* Rendering

	struct Scene {
		m4x4f32 view_projection;
		v2f32 parallax_pov;
		f32 alpha_discard;
		byte padding[4];
	};

	struct alignas(16) Quad {
		rtu32 layer_sprite;
		v2f32 parallax;
		f32 depth;
	};

	struct Renderer {
		GPUBuffer indices;
		GPUBuffer vertices;
		GPUBuffer quads;
		GPUBuffer sprites;
		GPUBuffer scene;
		TexBuffer layers;
		TexBuffer albedo;
		VertexArray vao;
	};

	Array<tmx_tile*> get_tiles(const tmx_map& source) { return carray(source.tiles, source.tilecount); }

	rtu32 make_tile(rtu32 spritesheet, const tmx_tile* tile) {
		if (!tile) return rtu32{};
		auto pos = v2u32(tile->ul_x, tile->ul_y);
		auto dims = v2u32(tile->tileset->tile_width, tile->tileset->tile_height);
		return sub_rect(spritesheet, rtu32{ pos, pos + dims });
	}

	struct Pipeline {
		GLuint id;

		struct {
			GLuint albedo_atlas;
			GLuint tilemap_layers;

			GLuint sprites;
			GLuint scene;

			struct {
				GLuint positions;
				GLuint quads;
			} vertices;
		} inputs;

		static Pipeline create(GLScope& ctx, const cstr path = "shaders/tilemap.glsl") {
			PROFILE_SCOPE(__PRETTY_FUNCTION__);
			auto ppl = load_pipeline(ctx, path);
			return {
				.id = ppl,
				.inputs = {
					.albedo_atlas = get_shader_input(ppl, "albedo_atlas", R_TEX),
					.tilemap_layers = get_shader_input(ppl, "tilemap_layers", R_TEX),
					.sprites = get_shader_input(ppl, "Sprites", R_SSBO),
					.scene = get_shader_input(ppl, "Scene", R_UBO),
					.vertices = {
						.positions = get_shader_input(ppl, "position", R_VERT),
						.quads = get_shader_input(ppl, "Quads", R_SSBO)
					}
				}
			};
		}

		Renderer make_renderer(GLScope& ctx, const tmx_map& source) {
			PROFILE_SCOPE(__PRETTY_FUNCTION__);
			auto [scratch, scope] = scratch_push_scope(0, &ctx.arena); defer{ scratch_pop_scope(scratch, scope); };
			string original_path = (char*)source.user_data.pointer;
			auto slash = original_path.find_last_of('/');
			string dir = slash != string::npos ? original_path.substr(0, slash) : ".";
			auto make_rel_path = [dir](Arena& arena, string path) -> string { return arena.format("%.*s/%.*s",
					i32(dir.size()), dir.data(),
					i32(path.size()), path.data()
			); };
			auto dimensions = v2u32(source.width, source.height);

			auto texture_atlas = Atlas2D::create(ctx, v2u32(2000));//TODO precompute atlas size

			//* Load tiles data
			auto tilesets_spritesheets = List{ scratch.push_array<rtu32>(8), 0 };
			for (auto& entry : traverse_by<tmx_tileset_list, &tmx_tileset_list::next>(source.ts_head)) {
				entry.tileset->user_data.integer = tilesets_spritesheets.current;
				string img = entry.tileset->image->source;
				string complete = make_rel_path(scratch, img);
				tilesets_spritesheets.push_growing(scratch, texture_atlas.load(complete));
			}
			tilesets_spritesheets.shrink_to_content(scratch);
			auto tiles = map(ctx.arena, get_tiles(source), [&](tmx_tile* tile)-> rtu32 { return make_tile(tilesets_spritesheets[tile ? tile->tileset->user_data.integer : 0], tile); });

			//* Count ressources

			//* x:rect y:layers z:images
			auto counts = [&](this const auto& recurse, tmx_layer* list) -> v3u64 {
				v3u64 tcount = v3u64(0);
				for (auto& layer : traverse_by<tmx_layer, &tmx_layer::next>(list)) if (layer.visible) switch(layer.type) {
					case L_LAYER: tcount += v3u64(1, 1, 0); break;
					case L_IMAGE: tcount += v3u64(1, 0, 1); break;
					case L_GROUP: tcount += recurse(layer.content.group_head); break;
					default: break;
				}
				return tcount;
			}(source.ly_head);
			auto rect_count = counts.x, layer_count = counts.y, img_count = counts.z;

			printf("Loading %llu rect : %llu layers + %llu images\n", rect_count, layer_count, img_count);

			auto layer_atlas = Atlas2D::create(ctx, v2u32(dimensions.x, dimensions.y * layer_count), R32UI);

			//* Load Tree vertex data
			v2f32 vertices[4 * rect_count];
			u32 indices[6 * rect_count];
			Quad quads[rect_count];
			u32 quad_idx = 0;

			auto tile_dimensions = v2u32(source.tile_width, source.tile_height);
			[&](this const auto& recurse, tmx_layer* list, v2f32 offset = v2f32(0)) -> void {
				for (auto& layer : traverse_by<tmx_layer, &tmx_layer::next>(list)) if (layer.visible) {
					auto local_offset = v2f32(layer.offsetx, layer.offsety) / v2f32(tile_dimensions);
					auto global_offset = offset + local_offset;
					auto parallax = v2f32(layer.parallaxx, layer.parallaxy);

					switch (layer.type) {
					case L_GROUP: recurse(layer.content.group_head, global_offset); break;
					case L_IMAGE: {
						auto sprite = texture_atlas.load(make_rel_path(scratch, layer.content.image->source));
						//TODO add geometry for image quad & handle its rendering in shader
						(void)sprite;
					} break;
					case L_LAYER: {
						auto rect = rtf32{ global_offset, global_offset + v2f32(dimensions) };
						auto [v, i] = QuadGeo::create(rect, 4 * quad_idx);
						copy(larray(v), carray(&vertices[4 * quad_idx], 4));
						copy(larray(i), carray(&indices[6 * quad_idx], 6));
						//TODO handle layer.tintcolor & layer.opacity
						auto cells = get_layer_tiles(scratch, layer, dimensions, false);
						quads[quad_idx] = {
							.layer_sprite = layer_atlas.push(make_image(cells.data, cells.dimensions, 1)),
							.parallax = v2f32(layer.parallaxx, layer.parallaxy),
							.depth = f32(quad_idx)//TODO decide how to assign depth
						};
						quad_idx += 1;
					} break;
					default: break;
					}
				}
			}(source.ly_head);

			Renderer rd = {
				.indices = GPUBuffer::upload(ctx, carray(indices, 6 * quad_idx)),
				.vertices = GPUBuffer::upload(ctx, carray(vertices, 4 * quad_idx)),
				.quads = GPUBuffer::upload(ctx, carray(quads, quad_idx)),
				.sprites = GPUBuffer::upload(ctx, tiles),
				.scene = GPUBuffer::create(ctx, sizeof(Scene), GL_DYNAMIC_STORAGE_BIT),
				.layers = layer_atlas.texture,
				.albedo = texture_atlas.texture,
				.vao = VertexArray::create(ctx)
			};

			rd.vao.conf_vattrib(inputs.vertices.positions, vattr_fmt<v2f32>(0));
			return rd;
		}

		RenderCommand operator()(Arena& arena, Renderer& renderer, const Scene& s) {
			PROFILE_SCOPE(__PRETTY_FUNCTION__);
			renderer.scene.write_one(s);
			return {
				.pipeline = id,
				.draw_type = RenderCommand::D_DE,
				.draw = {.d_element = {
					.count = GLuint(renderer.indices.content_as<u32>()),
					.instance_count = 1,
					.first_index = 0,
					.base_vertex = 0,
					.base_instance = 0
				}},
				.vao = renderer.vao.id,
				.ibo = {
					.buffer = renderer.indices.id,
					.index_type = GL_UNSIGNED_INT,
					.primitive = renderer.vao.draw_mode
				},
				.vertex_buffers = arena.push_array({
					VertexBinding{
						.buffer = renderer.vertices.id,
						.targets = arena.push_array({ inputs.vertices.positions }),
						.offset = 0,
						.stride = sizeof(v2f32),
						.divisor = 0
					}
				}),
				.textures = arena.push_array({
					TextureBinding{.textures = arena.push_array({ renderer.albedo.id }), .target = inputs.albedo_atlas },
					TextureBinding{.textures = arena.push_array({ renderer.layers.id }), .target = inputs.tilemap_layers },
				}),
				.buffers = arena.push_array({
					BufferObjectBinding {
						.buffer = renderer.scene.id,
						.type = GL_UNIFORM_BUFFER,
						.range = {},
						.target = inputs.scene
					},
					BufferObjectBinding {
						.buffer = renderer.sprites.id,
						.type = GL_SHADER_STORAGE_BUFFER,
						.range = {},
						.target = inputs.sprites
					},
					BufferObjectBinding {
						.buffer = renderer.quads.id,
						.type = GL_SHADER_STORAGE_BUFFER,
						.range = {},
						.target = inputs.vertices.quads
					}
				})
			};
		}
	};

	//* Physics
	struct Shape {
		Physics2D::Convex cvx;
		m3x3f32 transform;
	};

	struct TileCollider {
		Array<const Shape> shapes;
		m3x3f32 transform;
	};

	u32 get_layer_collision_layers(const tmx_layer& layer, const cstr name = "CollisionLayers") {
		auto prop = expect_property(layer.properties, PT_INT, name);
		if (prop)
			return prop->value.integer;
		else
			return 0;
	}

	Array<Shape> object_shape(Arena& arena, tmx_object* obj_head) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		auto surface_count = count<tmx_object, &tmx_object::next>(obj_head);
		auto [scratch, scope] = scratch_push_scope(0, &arena); defer{ scratch_pop_scope(scratch, scope); };
		auto cvxs = List{ scratch.push_array<Shape>(surface_count * 2), 0 };
		for (auto& obj : traverse_by<tmx_object, &tmx_object::next>(obj_head)) {
			auto push_content_points = [&](Arena& arena, const tmx_object& obj) { return map(arena, carray(obj.content.shape->points, obj.content.shape->points_len), [&](const f64* pt) -> v2f32 { return v2f32(pt[0], pt[1]);}); };
			m3x3f32 transform = Transform2D{
				.translation = v2f32(obj.x, obj.y),
				.scale = v2f32(1),
				.rotation = f32(obj.rotation)
			};
			rtf32 rect = { v2f32(0), v2f32(obj.width, obj.height) };
			switch (obj.obj_type) {
			case OT_POLYLINE: {
				auto points = push_content_points(scratch, obj);
				for (auto i : u64xrange{ 0, points.size() - 1 })
					cvxs.push_growing(scratch, { Physics2D::Convex::make(Segment{ points[i], points[i + 1] }, 0), transform });
			} break;
			case OT_POLYGON: {
				auto local_scope = scratch.current;
				auto concave = push_content_points(scratch, obj);
				auto [polys, verts] = ear_clip(arena, concave);
				scratch_pop_scope(scratch, local_scope);
				for (auto poly : polys)
					cvxs.push_growing(scratch, { Physics2D::Convex::make(poly, 0), transform });
			} break;
			case OT_POINT: { cvxs.push_growing(scratch, { Physics2D::Convex::ORIGIN(), transform }); } break;
			case OT_SQUARE: { cvxs.push_growing(scratch, { Physics2D::Convex::make(rect, 0), transform }); } break;
			case OT_ELLIPSE: {
				cvxs.push_growing(scratch, { Physics2D::Convex::UNIT_CIRCLE(), transform * m3x3f32(Transform2D{
					.translation = rect.center(),
					.scale = rect.size() / 2.f,
					.rotation = 0
				}) });
			} break;
			default: break;
			}
		}
		return arena.push_array(cvxs.used());
	}

	Array<TileCollider> tile_shapeset(Arena& arena, Array<tmx_tile*> tiles, v2u32 tile_dimensions) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		return map(arena, tiles,
			[&](tmx_tile* tile) -> TileCollider {
				if (tile)
					return {
						.shapes = object_shape(arena, tile->collision),
						.transform = Transform2D{
							.translation = v2f32(0),
							.scale = 1.f / v2f32(tile_dimensions),
							.rotation = 0
						}
					};
				else
					return {{}, {}};
			}
		);
	}

	struct Terrain {
		struct Collider {
			Array2D<u32> cells;
			rtf32 aabb;
			u32 collision_layers;
		};
		Array<const Collider> layers;
		Array<const TileCollider> tiles;

		static Terrain create(Arena& arena, const tmx_map& map) {
			PROFILE_SCOPE(__PRETTY_FUNCTION__);
			auto [scratch, scope] = scratch_push_scope(0, &arena); defer{ scratch_pop_scope(scratch, scope); };
			auto tile_dimensions = v2u32(map.tile_width, map.tile_height);
			auto layers = List{ scratch.push_array<Collider>(10), 0 };
			auto base_aabb = rtf32{ v2f32(0), v2f32(map.width, map.height) };
			[&](this const auto& recurse, tmx_layer* list, v2f32 offset = v2f32(0)) -> void {
				for (auto& layer : traverse_by<tmx_layer, &tmx_layer::next>(list)) if (layer.visible) {
					auto local_offset = v2f32(layer.offsetx, layer.offsety) / v2f32(tile_dimensions);
					auto global_offset = offset + local_offset;
					auto parallax = v2f32(layer.parallaxx, layer.parallaxy);
					switch (layer.type) {
						case L_GROUP: recurse(layer.content.group_head, global_offset); break;
						case L_LAYER:{
							auto flags = get_layer_collision_layers(layer);
							if (flags == 0) break;
							if (glm::all(glm::lessThan(abs(parallax - v2f32(1)), v2f32(0.001f)))) {
								fprintf(stderr, "Collision layer '%s' cannot have parallax != 1\n", layer.name);
								break;
							}
							layers.push_growing(scratch, {
								.cells = get_layer_tiles(arena, layer, v2u32(map.width, map.height)),
								.aabb = { base_aabb.min + global_offset, base_aabb.max + global_offset },
								.collision_layers = flags
							});
						} break;
						default: break;
					}
				}
			}(map.ly_head);
			return {
				.layers = arena.push_array(layers.used()),
				.tiles = tile_shapeset(arena, get_tiles(map), tile_dimensions)
			};
		}
	};

	tuple<i32xrange, i32xrange> grid_ranges(rti32 grid) {
		return {
			i32xrange{ grid.min.x, grid.max.x },
			i32xrange{ grid.min.y, grid.max.y }
		};
	}

	Array<Physics2D::NarrowTest> terrain_broadphase(Physics2D::SimStep& step, const Terrain& terrain, const Physics2D::FlagMatrix<u32>& detections, u32range collider_range = {}) {
		if (collider_range.size() == 0)
			collider_range = { 0, u32(step.colliders.current) };
		auto tests_start = step.tests.current;
		struct CellCache {
			num_range<u32> collider_range;
			i32 tile_collider_index;
		};
		auto terrain_bd = step.push_body({// TODO get properties from source
			.center_mass = v2f32(0),
			.momentum = { .vec = v3f32(0) },
			.props = {
				.inverse_mass = 0,
				.inverse_inertia = 0,
				.restitution = 0.5f,
				.friction = 0.5f
			}
		});
		auto cache_load_cell = [&](const Terrain::Collider& layer, v2u32 coord) -> CellCache {
			auto layer_offset = layer.aabb.min;
			m3x3f32 cell_xform = Transform2D{ .translation = layer_offset + v2f32(coord.x, layer.cells.dimensions.y - coord.y), .scale = v2f32(1, -1), .rotation = 0 };
			u32 start = step.colliders.current;
			auto tile = layer.cells[coord];
			auto tile_xform = terrain.tiles[tile].transform;
			// step.colliders.grow(*step.arena, terrain.tiles[tile].shapes.size());
			for (auto& shape : terrain.tiles[tile].shapes) {
				auto xform = cell_xform * tile_xform * shape.transform;
				step.push_collider(Physics2D::Collider{
					.transform = xform,
					.aabb = Physics2D::aabb_convex(shape.cvx, xform),
					.shape = &shape.cvx,
					.body_id = i32(terrain_bd),
					.layers = layer.collision_layers
				});
			}
			// if (step.colliders.current > start) {
			// 	fprintf(stderr, "col range (%u, %u)\n", start, u32(step.colliders.current));
			// }
			return {
				.collider_range = { .min = start, .max = u32(step.colliders.current) },
				.tile_collider_index = i32(tile)
			};
		};

		for (const auto& layer : terrain.layers) {
			CellCache cache[layer.cells.dimensions.y][layer.cells.dimensions.x];

			{//* init footprints to invalids
				auto [rx, ry] = grid_ranges({ .min = v2u32(0), .max = layer.cells.dimensions });
				for (auto y : ry) for (auto x : rx) cache[y][x] = {
					.collider_range = {0, 0},
					.tile_collider_index = -1
				};
			}

			auto layer_offset = layer.aabb.min;
			for (auto col_idx : iter_ex(collider_range)) if (
				step.colliders[col_idx].layers & layer.collision_layers &&
				collide(step.colliders[col_idx].aabb, layer.aabb)
			) { //* for every collider that intersects the tilemap layer
				auto intersection = step.colliders[col_idx].aabb & layer.aabb;
				//! rel_overlap is y up, cell coordinates in layer are y down (grid_overlap)
				rtf32 rel_overlap = { .min = intersection.min - layer_offset, .max = intersection.max - layer_offset };
				rti32 grid_overlap = {
					.min = glm::floor(v2f32(rel_overlap.min.x, layer.cells.dimensions.y - rel_overlap.max.y)),
					.max = glm::ceil(v2f32(rel_overlap.max.x, layer.cells.dimensions.y - rel_overlap.min.y)),
				};
				assert(glm::all(glm::greaterThanEqual(grid_overlap.min, v2i32(0))));
				assert(glm::all(glm::lessThanEqual(grid_overlap.max, v2i32(layer.cells.dimensions))));
				auto [rx, ry] = grid_ranges(grid_overlap);
				for (auto y : ry) for (auto x : rx) { //* every cell in the overlap
					if (cache[y][x].tile_collider_index < 0)
						cache[y][x] = cache_load_cell(layer, v2u32(x, y));
					for (auto i : iter_ex(cache[y][x].collider_range)) if (Physics2D::broadphase_test(
						step.colliders[col_idx],
						step.colliders[i],
						detections
					)) step.push_test({ .ids = { col_idx, i } });
				}
			}
		}
		return step.tests.used().subspan(tests_start);
	}

};


// Shape2D tilemap_layer_shape(Arena& arena, const tmx_layer& layer, v2u32 dimensions, Array<Shape2D> shapeset, v2u32 tile_dimensions) {
// 	PROFILE_SCOPE(__PRETTY_FUNCTION__);
// 	(void)shapeset;
// 	auto [scratch, scope] = scratch_push_scope(1ul << 19, &arena); defer{ scratch_pop_scope(scratch, scope); };

// 	Transform2D transform = identity_2d;
// 	transform.translation = v2f32(layer.offsetx, -layer.offsety) / v2f32(tile_dimensions);
// 	transform.scale.y = -1;

// 	v2u32 chunk_dimensions = dimensions;
// 	if (auto x_prop = tmx_get_property(layer.properties, "chunk_optimize_x"); x_prop && x_prop->type == PT_INT) chunk_dimensions.x = x_prop->value.integer;
// 	if (auto y_prop = tmx_get_property(layer.properties, "chunk_optimize_y"); y_prop && y_prop->type == PT_INT) chunk_dimensions.y = y_prop->value.integer;

// 	//TODO reimplement after redoing physics
// 	// auto gids = Tilemap::layer_gids(scratch, layer, dimensions);
// 	auto chunk_count = chunk_dimensions.x * chunk_dimensions.y;

// 	auto leaf_chunk = (
// 		[&](rtu32 tilemap_rect) -> Array<Shape2D> {
// 			auto rect_dims = dim_vec(tilemap_rect);
// 			auto shapes = List{ arena.push_array<Shape2D>(rect_dims.x * rect_dims.y), 0 };
// 			for (auto y : u64xrange{ tilemap_rect.min.y, tilemap_rect.max.y }) for (auto x : u64xrange{ tilemap_rect.min.x, tilemap_rect.max.x }) {
// 				// auto gid = cast<u32>(gids[v2f32(x, y)])[0];
// 				// if (gid == 0) continue;
// 				Transform2D cell_transform = identity_2d;
// 				cell_transform.translation = v2f32(x, y);
// 				cell_transform.scale = v2f32(1) / v2f32(tile_dimensions);
// 				// shapes.push(transform_shape(shapeset[gid], cell_transform));
// 			}
// 			return shapes.shrink_to_content(arena);
// 		}
// 		);

// 	auto sub_rect = (
// 		[&](rtu32 tilemap_rect, v2u32 coord) -> rtu32 {
// 			auto rect_dims = dim_vec(tilemap_rect);
// 			auto cell_size = rect_dims / chunk_dimensions;
// 			auto res = rtu32{ tilemap_rect.min + coord * cell_size, tilemap_rect.min + (coord + v2u32(1)) * cell_size };
// 			//* the ends of the rect gets extended to account for the tiles that might have been lost in the remainder of cell_size computation
// 			auto ends = glm::equal(coord, chunk_dimensions - v2u32(1));
// 			for (auto i : u64xrange{ 0, 2 }) if (ends[i])
// 				res.max[i] = tilemap_rect.max[i];
// 			return res;
// 		}
// 		);

// 	auto generate = (
// 		[&](auto& recurse, rtu32 tilemap_rect) -> Array<Shape2D> {
// 			auto rect_dims = dim_vec(tilemap_rect);
// 			auto tile_count = rect_dims.x * rect_dims.y;
// 			if (chunk_count < tile_count) {
// 				auto shapes = List{ scratch.push_array<Shape2D>(chunk_dimensions.x * chunk_dimensions.y), 0 };
// 				for (auto y : u64xrange{ 0, chunk_dimensions.y }) for (auto x : u64xrange{ 0, chunk_dimensions.x })
// 					if (auto sub_shapes = recurse(recurse, sub_rect(tilemap_rect, v2u32(x, y))); sub_shapes.size() > 0)
// 						shapes.push(make_shape_2d(m3x3f32(1), 0, {}, sub_shapes));
// 				return arena.push_array(shapes.shrink_to_content(scratch));
// 			} else return leaf_chunk(tilemap_rect);
// 		}
// 		);

// 	return make_shape_2d(transform, 0, {}, generate(generate, { v2u32(0), dimensions }));
// }

// Array<Shape2D> tilemap_shapes(Arena& arena, const tmx_map& tree, Array<Shape2D> shapeset) {
// 	PROFILE_SCOPE(__PRETTY_FUNCTION__);
// 	auto [scratch, scope] = scratch_push_scope(1lu << 17, &arena); defer{ scratch_pop_scope(scratch, scope); };
// 	auto traverse = (
// 		[&](auto const& recurse, tmx_layer* list)->Array<Shape2D> {
// 			auto local_scope = scratch.current; defer{ scratch_pop_scope(scratch, local_scope); };
// 			auto shapes = List{ scratch.push_array<Shape2D>(1000), 0 };
// 			for (auto& layer : traverse_by<tmx_layer, &tmx_layer::next>(list)) if (layer.visible) {
// 				switch (layer.type) {
// 				case L_NONE: {} break;
// 				case L_LAYER: {
// 					shapes.push(tilemap_layer_shape(arena, layer, v2u32(tree.width, tree.height), shapeset, v2f32(tree.tile_width, tree.tile_width)));
// 				} break;
// 				case L_OBJGR: {} break;
// 				case L_IMAGE: {} break;
// 				case L_GROUP: {
// 					Transform2D transform = identity_2d;
// 					transform.translation = v2f32(layer.offsetx, layer.offsety) / v2f32(tree.tile_width, tree.tile_width);
// 					shapes.push(make_shape_2d(transform, 0, {}, recurse(recurse, layer.content.group_head)));
// 				} break;
// 				}
// 			}
// 			return arena.push_array(shapes.used());
// 		}
// 		);
// 	return traverse(traverse, tree.ly_head);
// }

#endif
