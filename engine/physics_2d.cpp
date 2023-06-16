#ifndef GPHYSICS_2D
# define GPHYSICS_2D

#include <blblstd.hpp>
#include <math.cpp>
#include <imgui_extension.cpp>
#include <stdio.h>
#include <transform.cpp>

static const cstrp shape_type_names_array[] = { "None", "Circle", "Polygon", "Line", "Point", "Composite" };
static const auto shape_type_names = larray(shape_type_names_array);

struct Shape2D {
	enum Type : u32 { None, Circle, Polygon, Line, Point, Composite, TypeCount } type = None;
	union {
		Array<v2f32> polygon = {};
		Array<Shape2D> composite;
		v3f32 circle;//xy center, z radius
		Segment<v2f32> line;
		v2f32 point;
	};
};

struct Body {
	f32 inverse_mass = 0.f;
	f32 inverse_inertia = 0.f;
	f32 restitution = 0.f;
	f32 friction = 0.f;
};

template<Shape2D::Type type> struct shape_mapping {};
template<> struct shape_mapping<Shape2D::Circle> { static constexpr auto member = &Shape2D::circle; };
template<> struct shape_mapping<Shape2D::Polygon> { static constexpr auto member = &Shape2D::polygon; };
template<> struct shape_mapping<Shape2D::Line> { static constexpr auto member = &Shape2D::line; };
template<> struct shape_mapping<Shape2D::Point> { static constexpr auto member = &Shape2D::point; };
template<> struct shape_mapping<Shape2D::Composite> { static constexpr auto member = &Shape2D::composite; };

template<Shape2D::Type type> Shape2D make_shape_2d(auto&& init) {
	Shape2D shape = {};
	shape.type = type;
	shape.*shape_mapping<type>::member = init;
	return shape;
}

Array<v2f32> make_box_poly(Array<v2f32> buffer, v2f32 dimensions, v2f32 center) {
	assert(buffer.size() >= 4);
	buffer[0] = center + v2f32(-dimensions.x, dimensions.y) / 2.f;
	buffer[1] = center - dimensions / 2.f;
	buffer[2] = center - v2f32(-dimensions.x, dimensions.y) / 2.f;
	buffer[3] = center + dimensions / 2.f;
	return buffer.subspan(0, 4);
}

Shape2D make_box_shape(Array<v2f32> buffer, v2f32 dimensions, v2f32 center) {
	return make_shape_2d<Shape2D::Polygon>(make_box_poly(buffer, dimensions, center));
}

Segment<v2f32> support_circle(v2f32 center, f32 radius, v2f32 direction) {
	auto point = center + glm::normalize(direction) * radius;
	return { point, point };
}

Segment<v2f32> support_point_cloud(Array<v2f32> vertices, v2f32 direction) {
	struct {
		f32 dot = std::numeric_limits<f32>::lowest();
		v2f32 point;
	} buff[2];
	auto supports = List{ larray(buff), 0 };
	for (auto v : vertices) {
		auto dot = glm::dot(v, glm::normalize(direction));
		if (dot > supports.allocated()[0].dot) {
			supports.current = 0;
			supports.push({ dot, v });
		} else if (supports.current < supports.capacity.size() && dot == supports.allocated()[0].dot) {//TODO check if this == is enough or if it needs an epsilon comp
			supports.push({ dot, v });
		}
	}
	while (supports.current < supports.capacity.size())
		supports.push(supports.allocated().back());
	return { supports.allocated()[0].point, supports.allocated()[1].point };
}

Segment<v2f32> support_polygon(Array<v2f32> vertices, v2f32 direction) {
	return support_point_cloud(vertices, direction);
}

Segment<v2f32> support_line(Segment<v2f32> line, v2f32 direction) {
	return support_point_cloud(cast<v2f32>(carray(&line, 1)), direction);
}

Segment<v2f32> support(const Shape2D& shape, v2f32 direction) {
	switch (shape.type) {
	case Shape2D::Circle: return support_circle(v2f32(shape.circle), shape.circle.z, direction);
	case Shape2D::Polygon: return support_polygon(shape.polygon, direction);
	case Shape2D::Line: return support_line(shape.line, direction);
	case Shape2D::Point: return { shape.point, shape.point };
	case Shape2D::Composite: {
		Segment<v2f32> supports[shape.composite.size()];
		for (auto i : u64xrange{ 0, shape.composite.size() })
			supports[i] = support(shape.composite[i], direction);
		return support_point_cloud(cast<v2f32>(carray(supports, shape.composite.size())), direction);
	};
	default: return fail_ret("Unimplemented support function", Segment<v2f32>{});
	}
	return {};
}

