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

	T& add(T&& element = T{}) { return buffer[count++] = element; }
	void removeInvalidateAt(uint64_t index) { count = RemoveInvalidateAt(allocated(), index).size(); }
	std::span<T> allocated() { return buffer.subspan(0, count); }
	T& operator[](uint64_t i) { return buffer[i]; }
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

	int32_t indexOf(const T& element) { return &element - pool.buffer.data(); }

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
