#ifndef GENTITY
# define GENTITY

#include <blblstd.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <transform.cpp>
#include <rendering.cpp>
#include <physics_2d.cpp>
#include <sprite.cpp>
#include <top_down_controls.cpp>
#include <audio.cpp>

#define MAX_ENTITIES 100
#define MAX_DRAW_BATCH MAX_ENTITIES

constexpr auto flag_index_count = 32;

struct EntityDescriptor {
	u32 generation;
	u32 flags;
	string name;
	i16 index_hints[flag_index_count];
};

struct EntityRegistry {
	List<EntityDescriptor> pool;
	List<string> flag_names;
	enum : u8 {
		AllocatedEntity = 0,
		PendingRelease = 1,
		//runtime defined indices 2->flag_index_count-1
		FlagIndexCount = flag_index_count
	};
};

struct EntityHandle {
	EntityDescriptor* desc;
	u32 generation;
	inline bool valid() {
		return
			desc != null &&
			desc->generation <= generation &&
			(desc->flags & mask<u32>(EntityRegistry::AllocatedEntity)) &&
			!(desc->flags & mask<u32>(EntityRegistry::PendingRelease)) &&
			true;
	}
};

EntityRegistry create_entity_registry(Alloc allocator, u32 capacity) {
	EntityRegistry registry;
	registry.pool = List{ alloc_array<EntityDescriptor>(allocator, capacity), 0 };
	registry.flag_names = List{ alloc_array<string>(allocator, EntityRegistry::FlagIndexCount), 0 };
	// Base flags
	registry.flag_names.insert(EntityRegistry::AllocatedEntity, "Allocated Entity");
	registry.flag_names.insert(EntityRegistry::PendingRelease, "Pending Release");
	return registry;
}

void delete_registry(Alloc allocator, EntityRegistry& registry) {
	dealloc_array(allocator, registry.flag_names.capacity);
	dealloc_array(allocator, registry.pool.capacity);
	registry.flag_names = { {}, 0 };
	registry.pool = { {}, 0 };
}

inline EntityHandle allocate_entity(EntityRegistry& registry, string name = "__entity__", u64 flags = 0) {
	EntityDescriptor* slot = null;
	for (auto& ent : registry.pool.allocated()) if (ent.flags == 0) {
		slot = &ent;
		break;
	}
	if (slot == null)
		slot = &registry.pool.push(EntityDescriptor{ 0 });
	slot->generation++;
	slot->name = name;
	slot->flags = flags | mask<u64>(EntityRegistry::AllocatedEntity);
	return { slot, slot->generation };
}

void clear_pending_releases(EntityRegistry& registry) {
	for (auto& ent : registry.pool.allocated()) if (has_all(ent.flags, mask<u64>(EntityRegistry::AllocatedEntity, EntityRegistry::PendingRelease))) {
		ent.flags &= ~mask<u64>(EntityRegistry::AllocatedEntity);
	}
}

template<typename T> struct ComponentRegistry {
	List<T> buffer;
	List<EntityHandle> handles;
	u8 flag_index;

	auto iter() { return parallel_iter(handles.allocated(), buffer.allocated()); }

	bool has_flag(EntityHandle ent) {
		return has_all(ent.desc->flags, mask<u32>(flag_index));
	}

	T& add_to(EntityHandle ent, T&& comp) {
		ent.desc->index_hints[flag_index] = handles.current;
		ent.desc->flags |= mask<u32>(flag_index);
		handles.push(ent);
		return buffer.push(comp);
	}

	i64 index_of(EntityHandle ent) {
		if (!ent.valid())
			return -1;
		return ent.desc->index_hints[flag_index] = linear_search(handles.allocated(),
			[&](EntityHandle h) { return h.desc == ent.desc; },
			ent.desc->index_hints[flag_index]);
	}

	void remove_from(EntityHandle ent) {
		auto index = index_of(ent);
		if (index < 0)
			return fail_ret("Failed to remove component : Not found", void{});
		ent.desc->flags &= ~mask<u32>(flag_index);
		buffer.remove(index);
		handles.remove(index);
	}

	T* operator[](EntityHandle ent) {
		auto index = index_of(ent);
		if (index < 0)
			return null;
		else
			return &buffer.allocated()[index];
	}

	void clear_stale() {
		for (isize i = handles.current - 1; i >= 0; i--) if (!handles.allocated()[i].valid()) {
			buffer.remove(i);
			handles.remove(i);
		}
	}
};

template<typename... T> void clear_stale(ComponentRegistry<T>&... components) { (..., components.clear_stale()); }

template<typename T> ComponentRegistry<T> create_component_registry(Alloc allocator, u32 capacity) {
	ComponentRegistry<T> registry;
	registry.buffer = List{ alloc_array<T>(allocator, capacity), 0 };
	registry.handles = List{ alloc_array<EntityHandle>(allocator, capacity), 0 };
	registry.flag_index = -1;
	return registry;
}

template<typename T> void delete_registry(Alloc allocator, ComponentRegistry<T>& registry) {
	dealloc_array<T>(allocator, registry.buffer.capacity);
	dealloc_array<EntityHandle>(allocator, registry.handles.capacity);
	registry.buffer = {};
	registry.handles = { {}, 0 };
}

u8 register_flag_index(EntityRegistry& entities, string name) {
	defer{ entities.flag_names.push(name); };
	return entities.flag_names.current;
}

template<typename... T> bool entity_registry_window(const cstr title, EntityRegistry& entities, ComponentRegistry<T>&... comp) {
	bool opened = true;
	if (ImGui::Begin(title, &opened)) {
		auto flag_names = entities.flag_names.allocated();
		for (auto& desc : entities.pool.allocated()) {
			EntityHandle ent = { &desc, desc.generation };
			if (ent.valid() && ImGui::TreeNode(desc.name.data())) {
				defer{ ImGui::TreePop(); };
				ImGui::bit_flags("Flags", desc.flags, entities.flag_names.allocated());
				ImGui::Text("Slot Generation : %u", desc.generation);
				(...,
					[&]() {
						if (has_all(desc.flags, mask<u32>(comp.flag_index)) && ImGui::CollapsingHeader(flag_names[comp.flag_index].data()))
							EditorWidget(flag_names[comp.flag_index].data(), *comp[ent]);
					}
				());
			}
		}
	} ImGui::End();
	return opened;
}

#endif
