#ifndef GPHYSICS_2D
# define GPHYSICS_2D

#include <blblstd.hpp>
#include <math.cpp>
#include <imgui_extension.cpp>
#include <stdio.h>
#include <transform.cpp>
#include <polygon.cpp>
#include <shape_2d.cpp>

struct Body {
	f32 inverse_mass = 0.f;
	f32 inverse_inertia = 0.f;
	f32 restitution = 0.f;
	f32 friction = 0.f;
	i32 shape_index;
};

Segment<v2f32> support_fat_point_cloud(Array<v2f32> vertices, const m3x3f32& matrix, f32 radius, v2f32 direction) {
	using namespace glm;

	auto [scratch, scope] = scratch_push_scope(sizeof(f32) * vertices.size() * 3); defer{ scratch_pop_scope(scratch, scope); };
	auto dir = normalize(direction);
	auto world_verts = map(scratch, vertices, [&](v2f32 v) -> v2f32 { return matrix * v3f32(v + dir * radius, 1); });
	auto dots = map(scratch, world_verts, [=](v2f32 v) { return dot(v, dir); });
	//iA is best support, iB is either 2nd best support or iA
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
	return { world_verts[iA], world_verts[iB] };
}

template<typename F> concept support_function = requires(const F & f, v2f32 direction) { { f(direction) } -> std::same_as<Segment<v2f32>>; };

inline auto support_function_of(const Shape2D& shape, const m3x3f32& transform) {
	return ([&shape, transform](v2f32 direction) -> Segment<v2f32> { return support_fat_point_cloud(shape.points, transform * shape.transform, shape.radius, direction); });
}

template<support_function F1, support_function F2> inline Segment<v2f32> minkowski_diff_support(const F1& f1, const F2& f2, v2f32 direction) {
	auto s1 = f1(+direction);
	auto s2 = f2(-direction);
	return { s1.A - s2.A, s1.B - s1.B };
}

//* https://www.youtube.com/watch?v=ajv46BSqcK4&ab_channel=Reducible
//* Gilbert-Johnson-Keerthi
// TODO continuous collision -> support function only selects point, should sample from the convex hull of the shapes at time t & t+dt
template<support_function F1, support_function F2> inline tuple<bool, tuple<v2f32, v2f32, v2f32>> GJK(const F1& f1, const F2& f2, v2f32 start_direction = glm::normalize(v2f32(1))) {
	using namespace glm;
	constexpr tuple<bool, tuple<v2f32, v2f32, v2f32>> no_collision = { false, {{}, {}, {}} };
	PROFILE_SCOPE(__PRETTY_FUNCTION__);


	v2f32 triangle_vertices[3];
	auto triangle = List{ larray(triangle_vertices), 0 };
	auto direction = start_direction;

	for (auto i = 0; i < 999;i++) {//* inf loop safeguard TODO replace with configurable iteration limit
		auto new_point = minkowski_diff_support(f1, f2, direction).A;
		if (dot(new_point, direction) <= 0) //* Did we pass the origin to find A, return early otherwise
			return no_collision;
		triangle.push(new_point);

		if (triangle.current == 1) { //* first point case
			direction = -triangle[0];
		} else if (triangle.current == 2) { //* first line case
			auto [B, A] = to_tuple<2>(triangle.used());
			auto AB = B - A;
			auto AO = -A;//* origin - A
			auto ABperp_axis = v2f32(-AB.y, AB.x);
			if (dot(AO, ABperp_axis) == 0) {//* origin is on AB
				triangle.push(v2f32(0));
				return { true, to_tuple<3>(triangle.used()) };
			}
			//* direction should be perpendicular to AB & go towards the origin
			direction = normalize(ABperp_axis * dot(AO, ABperp_axis));
		} else { //* triangle case
			auto [C, B, A] = to_tuple<3>(triangle.used());
			auto AB = B - A;
			auto AC = C - A;
			auto AO = -A;//* origin - A
			auto ABperp_axis = v2f32(-AB.y, AB.x);
			auto ACperp_axis = v2f32(-AC.y, AC.x);
			if (dot(AO, ABperp_axis) == 0) //* origin is on AB
				return { true, to_tuple<3>(triangle.used()) };
			if (dot(AO, ACperp_axis) == 0) //* origin is on AC
				return { true, to_tuple<3>(triangle.used()) };
			auto ABperp = normalize(ABperp_axis * dot(ABperp_axis, -AC)); //* must go towards the exterior of triangle
			auto ACperp = normalize(ACperp_axis * dot(ACperp_axis, -AB)); //* must go towards the exterior of triangle
			if (dot(ABperp, AO) > 0) {//* region AB
				triangle.remove(0);//*C
				direction = ABperp;
			} else if (dot(ACperp, AO) > 0) { //* region AC
				triangle.remove(1);//*B
				direction = ACperp;
			} else { //* Inside triangle ABC
				return { true, to_tuple<3>(triangle.used()) };
			}
		}
	}

	return fail_ret("GJK iteration out of bounds", no_collision);
}

