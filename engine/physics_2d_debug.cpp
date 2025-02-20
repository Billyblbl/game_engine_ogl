#ifndef GPHYSICS_2D_DEBUG
# define GPHYSICS_2D_DEBUG

#include <physics_2d.cpp>
#include <rendering.cpp>

namespace Physics2D {

	namespace Debug {

		struct alignas(16) Instance {
			v4f32 rows[3];//* alignement stuff because of stf430 forcing vec3s to take the size of a vec4 in glsl, even inside a mat3
			v4f32 color;

			static Instance create(const m3x3f32& transform, v4f32 color) {
				return {
					.rows = {
						v4f32(transform[0], 0),
						v4f32(transform[1], 0),
						v4f32(transform[2], 0)
					},
					.color = color
				};
			}
		};

		const Instance ColliderAABB = Instance::create(m3x3f32(1), v4f32(1, 0, 0, 1));
		const Instance AABBIntersection = Instance::create(m3x3f32(1), v4f32(0, 1, 0, 1));

		enum InstanceID : GLuint {
			COLLIDER_AABB,
			AABB_INTERSECTION,
			SHAPE
		};//! hijacks DrawCommandVertex.base_instance during batch construction, replaced when passing the batch to the renderer for transmission to GPU buffer

		constexpr static u32 DEBUG_CIRCLE_SEGMENTS = 32;

		static struct {
			bool supports_scan = false;
			bool aabb = true;
			u32 circle_segments = DEBUG_CIRCLE_SEGMENTS;
			u32 scan_segments = DEBUG_CIRCLE_SEGMENTS * 4;
			v4f32 scan_color = v4f32(1, 0, 1, 1);
			v4f32 aabb_intersection = v4f32(0, 1, 0, 1);
			v4f32 collider_aabb = v4f32(1, 0, 0, 1);
			v4f32 supports = v4f32(0, 1, 1, 1);

			bool draw_edit_window(const cstr label) {
				auto changed = false;
				if (ImGui::Begin(label)) {
					changed |= ImGui::Checkbox("AABB", &aabb);
					changed |= ImGui::Checkbox("Supports Scan", &supports_scan);
					changed |= ImGui::InputInt("Circle Segments", (i32*)&circle_segments);
					circle_segments = glm::clamp((i32)circle_segments, 3, 256);
					changed |= ImGui::InputInt("Scan Segments", (i32*)&scan_segments);
					scan_segments = glm::clamp((i32)scan_segments, 3, 256);
					changed |= ImGui::ColorEdit4("Scan Color", &scan_color[0]);
					changed |= ImGui::ColorEdit4("AABB Intersection", &aabb_intersection[0]);
					changed |= ImGui::ColorEdit4("Collider AABB", &collider_aabb[0]);
					changed |= ImGui::ColorEdit4("supports color", &supports[0]);
				} ImGui::End();
				return changed;
			}

		} debug_config;

		struct Batch {
			Arena* arena;
			u32 total_instances;
			List<v2f32> vertices;
			List<const Convex*> shapes;
			List<DrawCommandVertex>	commands;
			List<List<Instance>> instances;

			static Batch create(Arena* arena, u32 expected_shapes = 1024) {
				Batch b = {
					.arena = arena,
					.total_instances = 0,
					.vertices = { arena->push_array<v2f32>(expected_shapes), 0 },
					.shapes = { arena->push_array<const Convex*>(expected_shapes), 0 },
					.commands = { arena->push_array<DrawCommandVertex>(expected_shapes), 0 },
					.instances = { arena->push_array<List<Instance>>(expected_shapes), 0 }
				};

				return b;
			}

			u32 write_polygon(Polygon poly) {
				auto index = vertices.current;
				vertices.push_growing(*arena, poly);
				return index;
			}

			u32 write_rect(rtf32 rect) {
				auto [v] = QuadGeo::make_vertices<v2f32>(rect);
				return write_polygon(larray(v));
			}

			u32 write_capsule(const v2f32 foci[2], f32 radius) {
				auto index = write_circle(foci[0], radius, debug_config.circle_segments);
				write_circle(foci[1], radius, debug_config.circle_segments);
				write_polygon(carray(foci, 2));
				//TODO replace middle line with sides
				return index;
			}

			u32 write_circle(v2f32 center, f32 radius, u32 segments = DEBUG_CIRCLE_SEGMENTS) {
				v2f32 v[segments];
				for (u32 i = 0; i < segments; i++)
					v[i] = center + v2f32(cosf(2 * glm::pi<f32>() / segments * i), sinf(2 * glm::pi<f32>() / segments * i)) * radius;
				return write_polygon(carray(v, segments));
			}

