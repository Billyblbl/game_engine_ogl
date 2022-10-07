#ifndef GPOOL
# define GPOOL

#include <span>
#include <stdint.h>

template<typename T> auto RemoveInvalidateAt(std::span<T> content, uint64_t index) {
	content[index] = content[content.size() - 1];
	return content.subspan(0, content.size() - 1);
}

template<typename T> struct Pool {
	std::span<T>	buffer;
	uint64_t	count = 0;

	int32_t indexOf(const T& element) { return &element - buffer.data(); }
	T& add(T&& element = T{}) { return buffer[count++] = element; }
	void removeInvalidateAt(uint64_t index) { count = RemoveInvalidateAt(allocated(), index).size(); }
	std::span<T> allocated() { return buffer.subspan(0, count); }
	T& operator[](uint64_t i) { return buffer[i]; }
	bool full() { return count == buffer.size(); }
};

struct Arena {
	Pool<std::byte> bytes;

	auto allocate(uint32_t size) {
		auto start = bytes.count;
		bytes.count += size;
		return bytes.allocated().subspan(start, size);
	}

	template<typename T> auto allocate(uint32_t count) {
		return std::span((T*)allocate(count * sizeof(T)).data(), count);
	}

	bool tryPop(std::span<std::byte> buffer) {
		if (buffer.end() == bytes.allocated().end()) {
			bytes.count -= buffer.size();
			return true;
		} else {
			return false;
		}
	}

};

struct FreeListArena {
	Arena suballocator;
	Pool<std::span<std::byte>> freeBuffers;

	static auto create(Arena& allocator, uint64_t capacity, uint32_t freelistCapacity) {
		return FreeListArena {
			Arena { { allocator.allocate(capacity), 0 } },
			{ allocator.allocate<std::span<std::byte>>(freelistCapacity), 0 }
		};
	}

	auto allocate(uint32_t size) {
		for (auto&& buffer : freeBuffers.allocated()) {
			if (buffer.size() >= size) {
				auto newBuffer = buffer.subspan(0, size);
				auto rest = buffer.subspan(size, buffer.size());
				if (rest.size() > 0)
					buffer = rest;
				else
					freeBuffers.removeInvalidateAt(freeBuffers.indexOf(buffer));
				return newBuffer;
			}
		}
		return freeBuffers.add(suballocator.allocate(size));
	}

	template<typename T> auto allocate(uint32_t count) {
		return std::span((T*)allocate(count * sizeof(T)).data(), count);
	}

	void deallocate(std::span<std::byte> buffer) {
		//Try to directly pop the buffer from the suballocator
		if (suballocator.tryPop(buffer)) return;

		//Try to merge into a previous free buffer
		for (auto&& freeBuffer : freeBuffers.allocated()) {
			auto newFreeBuffer = std::span<std::byte>();
			if (freeBuffer.end() == buffer.begin()) {
				newFreeBuffer = std::span(freeBuffer.data(), freeBuffer.size() + buffer.size());
			} else if (buffer.end() == freeBuffer.begin()) {
				newFreeBuffer = std::span(buffer.data(), buffer.size() + freeBuffer.size());
			}
			if (newFreeBuffer.size() > 0) {
				// Try to pop the new buffer from the suballocator
				if (!suballocator.tryPop(newFreeBuffer))
					freeBuffer = newFreeBuffer;
				return;
			}
		}

		// If all failed, just add to freelist
		freeBuffers.add(std::move(buffer));
	}

};

template<typename ID = uint32_t>
struct IDRegistry {
	Pool<ID> unusedIds;
	ID next = 0;

	static auto create(Arena& allocator, uint32_t capacity, ID startingID = 0) {
		return IDRegistry{ Pool<ID> { allocator.allocate<ID>(capacity - 1), 0 }, startingID };
	}

	auto allocate() {
		if (unusedIds.count > 0) {
			ID id = unusedIds[unusedIds.count - 1];
			unusedIds.removeInvalidateAt(unusedIds.count - 1);
			return id;
		} else {
			return next++;
		}
	}

	void deallocate(ID id) {
		if (next - 1 == id) {
			next--;
		} else {
			unusedIds.add(id);
		}
	}

	void reclaim() {
		for (int i = unusedIds.count - 1; i > -1; i--) {
			if (unusedIds[i] == next - 1) {
				unusedIds.removeInvalidateAt(i);
				next--;
			}
		}
	}
};

using EntityRegistry = IDRegistry<>;

template<typename T, typename ID = uint32_t> struct LinearDatabase {
	std::span<ID> ids;
	Pool<T>	pool;

	static auto create(Arena& arena, uint32_t capacity) {
		return LinearDatabase<T, ID> {
			arena.allocate<ID>(capacity),
				Pool<T> { arena.allocate<T>(capacity), 0 }
		};
	}

	int32_t indexOf(ID id) {
		for (auto i = 0; i < pool.count; i++) if (ids[i] == id) {
			return i;
		}
		return -1;
	}

	int32_t indexOf(const T& element) { return pool.indexOf(element); }

	T* find(ID id) {
		auto index = indexOf(id);
		if (index < 0)
			return nullptr;
		else
			return &pool[index];
	}

	ID idOf(const T& element) { return ids[indexOf(element)]; }

	auto& add(ID id, T&& element = {}) {
		auto& placed = pool.add(std::forward<T>(element));
		ids[pool.count - 1] = id;
		return placed;
	}

	void remove(ID id) {
		auto index = indexOf(id);
		if (index >= 0) {
			pool.removeInvalidateAt(index);
			RemoveInvalidateAt(ids, index);
		}
	}

};

#endif
