#ifndef GBUFFER
# define GBUFFER

#include <span>
#include <glutils.cpp>

template<typename T> struct MappedObject {
	GLuint id;
	T& obj;
};

template<typename T> MappedObject<T> mapObject(const T& obj) {
	T* ptr = nullptr;
	auto id = createBufferSingle(obj, &ptr);
	return MappedObject { id, *ptr };
}

template<typename T> struct MappedBuffer {
	GLuint id;
	std::span<T> obj;
};

template<typename T> MappedBuffer<T> mapBuffer(std::span<T> data) {
	auto span = std::span<T>();
	auto id = createBufferSpan(data, &span);
	return MappedBuffer { id, span };
}

template<typename T> MappedBuffer<T> mapBuffer(GLsizeiptr size) {
	auto data = std::span<std::byte>();
	auto id = createBuffer(size * sizeof(T), &data);
	return MappedBuffer { id, spanCast<T>(data) };
}

#endif