//* Expanding Polytope algorithm
//* https://www.youtube.com/watch?v=0XQ2FSz3EK8&ab_channel=Winterdev
template<support_function F1, support_function F2> v2f32 EPA(const F1& f1, const F2& f2, tuple<v2f32, v2f32, v2f32> triangle, u32 max_iteration = 64, f32 precision_threshold = 0.05f) {
	using namespace glm;
	PROFILE_SCOPE(__PRETTY_FUNCTION__);

	auto [A, B, C] = triangle;
	v2f32 points_buffer[max_iteration + 3];
	points_buffer[0] = A;
	points_buffer[1] = B;
	points_buffer[2] = C;

	auto points = List{ carray(points_buffer, max_iteration + 3), 3 };

	auto best_point_index = 0;
	for (auto iteration : u64xrange{ 0, max_iteration }) {
		(void)iteration;
		struct {
			f32 distance_to_origin = std::numeric_limits<f32>::max();
			v2f32 normal = v2f32(0);
			u64 index = 0;
		} closest_seg;

		for (auto i : u64xrange{ 0, points.current }) {//* find edge closest to origin
			auto j = (i + 1) % points.current;
			auto seg = Segment{ points[i], points[j] };
			auto normal = normalize(orthogonal_axis(direction(seg)));//* can be wrong way, using abs(ori_to_seg) & *sign(ori_to_seg) avoids problems from this
			auto ori_to_seg = dot(normal, seg.B);
			if (abs(ori_to_seg) < closest_seg.distance_to_origin)
				closest_seg = { abs(ori_to_seg), normal * sign(ori_to_seg), j };
		}

		auto support_point = minkowski_diff_support(f1, f2, closest_seg.normal).A;
		auto error = dot(closest_seg.normal, support_point) - closest_seg.distance_to_origin;

		if (error <= precision_threshold) //* new point is close enough
			return closest_seg.normal * closest_seg.distance_to_origin;
		else
			points.insert_ordered(best_point_index = closest_seg.index, support_point);
	}
	return points[best_point_index];
}

f32 cross_2(v2f32 a, v2f32 b) {
	return glm::cross(v3f32(a, 0), v3f32(b, 0)).z;
}

template<support_function F1, support_function F2> inline v2f32 contact_point(const F1& f1, const F2& f2, v2f32 normal) {
	//* assumes normal is in general direction s1 -> s2, aka normal is in direction of the penetration of s1 into s2
	using namespace glm;
	Segment<v2f32> s[] = { f1(+normal), f2(-normal) };


	v2f32 v[] = { direction(s[0]), direction(s[1]) };
	auto cr = cross_2(v[0], v[1]);
	if (cr == 0) {//* parallel -> has to overlap in our case, meaning the center of the intersection of their AABB is the middle of the intersection segments (since its one of its 2 diagonals)
		auto b = intersect(bounds(s[0]), bounds(s[1]));
		auto i = (b.min + b.max) / 2.f;
		return i;
	} else { //* https://www.youtube.com/watch?v=5FkOO1Wwb8w&ab_channel=EngineerNick
		auto v01A = direction(Segment{ s[0].A, s[1].A });//* AC
		auto t1 = +cross_2(v01A, v[1]) / cr;
		auto t2 = -cross_2(v01A, v[0]) / cr;
		auto i1 = s[0].A + v[0] * t1;
		if (!glm::any(isnan(i1))) return i1;
		auto i2 = s[1].A + v[1] * t2;
		if (!glm::any(isnan(i2))) return i2;
		assert(false);
		return {};
	}
}