template<typename F> concept support_function = requires(const F & f, v2f32 direction) { { f(direction) } -> std::same_as<Segment<v2f32>>; };

auto support_function_of(const Shape2D& shape, m4x4f32 transform) {
	return [&shape, transform](v2f32 direction) -> Segment<v2f32> {
		auto [lA, lB] = support(shape, glm::transpose(transform) * v4f32(direction, 0, 1));
		return { v2f32(transform * v4f32(lA, 0, 1)), v2f32(transform * v4f32(lB, 0, 1)) };
		};
}

inline rtf32 aabb_circle(v2f32 center, f32 radius, m4x4f32 transform) {
	auto scaling_factors = v2f32(
		glm::length(v3f32(transform[0])),
		glm::length(v3f32(transform[1]))
	);
	auto center_transformed = v2f32(transform * v4f32(center, 0, 1));
	auto radius_transformed = radius * max(scaling_factors.x, scaling_factors.y);

	return {
		center_transformed - v2f32(radius_transformed),
		center_transformed + v2f32(radius_transformed)
	};
}

inline rtf32 aabb_polygon(Array<v2f32> vertices, m4x4f32 transform) {
	auto mx = v2f32(std::numeric_limits<f32>::lowest());
	auto mn = v2f32(std::numeric_limits<f32>::max());

	for (auto&& v : vertices) {
		auto tfmd = v2f32(transform * v4f32(v, 0, 1));
		mx = glm::max(mx, tfmd);
		mn = glm::min(mn, tfmd);
	}
	return { mn, mx };
}

inline rtf32 aabb_line(Segment<v2f32> line, m4x4f32 transform) {
	auto transformed = rtf32{ v2f32(transform * v4f32(line.A, 0, 1)), v2f32(transform * v4f32(line.B, 0, 1)) };
	return { glm::min(transformed.min, transformed.max), glm::max(transformed.min, transformed.max) };
}

inline rtf32 aabb_point(v2f32 point, m4x4f32 transform) {
	auto transformed = v2f32(transform * v4f32(point, 0, 1));
	return { transformed, transformed };
}

inline rtf32 aabb_shape(const Shape2D& shape, m4x4f32 transform) {
	switch (shape.type) {
	case Shape2D::Circle: return aabb_circle(v2f32(shape.circle), shape.circle.z, transform);
	case Shape2D::Polygon: return aabb_polygon(shape.polygon, transform);
	case Shape2D::Line: return aabb_line(shape.line, transform);
	case Shape2D::Point: return aabb_point(shape.point, transform);
	case Shape2D::Composite: {
		rtf32 complete = { v2f32(std::numeric_limits<f32>::max()), v2f32(std::numeric_limits<f32>::lowest()) };
		for (auto&& s : shape.composite) {
			auto aabb = aabb_shape(s, transform);
			complete.min = glm::min(complete.min, aabb.min);
			complete.max = glm::max(complete.max, aabb.max);
		}
		return complete;
	};
	default: return { v2f32(1), v2f32(-1) };
	}
}

template<support_function F1, support_function F2> Segment<v2f32> minkowski_diff_support(const F1& f1, const F2& f2, v2f32 direction) {
	auto p = f1(direction).A - f2(-direction).A;
	return { p, p };
}

