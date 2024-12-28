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
		GLint textures;
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
				.textures = init_binding_texture(ppl, "textures"),
				.scene = seq.next(BufferBinding::UBO, "Scene", sizeof(Scene)),
				.entities = seq.next(BufferBinding::SSBO, "Entities", sizeof(Entity) * resources.entts, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT),
				.sprites = seq.next(BufferBinding::SSBO, "Sprites", sizeof(rtu32) * resources.entts * resources.quads_per_mesh, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT),
				.commands = GPUBuffer::create(ctx, sizeof(DrawCommandElement) * resources.entts, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT ),
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

		rd.bindings.meshes.vao.bind_index_buffer(rd.bindings.meshes.ibo.id);

		auto vertex_bindings = 0;
		auto binding_positions = rd.bindings.meshes.vao.bind_vertex_buffer(rd.bindings.meshes.vbos.vertices.id, vertex_bindings++, 0, sizeof(v2f32), 0);
		auto binding_quad_infos = rd.bindings.meshes.vao.bind_vertex_buffer(rd.bindings.meshes.vbos.quad_info.id, vertex_bindings++, 0, (sizeof(u32) + sizeof(f32)), 4);

		rd.bindings.meshes.vao.bind_vattrib(ppl, "position", binding_positions, vattr_fmt<v2f32>(0));
		rd.bindings.meshes.vao.bind_vattrib(ppl, "albedo_index", binding_quad_infos, vattr_fmt<u32>(offsetof(Quad, albedo_index)));
		rd.bindings.meshes.vao.bind_vattrib(ppl, "depth", binding_quad_infos, vattr_fmt<f32>(offsetof(Quad, depth)));

		{
			PROFILE_SCOPE("Waiting for GPU init work");
			wait_gpu();
		}

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
		auto commands = bindings.commands.map_as<DrawCommandElement>({ 0, bindings.commands.content_as<DrawCommandElement>()}, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
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

	void operator()(const m4x4f32& vp, const Batch& batch) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		if (!batch.has_content()) return;

		if (last_used_batch_id < batch.id)
			flush_batch(batch);

		GL_GUARD(glDisable(GL_CULL_FACE));
		GL_GUARD(glEnable(GL_DEPTH_TEST));
		GL_GUARD(glDepthFunc(GL_LEQUAL));
		GL_GUARD(glEnable(GL_BLEND));
		GL_GUARD(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

		GL_GUARD(glUseProgram(pipeline)); defer{ GL_GUARD(glUseProgram(0)); };
		bindings.meshes.vao.bind(); defer{ bindings.meshes.vao.unbind(); };
		push_texture_array(bindings.textures, 0, texture_ids.used()); defer { unbind_array({0, GLuint(texture_ids.current) }); };
		bindings.entities.bind(); defer{ bindings.entities.unbind(); };
		bindings.sprites.bind(); defer{ bindings.sprites.unbind(); };
		bindings.scene.push(Scene{ .view_projection = vp, .alpha_discard = 0.01f, .padding={}}); defer{ bindings.scene.unbind(); };
		bindings.commands.bind(GL_DRAW_INDIRECT_BUFFER); defer{ bindings.commands.unbind(GL_DRAW_INDIRECT_BUFFER); };

		GL_GUARD(glMultiDrawElementsIndirect(bindings.meshes.vao.draw_mode, bindings.meshes.vao.index_type, 0, batch.commands.size(), 0));
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
		struct { u32 albedo_index; f32 depth;} infos[quads.size()];
		for (auto i : u32xrange{0, quads.size()}) {

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

// struct SpriteRenderer {
// 	struct Scene {
// 		m4x4f32 view_projection;
// 		f32 alpha_discard;
// 	};

// 	GLuint pipeline;

// 	struct {
// 		GLint textures;
// 		struct {
// 			GPUBuffer ibo;
// 			struct {
// 				GPUBuffer vertices;
// 				GPUBuffer sprites;
// 			} vbos;
// 		} meshes;
// 	} bindings;
// 	List<GLuint> texture_ids;

// 	static Instance make_instance(const Sprite& sprite, const m4x4f32& matrix) {
// 		Instance instance;
// 		instance.uv_rect = sprite.view;
// 		instance.matrix = matrix * glm::scale(v3f32(sprite.dimensions, 1));
// 		instance.dimensions = v4f32(sprite.dimensions, sprite.depth, 0);
// 		return instance;
// 	}

// 	void operator()(
// 		Array<Instance> sprites,
// 		const m4x4f32& vp,
// 		const TexBuffer& textures
// 		) {
// 		GL_GUARD(glUseProgram(pipeline)); defer{ GL_GUARD(glUseProgram(0)); };
// 		GL_GUARD(glBindVertexArray(rect.vao.id)); defer{ GL_GUARD(glBindVertexArray(0)); };
// 		inputs.atlas.bind_texture(textures.id); defer{ inputs.atlas.unbind(); };
// 		inputs.instances.bind_content(sprites); defer{ inputs.instances.unbind(); };
// 		inputs.scene.bind_object(Scene{ vp, textures.dimensions, 0.01f }); defer{ inputs.scene.unbind(); };
// 		GL_GUARD(glDrawElementsInstanced(rect.vao.draw_mode, rect.element_count, rect.vao.index_type, null, sprites.size()));
// 	}

// 	static SpriteRenderer load(const cstr pipeline_path = "./shaders/sprite.glsl", u32 max_draw_batch = 256, const GPUGeometry* mesh = null) {
// 		PROFILE_SCOPE(__PRETTY_FUNCTION__);
// 		SpriteRenderer rd;
// 		rd.pipeline = load_pipeline(GLScope::global(), pipeline_path);
// 		rd.rect = mesh ? *mesh : create_rect_mesh(v2f32(1));
// 		rd.inputs.atlas = ShaderInput::create_slot(rd.pipeline, ShaderInput::Texture, "atlas");
// 		rd.inputs.instances = ShaderInput::create_slot(rd.pipeline, ShaderInput::SSBO, "Entities", sizeof(Instance) * max_draw_batch);
// 		rd.inputs.scene = ShaderInput::create_slot(rd.pipeline, ShaderInput::UBO, "Scene", sizeof(Scene));
// 		return rd;
// 	}

// };

#endif
