#ifndef GB2D_DEBUG_DRAW
# define GB2D_DEBUG_DRAW

#include <Box2D\Box2D.h>
#include <GL\glew.h>
#include <glm/glm.hpp>

class B2dDebugDraw : public b2Draw {
public:
	void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
		glColor4f(color.r, color.g, color.b, color.a);
		glBegin(GL_POLYGON);
		for (int i = 0; i < vertexCount; i++) {
			auto vertex = viewTransform * glm::vec4(vertices[i].x, vertices[i].y, 0.0f, 1.0f);
			glVertex2f(vertex.x, vertex.y);
		}
		glEnd();
	}

	void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {}
	void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) {}
	void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) {}
	void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) {}
	void DrawTransform(const b2Transform& xf) {}
	void DrawPoint(const b2Vec2& p, float size, const b2Color& color) {};

	glm::mat4 viewTransform;
};

#endif
