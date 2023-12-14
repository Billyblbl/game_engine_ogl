#ifndef GENTITY
# define GENTITY

#include <blblstd.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <high_order.cpp>
#include <utils.cpp>

#define MAX_ENTITIES 1000
#define MAX_DRAW_BATCH MAX_ENTITIES

constexpr auto flag_index_count = 32;

template<typename T, u64 T::* gen> struct genhandle {
	T* ptr;
	u64 generation;
	bool valid() { return ptr != null && ptr->*(gen) <= generation; }
	T* try_get() { return valid() ? ptr : null; }
	T* operator->() { return (assert(valid()), ptr); }
	T& operator*() { return (assert(valid()), *ptr); }
};

template<typename T, typename U> concept castable = std::derived_from<T, U> || std::same_as<T, U>;

template<typename T, u64 T::* gen> genhandle<T, gen> get_genhandle(T& slot) {
	return { &slot, slot.*(gen) };
}

struct EntitySlot {
	static constexpr auto entity_flag_count = 64;
	enum : u64 {
		AllocatedEntity = 1 << 0,
		Enabled = 1 << 1,
		Usable = AllocatedEntity | Enabled,
		PendingRelease = 1 << 2,
		UserFlag = 1 << 3,
		FlagIndexCount = entity_flag_count
	};
	static constexpr string BaseFlags[] = {
		"None",
		"Allocated",
		"Enabled",
		"Pending Release",
		"User Flag"
	};
	u64 flags = 0;
	u64 generation = 0;
	string name = "__entity__";
	bool available() const { return !has_one(flags, AllocatedEntity | PendingRelease); }
	void grab() { flags |= AllocatedEntity; }
	bool enabled() const { return flags & Enabled; }
	void enable(bool state = true) { state ? (flags |= Enabled) : (flags &= ~Enabled); }
	void discard() { flags = (flags & ~Usable) | PendingRelease; }
	void recycle() { flags = 0; generation++; }
	template<typename T> inline T& content() { return static_cast<T&>(*this); }
};

template<typename T> concept EntityLayout = castable<T, EntitySlot>;
using EntityHandle = genhandle<EntitySlot, &EntitySlot::generation>;

template<EntityLayout E = EntitySlot> inline E& allocate_entity(List<E>& entities, string name = "__entity__", u64 flags = 0) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	auto i = linear_search(entities.used(), [](const E& ent) { return ent.available(); });
	auto& slot = i < 0 ? entities.push(E{}) : entities[i];
	slot.grab();
	slot.name = name;
	slot.flags |= flags;
	return slot;
}

inline EntityHandle get_entity_genhandle(EntitySlot& ent) {
	return get_genhandle<EntitySlot, &EntitySlot::generation>(ent);
}

template<typename I> tuple<bool, I> use_as(EntityHandle handle);

template<typename I, castable<EntitySlot> E> auto gather(Arena& arena, Array<E> entities) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	auto list = List{ arena.push_array<I>(entities.size()), 0 };
	for (auto&& i : entities) if (has_all(i.flags, EntitySlot::Usable)) if (auto [good, res] = use_as<I>(get_entity_genhandle(i)); good)
		list.push(res);
	return list.shrink_to_content(arena);
}

template<castable<EntitySlot> E> auto gather(Arena& arena, Array<E> entities, u64 flags) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	auto list = List{ arena.push_array<E*>(entities.size()), 0 };
	for (auto& i : entities) if (has_all(i.flags, flags))
		list.push(&i);
	return list.shrink_to_content(arena);
}

#endif
