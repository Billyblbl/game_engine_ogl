#ifndef GPHYSICS_2D
# define GPHYSICS_2D

#include <blblstd.hpp>
#include <math.cpp>
#include <imgui_extension.cpp>
#include <stdio.h>
#include <transform.cpp>
#include <polygon.cpp>
#include <shape_2d.cpp>


namespace Physics2D {

	constexpr f32 GRAVITY = 9.807f;

	union Momentum {
		v3f32 vec = v3f32(0);
		struct {
			v2f32 velocity;
			f32 angular_velocity;
		};
	};

	union Properties {
		v4f32 vec = v4f32(0);
		struct {
			f32 inverse_mass;
			f32 inverse_inertia;
			f32 restitution;
			f32 friction;
		};
	};

	struct Body {
		v2f32 center_mass;
		Momentum momentum;
		Properties props;
	};

	struct Convex {
		enum Type : u32 { NONE, POLYGON, RECT, CAPSULE, ELLIPSE, CIRCLE } type;
		f32 radius;
		union {
			Polygon poly;
			rtf32 rect;
			v2f32 foci[2];
			v2f32 center;
		};
	};

	struct Collider {
		m3x3f32 transform;
		rtf32 aabb;
		Convex* shape;//TODO maybe replace with some sort of ressource handle instead ? invites complexity tho, so maybe keep it a pointer but ensure this is only used a s a temporary lifetime struct
		u32 body_id;
		u32 layers;
	};

	struct NarrowTest { u32 ids[2]; };

	struct Contact {
		v2f32 penetration;
		Segment<v2f32> supports[2];
		rtf32 aabb;
	};

	struct Manifold {
		NarrowTest src;
		Contact ctc;
	};

	inline v2f32 support_circle(v2f32 center, f32 radius, v2f32 normalized_direction) {
		return (normalized_direction * radius) + center;
	}

	v2f32 support_transformed_circle(const m3x3f32& transform, v2f32 center, f32 radius, v2f32 norm_dir) {
		v2f32 local_dir = transpose(transform) * v3f32(norm_dir, 0);
		v2f32 local_point = normalize(local_dir) * radius;
		assert(!glm::any(isnan(local_point)));
		return transform * v3f32(local_point + center, 1);
	}

	Segment<v2f32> support_circle_cloud(Polygon vertices, const m3x3f32& transform, f32 radius, v2f32 norm_dir) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		using namespace glm;
		auto [scratch, scope] = scratch_push_scope(sizeof(f32) * vertices.size() * 3); defer{ scratch_pop_scope(scratch, scope); };

		v2f32 local_dir = transpose(transform) * v3f32(norm_dir, 0);
		v2f32 local_offset = normalize(local_dir) * radius;
		assert(!glm::any(isnan(local_offset)));
		auto support_verts = map(scratch, vertices, [&](v2f32 v) -> v2f32 { return transform * v3f32(local_offset + v, 1); });
		auto dots = map(scratch, support_verts, [=](v2f32 v) { return dot(v, norm_dir); });

		//*iA is best support, iB is either 2nd best support or iA
		i64 iA = best_fit_search(dots, fit_highest<f32>);
		if (iA < 0)
			return { v2f32(0), v2f32(0) };
		i64 iBa = best_fit_search(dots.subspan(0, iA), fit_highest<f32>);
		i64 iBboffset = best_fit_search(dots.subspan(iA + 1), fit_highest<f32>);
		i64 iBb = iA + 1 + iBboffset;
		i64 iB = -1;