struct Contact2D {
	v2f32 penetration;
	v2f32 levers[2];
	f32 surface;
};

bool EditorWidget(const cstr label, Contact2D& contact) {
	auto changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("Penetration", contact.penetration);
		changed |= EditorWidget("Levers", larray(contact.levers));
		changed |= EditorWidget("Surface", contact.surface);
		ImGui::TreePop();
	}
	return changed;
}

Contact2D make_contact(
	v2f32 pen,
	const Shape2D& shape1, const Shape2D& shape2,
	const m3x3f32& transform1, const m3x3f32& transform2
) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	assert(length(pen) > 0);
	auto offset_t1 = translate(transform1, v2f32(-pen));
	auto contact_dir = v2f32(normalize(v2f64(pen)));
	auto contact = contact_point(support_function_of(shape1, offset_t1), support_function_of(shape2, transform2), contact_dir);
	return { pen, { contact - v2f32(offset_t1 * v3f32(0, 0, 1)), contact - v2f32(transform2 * v3f32(0, 0, 1)) }, 0 };
}

tuple<bool, Contact2D> intersect_convex(
	const Shape2D& shape1, const Shape2D& shape2,
	const m3x3f32& transform1, const m3x3f32& transform2,
	f32 penetration_tolerance = 0
) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	auto f1 = support_function_of(shape1, transform1);
	auto f2 = support_function_of(shape2, transform2);
	auto [collided, triangle] = GJK(f1, f2);
	auto pen = (collided ? EPA(f1, f2, triangle, 16, 0.0001) : v2f32(0));
	collided = collided && length(pen) > penetration_tolerance;
	return { collided, collided ? make_contact(pen, shape1, shape2, transform1, transform2) : Contact2D{} };
}

v2f32 velocity_at_point(const Transform2D& velocity, v2f32 point) {
	return velocity.translation + orthogonal(point) * glm::radians(velocity.rotation);
}

tuple<Transform2D, Transform2D> lever_impulsion(v2f32 impulse, Array<const v2f32> lever, Array<const f32> inverse_masses, Array<const f32> inverse_inertias) {
	using namespace glm;
	if (length(impulse) <= 0) return { null_transform_2d, null_transform_2d };

	assert(!isnan(f32(inverse_masses[0] + inverse_masses[1] +
		pow(dot(orthogonal_axis(lever[0]), normalize(impulse)), 2) * inverse_inertias[0] +
		pow(dot(orthogonal_axis(lever[1]), normalize(impulse)), 2) * inverse_inertias[1])));

	auto attenuated = impulse / f32(inverse_masses[0] + inverse_masses[1] +
		pow(dot(orthogonal_axis(lever[0]), normalize(impulse)), 2) * inverse_inertias[0] +
		pow(dot(orthogonal_axis(lever[1]), normalize(impulse)), 2) * inverse_inertias[1]);

	auto push_at_lever = (
		[](v2f32 attenuated_impulse, v2f32 lever, f32 inverse_mass, f32 inverse_inertia) -> Transform2D {
			Transform2D delta_v;
			delta_v.translation = attenuated_impulse * inverse_mass;
			delta_v.rotation = degrees(cross(v3f32(lever, 0), v3f32(attenuated_impulse, 0)).z * inverse_inertia);
			return delta_v;
		}
		);

	return {
		push_at_lever(-attenuated, lever[0], inverse_masses[0], inverse_inertias[0]),
		push_at_lever(+attenuated, lever[1], inverse_masses[1], inverse_inertias[1])
	};
}

tuple<Transform2D, Transform2D> lever_impulsion(v2f32 impulse, LiteralArray<v2f32> lever, LiteralArray<f32> inverse_masses, LiteralArray<f32> inverse_inertias) { return lever_impulsion(impulse, larray(lever), larray(inverse_masses), larray(inverse_inertias)); }

#ifndef DEFAULT_GRAVITY
#define DEFAULT_GRAVITY v2f32(0, -9.807f)
#endif

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
		changed |= EditorWidget("Shape Index", body.shape_index);
	}
	return changed;
}

#include <system_editor.cpp>
#include <entity.cpp>
#include <physics_2d_debug.cpp>