// https://www.youtube.com/watch?v=ajv46BSqcK4&ab_channel=Reducible
// Gilbert-Johnson-Keerthi
// TODO continuous collision -> support function only selects point, should sample from the convex hull of the shapes at time t & t+dt
template<support_function F1, support_function F2> tuple<bool, tuple<v2f32, v2f32, v2f32>> GJK(const F1& f1, const F2& f2, v2f32 start_direction = glm::normalize(v2f32(1))) {
	constexpr tuple<bool, tuple<v2f32, v2f32, v2f32>> no_collision = { false, {{}, {}, {}} };

	v2f32 triangle_vertices[3];
	auto triangle = List{ larray(triangle_vertices), 0 };
	auto direction = start_direction;

	for (auto i = 0; i < 999;i++) {// inf loop safeguard
		auto new_point = minkowski_diff_support(f1, f2, direction).A;
		if (glm::dot(new_point, direction) <= 0) // Did we pass the origin to find A, return early otherwise
			return no_collision;
		triangle.push(new_point);

		if (triangle.current == 1) { // first point case
			direction = -triangle.allocated()[0];
		} else if (triangle.current == 2) { // first line case
			auto [B, A] = to_tuple<2>(triangle.allocated());
			auto AB = B - A;
			auto AO = -A;// origin - A
			auto ABperp_axis = v2f32(-AB.y, AB.x);
			if (glm::dot(AO, ABperp_axis) == 0) {// origin is on AB
				triangle.push(v2f32(0));
				return { true, to_tuple<3>(triangle.allocated()) };
			}
			// direction should be perpendicular to AB & go towards the origin
			direction = glm::normalize(ABperp_axis * glm::dot(AO, ABperp_axis));
		} else { // triangle case
			auto [C, B, A] = to_tuple<3>(triangle.allocated());
			auto AB = B - A;
			auto AC = C - A;
			auto AO = -A;// origin - A
			auto ABperp_axis = v2f32(-AB.y, AB.x);
			auto ACperp_axis = v2f32(-AC.y, AC.x);
			if (glm::dot(AO, ABperp_axis) == 0) // origin is on AB
				return { true, to_tuple<3>(triangle.allocated()) };
			if (glm::dot(AO, ACperp_axis) == 0) // origin is on AC
				return { true, to_tuple<3>(triangle.allocated()) };
			auto ABperp = glm::normalize(ABperp_axis * glm::dot(ABperp_axis, -AC)); // must go towards the exterior of triangle
			auto ACperp = glm::normalize(ACperp_axis * glm::dot(ACperp_axis, -AB)); // must go towards the exterior of triangle
			if (glm::dot(ABperp, AO) > 0) {// region AB
				triangle.remove(0);//C
				direction = ABperp;
			} else if (glm::dot(ACperp, AO) > 0) { // region AC
				triangle.remove(1);//B
				direction = ACperp;
			} else { // Inside triangle ABC
				return { true, to_tuple<3>(triangle.allocated()) };
			}
		}
	}

	return fail_ret("GJK iteration out of bounds", no_collision);
}

// Expanding Polytope algorithm
// https://www.youtube.com/watch?v=0XQ2FSz3EK8&ab_channel=Winterdev
template<support_function F1, support_function F2> v2f32 EPA(const F1& f1, const F2& f2, tuple<v2f32, v2f32, v2f32> triangle, u32 max_iteration = 64, f32 iteration_threshold = 0.05f) {
	auto [A, B, C] = triangle;
	v2f32 points_buffer[max_iteration + 3] = { A, B, C };
	auto points = List{ carray(points_buffer, max_iteration + 3), 3 };

	auto min_index = 0;
	auto min_dist = std::numeric_limits<f32>::max();
	auto min_normal = v2f32(0);

	for (auto iteration : u64xrange{ 0, max_iteration }) {
		for (auto i : u64xrange{ 0, points.current }) {
			auto j = (i + 1) % points.current;
			auto vi = points.allocated()[i];
			auto vj = points.allocated()[j];
			auto normal = glm::normalize(orthogonal(vj - vi));
			auto dist = glm::dot(normal, vj);

			if (dist < 0) {
				dist *= -1;
				normal *= -1;
			}

			if (dist < min_dist) {
				min_dist = dist;
				min_normal = normal;
				min_index = j;
			}
		}

		auto support_point = minkowski_diff_support(f1, f2, min_normal).A;
		auto s_distance = glm::dot(min_normal, support_point);

		if (glm::abs(s_distance - min_dist) > iteration_threshold) {
			min_dist = std::numeric_limits<f32>::max();
			points.insert_ordered(min_index, std::move(support_point));
		} else {
			break;
		}

	}
	return min_normal * min_dist;
}