		if (iBa < 0 && iBboffset < 0) iB = iA;
		else if (iBa < 0 && iBboffset >= 0) iB = iBb;
		else if (iBboffset < 0 && iBa >= 0) iB = iBa;
		else if (dots[iBa] > dots[iBb]) iB = iBa;
		else iB = iBb;
		assert(!glm::any(isnan(support_verts[iA])));
		assert(!glm::any(isnan(support_verts[iB])));
		return { support_verts[iA], support_verts[iB] };
	}

	template<typename F> concept support_function = requires(const F & f, v2f32 direction) { { f(direction) } -> std::same_as<Segment<v2f32>>; };

	// TODO continuous collision -> support function only selects points from the convex hull, should sample from the convex hull of the shapes at time t & t+dt
	inline auto support_function_of(const Convex& shape, const m3x3f32& transform) {
		return [&shape, transform](v2f32 direction) -> Segment<v2f32> {
			PROFILE_SCOPE(__PRETTY_FUNCTION__);
			switch (shape.type) {
			case Convex::POLYGON: return support_circle_cloud(shape.poly, transform, shape.radius, direction);
				// case Convex::RECT: ;//TODO IMPLEMENT
			case Convex::CAPSULE: return support_circle_cloud(larray(shape.foci), transform, shape.radius, direction);
				// case Convex::ELLIPSE: ;//TODO IMPLEMENT
			case Convex::CIRCLE: return support_circle_cloud(carray(&shape.center, 1), transform, shape.radius, direction);
			default: return { v2f32(0), v2f32(0) };
			}
			};
	}

	rtf32 aabb_segment(const Segment<v2f32>& segment) {
		return {
			.min = glm::min(segment.A, segment.B),
			.max = glm::max(segment.A, segment.B),
		};
	}

	rtf32 aabb_convex(const Convex& shape, const m3x3f32& transform) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		auto b = support_function_of(shape, transform)(v2f32(+0, -1)).A.y;
		auto t = support_function_of(shape, transform)(v2f32(+0, +1)).A.y;
		auto l = support_function_of(shape, transform)(v2f32(-1, +0)).A.x;
		auto r = support_function_of(shape, transform)(v2f32(+1, +0)).A.x;
		return {
			.min = v2f32(l, b),
			.max = v2f32(r, t),
		};
	}

	template<support_function F1, support_function F2> inline v2f32 minkowski_diff_support(
		const F1& f1,
		const F2& f2,
		v2f32 direction
	) {
		return f1(+direction).A - f2(-direction).A;
	}

	//* Gilbert-Johnson-Keerthi
	//* https://www.youtube.com/watch?v=ajv46BSqcK4&ab_channel=Reducible
	template<support_function F1, support_function F2> inline tuple<bool, Triangle> GJK(
		const F1& f1,
		const F2& f2,
		v2f32 start_direction = glm::normalize(v2f32(1)),
		i32 max_iterations = 999
	) {
		using namespace glm;
		constexpr tuple<bool, Triangle> no_collision = { false, Triangle{} };
		PROFILE_SCOPE(__PRETTY_FUNCTION__);

		v2f32 triangle_vertices[3];
		auto triangle = List{ larray(triangle_vertices), 0 };
		auto direction = start_direction;
		auto O = v2f32(0);//*origin

		for (auto i = 0; i < max_iterations; i++) {
			assert(!glm::any(isnan(direction)));
			auto new_point = minkowski_diff_support(f1, f2, direction);
			if (dot(new_point, direction) <= 0) //* Did we pass the origin to find A, return early otherwise
				return no_collision;
			triangle.push(new_point);

			if (triangle.current == 1) { //* first point case
				direction = normalize(O - triangle[0]);
			} else if (triangle.current == 2) { //* first line case
				auto [B, A] = to_tuple<2>(triangle.used());
				auto AB = B - A;
				auto AO = O - A;
				auto ABperp_axis = orthogonal_axis(AB);
				if (dot(AO, ABperp_axis) == 0) {//* origin is on AB
					triangle.push(v2f32(0));
					return { true, Triangle::from_array(triangle.used()) };
				}
				//* direction should be perpendicular to AB & go towards the origin
				direction = normalize(ABperp_axis * dot(AO, ABperp_axis));
			} else { //* triangle case
				auto [C, B, A] = to_tuple<3>(triangle.used());
				auto AB = B - A;
				auto AC = C - A;
				auto AO = O - A;
				auto ABperp_axis = orthogonal_axis(AB);
				auto ACperp_axis = orthogonal_axis(AC);
				if (dot(AO, ABperp_axis) == 0) //* origin is on AB
					return { true, Triangle::from_array(triangle.used()) };
				if (dot(AO, ACperp_axis) == 0) //* origin is on AC
					return { true, Triangle::from_array(triangle.used()) };
				auto ABperp = normalize(ABperp_axis * dot(ABperp_axis, -AC)); //* must go towards the exterior of triangle
				auto ACperp = normalize(ACperp_axis * dot(ACperp_axis, -AB)); //* must go towards the exterior of triangle
				if (dot(ABperp, AO) > 0) {//* region AB
					triangle.remove(0);//*C
					direction = ABperp;
				} else if (dot(ACperp, AO) > 0) { //* region AC
					triangle.remove(1);//*B
					direction = ACperp;
				} else { //* Inside triangle ABC
					return { true, Triangle::from_array(triangle.used()) };
				}
			}
		}

		return fail_ret("GJK iteration out of bounds", no_collision);
	}

	//* Expanding Polytope algorithm
	//* https://www.youtube.com/watch?v=0XQ2FSz3EK8&ab_channel=Winterdev
	template<support_function F1, support_function F2> v2f32 EPA(
		const F1& f1,
		const F2& f2,
		const Triangle& triangle,
		u32 max_iteration = 64,
		f32 precision_threshold = 0.05f
	) {
		using namespace glm;
		PROFILE_SCOPE(__PRETTY_FUNCTION__);

		v2f32 points_buffer[max_iteration + 3];
		auto points = List{ carray(points_buffer, max_iteration + 3), 0 };
		points.push(larray(triangle.vertices));

		auto best_point_index = 0;
		while (max_iteration-- > 0) {
			auto O = average(points.used());
			struct {
				f32 distance_to_origin = std::numeric_limits<f32>::max();
				v2f32 normal = v2f32(0);
				u64 pindex = 0;
			} closest_seg;

			for (auto i : u64xrange{ 0, points.current }) {//* find edge closest to origin
				auto j = (i + 1) % points.current;
				auto seg = Segment{ points[i], points[j] };
				auto OA = seg.A - O;
				auto normal_axis = orthogonal_axis(direction(seg));
				auto side = sign(dot(normal_axis, OA));
				if (side == 0) {//* handle flat triangle
						closest_seg = {
							0,
							normalize(normal_axis),
							j//* using j so that the inserted point will be between i and j
						};
				} else {
					v2f32 normal = normalize(side * normal_axis);
					assert(!glm::any(glm::isnan(normal)));
					auto dist_to_ori = abs(dot(normal, seg.A));

					if (dist_to_ori < closest_seg.distance_to_origin) {
						closest_seg = {
							dist_to_ori,
							normal,
							j//* using j so that the inserted point will be between i and j
						};
					}
				}
			}

			auto support_point = minkowski_diff_support(f1, f2, closest_seg.normal);
			auto error = dot(closest_seg.normal, support_point) - closest_seg.distance_to_origin;

			if (error <= precision_threshold) //* new point is close enough
				return closest_seg.normal * closest_seg.distance_to_origin;
			else
				points.insert_ordered(best_point_index = closest_seg.pindex, support_point);//* slow af, might want to replace array list ? but we need ordered specifically for indexed access, so linked list would be slow too (unless maybe if we use ptr instead of index, replace (i, j) with (node, node->next))
		}
		return points[best_point_index];
	}

	f32 cross_2(v2f32 a, v2f32 b) {
		return glm::cross(v3f32(a, 0), v3f32(b, 0)).z;
	}

	template<support_function F1, support_function F2> tuple<bool, Contact> intersect_convex(
		F1 f1, F2 f2,
		f32 penetration_tolerance = 0
	) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		auto [collided, triangle] = GJK(f1, f2);

		if (!collided)
			return { false, Contact{} };

		auto penetration = EPA(f1, f2, triangle, 16, 0.01f);

		if (length2(penetration) <= penetration_tolerance)
			return { false, Contact{} };

		auto contact_dir = v2f32(normalize(v2f64(penetration)));

		Contact ctc = {
			.penetration = penetration,
			.supports = { f1(+contact_dir), f2(-contact_dir) },
			.aabb = {}
		};
		ctc.aabb = aabb_segment(ctc.supports[0]) & aabb_segment(ctc.supports[1]);
		return { collided, ctc };
	}

	v2f32 velocity_at_point(const Momentum& momentum, v2f32 point) {
		return momentum.velocity + orthogonal(point) * glm::radians(momentum.angular_velocity);
	}

	// //* linear + angular momentum deltas -> https://www.youtube.com/watch?v=VbvdoLQQUPs&ab_channel=Two-BitCoding
	// //* static friction taken from : https://gamedevelopment.tutsplus.com/how-to-create-a-custom-2d-physics-engine-friction-scene-and-jump-table--gamedev-7756t
	// //* heavily modified since first written from these videos
	tuple<Momentum, Momentum> contact_response(
		const Body& body0,
		const Body& body1,
		const Contact& contact,
		f32 elasticity,
		f32 kinetic_friction,
		f32 static_friction
	) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		using namespace glm;
		auto c = contact.aabb.center();
		v2f32 levers[] = { c - body0.center_mass, c - body1.center_mass };

		auto contact_relative_vel = velocity_at_point(body1.momentum, levers[1]) - velocity_at_point(body0.momentum, levers[0]);

		auto normal = length2(contact.penetration) > 0 ? normalize(contact.penetration) : v2f32(0);
		auto tangent = orthogonal_axis(normal) * sign(dot(orthogonal_axis(normal), contact_relative_vel));

		if (dot(contact_relative_vel, normal) > 0)
			return { {}, {} };

		auto cancel = [](v2f32 v, v2f32 d)->f32 { return length(d) > 0 && length(v) > 0 ? -dot(v, d) : 0; };
		f32 normal_energy = max(0.f, cancel(contact_relative_vel, normal));
		f32 friction_energy = cancel(contact_relative_vel, tangent);

		auto overcome_static = abs(friction_energy) > abs(normal_energy * static_friction);
		v2f32 impulse =
			normal * (1.f + elasticity) * normal_energy + //* contact elastic bounce
			tangent * (overcome_static ? kinetic_friction : 1.f) * friction_energy; //* static friction resistance | kinetic friction dragging

		if (length2(impulse) == 0)
			return { {},{} };

		auto impulse_dir = normalize(impulse);

		auto mass_attenuation = body0.props.inverse_mass + body1.props.inverse_mass;
		auto angular_attenuation =
			pow2(dot(orthogonal_axis(levers[0]), impulse_dir)) * body0.props.inverse_inertia +
			pow2(dot(orthogonal_axis(levers[1]), impulse_dir)) * body1.props.inverse_inertia;
		auto accel_impulse = impulse / (mass_attenuation + angular_attenuation);

		assert(!glm::any(isnan(accel_impulse)));
		return {
			{
				.velocity = -accel_impulse * body0.props.inverse_mass,
				.angular_velocity = degrees(cross(v3f32(levers[0], 0), v3f32(-accel_impulse, 0)).z * body0.props.inverse_inertia)
			},
			{
				.velocity = +accel_impulse * body1.props.inverse_mass,
				.angular_velocity = degrees(cross(v3f32(levers[1], 0), v3f32(+accel_impulse, 0)).z * body1.props.inverse_inertia)
			}
		};
	}

	tuple<v2f32, v2f32> contact_correction(v2f32 penetration, f32 inverse_mass[2]) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		auto total_inv_mass = inverse_mass[0] + inverse_mass[1];
		return {
			penetration * -inv_lerp(0.f, total_inv_mass, inverse_mass[0]),
			penetration * +inv_lerp(0.f, total_inv_mass, inverse_mass[1])
		};
	}
}