struct RigidBody {
	EntityHandle handle;
	Array<const Shape2D> shapes;
	Spacial2D* spacial;
	Body* body;
	f32 gravity_scale;
};

struct Collision2D {
	enum : u8 {
		DetectOnly = 0,
		Physical = 1 << 0,
		Linear = 1 << 1,
		Angular = 1 << 2,
	};
	Array<Contact2D> contacts;
	RigidBody entities[2];
	u8 flags;
};

u8 collision_type(Body* b1, Body* b2, i32 i, i32 j) {
	u8 flags = Collision2D::DetectOnly;
	if (b1 && b2 && b1->shape_index == i && b2->shape_index == j)
		flags |= Collision2D::Physical;
	else
		return Collision2D::DetectOnly;
	if (b1->inverse_mass > 0 || b2->inverse_mass > 0)
		flags |= Collision2D::Linear;
	if (b1->inverse_inertia > 0 || b2->inverse_inertia > 0)
		flags |= Collision2D::Angular;
	return flags;
}

using RigidBodyComp = tuple<EntityHandle, Shape2D*, Spacial2D*, Body*>;

constexpr auto MaxContactPerCollision = 10u;

//* linear + angular momentum deltas -> https://www.youtube.com/watch?v=VbvdoLQQUPs&ab_channel=Two-BitCoding
//* static friction taken from : https://gamedevelopment.tutsplus.com/how-to-create-a-custom-2d-physics-engine-friction-scene-and-jump-table--gamedev-7756t
//* heavily modified since first written from these videos
tuple<Transform2D, Transform2D> contact_response(
	Array<RigidBody> ents,
	const Contact2D& contact,
	f32 elasticity,
	f32 kinetic_friction,
	f32 static_friction
) {
	using namespace glm;
	auto normal = length2(contact.penetration) > 0 ? normalize(contact.penetration) : v2f32(0);
	auto contact_relative_vel = velocity_at_point(ents[1].spacial->velocity, contact.levers[1]) - velocity_at_point(ents[0].spacial->velocity, contact.levers[0]);
	auto tangent = orthogonal_axis(normal) * sign(dot(orthogonal_axis(normal), contact_relative_vel));

	auto cancel = [](v2f32 v, v2f32 d)->f32 { return length(d) > 0 && length(v) > 0 ? -dot(v, d) : 0; };
	f32 normal_impulse = max(0.f, cancel(contact_relative_vel, normal));
	f32 friction_impulse = cancel(contact_relative_vel, tangent);

	auto overcome_static = abs(friction_impulse) > abs(normal_impulse * static_friction);

	v2f32 impulse =
		normal * (1.f + elasticity) * normal_impulse + //* contact elastic bounce
		tangent * (overcome_static ? kinetic_friction : 1.f) * friction_impulse; //* static friction resistance | kinetic friction dragging

	return lever_impulsion(
		impulse,
		{ contact.levers[0], contact.levers[1] },
		{ ents[0].body->inverse_mass, ents[1].body->inverse_mass },
		{ ents[0].body->inverse_inertia, ents[1].body->inverse_inertia }
	);
}

Collision2D make_collision(Array<Contact2D> contacts, RigidBody ents[2], u32 shape_index[2]) {
	return { contacts, { ents[0], ents[1] }, collision_type(ents[0].body, ents[1].body, shape_index[0], shape_index[1]) };
}

