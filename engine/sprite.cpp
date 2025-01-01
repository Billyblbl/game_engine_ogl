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

template<typename T, i32 D> inline glm::vec<D, T> rect_to_world(reg_polytope<glm::vec<D, T>> rect, glm::vec<D, T> v) {
	return v * (rect.max - rect.min) + rect.min;
}

template<typename T, i32 D> inline reg_polytope<glm::vec<D, T>> rect_in_rect(reg_polytope<glm::vec<D, T>> reference, reg_polytope<glm::vec<D, T>> rect) {
	return { rect_to_world(reference, rect.min), rect_to_world(reference, rect.max) };
}

inline rtu32 sub_rect(rtu32 source, rtu32 sub) { return { sub.min + source.min, sub.max + source.min }; }

static constexpr auto MAX_REASONABLE_TEXTURE_UNITS = 32;//* actual limit is dependent on hardware
struct SpriteMeshRenderer {

	GLuint pipeline;

	struct {
		GLuint textures;
		BufferBinding scene;
		BufferBinding entities;
		BufferBinding sprites;
		GPUBuffer commands;
		struct {
			GPUBuffer ibo;
			struct {
				GPUBuffer vertices;
				GPUBuffer quad_info;
			} vbos;
			VertexArray vao;
		} meshes;
	} bindings;

	List<GLuint> texture_ids;

	struct Scene {
		m4x4f32 view_projection;
		f32 alpha_discard;
		byte padding[12];
	};

	struct Quad {
		u32 albedo_index;
		f32 depth;
		rtf32 rect;
	};

	struct alignas(16) Entity {
		m4x4f32 transform;
		v4f32 color;
		num_range<u32> sprite_range;
	};

	static constexpr auto IDX_PER_QUAD = 6;
	static constexpr auto VERT_PER_QUAD = 4;

	static constexpr auto DEFAULT_ENTITIES_CAP = 128;
	static constexpr auto DEFAULT_QUADS_PER_MESH_CAP = 16;
	static constexpr auto DEFAULT_MESH_CAP = 16;
	struct ResourceConfig {
		u32 entts = DEFAULT_ENTITIES_CAP;
		u32 quads_per_mesh = DEFAULT_QUADS_PER_MESH_CAP;
		u32 meshes = DEFAULT_MESH_CAP;
	};

	static SpriteMeshRenderer load(
		GLScope& ctx,
		const cstr pipeline_path = "shaders/sprite_mesh_2d.glsl",
		ResourceConfig resources = {
			.entts = DEFAULT_ENTITIES_CAP,
			.quads_per_mesh = DEFAULT_QUADS_PER_MESH_CAP,
			.meshes = DEFAULT_MESH_CAP
		}
	) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		auto ppl = load_pipeline(ctx, pipeline_path);
		BufferBinding::Sequencer seq = { .pipeline = ppl, .ctx = &ctx };

		SpriteMeshRenderer rd = {
			.pipeline = ppl,
			.bindings = {
				.textures = GLuint(init_binding_texture(ppl, "textures")),
				.scene = seq.next(BufferBinding::UBO, "Scene", sizeof(Scene)),
				.entities = seq.next(BufferBinding::SSBO, "Entities", sizeof(Entity) * resources.entts, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT),
				.sprites = seq.next(BufferBinding::SSBO, "Sprites", sizeof(rtu32) * resources.entts * resources.quads_per_mesh, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT),
				.commands = GPUBuffer::create(ctx, sizeof(DrawCommandElement) * resources.entts, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT),
				.meshes = {
					.ibo = GPUBuffer::create(ctx, sizeof(u32) * resources.meshes * resources.quads_per_mesh * IDX_PER_QUAD, GL_DYNAMIC_STORAGE_BIT),
					.vbos = {
						.vertices = GPUBuffer::create(ctx, sizeof(v2f32) * resources.meshes * VERT_PER_QUAD, GL_DYNAMIC_STORAGE_BIT),
						.quad_info = GPUBuffer::create(ctx, (sizeof(u32) + sizeof(f32)) * resources.meshes * resources.quads_per_mesh, GL_DYNAMIC_STORAGE_BIT)
					},
					.vao = VertexArray::create(ctx)
				}
			},
			.texture_ids = { ctx.arena.push_array<GLuint>(MAX_REASONABLE_TEXTURE_UNITS), 0 }//TODO get size from pipeline
		};