#pragma region Editor

// #include <imgui_extension.cpp>
bool EditorWidget(const cstr label, Physics2D::Momentum& props) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("Velocity", props.velocity);
		changed |= EditorWidget("Angular Velocity", props.angular_velocity);
	}
	return changed;
}

bool EditorWidget(const cstr label, Physics2D::Properties& props) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		f32 mass = (props.inverse_mass != 0) ? 1 / props.inverse_mass : 0;
		if (EditorWidget("Mass", mass)) {
			props.inverse_mass = mass != 0 ? 1 / mass : 0;
			changed = true;
		}
		f32 inertia = (props.inverse_inertia != 0) ? 1 / props.inverse_inertia : 0;
		if (EditorWidget("Inertia", inertia)) {
			props.inverse_inertia = inertia != 0 ? 1 / inertia : 0;
			changed = true;
		}
		changed |= EditorWidget("restitution", props.restitution);
		changed |= EditorWidget("friction", props.friction);
	}
	return changed;
}

bool EditorWidget(const cstr label, Physics2D::Contact& ctc) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("penetration", ctc.penetration);
		changed |= EditorWidgetArray("supports", larray(ctc.supports), [](auto l, auto e) { return EditorWidget(l, e);});
		changed |= EditorWidget("aabb", ctc.aabb);
	}
	return changed;
}

