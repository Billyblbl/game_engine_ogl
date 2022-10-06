#ifndef GPOOL
# define GPOOL

#include <span>
#include <stdint.h>

template<typename T> auto removeInvalidateAt(std::span<T> content, uint64_t index) {
	content[index] = content.last();
	return content.subspan(0, content.size() - 1);
}

template<typename T> struct Pool {
	std::span<T>	buffer;
	uint64_t	count = 0;

	T& add(T&& element = T{}) { return buffer[count++] = element; }
	void removeInvalidateAt(uint64_t index) { count = removeInvalidateAt(allocated(), index).size(); }
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

	int32_t indexOf(const T& element) {
		for (auto i = 0; i < pool.count; i++) if (pool[i] == element) {
			return i;
		}
		return -1;
	}

	T* find(ID id) {
		auto index = indexOf(id);
		if (index < 0)
			return nullptr;
		else
			return &pool[index];
	}

	auto& add(ID id, T&& element = {}) {
		auto& placed = pool.add(std::forward<T>(element));
		ids[pool.count - 1] = id;
		return placed;
	}

	void remove(ID id) {
		auto index = indexOf(id);
		if (index >= 0) {
			pool.removeInvalidateAt(index);
			removeInvalidateAt(ids, index);
		}
	}

};

#endif