Array<Collision2D> detect_collisions(Arena& arena, Array<RigidBody> entities, f32 penetration_tolerance = 0) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	if (entities.size() < 2)
		return {};

	struct BodyCache {
		m3x3f32 matrix;
		rtf32 aabb;
		Array<rtf32> trees_aabb;
		Array<Array<Shape2D>> flattened;
		static BodyCache cache(Arena& arena, RigidBody& rb) {
			BodyCache c;
			c.matrix = rb.spacial->transform;
			c.trees_aabb = map(arena, rb.shapes, [&](const Shape2D& shape) -> rtf32 { return aabb_transformed_aabb(shape.aabb, c.matrix); });
			c.aabb = aabb_shape_group(rb.shapes, c.matrix);
			c.flattened = map(arena, rb.shapes, [&](const Shape2D&) -> Array<Shape2D> { return {}; });
			return c;
		}
	};

	auto collisions_mem = entities.size() * entities.size() * sizeof(Collision2D);
	auto mat_mem = sizeof(m3x3f32) * entities.size();
	auto [scratch, scope] = scratch_push_scope(collisions_mem + mat_mem + (1ul << 20), &arena); defer{ scratch_pop_scope(scratch, scope); };

	auto cache = map(scratch, entities, [&](RigidBody& rb) -> BodyCache { return BodyCache::cache(scratch, rb); });
	auto cache_shape_tree = (
		[&](u64 entity_index, u64 tree_index) -> Array<Shape2D> {
			auto& tree_nodes = cache[entity_index].flattened[tree_index];
			if (tree_nodes.size() == 0)
				tree_nodes = flatten(scratch, entities[entity_index].shapes[tree_index], false, false);
			return tree_nodes;
		}
		);

	auto collisions = List{ scratch.push_array<Collision2D>(entities.size() * entities.size()), 0 };//! this can theoretically break when entities have multiple top level shapes
	for (auto ie : u64xrange{ 0, entities.size() - 1 }) for (auto je : u64xrange{ ie + 1, entities.size() }) { //* entities
		PROFILE_SCOPE("Intersect Entities");
		if (!collide(cache[ie].aabb, cache[je].aabb)) //* AABB skip entities
			continue;

		for (auto is : u64xrange{ 0, entities[ie].shapes.size() }) for (auto js : u64xrange{ 0, entities[je].shapes.size() }) { //* shapes
			PROFILE_SCOPE("Intersect Shape Trees");
			if (!((entities[ie].shapes.size() == 1 && entities[je].shapes.size() == 1) || collide(cache[ie].trees_aabb[is], cache[je].trees_aabb[js])))
				continue;

			u64 intersections[] = { entities[ie].shapes[is].size, entities[je].shapes[js].size };
			auto intersection_cache = scratch.push_array<bool>(intersections[0] * intersections[1], true);
			auto contacts = List{ arena.push_array<Contact2D>(intersections[0] * intersections[1]), 0 };

			for (auto in : u64xrange{ 0, intersections[0] }) for (auto jn : u64xrange{ 0, intersections[1] }) if (!intersection_cache[jn + in * intersections[1]]) {
				PROFILE_SCOPE("Intersect Flattened Nodes");
				auto& flati = cache_shape_tree(ie, is)[in];
				auto& flatj = cache_shape_tree(je, js)[jn];
				if (collide(aabb_transformed_aabb(flati.aabb, cache[ie].matrix), aabb_transformed_aabb(flatj.aabb, cache[je].matrix))) {
					intersection_cache[jn + in * intersections[1]] = true;
					if (auto [collided, contact] = intersect_convex(flati, flatj, cache[ie].matrix, cache[je].matrix, penetration_tolerance); collided)
						contacts.push(contact);
				} else for (auto im : u64xrange{ in, in + flati.size }) for (auto jm : u64xrange{ jn, jn + flatj.size }) {
					intersection_cache[jm + im * intersections[1]] = true;
				}
			}

			RigidBody ents[] = { entities[ie], entities[je] };
			u32 shape_indices[] = { u32(is), u32(js) };
			if (contacts.current > 0)
				collisions.push_growing(scratch, make_collision(contacts.shrink_to_content(arena), ents, shape_indices));
		}
	}


	return arena.push_array(collisions.used());//? thats an array copy, not having that means we may waste space on the physics scratch, is it worth it ?
}