bool EditorWidget(const cstr label, Physics2D::Convex& cvx) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		const cstrp types[] = { "NONE", "POLYGON", "RECT", "CAPSULE", "ELLIPSE", "CIRCLE" };
		i32 t = cvx.type;
		changed |= ImGui::Combo("type", &t, types, array_size(types));
		if (changed) {
			cvx.type = Physics2D::Convex::Type(t);
			cvx.poly = {};
		}
		changed |= EditorWidget("radius", cvx.radius);
		switch (cvx.type) {
		case Physics2D::Convex::POLYGON: changed |= EditorWidget("polygon", cvx.poly); break;
		case Physics2D::Convex::RECT: changed |= EditorWidget("rect", cvx.rect); break;
		case Physics2D::Convex::CAPSULE: changed |= EditorWidgetArray("foci", larray(cvx.foci), [](auto l, auto& e) { return EditorWidget(l, e); }); break;
		case Physics2D::Convex::ELLIPSE: changed |= EditorWidgetArray("foci", larray(cvx.foci), [](auto l, auto& e) { return EditorWidget(l, e); }); break;
		case Physics2D::Convex::CIRCLE: changed |= EditorWidget("center", cvx.center); break;
		default: break;
		}
	}
	return changed;
}

bool EditorWidget(const cstr label, Physics2D::Collider& col) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("transform", col.transform);
		changed |= EditorWidget("aabb", col.aabb);
		changed |= EditorWidgetPtr("shape", col.shape, [](auto l, auto& e) { return EditorWidget(l, e); });
		changed |= EditorWidget("body_id", col.body_id);
		changed |= EditorWidget("layers", col.layers);
	}
	return changed;
}

