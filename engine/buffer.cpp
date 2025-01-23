#ifndef GBUFFER
# define GBUFFER

#include <glutils.cpp>
#include <glresource.cpp>
#include <blblstd.hpp>

struct GPUBuffer {
	u64 size;
	u64 content;
	GLuint id;
	GLbitfield flags;

	static constexpr u32 STRETCHY_FLAG = ~(0
		| GL_STREAM_DRAW | GL_STREAM_READ | GL_STREAM_COPY | GL_STATIC_DRAW | GL_STATIC_READ | GL_STATIC_COPY | GL_DYNAMIC_DRAW | GL_DYNAMIC_READ | GL_DYNAMIC_COPY// All BufferData usages
		| GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT// All BufferStorage flags
	);
	GLenum stretchy_usage() const { return (GLenum)(flags & ~STRETCHY_FLAG); }
	bool stretchy() const { return (flags & STRETCHY_FLAG) == STRETCHY_FLAG; }

	static GPUBuffer create(GLScope& ctx, u64 size, GLbitfield flags = 0, ROBuffer initial_data = {}) {
		GLuint id;
		GL_GUARD(glCreateBuffers(1, &id));
		ctx.push<&GLScope::buffers>(id);
		auto initial_ptr = initial_data.size() > 0 ? initial_data.data() : null;
		GL_GUARD(glNamedBufferStorage(id, size, initial_ptr, flags));
		return {
			.size = size,
			.content = initial_data.size(),
			.id = id,
			.flags = flags
		};
	}

	template<typename T> static GPUBuffer upload(GLScope& ctx, Array<T> data, GLbitfield flags = 0) {
		return create(ctx, data.size_bytes(), flags, cast<byte>(data));
	}

	static GPUBuffer create_stretchy(GLScope& ctx, u64 size, GLenum usage = 0, ROBuffer initial_data = {}) {
		GLuint id;
		GL_GUARD(glCreateBuffers(1, &id));
		ctx.push<&GLScope::buffers>(id);
		auto initial_ptr = initial_data.size() > 0 ? initial_data.data() : null;
		GL_GUARD(glNamedBufferData(id, size, initial_ptr, usage));
		return {
			.size = size,
			.content = initial_data.size(),
			.id = id,
			.flags = usage | STRETCHY_FLAG
		};
	}

	GPUBuffer& resize(u64 new_size) {
		assert(stretchy() && "Cannot resize immutable GPU buffer");
		GLuint temp;
		//* We copy into a temp and back instead of replacing like ArrayList in order to conserve links between ogl objects like VAOs
		if (content) {
			GL_GUARD(glCreateBuffers(1, &temp));
			GL_GUARD(glNamedBufferStorage(temp, content, null, 0));
			GL_GUARD(glCopyNamedBufferSubData(id, temp, 0, 0, content));
		}
		GL_GUARD(glNamedBufferData(id, new_size, null, stretchy_usage()));
		if (content) {
			GL_GUARD(glCopyNamedBufferSubData(temp, id, 0, 0, content));
			GL_GUARD(glDeleteBuffers(1, &temp));
		}
		size = new_size;
		assert(size >= content);
		return *this;
	}

	template<typename T> GPUBuffer& resize_as(u64 count) { return resize(count * sizeof(T)); }

	num_range<u64> write(ROBuffer buff, u64 offset = 0) {
		GL_GUARD(glNamedBufferSubData(id, offset, buff.size_bytes(), buff.data()));
		return { offset, offset + buff.size_bytes() };
	}

	template<typename T> num_range<u64> write_as(Array<T> buff, u64 offset = 0) {
		auto range = write(carray((byte*)buff.data(), buff.size_bytes()), offset * sizeof(T));
		return { range.min / sizeof(T), range.max / sizeof(T) };
	}

	template<typename T> num_range<u64> write_one(T& obj, u64 offset = 0) {
		return write_as<const T>(carray(&obj, 1), offset);
	}

	num_range<u64> allocate(u64 allocation) {
		auto needed_size = content + allocation;
		if (needed_size > size)
			resize(round_up_bit(needed_size));
		num_range<u64> r = { content, content + allocation };
		content += allocation;
		return r;
	}

	template<typename T> num_range<u64> allocate_as(u64 count) { return allocate(count * sizeof(T)); }

	num_range<u64> push(ROBuffer buff) {
		auto r = allocate(buff.size_bytes());
		assert(size >= content);
		r = write(buff, r.min);
		return r;
	}

	template<typename T> num_range<u64> push_as(Array<T> buff) { return push(cast<byte>(buff)); }
	template<typename T> num_range<u64> push_one(const T& obj) { return push_as<const T>(carray(&obj, 1)); }


	Buffer map(num_range<u64> range = {}, GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT) {
		if (range.size() == 0)
			range = { 0, u64(size) };
		if (range.max > u64(size)) {
			assert(stretchy() && "Attempt to map past end of non-stretchy buffer");
			resize(range.max + 1);
		}

		auto mapping = GL_GUARD(glMapNamedBufferRange(id, range.min, range.size(), access));
		return carray((byte*)mapping, range.size());
	}

	template<typename T> Array<T> map_as(num_range<u64> range = {}, GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT) {
		return cast<T>(map({range.min * sizeof(T), range.max * sizeof(T)}, access));
	}

	template<typename T> u64 content_as() const { return content / sizeof(T); }

	void unmap(num_range<u64> flush_range = {}) {
		if (flush_range.size() > 0)
			flush(flush_range);
		GL_GUARD(glUnmapNamedBuffer(id));
	}

	template<typename T> void unmap_as(num_range<u64> flush_range = {}) {
		unmap({flush_range.min * sizeof(T), flush_range.size() * sizeof(T)});
	}

	num_range<u64> sync(ROBuffer buff, u64 offset = 0) {
		auto r = write(buff, offset);
		flush(r);
		return r;
	}

	void flush(num_range<u64> range = {}) {
		if (range.size() == 0)
			range = { 0, u64(size) };
		content = max(content, range.max);
		GL_GUARD(glFlushMappedNamedBufferRange(id, range.min, range.size()));
	}

	template<typename T> void flush_as(num_range<u64> range = {}) {
		flush({range.min * sizeof(T), range.size() * sizeof(T)});
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
