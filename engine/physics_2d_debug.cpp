#ifndef GPHYSICS_2D_DEBUG
# define GPHYSICS_2D_DEBUG

#include <physics_2d.cpp>
#include <rendering.cpp>


struct ShapeRenderer {
	struct ShapeRenderInfo { v4f32 color; };
	Pipeline pipeline;
	MappedObject<ShapeRenderInfo> instance;
	MappedObject<m4x4f32> vp_matrix;

	static Array<const VertexAttributeSpecs> get_v2f32_attributes() {
		static const auto v2f32Attribute = make_vertex_attribute_spec<v2f32>(0, sizeof(v2f32));
		return carray(&v2f32Attribute, 1);
	}

	void draw_polygon(Array<v2f32> polygon, bool wireframe = true) const {
		if (polygon.size() == 0) return;
		u32 indices[polygon.size()];
		for (auto i : u32xrange{ 0, polygon.size() })
			indices[i] = i;
		auto mesh = upload_mesh(get_v2f32_attributes(), polygon, carray(indices, polygon.size()), GL_TRIANGLE_FAN); defer{ delete_mesh(mesh); };
		if (wireframe)
			GL_GUARD(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
		pipeline(mesh, 1, { bind_to(vp_matrix, 0), bind_to(instance, 1) });
		if (wireframe)
			GL_GUARD(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
		wait_gpu();
	}

	void draw_circle(v3f32 circle, bool wireframe = true) const {
		constexpr auto detail = 8;
		u32 indices[detail + 1] = { 0 };
		v2f32 vertices[detail + 1] = { v2f32(0) };
		for (auto i : u32xrange{ 0, detail }) {
			indices[i + 1] = i;
			auto angle = lerp(0.f, 2 * glm::pi<f32>(), f32(i) / f32(detail - 2));
			auto v = v2f32(glm::cos(angle), glm::sin(angle));
			vertices[i + 1] = v * circle.z + v2f32(circle);
		}
		auto mesh = upload_mesh(get_v2f32_attributes(), larray(vertices), larray(indices), GL_TRIANGLE_FAN); defer{ delete_mesh(mesh); };
		if (wireframe)
			GL_GUARD(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
		pipeline(mesh, 1, { bind_to(vp_matrix, 0), bind_to(instance, 1) });
		if (wireframe)
			GL_GUARD(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
		wait_gpu();
	}

	void draw_line(Segment<v2f32> line) const {
		u32 indices[2] = { 0, 1 };
		v2f32 vertices[2] = { line.A, line.B };
		auto mesh = upload_mesh(get_v2f32_attributes(), larray(vertices), larray(indices), GL_LINE_STRIP); defer{ delete_mesh(mesh); };
		pipeline(mesh, 1, { bind_to(vp_matrix, 0), bind_to(instance, 1) });
		wait_gpu();
	}

	void draw_point(v2f32 point) const {
		u32 idx = 0;
		auto mesh = upload_mesh(get_v2f32_attributes(), carray(&point, 1), carray(&idx, 1), GL_POINTS); defer{ delete_mesh(mesh); };
		pipeline(mesh, 1, { bind_to(vp_matrix, 0), bind_to(instance, 1) });
		wait_gpu();
	}

	void operator()(
		const Shape2D& shape,
		const m3x3f32& model_matrix,
		const m4x4f32& vp,
		v4f32 color,
		bool wireframe = true,
		bool local_aabbs = true,
		bool world_aabbs = true,
		bool radius = true//TODO
		) const {
		sync(vp_matrix, vp);

		auto [scratch, scope] = scratch_push_scope(1 << 16); defer{ scratch_pop_scope(scratch, scope); };

		auto traverse = (
			[&](auto& recurse, const Shape2D& s, m3x3f32 mat) -> void {
				auto local = mat * s.transform;
				{
					sync(instance, { color });
					auto local_scope = scratch.current; defer{ scratch_pop_scope(scratch, local_scope); };
					auto points = map(scratch, s.points, [&](v2f32 p) { return v2f32(local * v3f32(p, 1)); });
					draw_polygon(points, wireframe);

					if (world_aabbs) {
						auto aabb = aabb_transformed_aabb(s.aabb, mat);
						v2f32 aabb_corners[] = { aabb.min, v2f32(aabb.min.x, aabb.max.y), aabb.max, v2f32(aabb.max.x, aabb.min.y), };
						sync(instance, { v4f32(1, 0, 1, 1) });
						draw_polygon(larray(aabb_corners), true);
					}
				}

				if (local_aabbs) {
					v2f32 aabb_corners[] = {
						v2f32(mat * v3f32(s.aabb.min, 1)),
						v2f32(mat * v3f32(v2f32(s.aabb.min.x, s.aabb.max.y), 1)),
						v2f32(mat * v3f32(s.aabb.max, 1)),
						v2f32(mat * v3f32(v2f32(s.aabb.max.x, s.aabb.min.y), 1)),
					};
					sync(instance, { v4f32(0, 1, 1, 1) });
					draw_polygon(larray(aabb_corners), true);
				}
				sync(instance, { color });

				for (auto child : s.children)
					recurse(recurse, child, local);
			}
		);
		traverse(traverse, shape, model_matrix);
	}

};

ShapeRenderer load_shape_renderer(const cstr path = "./shaders/physics_debug.glsl") {
	return {
		load_pipeline(path),
		map_object<ShapeRenderer::ShapeRenderInfo>({}),
		map_object<m4x4f32>({})
	};
}

void unload(ShapeRenderer& rd) {
	destroy_pipeline(rd.pipeline);
	unmap(rd.instance);
}

#endif