			u32 push_content(const Convex* shape, num_range<u32> vertex_range, List<Instance> instances_slot = {}) {
				auto index = shapes.current;
				shapes.push_growing(*arena, shape);
				commands.push_growing(*arena,
					DrawCommandVertex{
						.count = GLuint(vertex_range.size()),
						.instance_count = 0,
						.first_vertex = GLuint(vertex_range.min),
						.base_instance = SHAPE
					}
				);
				instances.push_growing(*arena, instances_slot);
				return index;
			}

			u32 register_shape(const Convex* shape, u32 expected_count = 1) {
				auto start_vertex = vertices.current;
				switch (shape->type) {
					case Convex::POLYGON: write_polygon(shape->poly); break;
					case Convex::CIRCLE: write_circle(shape->center, shape->radius); break;
					case Convex::RECT: write_rect(shape->rect); break;
					case Convex::CAPSULE: write_capsule(shape->foci, shape->radius); break;
					case Convex::SEGMENT: write_polygon(carray(&shape->segment.A, 2)); break;
				}
				return push_content(shape, num_range<u32>{ u32(start_vertex), u32(vertices.current) }, List { arena->push_array<Instance>(expected_count), 0 });
			}

			u32 cache_get_shape(const Convex* shape) {
				auto shape_index = index_of(cast<const Convex*>(shapes.used()), shape);
				if (shape_index == NILBODY)
					return register_shape(shape);
				return shape_index;
			}

			u32 push_collider(const Collider& collider, v4f32 color) {
				assert(collider.shape);
				if (debug_config.aabb)
					push_aabb(collider.aabb);
				if (debug_config.supports_scan)
					push_supports_scan(support_function_of(*collider.shape, collider.transform), debug_config.scan_color, debug_config.scan_segments);
				auto shape_index = cache_get_shape(collider.shape);
				auto instance_index = instances[shape_index].current;
				instances[shape_index].push_growing(*arena, Instance::create(collider.transform, color));
				commands[shape_index].instance_count += 1;
				total_instances += 1;
				return instance_index;
			}

			u32 push_aabb(rtf32 aabb, InstanceID id = COLLIDER_AABB) {
				auto start_vertex = vertices.current;
				write_rect(aabb);
				auto index = push_content(null, { u32(start_vertex), u32(vertices.current) }, { {}, 0 });
				commands[index].instance_count += 1;
				commands[index].base_instance = id;
				total_instances += 1;
				return 0;
			}

			template<support_function F> u32 push_supports_scan(const F& f, v4f32 color = v4f32(1, 0, 1, 1), u32 segments = DEBUG_CIRCLE_SEGMENTS * 4) {
				v2f32 v[segments * 2];
				for (u32 i = 0; i < segments; i++) {
					auto seg = f(glm::normalize(v2f32(
						cosf(2 * glm::pi<f32>() / segments * i),
						sinf(2 * glm::pi<f32>() / segments * i)
					)));
					v[i * 2 + 0] = seg.A;
					v[i * 2 + 1] = seg.B;
				}
				auto start_vertex = vertices.current;
				write_polygon(carray(v, segments * 2));
				auto shape_idx = push_content(null, num_range<u32>{ u32(start_vertex), u32(vertices.current) }, List { arena->push_array<Instance>(1), 0 });
				instances[shape_idx].push_growing(*arena, Instance::create(m3x3f32(1), color));
				commands[shape_idx].instance_count += 1;
				total_instances += 1;
				return shape_idx;
			}

			u32 push_segment(v2f32 a, v2f32 b, v4f32 color = v4f32(1, 0, 1, 1)) {
				v2f32 line[2] = { a, b };
				auto start_vertex = vertices.current;
				write_polygon(larray(line));
				auto shape_idx = push_content(null, num_range<u32>{ u32(start_vertex), u32(vertices.current) }, List { arena->push_array<Instance>(1), 0 });
				instances[shape_idx].push_growing(*arena, Instance::create(m3x3f32(1), color));
				commands[shape_idx].instance_count += 1;
				total_instances += 1;
				return shape_idx;
			}

			void push_sim_step(const SimStep& step) {
				for (auto& col : step.colliders.used())
					push_collider(col, v4f32(1, 1, 0, 1));
				for (auto& [cld] : step.tests.used())
					push_aabb(step.colliders[cld[0]].aabb & step.colliders[cld[1]].aabb, AABB_INTERSECTION);
			}

			void push_manifold(const Manifold& man) {
				push_aabb(man.ctc.aabb, AABB_INTERSECTION);
				push_segment(man.ctc.supports[0].A, man.ctc.supports[0].B, debug_config.supports);
				push_segment(man.ctc.supports[1].A, man.ctc.supports[1].B, debug_config.supports);
				push_segment(man.ctc.supports[0].A, man.ctc.supports[0].A - man.ctc.penetration, debug_config.supports);
				push_segment(man.ctc.supports[0].B, man.ctc.supports[0].B - man.ctc.penetration, debug_config.supports);
				push_segment(man.ctc.supports[1].A, man.ctc.supports[1].A + man.ctc.penetration, debug_config.supports);
				push_segment(man.ctc.supports[1].B, man.ctc.supports[1].B + man.ctc.penetration, debug_config.supports);
			}

		};

