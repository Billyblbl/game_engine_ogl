#ifndef GENTITY
# define GENTITY

#include <blblstd.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <high_order.cpp>
#include <utils.cpp>

#define MAX_ENTITIES 100
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
		PendingRelease = 1 << 1,
		UserFlag = 1 << 2,
		FlagIndexCount = entity_flag_count
	};
	u64 flags = 0;
	u64 generation = 0;
	string name = "__entity__";
	bool available() const { return !has_one(flags, AllocatedEntity | PendingRelease); }
	void grab() { flags |= AllocatedEntity; }
	void discard() { flags = (flags & ~AllocatedEntity) | PendingRelease; }
	void recycle() { flags = 0; generation++; }
	template<typename T> inline T& content() { return static_cast<T&>(*this); }
};

template<typename T> concept EntityLayout = castable<T, EntitySlot>;
using EntityHandle = genhandle<EntitySlot, &EntitySlot::generation>;

template<EntityLayout E> inline E& allocate_entity(List<E>& entities, string name = "__entity__", u64 flags = 0) {
	auto i = linear_search(entities.allocated(), [](const E& ent){ return ent.available(); });
	auto& slot = i < 0 ? entities.push({}) : entities.allocated()[i];
	slot.grab();
	slot.name = name;
	slot.flags |= flags;
	return slot;
}

inline EntityHandle get_entity_genhandle(EntitySlot& ent) {
	return get_genhandle<EntitySlot, &EntitySlot::generation>(ent);
}

template<castable<EntitySlot> E, typename F = u64> auto gather(Alloc allocator, Array<E> entities, F flags, auto mapper) {
	using R = decltype(mapper(entities[0]));
	auto list = List{ alloc_array<R>(allocator, entities.size()), 0 };
	for (auto&& i : entities) if (has_all(i.flags, flags))
		list.push(mapper(i));
	// avoids shrink to empty array situation which might break some allocators
	//TODO FIXTHIS(202307231757) -> shrink to content shouldn't break anything even if it reduces to 0 which would dealloc in most allocators
	if (list.current > 0) {
		list.shrink_to_content(allocator);
	}
	return list.allocated();
}

#endif
