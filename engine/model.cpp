#ifndef GMODEL
# define GMODEL

#include <GLFW/glfw3.h>
#include <math.cpp>
#include <vertex.cpp>
#include <glutils.cpp>
#include <buffer.cpp>

template<typename Vertex, typename Index> struct CPUGeometry {
	Array<Vertex> vertices;
	Array<Index> indices;
	GLuint draw_mode;
};

struct GPUGeometry {
	GPUBuffer vbo;
	GPUBuffer ibo;
	VertexArray vao;
	u32 element_count;
	u32 vertex_count;

	template<typename Vertex, typename Index> static inline GPUGeometry upload(const CPUGeometry<Vertex, Index>& geo) {
		return upload(geo.vertices, geo.indices, geo.draw_mode);
	}

	template<typename Vertex = DefaultVertex2D> static GPUGeometry upload(
		Array<const Vertex> vertex_data,
		Array<const u32> indices,
		GLuint draw_mode = GL_TRIANGLES
	) {
		GPUGeometry rm;
		rm.vbo = GPUBuffer::create(vertex_data.size_bytes(), GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT , cast<const byte>(vertex_data));
		rm.ibo = GPUBuffer::create(indices.size_bytes(), GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT, cast<const byte>(indices));
		rm.vao = VertexArray::create(draw_mode).associate(rm.vbo.id, rm.ibo.id, vertexAttributesOf<Vertex>);
		rm.element_count = indices.size();
		rm.vertex_count = vertex_data.size();
		return rm;
	}

	static GPUGeometry allocate(
		GeometryLayout geo_specs,
		u64 initial_v = (u64(1) << 16),
		u64 initial_i = (u64(1) << 16) * 6 / 4,//* index to vertices ratio for triangles
		GLuint draw_mode = GL_TRIANGLES
	) {
		GPUGeometry rm;
		rm.vbo = GPUBuffer::create(initial_v, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
		rm.ibo = GPUBuffer::create(initial_i, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
		rm.vao = VertexArray::create(draw_mode).associate(rm.vbo.id, rm.ibo.id, geo_specs);
		rm.element_count = 0;
		rm.vertex_count = 0;
		return rm;
	}

	template<typename Vertex> tuple<num_range<u64>, num_range<u64>> push(Array<const Vertex> vertices, Array<const u32> indices) {
		if (element_count == 0) {
			vao.associate(vbo.id, ibo.id, vertexAttributesOf<Vertex>);
			element_count = indices.size();
		}
		auto needed_vbo_size = vertex_count + vertices.size();
		auto needed_ibo_size = element_count + indices.size();
		if (needed_ibo_size > ibo.size)
			ibo.resize(round_up_bit(needed_ibo_size));
		if (needed_vbo_size > vbo.size)
			vbo.resize(round_up_bit(needed_vbo_size));
		ibo.write(cast<const byte>(indices), element_count);
		vbo.write(cast<const byte>(vertices), vertex_count);
		num_range<u64> index_range = { element_count, element_count + indices.size() };
		num_range<u64> vertex_range = { vertex_count, vertex_count + vertices.size() };
		element_count += indices.size();
		vertex_count += vertices.size();
		return {vertex_range, index_range};
	}

	void release() {
		vao.release();
		vbo.release();
		ibo.release();
	}

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
	std::array<v2f32, (dimensions.x + 1)* (dimensions.y + 1)> uvs;
	for (u32 y = 0; y < dimensions.y + 1; y++) {
		for (u32 x = 0; x < dimensions.x + 1; x++) {
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
	for (auto y : u32xrange{ 0, dimensions.y }) {
		for (auto x : u32xrange{ 0, dimensions.x }) {
			auto quad_index = (y * dimensions.x + x);
			auto index_index = (y * (3 * 2 * dimensions.x) + x);

			// // T1 clockwise
			// indices[index_index++] = quad_index;
			// indices[index_index++] = quad_index + 1;
			// indices[index_index++] = quad_index + dimensions.x + 1;

			// T1 counter clockwise
			indices[index_index++] = quad_index + dimensions.x + 1;
			indices[index_index++] = quad_index + 1;
			indices[index_index++] = quad_index;

			// // T2 clockwise
			// indices[index_index++] = quad_index + dimensions.x + 1;
			// indices[index_index++] = quad_index + 1;
			// indices[index_index++] = quad_index + dimensions.x + 2;

			// T2 counter clockwise
			indices[index_index++] = quad_index + dimensions.x + 2;
			indices[index_index++] = quad_index + 1;
			indices[index_index++] = quad_index + dimensions.x + 1;
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
			DefaultVertex2D { v2f32(-dimensions.x / 2.f, +dimensions.y / 2.f), uvs[0] },
			DefaultVertex2D { v2f32(+dimensions.x / 2.f, +dimensions.y / 2.f), uvs[1] },
			DefaultVertex2D { v2f32(-dimensions.x / 2.f, -dimensions.y / 2.f), uvs[2] },
			DefaultVertex2D { v2f32(+dimensions.x / 2.f, -dimensions.y / 2.f), uvs[3] }
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
		v2f32(-content.x / 2.f - borders.x / 2.f, +content.y / 2.f + borders.y / 2.f),
		v2f32(-content.x / 2.f									, +content.y / 2.f + borders.y / 2.f),
		v2f32(+content.x / 2.f									, +content.y / 2.f + borders.y / 2.f),
		v2f32(+content.x / 2.f + borders.x / 2.f, +content.y / 2.f + borders.y / 2.f),
		v2f32(-content.x / 2.f - borders.x / 2.f, +content.y / 2.f),
		v2f32(-content.x / 2.f									, +content.y / 2.f),
		v2f32(+content.x / 2.f									, +content.y / 2.f),
		v2f32(+content.x / 2.f + borders.x / 2.f, +content.y / 2.f),
		v2f32(-content.x / 2.f - borders.x / 2.f, -content.y / 2.f),
		v2f32(-content.x / 2.f									, -content.y / 2.f),
		v2f32(+content.x / 2.f									, -content.y / 2.f),
		v2f32(+content.x / 2.f + borders.x / 2.f, -content.y / 2.f),
		v2f32(-content.x / 2.f - borders.x / 2.f, -content.y / 2.f - borders.y / 2.f),
		v2f32(-content.x / 2.f									, -content.y / 2.f - borders.y / 2.f),
		v2f32(+content.x / 2.f									, -content.y / 2.f - borders.y / 2.f),
		v2f32(+content.x / 2.f + borders.x / 2.f, -content.y / 2.f - borders.y / 2.f)
	};
}

auto create_rect_9slice(v2f32 content, v2f32 borders) {
	auto points = create_9slice_points(content, borders);
	auto uvs = get_9slice_UV();
	auto indices = get_9slice_indices();

	std::array<DefaultVertex2D, points.size()> vertices;
	for (u32 i = 0; i < points.size(); i++)
		vertices[i] = DefaultVertex2D{ points[i], uvs[i] };

	return std::make_pair(
		vertices,
		indices
	);
}

GPUGeometry create_rect_mesh(v2f32 dimensions) {
	auto [vertices, indices] = create_rect(dimensions);
	return GPUGeometry::upload(Array<const DefaultVertex2D>(vertices.data(), vertices.size()), indices);
}

GPUGeometry create_rect_mesh(v2u32 source_dimensions, f32 ppu) {
	return create_rect_mesh(v2f32(source_dimensions) / ppu);
}

#endif
