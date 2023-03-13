#ifndef GBUFFER
# define GBUFFER

#include <glutils.cpp>
#include <blblstd.hpp>

template<typename T> struct MappedObject {
	GLuint id;
	T& obj;
};

template<typename T> MappedObject<T> map_object(const T& obj) {
	T* ptr = nullptr;
	auto id = create_buffer_single(obj, &ptr);
	return MappedObject { id, *ptr };
}

template<typename T> struct MappedBuffer {
	GLuint id;
	Array<T> obj;
};

template<typename T> MappedBuffer<T> map_buffer(Array<T> data) {
	auto arr = Array<T>();
	auto id = create_buffer_array(data, &arr);
	return MappedBuffer { id, arr };
}

template<typename T> MappedBuffer<T> map_buffer(GLsizeiptr size) {
	auto data = Array<byte>();
	auto id = create_buffer(size * sizeof(T), &data);
	return MappedBuffer { id, cast<T>(data) };
}

#endif