tuple<bool, rtf32, v2f32> intersect(const Shape2D& s1, const Shape2D& s2, m4x4f32 t1, m4x4f32 t2) {
	auto aabb_intersection = intersect(aabb_shape(s1, t1), aabb_shape(s2, t2));
	if (width(aabb_intersection) < 0 || height(aabb_intersection) < 0)
		return { false, aabb_intersection, v2f32(0) };

	auto f1 = support_function_of(s1, t1);
	auto f2 = support_function_of(s2, t2);

	auto [collided, triangle] = GJK(f1, f2);
	return { collided, aabb_intersection, (collided ? EPA(f1, f2, triangle) : v2f32(0)) };
}

bool can_resolve_collision(Body* b1, Body* b2) {
	return (b1 && b2) && // has bodies
		(b1->inverse_mass > 0 || b2->inverse_mass > 0) &&// can move
		(b1->inverse_inertia > 0 || b2->inverse_inertia > 0); // can turn
}

v2f32 velocity_at_point(const Transform2D& velocity, v2f32 point) {
	return velocity.translation + orthogonal(point) * glm::radians(velocity.rotation);
}

inline tuple<v2f32, v2f32> correction_offset(v2f32 penetration, f32 im1, f32 im2) {
	return {
		-penetration * inv_lerp(0.f, im1 + im2, im1),
		+penetration * inv_lerp(0.f, im1 + im2, im2)
	};
}

template<support_function F1, support_function F2> inline Segment<v2f32> contacts_segment(const F1& f1, const F2& f2, v2f32 normal) {
	//* assumes normal is in general direction s1 -> s2, aka normal is in direction of the penetration of s1 into s2
	Segment<v2f32> supports[] = { f1(+normal), f2(-normal) };
	for (auto [a, b] : supports) if (a == b)
		return { a, b };

	return Segment<v2f32>{// overlap of segments
		glm::min(glm::max(supports[0].A, supports[0].B), glm::max(supports[1].A, supports[1].B)),
			glm::max(glm::min(supports[0].A, supports[0].B), glm::min(supports[1].A, supports[1].B))
	};
}

Transform2D push_at_point(v2f32 impulse, v2f32 point, f32 inverse_mass, f32 inverse_inertia) {
	Transform2D delta_v;
	delta_v.translation = impulse * inverse_mass;
	delta_v.rotation = glm::degrees(glm::cross(v3f32(point, 0), v3f32(impulse, 0)).z * inverse_inertia);
	return delta_v;
}

struct PhysicalDescriptor {
	Transform2D velocity;
	v2f32 world_cmass;
	f32 inverse_mass;
	f32 inverse_inertia;
};

//linear + angular momentum -> https://www.youtube.com/watch?v=VbvdoLQQUPs&ab_channel=Two-BitCoding
//? modified to take in the average contact point -> convex only
tuple<Transform2D, Transform2D> bounce_contact(
	LiteralArray<PhysicalDescriptor> ents,
	v2f32 contact,
	v2f32 normal,
	f32 elasticity,
	f32 kinetic_friction // kinetic only for now
	//TODO static friction
) {
	auto lever1 = contact - larray(ents)[0].world_cmass;
	auto lever2 = contact - larray(ents)[1].world_cmass;
	auto ctc_rvel = velocity_at_point(larray(ents)[1].velocity, lever2) - velocity_at_point(larray(ents)[0].velocity, lever1);
	auto tangent = orthogonal_axis(normal) * glm::sign(glm::dot(orthogonal_axis(normal), ctc_rvel));

	auto normal_impulse = glm::dot(ctc_rvel, normal) /
		f32(larray(ents)[0].inverse_mass + larray(ents)[1].inverse_mass +
			glm::pow(glm::dot(orthogonal_axis(lever1), normal), 2) * larray(ents)[0].inverse_inertia +
			glm::pow(glm::dot(orthogonal_axis(lever2), normal), 2) * larray(ents)[1].inverse_inertia);

	auto impulse =
		normal * -(1.f + elasticity) * normal_impulse + //contact elastic bounce
		tangent * kinetic_friction * normal_impulse +// kinetic friction resistance
		v2f32(0);

	auto delta1 = push_at_point(-impulse, lever1, larray(ents)[0].inverse_mass, larray(ents)[0].inverse_inertia);
	auto delta2 = push_at_point(+impulse, lever2, larray(ents)[1].inverse_mass, larray(ents)[1].inverse_inertia);

	return { delta1, delta2 };
}

