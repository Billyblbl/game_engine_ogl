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
		u32	indices[vertexCount];
		for (u32 i = 0; i < vertexCount; i++) {
			indices[i] = i;
			// printf("v[%u] = (%f,%f)\n", i, vertices[i].x, vertices[i].y);
			auto onscreen = view_transform.obj * v4f32(vertices[i].x, vertices[i].y, 0, 1);
			// printf("s[%u] = (%f,%f)\n", i, onscreen.x, onscreen.y);
		}


		auto mesh = upload_mesh(
			std::span(v2f32Attribute),
			std::span(vertices, vertexCount),
			std::span(indices, vertexCount),
			GL_TRIANGLE_FAN
		);
		mesh.element_count = vertexCount;

		GPUBinding ubos[] = { bind(view_transform, 0) };

		draw(draw_pipeline, mesh, 1, {}, {}, std::span(ubos, 1));

		delete_mesh(mesh);
	}

	void DrawPolygon(const b2Vec2* vertices, i32 vertexCount, const b2Color& color) {}
	void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) {}
	void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) {}
	void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) {}
	void DrawTransform(const b2Transform& xf) {}
	void DrawPoint(const b2Vec2& p, f32 size, const b2Color& color) {};

	MappedObject<m4x4f32> view_transform = map_object(m4x4f32(1));
	GLuint draw_pipeline = create_render_pipeline(
		load_shader("./shaders/simpleDraw.vert", GL_VERTEX_SHADER),
		load_shader("./shaders/simpleDraw.frag", GL_FRAGMENT_SHADER)
	);
};

#endif
