#ifndef GMATH
# define GMATH

#include <blblstd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>


using v1f32 = glm::f32vec1;
using v1f64 = glm::f64vec1;

using v1u8 = glm::u8vec1;
using v1u16 = glm::u16vec1;
using v1u32 = glm::u32vec1;
using v1u64 = glm::u64vec1;

using v1i8 = glm::i8vec1;
using v1i16 = glm::i16vec1;
using v1i32 = glm::i32vec1;
using v1i64 = glm::i64vec1;

using v2f32 = glm::f32vec2;
using v2f64 = glm::f64vec2;

using v2u8 = glm::u8vec2;
using v2u16 = glm::u16vec2;
using v2u32 = glm::u32vec2;
using v2u64 = glm::u64vec2;

using v2i8 = glm::i8vec2;
using v2i16 = glm::i16vec2;
using v2i32 = glm::i32vec2;
using v2i64 = glm::i64vec2;

using v3f32 = glm::f32vec3;
using v3f64 = glm::f64vec3;

using v3u8 = glm::u8vec3;
using v3u16 = glm::u16vec3;
using v3u32 = glm::u32vec3;
using v3u64 = glm::u64vec3;

using v3i8 = glm::i8vec3;
using v3i16 = glm::i16vec3;
using v3i32 = glm::i32vec3;
using v3i64 = glm::i64vec3;

using v4f32 = glm::f32vec4;
using v4f64 = glm::f64vec4;

using v4u8 = glm::u8vec4;
using v4u16 = glm::u16vec4;
using v4u32 = glm::u32vec4;
using v4u64 = glm::u64vec4;

using v4i8 = glm::i8vec4;
using v4i16 = glm::i16vec4;
using v4i32 = glm::i32vec4;
using v4i64 = glm::i64vec4;

using m2x2f32 = glm::f32mat2x2;
using m2x2f64 = glm::f64mat2x2;

using m3x3f32 = glm::f32mat3x3;
using m3x3f64 = glm::f64mat3x3;

using m4x4f32 = glm::f32mat4x4;
using m4x4f64 = glm::f64mat4x4;

using qf32 = glm::fquat;
using qf64 = glm::dquat;

template<typename P> struct Segment {
	P A;
	P B;
};

template<typename P> P direction(Segment<P> s) { return s.B - s.A; }

template<typename P> struct reg_polytope {
	P min;
	P max;
	template<typename OP> operator reg_polytope<OP>() const { return { OP(min), OP(max) }; }
	inline Segment<P> diagonal() const { return { min, max }; }
	bool contain(P p) const { return glm::all(glm::lessThanEqual(p, max)) && glm::all(glm::lessThanEqual(min, p)); }
	P size() const { return max - min; }
	P center() const { return (min + max) / 2.f; }
};

template<typename P> reg_polytope<P> bounds(Segment<P> s) {
	return { glm::min(s.A, s.B), glm::max(s.A, s.B) };
}

template<typename P> auto width(reg_polytope<P> p) { return p.max.x - p.min.x; }
template<typename P> auto height(reg_polytope<P> p) { return p.max.y - p.min.y; }
template<typename P> auto depth(reg_polytope<P> p) { return p.max.z - p.min.z; }
template<typename P> auto dim_4(reg_polytope<P> p) { return p.max.w - p.min.w; }

template<typename P> P dims_p1(reg_polytope<P> p) { return P(width(p)); }
template<typename P> P dims_p2(reg_polytope<P> p) { return P(width(p), height(p)); }
template<typename P> P dims_p3(reg_polytope<P> p) { return P(width(p), height(p), depth(p)); }
template<typename P> P dims_p4(reg_polytope<P> p) { return P(width(p), height(p), depth(p), dim_4(p)); }
template<typename P> auto dim_vec(reg_polytope<P> p) { return p.max - p.min; }

template<typename P> inline reg_polytope<P> intersect(const reg_polytope<P> a, const reg_polytope<P> b) {
	return { glm::max(a.min, b.min), glm::min(a.max, b.max) };
}

template<typename P> inline reg_polytope<P> combined_aabb(const reg_polytope<P> a, const reg_polytope<P> b) {
	return { glm::min(a.min, b.min), glm::max(a.max, b.max) };
}

template<typename P> inline auto negative(const reg_polytope<P> p) {
	return glm::lessThan(p.max, p.min);
}

template<typename P> inline bool contains(const reg_polytope<P> a, const reg_polytope<P> b) {
	auto inter = intersect(a, b);
	return (b.min == inter.min && b.max == inter.max);
}

template<typename P> inline bool collide(const reg_polytope<P> a, const reg_polytope<P> b) {
	using namespace glm;
	return (all(greaterThan(dim_vec(intersect(a, b)), P(0))));
}

template<typename P> inline reg_polytope<P> operator&(const reg_polytope<P> a, const reg_polytope<P> b) { return intersect(a, b); }
template<typename P> inline reg_polytope<P> operator|(const reg_polytope<P> a, const reg_polytope<P> b) { return combined_aabb(a, b); }

template<typename T> T lerp(T a, T b, f32 t) { return a + t * (b - a); }
template<typename T> f32 inv_lerp(T a, T b, T v) { return (b == a) ? 0 : (v - a) / (b - a); }

template<typename T> T average(Array<T> elements) {
	auto sum = T(0);
	for (auto&& e : elements)
		sum += e;
	return sum / f32(elements.size());
}