#ifndef DEFAULT_GRAVITY
#define DEFAULT_GRAVITY v2f32(0, -9.81f)
#endif

constexpr auto GRAVITY = DEFAULT_GRAVITY;

bool EditorWidget(const cstr label, Shape2D& shape) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= ImGui::Combo("Type", (i32*)&shape.type, shape_type_names.data(), shape_type_names.size());
		switch (shape.type) {
		case Shape2D::Circle: changed |= EditorWidget("Radius", shape.circle.z);break;
		case Shape2D::Polygon: changed |= EditorWidget("Polygon", shape.polygon);break;
		case Shape2D::Line: changed |= EditorWidget("Line", shape.line);break;
		case Shape2D::Point: changed |= EditorWidget("Point", shape.point);break;
		}
	}
	return changed;
}


bool EditorWidget(const cstr label, Body& body) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("Restitution", body.restitution);
		changed |= EditorWidget("Friction", body.friction);
		f32 mass = (body.inverse_mass != 0) ? 1 / body.inverse_mass : 0;
		if (EditorWidget("Mass", mass)) {
			body.inverse_mass = mass != 0 ? 1 / mass : 0;
			changed = true;
		}
		f32 inertia = (body.inverse_inertia != 0) ? 1 / body.inverse_inertia : 0;
		if (EditorWidget("Inertia", inertia)) {
			body.inverse_inertia = inertia != 0 ? 1 / inertia : 0;
			changed = true;
		}
	}
	return changed;
}

tuple<Array<v2f32>, usize> read_polygon(FILE* file, Alloc allocator) {
	usize read = 0;
	usize count = 0;
	read += fread(&count, sizeof(usize), 1, file);
	auto array = alloc_array<v2f32>(allocator, count);
	read += fread(array.data(), sizeof(v2f32), count, file);
	return tuple(array, read);
}

usize write_polygon(FILE* file, Array<v2f32> polygon) {
	usize wrote = 0;
	usize count = polygon.size();
	wrote += fwrite(&count, sizeof(usize), 1, file);
	wrote += fwrite(polygon.data(), sizeof(v2f32), count, file);
	return wrote;
}

struct PhysicsConfig {
	f32 time_step = 1.f / 60.f;
	u8 velocity_iterations = 8;
	u8 position_iterations = 3;
};

bool EditorWidget(const cstr label, PhysicsConfig& config) {
	auto changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("Physics time step", config.time_step);
		changed |= EditorWidget("Velocity iterations", config.velocity_iterations);
		changed |= EditorWidget("Position iterations", config.position_iterations);
		ImGui::TreePop();
	}
	if (config.time_step <= 0.0000f)
		config.time_step = 0.0001f;
	return changed;
}

#include <system_editor.cpp>
#include <entity.cpp>
#include <physics_2d_debug.cpp>

struct Physics2D {
	struct Collision {
		v2f32 penetration;
		rtf32 aabbi;
		EntityHandle e1;
		EntityHandle e2;
		bool physical;
		v2f32 contacts[2];
		Transform2D bounce_deltas[2];
	};
	List<Collision> collisions;

