#ifndef GMODEL
# define GMODEL

#include <tuple>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vertex.cpp>

struct RenderMesh {
	GLuint vbo;
	GLuint ibo;
	GLuint vao;
};


//
// 9 Slice Quad sheets
//
// 0 - 1 - 2 - 3
// |   |   |   |
// 4 - 5 - 6 - 7
// |   |   |   |
// 8 - 9 - 10- 11
// |   |   |   |
// 12- 13- 14- 15
//

template<glm::uvec2 dimensions> constexpr auto createQuadSheetUV() {
	std::array<glm::vec2, (dimensions.x + 1)* (dimensions.y + 1)> uvs;
	for (auto y = 0; y < dimensions.y + 1; y++) {
		for (auto x = 0; x < dimensions.x + 1; x++) {
			uvs[y * (dimensions.x + 1) + x] = glm::vec2(
				(float)x / (float)dimensions.x,
				(float)y / (float)dimensions.y
			);
		}
	}
	return uvs;
}

template<glm::uvec2 dimensions> auto createQuadSheetIndices() {
	std::array<uint32_t, 3 * 2 * dimensions.x * dimensions.y> indices;
	for (auto y = 0; y < dimensions.y; y++) {
		for (auto x = 0; x < dimensions.x; x++) {
			auto quadIndex = (y * dimensions.x + x);
			indices[y * (3 * 2 * dimensions.x) + x] = quadIndex;
			indices[y * (3 * 2 * dimensions.x) + x + 1] = quadIndex + 1;
			indices[y * (3 * 2 * dimensions.x) + x + 2] = quadIndex + dimensions.x + 1;
			indices[y * (3 * 2 * dimensions.x) + x + 3] = quadIndex + dimensions.x + 1;
			indices[y * (3 * 2 * dimensions.x) + x + 4] = quadIndex + 1;
			indices[y * (3 * 2 * dimensions.x) + x + 5] = quadIndex + dimensions.x + 2;
		}
	}
	return indices;
}

std::span<glm::vec2>	getSingleQuadUV() {
	static auto uvs = createQuadSheetUV<glm::uvec2(1)>();
	return uvs;
}

std::span<uint32_t> getSingleQuadIndices() {
	static auto indices = createQuadSheetIndices<glm::uvec2(1)>();
	return indices;
}

auto createRect(glm::vec2 dimensions) {
	auto uvs = getSingleQuadUV();
	return std::make_pair(
		std::array{
			DefaultVertex2D { glm::vec2(-dimensions.x / 2.f,  dimensions.y / 2.f), uvs[0] },
			DefaultVertex2D { glm::vec2( dimensions.x / 2.f,  dimensions.y / 2.f), uvs[1] },
			DefaultVertex2D { glm::vec2(-dimensions.x / 2.f, -dimensions.y / 2.f), uvs[2] },
			DefaultVertex2D { glm::vec2( dimensions.x / 2.f, -dimensions.y / 2.f), uvs[3] }
		},
		getSingleQuadIndices()
	);
}

std::span<glm::vec2>	get9SliceUV() {
	static auto uvs = createQuadSheetUV<glm::uvec2(3, 3)>();
	return uvs;
}

std::span<uint32_t> get9SliceIndices() {
	static auto indices = createQuadSheetIndices<glm::uvec2(3, 3)>();
	return indices;
}

auto create9SlicePoints(glm::vec2 content, glm::vec2 borders) {
	return std::array{
		glm::vec2(-content.x / 2.f - borders.x / 2.f,  content.y / 2.f + borders.y / 2.f),
		glm::vec2(-content.x / 2.f									,  content.y / 2.f + borders.y / 2.f),
		glm::vec2(content.x / 2.f									,  content.y / 2.f + borders.y / 2.f),
		glm::vec2(content.x / 2.f + borders.x / 2.f,  content.y / 2.f + borders.y / 2.f),
		glm::vec2(-content.x / 2.f - borders.x / 2.f,  content.y / 2.f),
		glm::vec2(-content.x / 2.f									,  content.y / 2.f),
		glm::vec2(content.x / 2.f									,  content.y / 2.f),
		glm::vec2(content.x / 2.f + borders.x / 2.f,  content.y / 2.f),
		glm::vec2(-content.x / 2.f - borders.x / 2.f, -content.y / 2.f),
		glm::vec2(-content.x / 2.f									, -content.y / 2.f),
		glm::vec2(content.x / 2.f									, -content.y / 2.f),
		glm::vec2(content.x / 2.f + borders.x / 2.f, -content.y / 2.f),
		glm::vec2(-content.x / 2.f - borders.x / 2.f, -content.y / 2.f - borders.y / 2.f),
		glm::vec2(-content.x / 2.f									, -content.y / 2.f - borders.y / 2.f),
		glm::vec2(content.x / 2.f									, -content.y / 2.f - borders.y / 2.f),
		glm::vec2(content.x / 2.f + borders.x / 2.f, -content.y / 2.f - borders.y / 2.f)
	};
}

auto createRect9Slice(glm::vec2 content, glm::vec2 borders) {
	auto points = create9SlicePoints(content, borders);
	auto uvs = get9SliceUV();
	auto indices = get9SliceIndices();

	std::array<DefaultVertex2D, points.size()>	vertices;
	for (auto i = 0; i < points.size(); i++)
		vertices[i] = DefaultVertex2D{ points[i], uvs[i] };

	return std::make_pair(
		vertices,
		indices
	);
}

template<typename Vertex = DefaultVertex2D, typename Index = uint32_t>
RenderMesh uploadMesh(std::span<Vertex> vertices, std::span<Index> indices) {
	auto vbo = createBufferSpan(vertices);
	auto ibo = createBufferSpan(indices);
	auto vao = recordVAO<Vertex>(vbo, ibo);
	return { vbo, ibo, vao };
}

void deleteMesh(RenderMesh& mesh) {
	GL_GUARD(glDeleteVertexArrays(1, &mesh.vao));
	GLuint buffers[] = { mesh.vbo, mesh.ibo };
	GL_GUARD(glDeleteBuffers(2, buffers));
}

#endif
