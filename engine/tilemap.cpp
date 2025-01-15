#ifndef GTILEMAP
# define GTILEMAP

#include <rendering.cpp>
#include <textures.cpp>
#include <atlas.cpp>
#include <sprite.cpp>
#include <tmx_module.h>
#include <spall/profiling.cpp>

rtu32 make_tile(rtu32 spritesheet, const tmx_tile* tile) {
	if (!tile) return rtu32{};
	auto pos = v2u32(tile->ul_x, tile->ul_y);
	auto dims = v2u32(tile->tileset->tile_width, tile->tileset->tile_height);
	return sub_rect(spritesheet, rtu32{ pos, pos + dims });
}

struct Tilemap {
	struct Node {
		string name;
		v2f32 offset;
		v2f32 parallax_factor;
		v4f32 tint;
		f32 opacity;
		enum Type : u32 { GROUP, LAYER, IMAGE, OBJECT, TYPE_COUNT } type;
		u32 index;
		Array<Node> children;

		static Type translate_type(tmx_layer_type type) {
			switch (type) {
			case L_LAYER: return LAYER;
			case L_GROUP: return GROUP;
			case L_OBJGR: return OBJECT;
			case L_IMAGE: return IMAGE;
			default: return TYPE_COUNT;
			}
		}
	};

	Atlas2D layer_atlas;
	Array<rtu32> tiles;
	Array<rtu32> layers;
	Array<Node> nodes;
	v2u32 dimensions;
	v2u32 tile_pixels;

