#ifndef GPHYSICS_2D_DEBUG
# define GPHYSICS_2D_DEBUG

#include <physics_2d.cpp>
#include <rendering.cpp>


struct ShapeRenderer {
	struct ShapeRenderInfo {
		m4x4f32 matrix;
		v4f32 color;
	};
	Pipeline pipeline;
	MappedObject<ShapeRenderInfo> render_info;

	static Array<const VertexAttributeSpecs> get_v2f32_attributes() {
		static const auto v2f32Attribute = make_vertex_attribute_spec<v2f32>(0, sizeof(v2f32));
		return carray(&v2f32Attribute, 1);
	}

	void draw_polygon(Array<v2f32> polygon, MappedObject<m4x4f32> view_matrix, bool wireframe = true) const {
		u32 indices[polygon.size()];
		for (auto i : u32xrange{ 0, polygon.size() })
			indices[i] = i;
		auto mesh = upload_mesh(get_v2f32_attributes(), polygon, carray(indices, polygon.size()), GL_TRIANGLE_FAN); defer{ delete_mesh(mesh); };
		if (wireframe)
			GL_GUARD(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
		pipeline(mesh, 1, { bind_to(view_matrix, 0), bind_to(render_info, 1) });
		if (wireframe)
			GL_GUARD(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
		wait_gpu();
	}

	void draw_circle(v3f32 circle, MappedObject<m4x4f32> view_matrix, bool wireframe = true) const {
		constexpr auto detail = 16;
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
		pipeline(mesh, 1, { bind_to(view_matrix, 0), bind_to(render_info, 1) });
		if (wireframe)
			GL_GUARD(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
		wait_gpu();
	}

	void draw_line(Segment<v2f32> line, MappedObject<m4x4f32> view_matrix, bool wireframe = true) const {
		u32 indices[2] = { 0, 1 };
		v2f32 vertices[2] = { line.A, line.B };
		auto mesh = upload_mesh(get_v2f32_attributes(), larray(vertices), larray(indices), GL_LINE_STRIP); defer{ delete_mesh(mesh); };
		pipeline(mesh, 1, { bind_to(view_matrix, 0), bind_to(render_info, 1) });
		wait_gpu();
	}

	void draw_point(v2f32 point, MappedObject<m4x4f32> view_matrix, bool wireframe = true) const {
		u32 idx = 0;
		auto mesh = upload_mesh(get_v2f32_attributes(), carray(&point, 1), carray(&idx, 1), GL_POINTS); defer{ delete_mesh(mesh); };
		pipeline(mesh, 1, { bind_to(view_matrix, 0), bind_to(render_info, 1) });
		wait_gpu();
	}

	void operator()(
		const Shape2D& shape,
		m4x4f32 model_matrix,
		MappedObject<m4x4f32> view_matrix,
		v4f32 color,
		bool wireframe
		) const {
		sync(view_matrix);
		sync(render_info, { model_matrix, color });

		switch (shape.type) {
		case Shape2D::Polygon: return draw_polygon(shape.polygon, view_matrix, wireframe);
		case Shape2D::Circle: return draw_circle(shape.circle, view_matrix, wireframe);
		case Shape2D::Line: return draw_line(shape.line, view_matrix, wireframe);
		case Shape2D::Point: return draw_point(shape.point, view_matrix, wireframe);
		case Shape2D::Composite:
			for (auto&& sub_shape : shape.composite)
				(*this)(sub_shape, model_matrix, view_matrix, color, wireframe);
		}
	}

};

ShapeRenderer load_shape_renderer(const cstr path = "./shaders/physics_debug.glsl") {
	return {
		load_pipeline(path),
		map_object<ShapeRenderer::ShapeRenderInfo>({})
	};
}

void unload(ShapeRenderer& rd) {
	destroy_pipeline(rd.pipeline);
	unmap(rd.render_info);
}

#endif