		struct Renderer {
			VertexArray vao;
			GPUBuffer vertices;
			GPUBuffer instances;
			GPUBuffer commands;
			GPUBuffer view_projection;

			Renderer& apply_batch(Batch& batch, m4x4f32 vp) {
				reset();
				vertices.push_as(batch.vertices.used());
				{
					auto mapping = List{ instances.map_as<Instance>({0, batch.total_instances + 2}) , 0 }; defer{ instances.unmap_as<Instance>({0, mapping.current}); };
					mapping.push(Instance::create(m3x3f32(1), debug_config.collider_aabb));
					mapping.push(Instance::create(m3x3f32(1), debug_config.aabb_intersection));
					for (usize i = 0; i < batch.commands.current; i++) switch (batch.commands[i].base_instance) {
						case COLLIDER_AABB: batch.commands[i].base_instance = 0; break;
						case AABB_INTERSECTION: batch.commands[i].base_instance = 1; break;
						case SHAPE: {
							batch.commands[i].base_instance = mapping.current;
							mapping.push(batch.instances[i].used());
						} break;
						default: break;
					}
				}
				commands.push_as(batch.commands.used());
				view_projection.push_one(vp);
				return *this;
			}

			void reset() {
				vertices.content = 0;
				instances.content = 0;
				commands.content = 0;
				view_projection.content = 0;
			}
		};

		struct Pipeline {
			GLuint id;
			GLuint view_projection;
			GLuint position;
			GLuint instances;

			static Pipeline create(GLScope& ctx) {
				auto ppl = load_pipeline(ctx, "shaders/physics_debug.glsl");
				return {
					.id = ppl,
					.view_projection = get_shader_input(ppl, "Camera", R_UBO),
					.position = get_shader_input(ppl, "position", R_VERT),
					.instances = get_shader_input(ppl, "Instances", R_SSBO)
				};
			}

			Renderer create_renderer(GLScope& ctx) {
				//TODO reasonable starting sizes
				Renderer rd = {
					.vao = VertexArray::create(ctx, GL_LINE_LOOP),
					.vertices = GPUBuffer::create_stretchy(ctx, sizeof(v2f32) * 4, GL_DYNAMIC_DRAW),
					.instances = GPUBuffer::create_stretchy(ctx, sizeof(Instance) * 2, GL_DYNAMIC_DRAW),
					.commands = GPUBuffer::create_stretchy(ctx, sizeof(DrawCommandVertex) * 2, GL_DYNAMIC_DRAW),
					.view_projection = GPUBuffer::create(ctx, sizeof(m4x4f32), GL_DYNAMIC_STORAGE_BIT)
				};

				rd.vao.conf_vattrib(position, vattr_fmt<v2f32>(0));
				return rd;
			}

			RenderCommand operator()(Arena& arena, const Renderer& rd) const {
				return {
					.pipeline = id,
					.draw_type = RenderCommand::D_MDAI,
					.draw = {.d_indirect = {
						.buffer = rd.commands.id,
						.stride = sizeof(DrawCommandVertex),
						.range = { 0, GLsizei(rd.commands.content_as<DrawCommandVertex>()) }
					}},
					.vao = rd.vao.id,
					.ibo = {
						.buffer = 0,
						.index_type = 0,
						.primitive = rd.vao.draw_mode
					},
					.vertex_buffers = arena.push_array({VertexBinding{
						.buffer = rd.vertices.id,
						.targets = arena.push_array({ position }),
						.offset = 0,
						.stride = sizeof(v2f32),
						.divisor = 0
					}}),
					.textures = {},
					.buffers = arena.push_array({
						BufferObjectBinding{
							.buffer = rd.view_projection.id,
							.type = GL_UNIFORM_BUFFER,
							.range = { 0, sizeof(m4x4f32) },
							.target = view_projection
						},
						BufferObjectBinding{
							.buffer = rd.instances.id,
							.type = GL_SHADER_STORAGE_BUFFER,
							.range = { 0, GLuint(rd.instances.content) },
							.target = instances
						}
					}),
				};
			}
		};

		Pipeline& gppl() {
			static Pipeline ppl = Pipeline::create(GLScope::global());
			return ppl;
		}

		Renderer& grd() {
			static Renderer rd = gppl().create_renderer(GLScope::global());
			return rd;
		}

		RenderCommand render(Arena& arena, Batch& batch, m4x4f32 vp) { return gppl()(arena, grd().apply_batch(batch, vp)); }
	}
}

#endif
