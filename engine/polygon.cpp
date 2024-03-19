#ifndef GPOLYGON
# define GPOLYGON

#include <blblstd.hpp>
#include <math.cpp>

using Polygon = Array<v2f32>;

tuple<Polygon, usize> read_polygon(FILE* file, Arena& arena) {
	usize read = 0;
	usize count = 0;
	read += fread(&count, sizeof(usize), 1, file);
	auto array = arena.push_array<v2f32>(count);
	read += fread(array.data(), sizeof(v2f32), count, file);
	return tuple(array, read);
}

usize write_polygon(FILE* file, Polygon polygon) {
	usize wrote = 0;
	usize count = polygon.size();
	wrote += fwrite(&count, sizeof(usize), 1, file);
	wrote += fwrite(polygon.data(), sizeof(v2f32), count, file);
	return wrote;
}

enum WindingOrder : i8 {
	Unknown = 0,
	Clockwise = -1,
	AntiClockwise = 1
};

inline bool is_convex(Polygon poly) {
	if (poly.size() < 3)
		return false;

	bool clockwise = false;
	bool anti_clockwise = false;

	for (auto i : u64xrange{ 0, poly.size() }) {
		v2f32 verts[] = { poly[i], poly[(i + 1) % poly.size()], poly[(i + 2) % poly.size()] };
		v2f32 edges[] = { verts[1] - verts[0], verts[2] - verts[1] };
		auto cross = glm::cross(v3f32(edges[0], 0), v3f32(edges[1], 0)).z;

		if (cross < 0)
			clockwise = true;
		else if (cross > 0)
			anti_clockwise = true;

		if (clockwise && anti_clockwise)
			return false;
	}
	return true;

}

inline f32 double_signed_area(Polygon poly) {
	f32 signed_area = 0;
	for (auto i : u64xrange{ 0, poly.size() }) {
		v2f32 A = poly[i];
		v2f32 B = poly[(i + 1) % poly.size()];
		signed_area += A.x * B.y - B.x * A.y;
	}
	return signed_area;
}

inline f32 signed_area(Polygon poly) {
	return double_signed_area(poly) / 2;
}

inline WindingOrder poly_wo(Polygon poly) {
	return double_signed_area(poly) > 0 ? AntiClockwise : Clockwise;
}

WindingOrder wo_at(Polygon poly, i64 i) {
	using namespace glm;
	v2f32 verts[] = {
		poly[modidx(i - 1, poly.size())],
		poly[modidx(i, poly.size())],
		poly[modidx(i + 1, poly.size())]
	};
	v2f32 edges[] = { verts[1] - verts[0], verts[2] - verts[1] };
	return WindingOrder(i8(sign(cross(v3f32(edges[0], 0), v3f32(edges[1], 0)).z)));
}

inline bool convex_at(Polygon poly, i64 i, WindingOrder wo = Clockwise) { return wo_at(poly, i) != -wo; }

inline bool is_convex(Polygon poly, WindingOrder wo) {
	if (wo == Unknown)
		return is_convex(poly);

	using namespace glm;
	if (poly.size() < 3)
		return false;

	for (auto i : u64xrange{ 0, poly.size() }) if (!convex_at(poly, i, wo))
		return false;
	return true;
}

bool intersect_poly_poly(Array<v2f32> p1, Array<v2f32> p2);

//*from https://mathworld.wolfram.com/TriangleInterior.html#:~:text=The%20simplest%20way%20to%20determine,it%20lies%20outside%20the%20triangle.
bool intersect_tri_point(Polygon triangle, v2f32 point) {
	using namespace glm;
	auto det = [](v2f64 u, v2f64 v) -> f64 { return u.x * v.y - u.y * v.x; };

	v2f64 v = point;
	v2f64 v0 = v2f64(triangle[0]);
	v2f64 v1 = v2f64(triangle[1]) - v0;
	v2f64 v2 = v2f64(triangle[2]) - v0;

	f64 a = +(det(v, v2) - det(v0, v2)) / det(v1, v2);
	f64 b = -(det(v, v1) - det(v0, v1)) / det(v1, v2);

	return (a >= 0.f) && (b >= 0.f) && (a + b <= 1.f);
}

//* mostly from https://www.youtube.com/watch?v=QAdfkylpYwc&t=265s&ab_channel=Two-BitCoding
tuple<Array<Polygon>, Array<v2f32>> ear_clip(Arena& arena, Polygon polygon, bool early_out = true) {

	if (polygon.size() < 4 || is_convex(polygon)) {
		auto p = arena.push_array(polygon);
		auto dec = arena.push_array<Polygon>({ p });
		return { dec, p };
	}

	auto [scratch, scope] = scratch_push_scope(5000000); defer{ scratch_pop_scope(scratch, scope); };

	auto wo = poly_wo(polygon);
	auto remaining = List{ arena.push_array(polygon), polygon.size() };
	auto polys = List{ arena.push_array<Polygon>(polygon.size() - 2), 0 };
	auto poly_verts = List{ arena.push_array<v2f32>((polygon.size() - 2) * 3), 0 };

	struct Triangle { v2f32 points[3]; };

	auto angle_triangle = (
		[&](i64 i) -> Triangle {
			return { {
				remaining[modidx(i - 1, remaining.current)],
				remaining[modidx(i * 1, remaining.current)],
				remaining[modidx(i + 1, remaining.current)]
			} };
		}
	);

	auto is_ear = (
		[&](v2f32, i64 i) -> bool {
			auto tri = angle_triangle(i);
			if (!convex_at(remaining.used(), i, wo))
				return false;
			//* if there's a point of the remaining polygon that isn't part of the triangle yet still intersects with it
			for (auto v : remaining.used()) if (linear_search(larray(tri.points), v) < 0 && intersect_tri_point(larray(tri.points), v))
				return false;
			return true;
		}
	);

	while (remaining.current > 3 && !(early_out && is_convex(remaining.used(), wo))) {
		auto reflex = linear_search_idx(remaining.used(), [&](v2f32, i64 i) { return !convex_at(remaining.used(), i, wo); });
		auto ear = linear_search_idx(remaining.used(), is_ear, reflex + remaining.current - 1);
		assert(ear >= 0);
		auto tri = angle_triangle(ear);
		polys.push(poly_verts.push(larray(tri.points)));
		remaining.remove_ordered(ear);
	}
	polys.push(poly_verts.push(remaining.used()));
	return { polys.used(), poly_verts.shrink_to_content(arena) };
}

#endif