bool EditorWidget(const cstr label, Physics2D::NarrowTest& test) {
	return EditorWidgetArray(label, larray(test.ids), [](const cstr label, auto& c) { return EditorWidget(label, c); });
}

bool EditorWidget(const cstr label, Physics2D::Manifold& man) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("contact", man.ctc);
		changed |= EditorWidget("colliders", man.src);
	}
	return changed;
}

#pragma endregion Editor

#pragma region OLD

// #include <system_editor.cpp>
// #include <entity.cpp>
// #include <physics_2d_debug.cpp>

// struct RigidBody {
// 	EntityHandle handle;
// 	Array<const Shape2D> shapes;
// 	Spacial2D* spacial;
// 	Properties* body;
// 	f32 gravity_scale;
// };

// struct Collision2D {
// 	enum : u8 {
// 		DetectOnly = 0,
// 		Physical = 1 << 0,
// 		Linear = 1 << 1,
// 		Angular = 1 << 2,
// 	};
// 	Array<Contact2D> contacts;
// 	RigidBody entities[2];
// 	u8 flags;
// };

// u8 collision_type(Properties* b1, Properties* b2) {
// 	u8 flags = Collision2D::DetectOnly;
// 	if (b1 && b2)
// 		flags |= Collision2D::Physical;
// 	else
// 		return Collision2D::DetectOnly;
// 	if (b1->props.inverse_mass > 0 || b2->props.inverse_mass > 0)
// 		flags |= Collision2D::Linear;
// 	if (b1->props.inverse_inertia > 0 || b2->props.inverse_inertia > 0)
// 		flags |= Collision2D::Angular;
// 	return flags;
// }

// // using RigidBodyComp = tuple<EntityHandle, Shape2D*, Spacial2D*, Body*>;

// constexpr auto MAX_CONTACT_PER_COLLISION = 10u;