	static Tilemap load(GLScope& ctx, Atlas2D& texture_atlas, const cstr path) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		printf("Loading tilemap %s\n", path);
		auto source = tmx_load(path);
		if (!source) {
			tmx_perror("Tilemap"); //TODO handle failed load
			return {};
		}
		source->user_data.pointer = (void*)path;
		return create(ctx, texture_atlas, *source);
	}

	static Tilemap create(GLScope& ctx, Atlas2D& texture_atlas, const tmx_map& source) {
		//* https://libtmx.readthedocs.io/en/latest/renderer-from-scratch.html
		//TODO shared ressource access, like for spritesheets
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		auto [scratch, scope] = scratch_push_scope(1lu << 17, &ctx.arena); defer{ scratch_pop_scope(scratch, scope); };
		string original_path = (char*)source.user_data.pointer;
		auto slash = original_path.find_last_of('/');
		string dir = slash != string::npos ? original_path.substr(0, slash) : ".";

		//* Load tiles data
		auto tilesets = List{ scratch.push_array<rtu32>(8), 0 };
		for (auto& entry : traverse_by<tmx_tileset_list, &tmx_tileset_list::next>(source.ts_head)) {
			entry.tileset->user_data.integer = tilesets.current;
			tilesets.push_growing(scratch, texture_atlas.load(scratch.format("%.*s/%.*s",
				i32(dir.size()), dir.data(),
				strlen(entry.tileset->image->source), entry.tileset->image->source
			)));
		}
		tilesets.shrink_to_content(scratch);
		auto tiles = map(ctx.arena, carray(source.tiles, source.tilecount), [&](tmx_tile* tile) { return make_tile(tilesets[tile ? tile->tileset->user_data.integer : 0], tile); });

		//* Load Tree
		auto layer_count = [&](this const auto& recurse, tmx_layer* list) -> u64 {
			u64 lcount = 0;
			for (auto& layer : traverse_by<tmx_layer, &tmx_layer::next>(list)) if (layer.visible) {
				if (layer.type == L_LAYER)
					lcount++;
				else if (layer.type == L_GROUP)
					lcount += recurse(layer.content.group_head);
			}
			return lcount;
			}(source.ly_head);
		auto layer_atlas = Atlas2D::create(ctx, v2u32(source.width, source.height * layer_count), R32UI);
		auto layers = List{ scratch.push_array<rtu32>(256), 0 };

		auto nodes = [&](this const auto& recurse, tmx_layer* list) -> Array<Node> {
			auto n = List{ ctx.arena.push_array<Node>(count<tmx_layer, &tmx_layer::next>(list)), 0 };
			for (auto& layer : traverse_by<tmx_layer, &tmx_layer::next>(list)) if (layer.visible) {
				union { u32 c_u32; v4u8 c_4u8; } color = { .c_u32 = layer.tintcolor };
				Node node = {
					.name = layer.name,
					.offset = v2f32(layer.offsetx, layer.offsety) / v2f32(source.tile_width, source.tile_height),
					.parallax_factor = v2f32(layer.parallaxx, layer.parallaxy),
					.tint = v4f32(color.c_4u8.r, color.c_4u8.g, color.c_4u8.b, color.c_4u8.a) / 255.0f,
					.opacity = f32(layer.opacity),
					.type = Node::translate_type(layer.type),
					.index = u32(layers.current),
					.children = {},
				};
				switch (layer.type) {
				case L_GROUP: node.children = recurse(layer.content.group_head); break;
				case L_OBJGR: break; //TODO implement
				case L_IMAGE: layers.push_growing(scratch, texture_atlas.load(scratch.format("%.*s/%.*s", i32(dir.size()), dir.data(), strlen(layer.content.image->source), layer.content.image->source))); break;
				case L_LAYER: layers.push_growing(scratch, layer_atlas.push(make_image(carray(layer.content.gids, source.width * source.height), v2u32(source.width, source.height), 1))); break;
				default: break;
				}
				n.push(node);
			}
			return n.used();
			}(source.ly_head);

		Tilemap map = {
			.layer_atlas = layer_atlas,
			.tiles = tiles,
			.layers = ctx.arena.push_array(layers.used()),
			.nodes = nodes,
			.dimensions = v2u32(source.width, source.height),
			.tile_pixels = v2u32(source.tile_width, source.tile_height),
		};
		map.describe();
		return map;
	}

	void describe() const {
		printf("Tilemap:\n");
		printf("%*sDimensions %ux%u\n", 2, "", dimensions.x, dimensions.y);
		printf("%*sTile Size %ux%u\n", 2, "", tile_pixels.x, tile_pixels.y);
		printf("%*sTiles %llu\n", 2, "", tiles.size());
		printf("%*sLayers %llu\n", 2, "", layers.size());
		printf("%*sNodes %llu\n", 2, "",
			[&](this const auto& recurse, Array<Node> n) -> u64 {
				u64 count = 0;
				for (auto& node : n) {
					count++;
					if (node.type == Node::GROUP)
						count += recurse(node.children);
				}
				return count;
			}(nodes));
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
		VertexArray vao;
		GLuint layers;
	};

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

		Renderer make_renderer(GLScope& ctx, const Tilemap& tm) {
			PROFILE_SCOPE(__PRETTY_FUNCTION__);
			v2f32 vertices[4 * tm.layers.size()];
			u32 indices[6 * tm.layers.size()];
			Quad quads[tm.layers.size()];
			u32 quad_count = 0;

			[&](this const auto& recurse, Array<const Tilemap::Node> nodes, v2f32 offset = v2f32(0)) -> void {
				for (auto& node : nodes) {
					auto local_offset = offset + node.offset;
					if (node.type == Tilemap::Node::GROUP)
						recurse(node.children, local_offset);
					else if (node.type == Tilemap::Node::LAYER) {
						auto rect = rtf32{ local_offset, local_offset + v2f32(tm.dimensions) };
						auto [v, i] = QuadGeo::create(rect, 4 * quad_count);
						copy(larray(v), carray(&vertices[4 * quad_count], 4));
						copy(larray(i), carray(&indices[6 * quad_count], 6));
						quads[quad_count] = {
							.layer_sprite = tm.layers[node.index],
							.parallax = v2f32(node.parallax_factor),
							.depth = length(node.parallax_factor)
						};
						quad_count += 1;
					}
				}
			}(tm.nodes);

			Renderer rd = {
				.indices = GPUBuffer::upload(ctx, carray(indices, 6 * quad_count)),
				.vertices = GPUBuffer::upload(ctx, carray(vertices, 4 * quad_count)),
				.quads = GPUBuffer::upload(ctx, carray(quads, quad_count)),
				.sprites = GPUBuffer::upload(ctx, tm.tiles),
				.scene = GPUBuffer::create(ctx, sizeof(Scene), GL_DYNAMIC_STORAGE_BIT),
				.vao = VertexArray::create(ctx),
				.layers = tm.layer_atlas.texture.id
			};

			rd.vao.conf_vattrib(inputs.vertices.positions, vattr_fmt<v2f32>(0));
			return rd;
		}

		RenderCommand operator()(Arena& arena, Renderer& renderer, const Scene& s, GLuint albedo) {
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
					TextureBinding{.textures = arena.push_array({ albedo }), .target = inputs.albedo_atlas },
					TextureBinding{.textures = arena.push_array({ renderer.layers }), .target = inputs.tilemap_layers },
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
};

