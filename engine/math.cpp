#ifndef GMATH
# define GMATH

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

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

template<typename P> struct polytope {
	P min;
	P max;
	template<typename OP> operator polytope<OP>() { return { OP(min), OP(max) }; }
};

template<typename P> auto width(polytope<P> p) { return p.max.x - p.min.x; }
template<typename P> auto height(polytope<P> p) { return p.max.y - p.min.y; }
template<typename P> auto depth(polytope<P> p) { return p.max.z - p.min.z; }
template<typename P> auto dim_4(polytope<P> p) { return p.max.w - p.min.w; }

template<typename P> P dims_p1(polytope<P> p) { return P(width(p)); }
template<typename P> P dims_p2(polytope<P> p) { return P(width(p), height(p)); }
template<typename P> P dims_p3(polytope<P> p) { return P(width(p), height(p), depth(p)); }
template<typename P> P dims_p4(polytope<P> p) { return P(width(p), height(p), depth(p), dim_4(p));  }

using sgf32 = polytope<v1f32>;
using sgf64 = polytope<v1f64>;

using sgu8 = polytope<v1u8>;
using sgu16 = polytope<v1u16>;
using sgu32 = polytope<v1u32>;
using sgu64 = polytope<v1u64>;

using sgi8 = polytope<v1i8>;
using sgi16 = polytope<v1i16>;
using sgi32 = polytope<v1i32>;
using sgi64 = polytope<v1i64>;

using rtf32 = polytope<v2f32>;
using rtf64 = polytope<v2f64>;

using rtu8 = polytope<v2u8>;
using rtu16 = polytope<v2u16>;
using rtu32 = polytope<v2u32>;
using rtu64 = polytope<v2u64>;

using rti8 = polytope<v2i8>;
using rti16 = polytope<v2i16>;
using rti32 = polytope<v2i32>;
using rti64 = polytope<v2i64>;

using bxf32 = polytope<v3f32>;
using bxf64 = polytope<v3f64>;

using bxu8 = polytope<v3u8>;
using bxu16 = polytope<v3u16>;
using bxu32 = polytope<v3u32>;
using bxu64 = polytope<v3u64>;

using bxi8 = polytope<v3i8>;
using bxi16 = polytope<v3i16>;
using bxi32 = polytope<v3i32>;
using bxi64 = polytope<v3i64>;

using tsf32 = polytope<v4f32>;
using tsf64 = polytope<v4f64>;

using tsu8 = polytope<v4u8>;
using tsu16 = polytope<v4u16>;
using tsu32 = polytope<v4u32>;
using tsu64 = polytope<v4u64>;

using tsi8 = polytope<v4i8>;
using tsi16 = polytope<v4i16>;
using tsi32 = polytope<v4i32>;
using tsi64 = polytope<v4i64>;

#endif