void resolve_collisions(Array<Collision2D> collisions) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	for (auto& col : collisions) if (has_all(col.flags, Collision2D::Physical) && col.contacts.size() > 0) {

		{
			PROFILE_SCOPE("Correct penetration");
			auto pen = average(col.contacts, &Contact2D::penetration);
			auto total_inv_mass = col.entities[0].body->inverse_mass + col.entities[1].body->inverse_mass;
			col.entities[0].spacial->transform.translation += pen * -inv_lerp(0.f, total_inv_mass, col.entities[0].body->inverse_mass);
			col.entities[1].spacial->transform.translation += pen * +inv_lerp(0.f, total_inv_mass, col.entities[1].body->inverse_mass);
		}

		if (!has_one(col.flags, Collision2D::Linear | Collision2D::Angular)) {
			// assert(false);//TODO remove this once physics stuff is fixed
			continue;
		}

		{
			PROFILE_SCOPE("Velocity Impulse");
			f32 elasticity = min(col.entities[0].body->restitution, col.entities[1].body->restitution);
			f32 kinetic_friction = average({ col.entities[0].body->friction, col.entities[1].body->friction });
			f32 static_friction = col.entities[0].body->friction * col.entities[1].body->friction;
			auto [d1, d2] = fold(tuple<Transform2D, Transform2D>{}, col.contacts,
				[&](const tuple<Transform2D, Transform2D>& acc, const Contact2D& contact) -> tuple<Transform2D, Transform2D> {
					auto [acc1, acc2] = acc;
					auto [r1, r2] = contact_response(col.entities, contact, elasticity, kinetic_friction, static_friction);
					return { acc1 + r1, acc2 + r2 };
				}
			);
			col.entities[0].spacial->velocity = col.entities[0].spacial->velocity + d1 * (1.f / f32(col.contacts.size()));
			col.entities[1].spacial->velocity = col.entities[1].spacial->velocity + d2 * (1.f / f32(col.contacts.size()));
		}
	}
}

struct Physics2D {
	Arena physics_scratch;
	List<Collision2D> collisions;
	f32 dt;
	u32 intersection_iterations;
	u32 max_ticks;
	v2f32 gravity;
	f32 penetration_tolerance;
	f32 tpu;
	f32 time;

	static constexpr u64 default_physics_memory = (MAX_ENTITIES * MAX_ENTITIES * sizeof(Collision2D) + MAX_ENTITIES * MAX_ENTITIES * sizeof(Contact2D) * MaxContactPerCollision);

	static Physics2D create(v2f32 gravity = v2f32(0, -9), f32 dt = 1.f / 60.f, u64 memory = default_physics_memory) {
		Physics2D sim;
		sim.physics_scratch = Arena::from_vmem(memory);
		sim.collisions = { {}, 0 };
		sim.dt = dt;
		sim.gravity = gravity;
		sim.intersection_iterations = 2;
		sim.max_ticks = 1;
		sim.penetration_tolerance = 0;
		sim.tpu = 0;
		sim.time = 0;
		return sim;
	}

	void release() {
		physics_scratch.vmem_release();
		time = 0;
	}

	Arena& flush_state(u64 estimated_collisions_count = 10) {
		u64 heuristic_collision_count = max(estimated_collisions_count, collisions.current * 2);
		collisions = List{ physics_scratch.reset().push_array<Collision2D>(heuristic_collision_count), 0 };
		return physics_scratch;
	}

	void apply_gravity(Array<RigidBody> bodies) {
		for (auto& rb : bodies) if (rb.body && rb.body->inverse_mass > 0)
			rb.spacial->velocity.translation += gravity * dt * rb.gravity_scale;
	}

	void step_sim(Array<Spacial2D*> ents) {
		time += dt;
		for (auto s : ents)
			euler_integrate(*s, dt);
	}

	inline u32 iteration_count(f32 real_time) { return u32(glm::clamp(i32((real_time - time) / dt), i32(0), i32(max_ticks))); }

	inline void operator()(Array<RigidBody> bodies, Array<Spacial2D*> entities, u32 iterations, auto fixed_update) {
		PROFILE_SCOPE("Physics");
		flush_state(bodies.size() * bodies.size() * iterations * intersection_iterations / 2);
		tpu = (tpu + iterations) / 2;
		for (auto i : u64xrange{ 0, iterations }) {
			PROFILE_SCOPE("Tick");
			fixed_update(i);
			apply_gravity(bodies);
			step_sim(entities);
			for (auto r : u64xrange{ 0, intersection_iterations }) {
				(void)r;
				PROFILE_SCOPE("Intersection iteration");
				auto col = collisions.push_growing(physics_scratch, detect_collisions(physics_scratch, bodies, penetration_tolerance));
				resolve_collisions(col);
			}
		}
	}

	inline void operator()(Array<RigidBody> bodies, Array<Spacial2D*> entities, u32 iterations) {
		(*this)(bodies, entities, iterations, [](u64) {});
	}

};

#endif
