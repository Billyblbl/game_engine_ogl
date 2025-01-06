#ifndef GSPRITE
# define GSPRITE

#include <rendering.cpp>
#include <textures.cpp>
#include <imgui_extension.cpp>
#include <image.cpp>

struct Sprite {
	rtu32 view;
	v2f32 dimensions;
	f32 depth;
};

bool EditorWidget(const cstr label, Sprite& data) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("View", data.view);
		changed |= EditorWidget("Dimensions", data.dimensions);
		changed |= EditorWidget("Depth", data.depth);
		ImGui::TreePop();
	}
	return changed;
}

inline rtu32 sub_rect(rtu32 source, rtu32 sub) { return { sub.min + source.min, sub.max + source.min }; }

namespace SpriteMesh {

	static constexpr auto IDX_PER_QUAD = 6;
	static constexpr auto VERT_PER_QUAD = 4;

	static constexpr auto DEFAULT_ENTITIES_CAP = 128;
	static constexpr auto DEFAULT_QUADS_PER_MESH_CAP = 16;
	static constexpr auto DEFAULT_MESH_CAP = 16;

	struct Scene {
		m4x4f32 view_projection;
		f32 alpha_discard;
		byte padding[12];
	};

	struct alignas(16) Quad {
		struct Info {
			u32 albedo_index;
			f32 depth;
		} info;
		rtf32 rect;
	};

	struct alignas(16) Entity {
		m4x4f32 transform;
		v4f32 color;
		num_range<u32> sprite_range;
	};

	struct Batch {
		u32 id;
		Array<DrawCommandElement> commands;
		Array<Entity> entities;
		List<rtu32> sprites;

		u32 push_entity(m4x4f32 transform, v4f32 color, u32 mesh_index, Array<rtu32> animation_states) {
			PROFILE_SCOPE(__PRETTY_FUNCTION__);
			auto& command = commands[mesh_index];
			auto index = command.base_instance + command.instance_count;
			entities[index] = {
				.transform = transform,
				.color = color,
				.sprite_range = num_range<u32>(sprites.current, sprites.current + animation_states.size())
			};
			sprites.push(animation_states);
			command.instance_count++;
			return index;
		}

		bool has_content() const { return commands.size() > 0 && entities.size() > 0 && sprites.current > 0; }
	};

	struct Renderer {
		struct {
			GPUBuffer indices;
			GPUBuffer vertices;
			GPUBuffer quads;
		} meshes;
		GPUBuffer sprites;
		GPUBuffer entities;
		GPUBuffer scene;
		GPUBuffer commands;
		VertexArray vao;
		List<GLuint> albedos;

		u32 push_texture(GLuint id) {
			PROFILE_SCOPE(__PRETTY_FUNCTION__);
			auto index = albedos.current;
			albedos.push(id);
			return index;
		}

		u32 push_quad_mesh(Array<const Quad> quads, u32 max_instances) {
			PROFILE_SCOPE(__PRETTY_FUNCTION__);
			auto mesh_index = commands.content_as<DrawCommandElement>();

			//*Prepare the command
			commands.push_one<DrawCommandElement>(
				{
					.count = GLuint(quads.size() * IDX_PER_QUAD),
					.instance_count = 0,
					.first_index = GLuint(meshes.indices.content_as<u32>()),
					.base_vertex = GLint(meshes.vertices.content_as<v2f32>()),
					.base_instance = GLuint(entities.content_as<Entity>())
				}
			);

			//* Allocate new slots for entities
			entities.allocate_as<Entity>(max_instances);

			//* Generate mesh data
			struct { u32 i[6]; } indices[quads.size()];
			struct { v2f32 v[4]; } vertices[quads.size()];
			Quad::Info infos[quads.size()];
			for (auto i : u32xrange{ 0, quads.size() }) {
				auto [vert, idx] = QuadGeometry::create(quads[i].rect, 4 * i);
				copy(larray(idx), carray(&indices[i].i[0], 6));
				copy(larray(vert), carray(&vertices[i].v[0], 4));
				infos[i] = quads[i].info;
			}

			//* Write mesh to GPU buffers
			meshes.indices.push_as(carray(indices, quads.size()));
			meshes.vertices.push_as(carray(vertices, quads.size()));
			meshes.quads.push_as(carray(infos, quads.size()));

			return mesh_index;
		}

		u32 next_batch_id = 1;
		u32 current_batch = 0;
		Batch start_batch() {
			auto cmds = commands.map_as<DrawCommandElement>({ 0, commands.content_as<DrawCommandElement>() }, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

			//* Reset batch data
			sprites.content = 0;
			for (auto& c : cmds)
				c.instance_count = 0;

			//* create new batch
			return {
				.id = next_batch_id++,
				.commands = cmds,
				.entities = entities.map_as<Entity>({ 0, entities.content_as<Entity>() }, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT),
				.sprites = List { sprites.map_as<rtu32>({}, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT), 0 },
			};
		}

		u32 consume_batch(const Batch& batch) {
			PROFILE_SCOPE(__PRETTY_FUNCTION__);
			if (batch.id <= current_batch)
				return current_batch;
			//TODO track dirty ranges instead of flushing all filled
			commands.unmap_as<DrawCommandElement>({ 0, batch.commands.size() });
			entities.unmap_as<Entity>({ 0, batch.entities.size() });
			sprites.unmap_as<rtu32>({ 0, batch.sprites.current });
			current_batch = batch.id;
			return current_batch;
		}
	};


