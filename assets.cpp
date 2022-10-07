#ifndef GASSETS
# define GASSETS

#include <span>
#include <string_view>
#include <pool.cpp>

struct AssetCache {
	FreeListArena scope;
	Pool<std::span<std::byte>> buffers;
	std::span<std::string_view> ids;
	std::span<uint32_t> referenceCounts;

	static auto create(Arena& allocator, uint64_t capacity, uint64_t assetMemoryCapacity) {
		return AssetCache{
			FreeListArena::create(allocator, assetMemoryCapacity, 128),
			{ allocator.allocate<std::span<std::byte>>(capacity), 0 },
			allocator.allocate<std::string_view>(capacity),
			allocator.allocate<uint32_t>(capacity)
		};
	}

	uint32_t& add(std::string_view id, std::span<std::byte> buffer) {
		auto index = buffers.count;
		buffers.add(std::move(buffer));
		ids[index] = id;
		return referenceCounts[index] = 0;
	}

	void removeAt(uint32_t& index) {
		scope.deallocate(buffers[index]);
		buffers[index] = {};
		ids[index] = "";
	}

	auto& store(std::string_view id, auto& element) {
		auto buffer = scope.allocate(sizeof(std::remove_reference_t<decltype(element>)));
		auto& stored = *((decltype(element)*)buffer.data());
		stored = element;
		if (buffers.full()) {
			replaceUnused(id, buffer)++;
		} else {
			add(id, buffer)++;
		}
		return stored;
	}

	uint32_t& replaceUnused(std::string_view id, std::span<std::byte> buffer) {
		for (auto i = 0; i < buffers.count; i++) {
			if (referenceCounts[i] == 0) {
				scope.deallocate(buffers[i]);
				buffers[i] = buffer;
				ids[i] = id;
				return referenceCounts[i];
			}
		}
	}

	uint32_t indexOf(std::string_view id) {
		for (auto i = 0; i < ids.size(); i++) {
			if (ids[i] == id) {
				return i;
			}
		}
		return -1;
	}

	void drop(std::string_view id) { referenceCounts[indexOf(id)]--; }

	template<typename T> T* grab(std::string_view id) {
		referenceCounts[indexOf(id)] += count;
		return (T*)buffers[index].data();
	}

};

#endif