bool EditorWidget(const cstr label, Tilemap& tm) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("Layer Atlas", tm.layer_atlas);
		changed |= EditorWidget("tiles", tm.tiles);
		changed |= EditorWidget("layers", tm.layers);
		changed |= EditorWidget("nodes", tm.nodes);
		changed |= EditorWidget("dimensions", tm.dimensions);
		changed |= EditorWidget("tile_pixels", tm.tile_pixels);
	}
	return changed;
}

#include <shape_2d.cpp>

// Array<Shape2D> object_shape(Arena& arena, tmx_object* obj_head) {
// 	static v2f32 g_origin[] = { v2f32(0) };
// 	PROFILE_SCOPE(__PRETTY_FUNCTION__);
// 	auto surface_count = count<tmx_object, &tmx_object::next>(obj_head);
// 	auto [scratch, scope] = scratch_push_scope(max(1lu << 22, surface_count * 2), &arena); defer{ scratch_pop_scope(scratch, scope); };
// 	auto objs = List{ scratch.push_array<Shape2D>(surface_count * 2), 0 };
// 	for (auto& obj : traverse_by<tmx_object, &tmx_object::next>(obj_head)) {
// 		auto transform = identity_2d;
// 		transform.translation = v2f32(obj.x, obj.y);
// 		transform.rotation = glm::radians(obj.rotation);
// 		auto push_content_points = [&](Arena& arena, tmx_object& obj) { return map(arena, carray(obj.content.shape->points, obj.content.shape->points_len), [&](f64* pt) -> v2f32 { return v2f32(pt[0], pt[1]);}); };
// 		switch (obj.obj_type) {
// 		case OT_POLYLINE: {
// 			auto points = push_content_points(arena, obj);
// 			for (auto i : u64xrange{ 0, points.size() - 1 })
// 				objs.push_growing(scratch, make_shape_2d(transform, 0, points.subspan(i, 2)));
// 		} break;
// 		case OT_POLYGON: {
// 			auto local_scope = scratch.current;
// 			auto concave = push_content_points(scratch, obj);
// 			auto [polys, verts] = ear_clip(arena, concave);
// 			scratch_pop_scope(scratch, local_scope);
// 			for (auto poly : polys)
// 				objs.push_growing(scratch, make_shape_2d(transform, 0, poly));
// 		} break;
// 		case OT_POINT: { objs.push_growing(scratch, make_shape_2d(transform, 0, larray(g_origin))); } break;
// 		case OT_SQUARE: {
// 			objs.push_growing(scratch, make_box_shape(arena.push_array<v2f32>(4), v2f32(obj.width, obj.height), v2f32(obj.width, obj.height) / 2.f, transform));
// 		} break;
// 		case OT_ELLIPSE: {
// 			auto dims = v2f32(obj.width, obj.height);
// 			transform.translation += dims / 2.f;
// 			transform.scale *= dims / v2f32(max(dims.x, dims.y));
// 			objs.push_growing(scratch, make_circle_shape(max(dims.x, dims.y) / 2.f, transform));
// 		} break;
// 		default: break;;
// 		}
// 	}
// 	return arena.push_array(objs.used());
// }

// Array<Shape2D> tile_shapeset(Arena& arena, Array<tmx_tile*> tiles) {
// 	PROFILE_SCOPE(__PRETTY_FUNCTION__);
// 	return map(arena, tiles, [&](tmx_tile* tile) -> Shape2D { return make_shape_2d(identity_2d, 0, {}, tile ? object_shape(arena, tile->collision) : Array<Shape2D>{}); });
// }

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