// //* linear + angular momentum deltas -> https://www.youtube.com/watch?v=VbvdoLQQUPs&ab_channel=Two-BitCoding
// //* static friction taken from : https://gamedevelopment.tutsplus.com/how-to-create-a-custom-2d-physics-engine-friction-scene-and-jump-table--gamedev-7756t
// //* heavily modified since first written from these videos
// tuple<Transform2D, Transform2D> contact_response(
// 	RigidBody ents[2],
// 	const Contact2D& contact,
// 	f32 elasticity,
// 	f32 kinetic_friction,
// 	f32 static_friction
// ) {
// 	using namespace glm;
// 	auto normal = length2(contact.penetration) > 0 ? normalize(contact.penetration) : v2f32(0);
// 	auto contact_relative_vel = velocity_at_point(ents[1].spacial->velocity, contact.levers[1]) - velocity_at_point(ents[0].spacial->velocity, contact.levers[0]);
// 	auto tangent = orthogonal_axis(normal) * sign(dot(orthogonal_axis(normal), contact_relative_vel));

// 	auto cancel = [](v2f32 v, v2f32 d)->f32 { return length(d) > 0 && length(v) > 0 ? -dot(v, d) : 0; };
// 	f32 normal_impulse = max(0.f, cancel(contact_relative_vel, normal));
// 	f32 friction_impulse = cancel(contact_relative_vel, tangent);

// 	auto overcome_static = abs(friction_impulse) > abs(normal_impulse * static_friction);

// 	v2f32 impulse =
// 		normal * (1.f + elasticity) * normal_impulse + //* contact elastic bounce
// 		tangent * (overcome_static ? kinetic_friction : 1.f) * friction_impulse; //* static friction resistance | kinetic friction dragging

// 	return lever_impulsion(
// 		impulse,
// 		{ contact.levers[0], contact.levers[1] },
// 		{ ents[0].body->props.inverse_mass, ents[1].body->props.inverse_mass },
// 		{ ents[0].body->props.inverse_inertia, ents[1].body->props.inverse_inertia }
// 	);
// }

// Collision2D make_collision(Array<Contact2D> contacts, RigidBody ents[2]) {
// 	return { contacts, { ents[0], ents[1] }, collision_type(ents[0].body, ents[1].body) };
// }

// Array<Collision2D> detect_collisions(Arena& arena, Array<RigidBody> entities, f32 penetration_tolerance = 0) {
// 	PROFILE_SCOPE(__PRETTY_FUNCTION__);
// 	if (entities.size() < 2)
// 		return {};

// 	struct BodyCache {
// 		m3x3f32 transform;
// 		rtf32 aabb;
// 		Array<rtf32> trees_aabb;
// 		Array<Array<Shape2D>> flattened;
// 		static BodyCache cache(Arena& arena, RigidBody& rb) {
// 			BodyCache c;
// 			c.transform = rb.spacial->transform;
// 			c.trees_aabb = map(arena, rb.shapes, [&](const Shape2D& shape) -> rtf32 { return aabb_transformed_rect(shape.aabb, c.transform); });
// 			c.aabb = aabb_shape_group(rb.shapes, c.transform);
// 			c.flattened = map(arena, rb.shapes, [&](const Shape2D&) -> Array<Shape2D> { return {}; });
// 			return c;
// 		}
// 	};

// 	auto collisions_mem = entities.size() * entities.size() * sizeof(Collision2D);
// 	auto mat_mem = sizeof(m3x3f32) * entities.size();
// 	auto [scratch, scope] = scratch_push_scope(collisions_mem + mat_mem + (1ul << 20), &arena); defer{ scratch_pop_scope(scratch, scope); };

// 	auto cache = map(scratch, entities, [&](RigidBody& rb) -> BodyCache { return BodyCache::cache(scratch, rb); });
// 	auto cache_shape_tree = [&](u64 entity_index, u64 tree_index) -> Array<Shape2D> {
// 		auto& tree_nodes = cache[entity_index].flattened[tree_index];
// 		if (tree_nodes.size() == 0)
// 			tree_nodes = flatten(scratch, entities[entity_index].shapes[tree_index], false, false);
// 		return tree_nodes;
// 		};