	struct Pipeline {
		GLuint id;
		GLuint textures;
		GLuint scene;
		GLuint entities;
		GLuint sprites;
		struct {
			GLuint positions;
			GLuint quads;
		} vertices;

		static Pipeline create(GLScope& ctx, const cstr pipeline_path = "shaders/sprite_mesh_2d.glsl") {
			PROFILE_SCOPE(__PRETTY_FUNCTION__);
			auto ppl = load_pipeline(ctx, pipeline_path);

			return {
				.id = ppl,
				.textures = get_shader_input(ppl, "textures", R_TEX),
				.scene = get_shader_input(ppl, "Scene", R_UBO),
				.entities = get_shader_input(ppl, "Entities", R_SSBO),
				.sprites = get_shader_input(ppl, "Sprites", R_SSBO),
				.vertices = {
					.positions = get_shader_input(ppl, "position", R_VERT),
					.quads = get_shader_input(ppl, "Quads", R_SSBO)
				}
			};
		}

		struct ResourceConfig {
			u32 entts = DEFAULT_ENTITIES_CAP;
			u32 quads_per_mesh = DEFAULT_QUADS_PER_MESH_CAP;
			u32 meshes = DEFAULT_MESH_CAP;
		};

		Renderer make_renderer(GLScope& ctx, ResourceConfig config = {
			.entts = DEFAULT_ENTITIES_CAP,
			.quads_per_mesh = DEFAULT_QUADS_PER_MESH_CAP,
			.meshes = DEFAULT_MESH_CAP
			}) {
			Scene sc = {
				.view_projection = m4x4f32(1),
				.alpha_discard = 0.1f,
				.padding = {}
			};
			Renderer rd = {
				.meshes {
					.indices = GPUBuffer::create_stretchy(ctx, sizeof(u32) * IDX_PER_QUAD * config.quads_per_mesh * config.meshes, GL_DYNAMIC_DRAW),
					.vertices = GPUBuffer::create_stretchy(ctx, sizeof(v2f32) * VERT_PER_QUAD * config.quads_per_mesh * config.meshes, GL_DYNAMIC_DRAW),
					.quads = GPUBuffer::create_stretchy(ctx, sizeof(Quad::Info) * config.quads_per_mesh * config.meshes, GL_DYNAMIC_DRAW),
				},
				.sprites = GPUBuffer::create_stretchy(ctx, sizeof(rtu32) * config.quads_per_mesh * config.meshes * config.entts, GL_DYNAMIC_DRAW),
				.entities = GPUBuffer::create_stretchy(ctx, sizeof(Entity) * config.entts, GL_DYNAMIC_DRAW),
				.scene = GPUBuffer::upload(ctx, carray(&sc, 1), GL_DYNAMIC_STORAGE_BIT),
				.commands = GPUBuffer::create_stretchy(ctx, sizeof(DrawCommandElement) * config.meshes, GL_DYNAMIC_DRAW),
				.vao = VertexArray::create(ctx),
				.albedos = { ctx.arena.push_array<GLuint>(get_max_textures_frag()), 0 }
			};

			rd.vao.conf_vattrib(vertices.positions, vattr_fmt<v2f32>(0));

			return rd;
		}

		RenderCommand operator()(Arena& arena, Renderer& rd, const Scene& sc) {
			rd.scene.write_one(sc);

			return {
				.pipeline = id,
				.draw_type = RenderCommand::D_MDEI,
				.draw = {.d_indirect = {
					.buffer = rd.commands.id,
					.stride = sizeof(DrawCommandElement),
					.range = {0, GLsizei(rd.commands.content_as<DrawCommandElement>())}
				}},
				.vao = rd.vao.id,
				.ibo = {
					.buffer = rd.meshes.indices.id,
					.index_type = rd.vao.index_type,
					.primitive = rd.vao.draw_mode
				},
				.vertex_buffers = arena.push_array({VertexBinding{
					.buffer = rd.meshes.vertices.id,
					.targets = arena.push_array({ vertices.positions }),
					.offset = 0,
					.stride = sizeof(v2f32),
					.divisor = 0
				}}),
				.textures = arena.push_array({ TextureBinding{.textures = arena.push_array(rd.albedos.used()), .target = textures} }),
				.buffers = arena.push_array({
					BufferObjectBinding{
						.buffer = rd.sprites.id,
						.type = GL_SHADER_STORAGE_BUFFER,
						.range = { 0, GLuint(rd.sprites.content) },
						.target = sprites
					},
					BufferObjectBinding{
						.buffer = rd.entities.id,
						.type = GL_SHADER_STORAGE_BUFFER,
						.range = { 0, GLuint(rd.entities.content) },
						.target = entities
					},
					BufferObjectBinding{
						.buffer = rd.scene.id,
						.type = GL_UNIFORM_BUFFER,
						.range = { 0, GLuint(rd.scene.content) },
						.target = scene
					}
				})
			};

		}

	};

};

#endif
