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

	void operator()(
		Shape2D& shape,
		m4x4f32 model_matrix,
		MappedObject<m4x4f32> view_matrix,
		v4f32 color,
		bool wireframe
	) {
		//TODO collider types other than polygon
		if (shape.type != Shape2D::Polygon) return;
		static const auto v2f32Attribute = make_vertex_attribute_spec<v2f32>(0, sizeof(v2f32));
		sync(view_matrix);
		sync(render_info, { model_matrix, color });

		// printf("herrtsh\n");

		u32 indices[shape.polygon.size()];
		for (auto i : u32xrange {0, shape.polygon.size()})
			indices[i] = i;

		auto mesh = upload_mesh(carray(&v2f32Attribute, 1), shape.polygon, carray(indices, shape.polygon.size()), GL_TRIANGLE_FAN);

		if (wireframe)
			GL_GUARD(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
		pipeline(mesh, 1, {
			bind_to(view_matrix, 0),
			bind_to(render_info, 1)
		});
		if (wireframe)
			GL_GUARD(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
		wait_gpu();
		delete_mesh(mesh);
	}

};

ShapeRenderer load_collider_renderer(const cstr path = "./shaders/physics_debug.glsl") {
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