// 	auto collisions = List{ scratch.push_array<Collision2D>(entities.size() * entities.size()), 0 };//! this can theoretically break when entities have multiple top level shapes
// 	for (auto ie : u64xrange{ 0, entities.size() - 1 }) for (auto je : u64xrange{ ie + 1, entities.size() }) { //* entities
// 		PROFILE_SCOPE("Intersect Entities");
// 		if (!collide(cache[ie].aabb, cache[je].aabb)) //* AABB skip entities
// 			continue;

// 		for (auto is : u64xrange{ 0, entities[ie].shapes.size() }) for (auto js : u64xrange{ 0, entities[je].shapes.size() }) { //* shapes
// 			PROFILE_SCOPE("Intersect Shape Trees");
// 			if (!((entities[ie].shapes.size() == 1 && entities[je].shapes.size() == 1) || collide(cache[ie].trees_aabb[is], cache[je].trees_aabb[js])))
// 				continue;

// 			u64 intersections[] = { entities[ie].shapes[is].size, entities[je].shapes[js].size };
// 			auto intersection_cache = scratch.push_array<bool>(intersections[0] * intersections[1], true);
// 			auto contacts = List{ arena.push_array<Contact2D>(intersections[0] * intersections[1]), 0 };

// 			for (auto in : u64xrange{ 0, intersections[0] }) for (auto jn : u64xrange{ 0, intersections[1] }) if (!intersection_cache[jn + in * intersections[1]]) {
// 				PROFILE_SCOPE("Intersect Flattened Nodes");
// 				auto& flati = cache_shape_tree(ie, is)[in];
// 				auto& flatj = cache_shape_tree(je, js)[jn];
// 				if (collide(aabb_transformed_rect(flati.aabb, cache[ie].transform), aabb_transformed_rect(flatj.aabb, cache[je].transform))) {
// 					intersection_cache[jn + in * intersections[1]] = true;
// 					if (auto [collided, contact] = intersect_convex(flati, flatj, cache[ie].transform, cache[je].transform, penetration_tolerance); collided)
// 						contacts.push(contact);
// 				} else for (auto im : u64xrange{ in, in + flati.size }) for (auto jm : u64xrange{ jn, jn + flatj.size }) {
// 					intersection_cache[jm + im * intersections[1]] = true;
// 				}
// 			}

// 			RigidBody ents[] = { entities[ie], entities[je] };
// 			u32 shape_indices[] = { u32(is), u32(js) };
// 			if (contacts.current > 0)
// 				collisions.push_growing(scratch, make_collision(contacts.shrink_to_content(arena), ents, shape_indices));
// 		}
// 	}


// 	return arena.push_array(collisions.used());//? thats an array copy, not having that means we may waste space on the physics scratch, is it worth it ?
// }

// void resolve_collisions(Array<Collision2D> collisions) {
// 	PROFILE_SCOPE(__PRETTY_FUNCTION__);
// 	for (auto& col : collisions) if (has_all(col.flags, Collision2D::Physical) && col.contacts.size() > 0) {

// 		{
// 			PROFILE_SCOPE("Correct penetration");
// 			auto pen = average(col.contacts, &Contact2D::penetration);
// 			auto total_inv_mass = col.entities[0].body->props.inverse_mass + col.entities[1].body->props.inverse_mass;
// 			col.entities[0].spacial->transform.translation += pen * -inv_lerp(0.f, total_inv_mass, col.entities[0].body->props.inverse_mass);
// 			col.entities[1].spacial->transform.translation += pen * +inv_lerp(0.f, total_inv_mass, col.entities[1].body->props.inverse_mass);
// 		}

// 		if (!has_one(col.flags, Collision2D::Linear | Collision2D::Angular)) {
// 			// assert(false);//TODO remove this once physics stuff is fixed
// 			continue;
// 		}

