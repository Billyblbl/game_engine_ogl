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
	Array<T> content;
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

template<typename T> void unmap(MappedBuffer<T>& buffer) {
	unmap(buffer.id, buffer.content.size_bytes());
	delete_buffer(buffer.id);
	buffer.id = 0;
	buffer.content = {};
}

template<typename T> void unmap(MappedObject<T>& obj) {
	unmap(obj.id, sizeof(T));
	delete_buffer(obj.id);
	obj.id = 0;
}

template<typename T> Array<T> sync(MappedBuffer<T> buffer) {
	flush_mapped_buffer(buffer.id, {0, buffer.content.size_bytes()});
	return buffer.content;
}

template<typename T> T& sync(MappedObject<T> obj) {
	flush_mapped_buffer(obj.id, {0, sizeof(T)});
	return obj.obj;
}

#endif