	void operator()(
		ComponentRegistry<Body>& bodies,
		ComponentRegistry<Shape2D>& shapes,
		ComponentRegistry<Spacial2D>& spacials,
		f32 dt) {

		//TODO chose another way for this buffer
		static Collision _buff[MAX_ENTITIES * MAX_ENTITIES];

		for (auto [_, sp] : spacials.iter())
			euler_integrate(*sp, dt);

		collisions = List{ larray(_buff), 0 };
		for (auto i : u64xrange{ 0, shapes.handles.current - 1 }) {
			for (auto j : u64xrange{ i + 1, shapes.handles.current }) {
				auto ent1 = shapes.handles.allocated()[i];
				auto ent2 = shapes.handles.allocated()[j];

				auto shape1 = shapes[ent1];
				auto shape2 = shapes[ent2];
				auto sp1 = spacials[ent1];
				auto sp2 = spacials[ent2];

				auto [collided, aabbi, pen] = intersect(*shape1, *shape2, trs_2d(sp1->transform), trs_2d(sp2->transform));
				if (collided)
					collisions.push({ pen, aabbi, ent1, ent2, false, {v2f32(0), {}} });
				else continue;

				auto body1 = bodies[ent1];
				auto body2 = bodies[ent2];
				if (!can_resolve_collision(body1, body2))
					continue;
				collisions.allocated().back().physical = true;

				// Correct penetration
				auto [o1, o2] = correction_offset(pen, body1->inverse_mass, body2->inverse_mass);
				sp1->transform.translation += o1;
				sp2->transform.translation += o2;
				auto normal = glm::normalize(pen);

				// Compute contact points with corrected positions
				auto [ctct1, ctct2] = contacts_segment(
					support_function_of(*shape1, trs_2d(sp1->transform)),
					support_function_of(*shape2, trs_2d(sp2->transform)),
					normal
				);
				collisions.allocated().back().contacts[0] = ctct1;
				collisions.allocated().back().contacts[1] = ctct2;

				// Bounce
				auto [delta1, delta2] = bounce_contact(
					{ { sp1->velocity, sp1->transform.translation, body1->inverse_mass, body1->inverse_inertia },
						{ sp2->velocity, sp2->transform.translation, body2->inverse_mass, body2->inverse_inertia } },
					average({ ctct1, ctct2 }),
					normal,
					min(body1->restitution, body2->restitution),
					average({ body1->friction, body2->friction })
				);
				collisions.allocated().back().bounce_deltas[0] = delta1;
				collisions.allocated().back().bounce_deltas[1] = delta2;
				sp1->velocity = sp1->velocity + delta1;
				sp2->velocity = sp2->velocity + delta2;
			}
		}
	}
	struct Editor : public SystemEditor {
		ShapeRenderer debug_draw = load_shape_renderer();
		bool debug = false;
		bool wireframe = true;

		Editor() : SystemEditor("Physics2D", "Alt+P", { Input::KB::K_LEFT_ALT, Input::KB::K_P }) {}

		void draw_debug(
			Array<Collision> collisions,
			ComponentRegistry<Shape2D>& shapes,
			ComponentRegistry<Spacial2D>& spacials,
			MappedObject<m4x4f32>& view_projection_matrix
		) {
			for (auto [ent, shape] : shapes.iter()) {
				auto spacial = spacials[*ent];
				auto mat = trs_2d(spacial->transform);
				debug_draw(*shape, mat, view_projection_matrix, v4f32(1, 0, 0, 1), wireframe);
				v2f32 local_vel = glm::transpose(mat) * v4f32(spacial->velocity.translation, 0, 1);

				sync(debug_draw.render_info, { mat, v4f32(1, 1, 0, 1) });
				debug_draw.draw_line(
					Segment<v2f32>{v2f32(0), local_vel},
					view_projection_matrix,
					wireframe
				);
			}

			for (auto [pen, aabbi, ent1, ent2, physical, contacts, _] : collisions) {
				v2f32 verts[4];
				sync(debug_draw.render_info, { m4x4f32(1), v4f32(0, 1, 0, 1) });
				debug_draw.draw_polygon(make_box_poly(larray(verts), dims_p2(aabbi), (aabbi.max + aabbi.min) / 2.f), view_projection_matrix, wireframe);
				if (physical) {
					sync(debug_draw.render_info, { m4x4f32(1), v4f32(0, 1, 1, 1) });
					debug_draw.draw_line({ contacts[0], contacts[1] }, view_projection_matrix, wireframe);
				}
			}
		}

		void editor_window(Physics2D& system) {
			EditorWidget("Draw debug", debug);
			if (debug)
				EditorWidget("Wireframe", wireframe);
			if (ImGui::TreeNode("Collisions")) {
				defer{ ImGui::TreePop(); };
				auto id = 0;
				for (auto [penetration, aabbi, ent1, ent2, physical, contacts, deltas] : system.collisions.allocated()) {
					char buffer[999];
					snprintf(buffer, sizeof(buffer), "%u:%s:%s", id, ent1.desc->name.data(), ent2.desc->name.data());
					ImGui::PushID(id++);
					if (ImGui::TreeNode(buffer)) {
						EditorWidget("aabbi", aabbi);
						EditorWidget("Penetration", penetration);
						if (physical) {
							EditorWidget("Contacts", larray(contacts));
							EditorWidget("Deltas", larray(deltas));
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}
		}
	};

	static auto default_editor() { return Editor(); }

};


#endif