// 		{
// 			PROFILE_SCOPE("Velocity Impulse");
// 			f32 elasticity = min(col.entities[0].body->props.restitution, col.entities[1].body->props.restitution);
// 			f32 kinetic_friction = average({ col.entities[0].body->props.friction, col.entities[1].body->props.friction });
// 			f32 static_friction = col.entities[0].body->props.friction * col.entities[1].body->props.friction;
// 			auto [d1, d2] = fold(tuple<Transform2D, Transform2D>{}, col.contacts,
// 				[&](const tuple<Transform2D, Transform2D>& acc, const Contact2D& contact) -> tuple<Transform2D, Transform2D> {
// 					auto [acc1, acc2] = acc;
// 					auto [r1, r2] = contact_response(col.entities, contact, elasticity, kinetic_friction, static_friction);
// 					return { acc1 + r1, acc2 + r2 };
// 				}
// 			);
// 			col.entities[0].spacial->velocity = col.entities[0].spacial->velocity + d1 * (1.f / f32(col.contacts.size()));
// 			col.entities[1].spacial->velocity = col.entities[1].spacial->velocity + d2 * (1.f / f32(col.contacts.size()));
// 		}
// 	}
// }

// struct Physics2D {
// 	Arena physics_scratch;
// 	List<Collision2D> collisions;
// 	f32 dt;
// 	u32 intersection_iterations;
// 	u32 max_ticks;
// 	v2f32 gravity;
// 	f32 penetration_tolerance;
// 	f32 tpu;
// 	f32 time;

// 	static constexpr u64 default_physics_memory = (MAX_ENTITIES * MAX_ENTITIES * sizeof(Collision2D) + MAX_ENTITIES * MAX_ENTITIES * sizeof(Contact2D) * MAX_CONTACT_PER_COLLISION);

// 	static Physics2D create(v2f32 gravity = v2f32(0, -9), f32 dt = 1.f / 60.f, u64 memory = default_physics_memory) {
// 		PROFILE_SCOPE(__PRETTY_FUNCTION__);
// 		Physics2D sim;
// 		sim.physics_scratch = Arena::from_vmem(memory);
// 		sim.collisions = { {}, 0 };
// 		sim.dt = dt;
// 		sim.gravity = gravity;
// 		sim.intersection_iterations = 2;
// 		sim.max_ticks = 1;
// 		sim.penetration_tolerance = 0;
// 		sim.tpu = 0;
// 		sim.time = 0;
// 		return sim;
// 	}

// 	void release() {
// 		physics_scratch.vmem_release();
// 		time = 0;
// 	}

// 	Arena& flush_state(u64 estimated_collisions_count = 10) {
// 		u64 heuristic_collision_count = max(estimated_collisions_count, collisions.current * 2);
// 		collisions = List{ physics_scratch.reset().push_array<Collision2D>(heuristic_collision_count), 0 };
// 		return physics_scratch;
// 	}

// 	void apply_gravity(Array<RigidBody> bodies) {
// 		for (auto& rb : bodies) if (rb.body && rb.body->props.inverse_mass > 0)
// 			rb.spacial->velocity.translation += gravity * dt * rb.gravity_scale;
// 	}

// 	void step_sim(Array<Spacial2D*> ents) {
// 		time += dt;
// 		for (auto s : ents)
// 			euler_integrate(*s, dt);
// 	}

// 	inline u32 iteration_count(f32 real_time) { return u32(glm::clamp(i32((real_time - time) / dt), i32(0), i32(max_ticks))); }

// 	inline void operator()(Array<RigidBody> bodies, Array<Spacial2D*> entities, u32 iterations, auto fixed_update) {
// 		PROFILE_SCOPE("Physics");
// 		flush_state(bodies.size() * bodies.size() * iterations * intersection_iterations / 2);
// 		tpu = (tpu + iterations) / 2;
// 		for (auto i : u64xrange{ 0, iterations }) {
// 			PROFILE_SCOPE("Tick");
// 			fixed_update(i);
// 			apply_gravity(bodies);
// 			step_sim(entities);
// 			for (auto r : u64xrange{ 0, intersection_iterations }) {
// 				(void)r;
// 				PROFILE_SCOPE("Intersection iteration");
// 				auto col = collisions.push_growing(physics_scratch, detect_collisions(physics_scratch, bodies, penetration_tolerance));
// 				resolve_collisions(col);
// 			}
// 		}
// 	}

// 	inline void operator()(Array<RigidBody> bodies, Array<Spacial2D*> entities, u32 iterations) {
// 		(*this)(bodies, entities, iterations, [](u64) {});
// 	}

// };

#pragma endregion OLD

#endif