		rd.bindings.meshes.vao.conf_vattrib(get_shader_input(ppl, "position", R_VERT), vattr_fmt<v2f32>(0));
		rd.bindings.meshes.vao.conf_vattrib(get_shader_input(ppl, "albedo_index", R_VERT), vattr_fmt<u32>(offsetof(Quad, albedo_index)));
		rd.bindings.meshes.vao.conf_vattrib(get_shader_input(ppl, "depth", R_VERT), vattr_fmt<f32>(offsetof(Quad, depth)));

		return rd;
	}

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

	u32 next_batch_id = 1;
	u32 last_used_batch_id = 0;

	Batch start_batch() {
		auto commands = bindings.commands.map_as<DrawCommandElement>({ 0, bindings.commands.content_as<DrawCommandElement>() }, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
		for (auto& c : commands)
			c.instance_count = 0;
		return {
			.id = next_batch_id++,
			.commands = commands,
			.entities = bindings.entities.buffer.map_as<Entity>({ 0, bindings.entities.buffer.content_as<Entity>() }, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT),
			.sprites = List { bindings.sprites.buffer.map_as<rtu32>({}, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT), 0 },
		};
	}

	void flush_batch(const Batch& batch) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		//TODO track dirty ranges instead of flushing all filled
		bindings.commands.unmap_as<DrawCommandElement>({ 0, batch.commands.size() });
		bindings.entities.buffer.unmap_as<Entity>({ 0, batch.entities.size() });
		bindings.sprites.buffer.unmap_as<rtu32>({ 0, batch.sprites.current });
		last_used_batch_id = batch.id;
	}

	//! only supports 1 RenderCommand in flight at a time, must consume before starting a new batch
	RenderCommand operator()(Arena& arena, const m4x4f32& vp, const Batch& batch) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		if (!batch.has_content()) return {};

		if (last_used_batch_id < batch.id)
			flush_batch(batch);

		GL_GUARD(glDisable(GL_CULL_FACE));
		GL_GUARD(glEnable(GL_DEPTH_TEST));
		GL_GUARD(glDepthFunc(GL_LEQUAL));
		GL_GUARD(glEnable(GL_BLEND));
		GL_GUARD(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

		Scene scene = { .view_projection = vp, .alpha_discard = 0, .padding = {} };
		bindings.scene.buffer.write_as<Scene>(carray(&scene, 1), 0);

		return {
			.pipeline = pipeline,
			.draw_type = RenderCommand::D_MDEI,
			.draw = {.d_indirect = {
				.buffer = bindings.commands.id,
				.stride = sizeof(DrawCommandElement),
				.range = { 0, GLsizei(batch.commands.size()) }
			}},
			.vao = bindings.meshes.vao.id,
			.ibo = {
				.buffer = bindings.meshes.ibo.id,
				.index_type = bindings.meshes.vao.index_type,
				.primitive = bindings.meshes.vao.draw_mode
			},
			.vertex_buffers = arena.push_array({
				VertexBinding{
					.buffer = bindings.meshes.vbos.vertices.id,
					.targets = arena.push_array({ get_shader_input(pipeline, "position", R_VERT) }),
					.offset = 0,
					.stride = sizeof(v2f32),
					.divisor = 0
				},
				VertexBinding{
					.buffer = bindings.meshes.vbos.quad_info.id,
					.targets = arena.push_array({
						get_shader_input(pipeline, "depth", R_VERT),
						get_shader_input(pipeline, "albedo_index", R_VERT)
					}),
					.offset = 0,
					.stride = sizeof(u32) + sizeof(f32),
					.divisor = 4
				}
			}),
			.textures = arena.push_array({
				TextureBinding{
					// .textures = texture_ids.used(),//? move or refer ?
					.textures = arena.push_array(texture_ids.used()),
					.target = bindings.textures
				}
			}),
			.buffers = arena.push_array({
				BufferObjectBinding{
					.buffer = bindings.entities.buffer.id,
					.type = GL_SHADER_STORAGE_BUFFER,
					.range = { 0, u32(batch.entities.size() * sizeof(Entity)) },
					.target = GLuint(bindings.entities.index)
				},
				BufferObjectBinding{
					.buffer = bindings.sprites.buffer.id,
					.type = GL_SHADER_STORAGE_BUFFER,
					.range = { 0, u32(batch.sprites.current * sizeof(rtu32)) },
					.target = GLuint(bindings.sprites.index)
				},
				BufferObjectBinding{
					.buffer = bindings.scene.buffer.id,
					.type = GL_UNIFORM_BUFFER,
					.range = { 0, u32(sizeof(Scene)) },
					.target = GLuint(bindings.scene.index)
				}
			})
		};
	}

	u32 push_texture(GLuint id) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		auto index = texture_ids.current;
		texture_ids.push(id);
		return index;
	}

	u32 push_quad_mesh(Array<const Quad> quads, u32 max_instances) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		auto mesh_index = bindings.commands.content_as<DrawCommandElement>();

		//*Prepare the command
		bindings.commands.push_one<DrawCommandElement>({
			.count = GLuint(quads.size() * IDX_PER_QUAD),
			.instance_count = 0,
			.first_index = GLuint(bindings.meshes.ibo.content_as<u32>()),
			.base_vertex = GLint(bindings.meshes.vbos.vertices.content_as<v2f32>()),
			.base_instance = GLuint(bindings.entities.buffer.content_as<Entity>())
			});

		//* Allocate new slots for entities
		bindings.entities.buffer.allocate_as<Entity>(max_instances);

		//* Generate mesh data
		struct { u32 i[6]; } indices[quads.size()];
		struct { v2f32 v[4]; } vertices[quads.size()];
		struct { u32 albedo_index; f32 depth; } infos[quads.size()];
		for (auto i : u32xrange{ 0, quads.size() }) {

			indices[i] = { .i = {
				i * 4 + 0,
				i * 4 + 1,
				i * 4 + 2,
				i * 4 + 2,
				i * 4 + 1,
				i * 4 + 3
			} };

			vertices[i] = { .v = {
				quads[i].rect.min,
				v2f32(quads[i].rect.max.x, quads[i].rect.min.y),
				v2f32(quads[i].rect.min.x, quads[i].rect.max.y),
				quads[i].rect.max
			} };

			infos[i] = {
				.albedo_index = quads[i].albedo_index,
				.depth = quads[i].depth
			};

		}

		//* Write mesh to GPU buffers
		bindings.meshes.ibo.push_as(carray(indices, quads.size()));
		bindings.meshes.vbos.vertices.push_as(carray(vertices, quads.size()));
		bindings.meshes.vbos.quad_info.push_as(carray(infos, quads.size()));

		return mesh_index;
	}

};