template<typename T, typename U> inline U average(Array<T*> elements, U T::* member) {
	auto sum = U(0);
	for (T* e : elements)
		sum += e->*member;
	return sum / f32(elements.size());
}

template<typename T, typename U> inline U average(Array<T> elements, U T::* member) {
	auto sum = U(0);
	for (auto&& e : elements)
		sum += e.*member;
	return sum / f32(elements.size());
}

template<typename T> inline T average(LiteralArray<T> elements) { return average(larray(elements)); }

inline v2f32 orthogonal_axis(v2f32 v) { return v2f32(-v.y, v.x); }
inline v2f32 orthogonal(v2f32 v, f32 direction = 1) { return glm::rotate(v, direction * glm::radians(90.f)); }

using axf32 = reg_polytope<v1f32>;
using axf64 = reg_polytope<v1f64>;

using axu8 = reg_polytope<v1u8>;
using axu16 = reg_polytope<v1u16>;
using axu32 = reg_polytope<v1u32>;
using axu64 = reg_polytope<v1u64>;

using axi8 = reg_polytope<v1i8>;
using axi16 = reg_polytope<v1i16>;
using axi32 = reg_polytope<v1i32>;
using axi64 = reg_polytope<v1i64>;

using rtf32 = reg_polytope<v2f32>;
using rtf64 = reg_polytope<v2f64>;

using rtu8 = reg_polytope<v2u8>;
using rtu16 = reg_polytope<v2u16>;
using rtu32 = reg_polytope<v2u32>;
using rtu64 = reg_polytope<v2u64>;

using rti8 = reg_polytope<v2i8>;
using rti16 = reg_polytope<v2i16>;
using rti32 = reg_polytope<v2i32>;
using rti64 = reg_polytope<v2i64>;

using bxf32 = reg_polytope<v3f32>;
using bxf64 = reg_polytope<v3f64>;

using bxu8 = reg_polytope<v3u8>;
using bxu16 = reg_polytope<v3u16>;
using bxu32 = reg_polytope<v3u32>;
using bxu64 = reg_polytope<v3u64>;

using bxi8 = reg_polytope<v3i8>;
using bxi16 = reg_polytope<v3i16>;
using bxi32 = reg_polytope<v3i32>;
using bxi64 = reg_polytope<v3i64>;

using tsf32 = reg_polytope<v4f32>;
using tsf64 = reg_polytope<v4f64>;

using tsu8 = reg_polytope<v4u8>;
using tsu16 = reg_polytope<v4u16>;
using tsu32 = reg_polytope<v4u32>;
using tsu64 = reg_polytope<v4u64>;

using tsi8 = reg_polytope<v4i8>;
using tsi16 = reg_polytope<v4i16>;
using tsi32 = reg_polytope<v4i32>;
using tsi64 = reg_polytope<v4i64>;

template<typename T> auto pow2(T x) { return x * x; }

template<typename V> struct grid_iterator {
	V dimensions;
	V current;

	auto& operator++() {
		for (auto i : u64xrange{ 0 , dimensions.length() }) {
			if (++current[i] < dimensions[i])
				return *this;
			else if (i < dimensions.length() - 1)
				current[i] = 0;
			else {
				current = dimensions;
				return *this;
			}
		}
		return *this;
	}

	auto operator*() { return current; };
	bool operator!=(grid_iterator<V> rhs) { return current != rhs.current; }
};

template<typename V> using grid_range = it_range<grid_iterator<V>>;

using r2u32 = grid_range<v2u32>;
using r2u64 = grid_range<v2u64>;

using r2i32 = grid_range<v2i32>;
using r2i64 = grid_range<v2i64>;

inline i64 modidx(i64 index, i64 size) { return ((index % size) + size) % size; }

inline auto range2(v2u64 dimensions) {
	return r2u64{
		grid_iterator<v2u64>{ dimensions, v2u64(0) },
		grid_iterator<v2u64>{ dimensions, dimensions },
	};
}

using xf32 = std::numeric_limits<f32>;


struct QuadGeo {
	static constexpr u32 QUAD_INDICES[6] = { 0, 1, 2, 2, 3, 0 };
	template<typename I> struct Idx { I i[6]; };
	template<typename V> struct Vert { V v[4]; };

	v2f32 vertices[4];
	u32 indices[6];

	template<typename I> static Idx<I> make_indices(I start = 0) {
		return { .i = {
			start + QUAD_INDICES[0],
			start + QUAD_INDICES[1],
			start + QUAD_INDICES[2],
			start + QUAD_INDICES[3],
			start + QUAD_INDICES[4],
			start + QUAD_INDICES[5],
		}};
	}

	template<typename V> static Vert<V> make_vertices(reg_polytope<V> rect) {
		return { .v = {
			rect.min,
			V(rect.max.x, rect.min.y),
			rect.max,
			V(rect.min.x, rect.max.y)
		}};
	}

	static QuadGeo create(rtf32 rect, u32 starting_index = 0) {
		return {
			.vertices = {
				rect.min,
				v2f32(rect.max.x, rect.min.y),
				rect.max,
				v2f32(rect.min.x, rect.max.y)
			},
			.indices = {
				starting_index + QUAD_INDICES[0],
				starting_index + QUAD_INDICES[1],
				starting_index + QUAD_INDICES[2],
				starting_index + QUAD_INDICES[3],
				starting_index + QUAD_INDICES[4],
				starting_index + QUAD_INDICES[5]
			}
		};
	}
};


#endif
