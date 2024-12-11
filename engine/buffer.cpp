#ifndef GBUFFER
# define GBUFFER

#include <glutils.cpp>
#include <blblstd.hpp>

struct GPUBuffer {
	GLuint id;
	u64 size;
	GLbitfield flags;

	static GPUBuffer create(u64 size, GLbitfield flags = 0, ROBuffer initial_data = {}) {
		GPUBuffer buff;
		buff.size = size;
		buff.flags = flags;
		GL_GUARD(glCreateBuffers(1, &buff.id));
		auto initial_ptr = initial_data.size() > 0 ? initial_data.data() : null;
		GL_GUARD(glNamedBufferStorage(buff.id, buff.size, initial_ptr, buff.flags));
		return buff;
	}

	void release() { GL_GUARD(glDeleteBuffers(1, &id)); }

	GPUBuffer& resize(u64 new_size) {
		//* We copy into a temp and back instead of replacing like ArrayList in order to conserve links between ogl objects like VAOs
		GLuint temp;
		GL_GUARD(glCreateBuffers(1, &temp));
		GL_GUARD(glNamedBufferStorage(temp, size, null, 0));
		GL_GUARD(glCopyNamedBufferSubData(id, temp, 0, 0, size));
		GL_GUARD(glNamedBufferStorage(id, new_size, null, flags));
		GL_GUARD(glCopyNamedBufferSubData(temp, id, 0, 0, size));
		GL_GUARD(glDeleteBuffers(1, &temp));
		size = new_size;
		return *this;
	}

	num_range<u64> write(ROBuffer buff, u64 offset = 0) {
		GL_GUARD(glNamedBufferSubData(id, offset, buff.size_bytes(), buff.data()));
		return { offset, offset + buff.size_bytes() };
	}

	Buffer map(num_range<u64> range = {}, GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT) {
		if (range.size() == 0)
			range = { 0, u64(size) };
		auto mapping = GL_GUARD(glMapNamedBufferRange(id, range.min, range.size(), access));
		return carray((byte*)mapping, range.size());
	}

	void unmap(num_range<u64> flush_range = {}) {
		if (flush_range.size() > 0)
			flush(flush_range);
		GL_GUARD(glUnmapNamedBuffer(id));
	}

	num_range<u64> sync(ROBuffer buff, u64 offset = 0) {
		auto r = write(buff, offset);
		flush(r);
		return r;
	}

	void flush(num_range<u64> range = {}) const {
		if (range.size() == 0)
			range = { 0, u64(size) };
		GL_GUARD(glFlushMappedNamedBufferRange(id, range.min, range.size()));
	}

	void bind(GLuint target) const { GL_GUARD(glBindBuffer(target, id)); }
	void unbind(GLuint target) const { GL_GUARD(glBindBuffer(target, 0)); }

	void bind(GLuint target, GLuint index, num_range<u64> range = {}) const {
		if (range.size() == 0)
			range = { 0, u64(size) };
		GL_GUARD(glBindBufferRange(target, index, id, range.min, range.size()));
	}

	void unbind(GLuint target, GLuint index) const { GL_GUARD(glBindBufferRange(target, index, 0, 0, 0)); }
};

#endif
