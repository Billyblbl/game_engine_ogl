#ifndef GMODEL
# define GMODEL

#include <GLFW/glfw3.h>
#include <math.cpp>
#include <vertex.cpp>
#include <glutils.cpp>

struct RenderMesh {
	GLuint vbo;
	GLuint ibo;
	GLuint vao;
	uint32_t element_count;
	GLenum index_type;
	GLenum draw_mode;
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

template<v2u32 dimensions> constexpr auto create_quad_sheet_UV() {
	std::array<v2f32, (dimensions.x + 1)*(dimensions.y + 1)> uvs;
	for (auto y = 0; y < dimensions.y + 1; y++) {
		for (auto x = 0; x < dimensions.x + 1; x++) {
			uvs[y * (dimensions.x + 1) + x] = v2f32(
				(f32)x / (f32)dimensions.x,
				(f32)y / (f32)dimensions.y
			);
		}
	}
	return uvs;
}

template<v2u32 dimensions> auto create_quad_sheet_indices() {
	std::array<uint32_t, 3 * 2 * dimensions.x * dimensions.y> indices;
	for (auto y = 0; y < dimensions.y; y++) {
		for (auto x = 0; x < dimensions.x; x++) {
			auto quadIndex = (y * dimensions.x + x);
			auto offset = 0;

			// // T1 clockwise
			// indices[y * (3 * 2 * dimensions.x) + x + offset++] = quadIndex;
			// indices[y * (3 * 2 * dimensions.x) + x + offset++] = quadIndex + 1;
			// indices[y * (3 * 2 * dimensions.x) + x + offset++] = quadIndex + dimensions.x + 1;

			// T1 counter clockwise
			indices[y * (3 * 2 * dimensions.x) + x + offset++] = quadIndex + dimensions.x + 1;
			indices[y * (3 * 2 * dimensions.x) + x + offset++] = quadIndex + 1;
			indices[y * (3 * 2 * dimensions.x) + x + offset++] = quadIndex;

			// // T2 clockwise
			// indices[y * (3 * 2 * dimensions.x) + x + offset++] = quadIndex + dimensions.x + 1;
			// indices[y * (3 * 2 * dimensions.x) + x + offset++] = quadIndex + 1;
			// indices[y * (3 * 2 * dimensions.x) + x + offset++] = quadIndex + dimensions.x + 2;

			// T2 counter clockwise
			indices[y * (3 * 2 * dimensions.x) + x + offset++] = quadIndex + dimensions.x + 2;
			indices[y * (3 * 2 * dimensions.x) + x + offset++] = quadIndex + 1;
			indices[y * (3 * 2 * dimensions.x) + x + offset++] = quadIndex + dimensions.x + 1;
		}
	}
	return indices;
}

Array<v2f32>	get_single_quad_UV() {
	static auto uvs = create_quad_sheet_UV<v2u32(1)>();
	return uvs;
}

Array<u32> get_single_quad_indices() {
	static auto indices = create_quad_sheet_indices<v2u32(1)>();
	return indices;
}

auto create_rect(v2f32 dimensions) {
	auto uvs = get_single_quad_UV();
	return std::make_pair(
		std::array{
			DefaultVertex2D { v2f32(-dimensions.x / 2.f,  dimensions.y / 2.f), uvs[0] },
			DefaultVertex2D { v2f32( dimensions.x / 2.f,  dimensions.y / 2.f), uvs[1] },
			DefaultVertex2D { v2f32(-dimensions.x / 2.f, -dimensions.y / 2.f), uvs[2] },
			DefaultVertex2D { v2f32( dimensions.x / 2.f, -dimensions.y / 2.f), uvs[3] }
		},
		get_single_quad_indices()
	);
}


Array<v2f32>	get_9slice_UV() {
	static auto uvs = create_quad_sheet_UV<v2u32(3, 3)>();
	return uvs;
}

Array<u32> get_9slice_indices() {
	static auto indices = create_quad_sheet_indices<v2u32(3, 3)>();
	return indices;
}

auto create_9slice_points(v2f32 content, v2f32 borders) {
	return std::array{
		v2f32(-content.x / 2.f - borders.x / 2.f,  content.y / 2.f + borders.y / 2.f),
		v2f32(-content.x / 2.f									,  content.y / 2.f + borders.y / 2.f),
		v2f32(content.x / 2.f									,  content.y / 2.f + borders.y / 2.f),
		v2f32(content.x / 2.f + borders.x / 2.f,  content.y / 2.f + borders.y / 2.f),
		v2f32(-content.x / 2.f - borders.x / 2.f,  content.y / 2.f),
		v2f32(-content.x / 2.f									,  content.y / 2.f),
		v2f32(content.x / 2.f									,  content.y / 2.f),
		v2f32(content.x / 2.f + borders.x / 2.f,  content.y / 2.f),
		v2f32(-content.x / 2.f - borders.x / 2.f, -content.y / 2.f),
		v2f32(-content.x / 2.f									, -content.y / 2.f),
		v2f32(content.x / 2.f									, -content.y / 2.f),
		v2f32(content.x / 2.f + borders.x / 2.f, -content.y / 2.f),
		v2f32(-content.x / 2.f - borders.x / 2.f, -content.y / 2.f - borders.y / 2.f),
		v2f32(-content.x / 2.f									, -content.y / 2.f - borders.y / 2.f),
		v2f32(content.x / 2.f									, -content.y / 2.f - borders.y / 2.f),
		v2f32(content.x / 2.f + borders.x / 2.f, -content.y / 2.f - borders.y / 2.f)
	};
}

auto create_rect_9slice(v2f32 content, v2f32 borders) {
	auto points = create_9slice_points(content, borders);
	auto uvs = get_9slice_UV();
	auto indices = get_9slice_indices();

	std::array<DefaultVertex2D, points.size()> vertices;
	for (auto i = 0; i < points.size(); i++)
		vertices[i] = DefaultVertex2D{ points[i], uvs[i] };

	return std::make_pair(
		vertices,
		indices
	);
}

template<typename Vertex = DefaultVertex2D, typename Index = glm::u32>
RenderMesh upload_mesh(Array<const VertexAttributeSpecs> vertex_attributes, Array<Vertex> vertices, Array<Index> indices, GLuint draw_mode = GL_TRIANGLES) {
	auto vbo = create_buffer_array(vertices);
	auto ibo = indices.size() != 0 ? create_buffer_array(indices) : 0;
	// auto vao = recordVAO<Vertex>(vbo, ibo);
	auto vao = record_VAO(vertex_attributes, vbo, ibo);
	return {
		vbo, ibo, vao,
		(u32)indices.size(),
		gl_type_table<Index>.upload_type,
		draw_mode
	};
}

template<typename Vertex = DefaultVertex2D, typename Index = glm::u32>
RenderMesh map_mesh(Array<const VertexAttributeSpecs> vertex_attributes, u32 vertex_count, u32 index_count, GLuint draw_mode = GL_TRIANGLES) {
	auto vbo = map_buffer<Vertex>(vertex_count);
	auto ibo = map_buffer<Index>(index_count);
	auto vao = record_VAO(vertex_attributes, vbo.id, ibo.id);
	return {
		vbo.id, ibo.id, vao.id,
		index_count,
		gl_type_table<Index>.type,
		draw_mode
	};
}

void delete_mesh(RenderMesh& mesh) {
	GL_GUARD(glDeleteVertexArrays(1, &mesh.vao));
	GLuint buffers[] = { mesh.vbo, mesh.ibo };
	GL_GUARD(glDeleteBuffers(2, buffers));
}

RenderMesh create_rect_mesh(v2f32 dimensions) {
	auto [vertices, indices] = create_rect(dimensions);
	return upload_mesh(vertexAttributesOf<DefaultVertex2D>, Array<DefaultVertex2D>(vertices.data(), vertices.size()), indices);
}

RenderMesh create_rect_mesh(v2u32 source_dimensions, f32 ppu) {
	return create_rect_mesh(v2f32(source_dimensions) / ppu);
}

RenderMesh get_unit_rect_mesh() {
	static auto mesh = create_rect_mesh(v2f32(1));
	return mesh;
}

#endif