struct SpriteMeshPipeline {
	GLuint id;
	GLuint textures;
	GLuint scenes;
	GLuint entities;
	GLuint sprites;
	struct {
		GLuint positions;
		GLuint depth;
		GLuint albedo_index;
	} vertices;

	struct Scene {
		m4x4f32 view_projection;
		f32 alpha_discard;
		byte padding[12];
	};

	struct Quad {
		u32 albedo_index;
		f32 depth;
		rtf32 rect;
	};

	struct alignas(16) Entity {
		m4x4f32 transform;
		v4f32 color;
		num_range<u32> sprite_range;
	};

	static SpriteMeshPipeline create(GLScope& ctx, const cstr pipeline_path = "shaders/sprite_mesh_2d.glsl") {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		auto ppl = load_pipeline(ctx, pipeline_path);

		return {
			.id = ppl,
			.textures = get_shader_input(ppl, "textures", R_TEX),
			.scenes = get_shader_input(ppl, "Scene", R_UBO),
			.entities = get_shader_input(ppl, "Entities", R_SSBO),
			.sprites = get_shader_input(ppl, "Sprites", R_SSBO),
			.vertices = {
				.positions = get_shader_input(ppl, "positions", R_VERT),
				.depth = get_shader_input(ppl, "depth", R_VERT),
				.albedo_index = get_shader_input(ppl, "albedo_index", R_VERT)
			}
		};
	}

	GLuint make_vao(GLScope& ctx) {
		auto vao = VertexArray::create(ctx);
		vao.conf_vattrib(vertices.positions, vattr_fmt<v2f32>(0));
		vao.conf_vattrib(vertices.depth, vattr_fmt<f32>(offsetof(Quad, depth)));
		vao.conf_vattrib(vertices.albedo_index, vattr_fmt<u32>(offsetof(Quad, albedo_index)));
		return vao.id;
	}

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

};

#endif
