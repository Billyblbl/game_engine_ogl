#ifndef GB2D_DEBUG_DRAW
# define GB2D_DEBUG_DRAW

#include <Box2D\Box2D.h>
#include <GL\glew.h>
#include <rendering.cpp>
#include <math.cpp>

const VertexAttributeSpecs v2f32Attribute[1] = { make_vertex_attribute_spec<v2f32>(0, sizeof(v2f32)) };

class B2dDebugDraw : public b2Draw {
public:
	void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
		if (view_transform == null) return;
		u32	indices[vertexCount];
		for (u32 i = 0; i < vertexCount; i++)
			indices[i] = i;
		auto mesh = upload_mesh(
			larray(v2f32Attribute),
			carray(vertices, vertexCount),
			carray(indices, vertexCount),
			GL_TRIANGLE_FAN
		);
		mesh.element_count = vertexCount;
		sync(*view_transform);
		sync(color_ubo, v4f32(color.r, color.g, color.b, color.a));

		if (wireframe)
			GL_GUARD(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
		draw_pipeline(mesh, 1, { bind_to(*view_transform, 0), bind_to(color_ubo, 1) });
		if (wireframe)
			GL_GUARD(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));

		wait_gpu();
		delete_mesh(mesh);
	}

	void DrawPolygon(const b2Vec2* vertices, i32 vertexCount, const b2Color& color) {}
	void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) {}
	void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) {}
	void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) {}
	void DrawTransform(const b2Transform& xf) {}
	void DrawPoint(const b2Vec2& p, f32 size, const b2Color& color) {};

	MappedObject<v4f32> color_ubo = map_object(v4f32(1, 0, 0, 1));
	MappedObject<m4x4f32>* view_transform = null;
	Pipeline draw_pipeline = load_pipeline("./shaders/physics_debug.glsl");
	bool wireframe = true;

	~B2dDebugDraw() {
		destroy_pipeline(draw_pipeline);
		unmap(color_ubo);
	}
};

#endif
