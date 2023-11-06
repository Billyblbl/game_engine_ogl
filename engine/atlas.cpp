#ifndef GATLAS
# define GATLAS

#include <textures.cpp>
#include <math.cpp>
#include <image.cpp>
#include <animation.cpp>

//TODO explore use of this
//* https://stackoverflow.com/questions/17152340/when-to-use-texture-views
//* especially the ability to use a tecture view as a 2d texture referencing an array of 2D textures

struct Atlas2D {
	TexBuffer texture;
	v2u32 current = v2u32(0);
	u32 next_line = 0;

	rtu32 push(Image img) {
		auto available = rtu32{ current, texture.dimensions };
		auto rect = rtu32{ current, current + v2u32(img.dimensions) };
		if (!contains(available, rect)) {
			current = v2u32(0, next_line);
			available = rtu32{ current, texture.dimensions };
			rect = rtu32{ current, current + v2u32(img.dimensions) };
			assert(contains(available, rect));
		}
		upload_texture_data(texture, img.data, img.format, slice_to_area<2>(rect, 0));
		next_line = max(next_line, current.y + height(rect));
		current.x += width(rect);
		return rect;
	}

	rtu32 load(const cstr path) {
		auto img = load_image(path); defer{ unload(img); };
		return push(img);
	}

	void reset() {
		current = v2u32(0);
		next_line = 0;
	}

	static Atlas2D create(v2u32 dimensions, GPUFormat format = RGBA32F) {
		Atlas2D atlas;
		atlas.texture = create_texture(TX2D, v4u32(dimensions, 1, 1), format);
		atlas.current = v2u32(0);
		atlas.next_line = 0;
		atlas.texture.conf_sampling({ Nearest, Nearest });
		return atlas;
	}

	void release() {
		unload(texture);
		reset();
	}

};


#endif
