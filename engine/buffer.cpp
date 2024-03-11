#ifndef GBUFFER
# define GBUFFER

#include <glutils.cpp>
#include <blblstd.hpp>

struct GPUBuffer {
	GLuint id;
	u64 size;
	GLbitfield flags;

	static GPUBuffer create(u64 size, GLbitfield flags = 0, Buffer initial_data = {}) {
		GPUBuffer buff;
		buff.size = size;
		buff.flags = flags;
		GL_GUARD(glCreateBuffers(1, &buff.id));
		auto initial_ptr = initial_data.data() ? initial_data.data() : null;
		GL_GUARD(glNamedBufferStorage(buff.id, buff.size, initial_ptr, buff.flags));
		return buff;
	}

	void release() { GL_GUARD(glDeleteBuffers(1, &id)); }

	GPUBuffer& resize(u64 new_size) {
		GL_GUARD(glNamedBufferStorage(id, new_size, null, flags));
		size = new_size;
		return *this;
	}

	num_range<u64> write(Buffer buff, u64 offset = 0) {
		GL_GUARD(glNamedBufferSubData(id, offset, buff.size_bytes(), buff.data()));
		return { offset, offset + buff.size_bytes() };
	}

	Buffer map(num_range<u64> range = {}) {
		if (range.size() == 0)
			range = { 0, u64(size) };
		auto mapping = GL_GUARD(glMapNamedBufferRange(id, range.min, range.size(), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));
		return carray((byte*)mapping, range.size());
	}

	void unmap() { GL_GUARD(glUnmapNamedBuffer(id)); }

	num_range<u64> sync(Buffer buff, u64 offset = 0) {
		auto r = write(buff, offset);
		flush(r);
		return r;
	}

	void flush(num_range<u64> range = {}) const {
		if (range.size() == 0)
			range = { 0, u64(size) };
		GL_GUARD(glFlushMappedNamedBufferRange(id, range.min, range.size()));
	}
};

#endif
