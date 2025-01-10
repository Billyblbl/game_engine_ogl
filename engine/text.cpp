#ifndef GTEXT
# define GTEXT

#include <blblstd.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BBOX_H
#include FT_GLYPH_H

#include <rendering.cpp>
#include <atlas.cpp>


// struct Font;

namespace Text {
	enum Axis : u32 { H = 0, V = 1 };

	struct Style {
		v4f32 color;
		f32 scale;
		f32 linespace;
		Text::Axis axis;
	};

	struct Glyph {
		u64 code = 0;
		u64 gindex = 0;
		v2f32 size = v2f32(0);
		struct {
			v2f32 bearing = v2f32(0);
			f32 advance = 0;
		} orient[2] = {};
		rtu32 sprite = { v2u32(0), v2u32(0) };

		static Glyph create(FT_ULong code, FT_UInt gindex, FT_Face face, rtu32 sprite) {
			return {
				.code = code,
				.gindex = gindex,
				.size = v2f32(face->glyph->metrics.width, face->glyph->metrics.height) / 64.f,
				.orient = {
					{
						.bearing = v2f32(face->glyph->metrics.horiBearingX, face->glyph->metrics.horiBearingY) / 64.f,
						.advance = face->glyph->metrics.horiAdvance / 64.f
					},
					{
						.bearing = v2f32(face->glyph->metrics.vertBearingX, face->glyph->metrics.vertBearingY) / 64.f,
						.advance = face->glyph->metrics.vertAdvance / 64.f
					}
				},
				.sprite = sprite
			};
		}

		rtf32 bbox(Text::Axis ori = Text::H) const {
			rtf32 rt;
			rt.min.x = orient[ori].bearing.x;
			rt.min.y = orient[ori].bearing.y - size.y;
			rt.max.x = orient[ori].bearing.x + size.x;
			rt.max.y = orient[ori].bearing.y;
			return rt;
		}
	};

	Image make_bitmap_image_sdf(Arena& arena, const FT_Bitmap& bitmap) {
		//TODO handle other formats
		if (bitmap.pitch == 0 || bitmap.rows == 0) return make_image<f32>({}, v2u32(0), 1);
		switch (bitmap.pixel_mode) {
		case FT_PIXEL_MODE_NONE: { panic(); } break;
		case FT_PIXEL_MODE_MONO: { panic(); } break;
		case FT_PIXEL_MODE_GRAY: {
			auto true_pitch = abs(bitmap.pitch);
			auto bitmap_buffer = cast<u8>(carray(bitmap.buffer, bitmap.rows * true_pitch));
			auto image = List{ arena.push_array<f32>(bitmap.rows * bitmap.width), 0 };
			for (auto i : u64xrange{ 0, bitmap.rows }) if (bitmap.pitch < 0) for (u8 p : bitmap_buffer.subspan(bitmap_buffer.size() - (1 + i) * true_pitch, bitmap.width))
				image.push(f32(p) / 255.f * 2 - 1.f);
			else for (u8 p : bitmap_buffer.subspan(i* true_pitch, bitmap.width))
				image.push(f32(p) / 255.f * 2 - 1.f);
			return make_image<f32>(image.used(), v2u32(bitmap.width, bitmap.rows), 1);
		} break;
		case FT_PIXEL_MODE_GRAY2: { panic(); } break;
		case FT_PIXEL_MODE_GRAY4: { panic(); } break;
		case FT_PIXEL_MODE_LCD: { panic(); } break;
		case FT_PIXEL_MODE_LCD_V: { panic(); } break;
		case FT_PIXEL_MODE_BGRA: { panic(); } break;
		default: { panic(); };
		}
		panic();
		return {};
	}

	Image make_bitmap_image_alpha(Arena& arena, const FT_Bitmap& bitmap) {
		//TODO handle other formats
		if (bitmap.pitch == 0 || bitmap.rows == 0) return make_image<f32>({}, v2u32(0), 1);
		switch (bitmap.pixel_mode) {
		case FT_PIXEL_MODE_NONE: { panic(); } break;
		case FT_PIXEL_MODE_MONO: { panic(); } break;
		case FT_PIXEL_MODE_GRAY: {
			auto true_pitch = abs(bitmap.pitch);
			auto bitmap_buffer = cast<u8>(carray(bitmap.buffer, bitmap.rows * true_pitch));
			auto image = List{ arena.push_array<f32>(bitmap.rows * bitmap.width), 0 };
			for (auto i : u64xrange{ 0, bitmap.rows }) if (bitmap.pitch < 0) for (u8 p : bitmap_buffer.subspan(bitmap_buffer.size() - (1 + i) * true_pitch, bitmap.width))
				image.push(p / 255.0f);
			else for (u8 p : bitmap_buffer.subspan(i* true_pitch, bitmap.width))
				image.push(p / 255.0f);
			return make_image<f32>(image.used(), v2u32(bitmap.width, bitmap.rows), 1);
		} break;
		case FT_PIXEL_MODE_GRAY2: { panic(); } break;
		case FT_PIXEL_MODE_GRAY4: { panic(); } break;
		case FT_PIXEL_MODE_LCD: { panic(); } break;
		case FT_PIXEL_MODE_LCD_V: { panic(); } break;
		case FT_PIXEL_MODE_BGRA: { panic(); } break;
		default: { panic(); };
		}
		panic();
		return {};
	}

	void pfterror(string file_name, u32 line_number, FT_Error err) {
		fprintf(stderr, "%s:%u, Freetype error %u: %s\n", file_name.data(), line_number, err, FT_Error_String(err));
	}

	FT_Library FT_Global() {
		static FT_Library lib;
		static auto err = FT_Init_FreeType(&lib);
		if (err) {
			pfterror(__FILE__, __LINE__, err);
			abort();
		}
		return lib;
	}

	struct Font {
		// FT_Face face;
		Atlas2D glyph_atlas;
		Array<Glyph> glyphs;
		Array<u32> mappings;
		v2f32 linespace;
		axu32 code_range;

		static constexpr num_range<u32> DEFAULT_PRINTABLE = { 32, 127 };
		static Font load(
			GLScope& ctx,
			FT_Library lib,
			const cstr path,
			u32 index = 0,
			v2u32 font_size = v2u32(64),
			v2u32 atlas_size = v2u32(1024)
		) {
			//TODO replace constants with import parameters
			//TODO fix SDF bitmap mapping (bitmap rect in atlas is different from what metrics indicate)

			printf("Loading Font %s:%u\n", path, index);
			FT_Face face;
			if (auto err = FT_New_Face(lib, path, index, &face))
				return (pfterror(__FILE__, __LINE__, err), Font{});
			defer{ FT_Done_Face(face); };

			if (auto err = FT_Set_Pixel_Sizes(face, font_size.x, font_size.y))
				pfterror(__FILE__, __LINE__, err);

			auto [scratch, scope] = scratch_push_scope(1 << 18); defer{ scratch_pop_scope(scratch, scope); };

			struct Mapping { FT_ULong code; FT_UInt gindex; };
			auto available_glyphs = List{ scratch.push_array<Mapping>(DEFAULT_PRINTABLE.size()), 0 };
			axu32 code_range = { v1u32(9999), v1u32(0) };
			{ //* fetch available glyphs
				FT_UInt gindex;
				for (auto c = FT_Get_First_Char(face, &gindex); gindex != 0; c = FT_Get_Next_Char(face, c, &gindex)) {
					available_glyphs.push_growing(scratch, { c, gindex });
					code_range = combined_aabb(code_range, axu32{ v1u32(c), v1u32(c) });
				}
			}

			auto mipmaps = 4;
			auto glyph_atlas = Atlas2D::create(GLScope::global(), atlas_size, R32F, mipmaps);
			glyph_atlas.texture
				.conf_border_color(v4f32(1.0, 0, 1.0, 1.0))
				.conf_wrap({ ClampToBorder, ClampToBorder, ClampToBorder })
				.conf_sampling({ LinearMipmapLinear, Linear })
				.conf_max_sample_count(8);

			auto mappings = ctx.arena.push_array<u32>(code_range.size().x, true);
			auto glyphs = List{ ctx.arena.push_array<Glyph>(available_glyphs.current + 1), 0 };
			glyphs.push(Glyph{});

			//* load glyphs
			for (auto [c, gindex] : available_glyphs.used()) if (u32 err = 0;
				(err = FT_Load_Glyph(face, gindex, 0)) ||
				(err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))
				// (err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_SDF))
				) {
				pfterror(__FILE__, __LINE__, err);
			} else {
				mappings[c - code_range.min.x] = glyphs.current;
				glyphs.push(Glyph::create(c, gindex, face,
					glyph_atlas.push(make_bitmap_image_alpha(scratch, face->glyph->bitmap), v2u32(mipmaps * 2))
				));
			}

			glyph_atlas.texture.generate_mipmaps(); //* auto generation since the manual lower res font is broken (cf below)

			return {
				.glyph_atlas = glyph_atlas,
				.glyphs = glyphs.used(),
				.mappings = mappings,
				.linespace = v2f32(face->size->metrics.height / 64.f, face->size->metrics.max_advance / 64.f),
				.code_range = code_range
			};
			//TODO fix this so that it can be usable (offset problem with bitmap of the same glyph being different)
			// {//* generate lower res glyphs in mipmaps
			// 	u32 div = 1;
			// 	for (auto i : u64xrange{ 1, mipmaps }) {
			// 		div *= 2;
			// 		if (auto err = FT_Set_Pixel_Sizes(face, font_size.x / div, font_size.y / div)) {
			// 			pfterror(__FILE__, __LINE__, err);
			// 			continue;
			// 		}
			// 		for (auto [c, gindex] : available_glyphs.used()) if (u32 err = 0;
			// 			(err = FT_Load_Glyph(face, gindex, 0)) ||
			// 			(err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))
			// 			) {
			// 			pfterror(__FILE__, __LINE__, err);
			// 		} else {
			// 			auto atlas_view_index = gindex - f.glyph_indices.min.x;
			// 			auto base_rect = f.glyph_views[atlas_view_index];
			// 			f32 buffer[face->glyph->bitmap.rows * face->glyph->bitmap.pitch];
			// 			auto scratch = Arena::from_array(carray(buffer, face->glyph->bitmap.rows * face->glyph->bitmap.pitch));
			// 			auto img = make_bitmap_image_alpha(scratch, face->glyph->bitmap);
			// 			auto rect = rtu32{ base_rect.min / div, base_rect.min / div + img.dimensions };
			// 			upload_texture_data(f.glyph_atlas.texture, img.data, img.format, slice_to_area<2>(rect, 0), i);
			// 		}
			// 	}
			// }

			// return f;
		}

		u32 glyph_index_of(u64 character_code) const {
			assert(code_range.min.x <= character_code && character_code <= code_range.max.x);
			return mappings[character_code - code_range.min.x];
		}

		Glyph& operator[](u64 character_code) { return glyphs[glyph_index_of(character_code)]; }
		const Glyph& operator[](u64 character_code) const { return glyphs[glyph_index_of(character_code)]; }
	};
}

bool EditorWidget(const cstr label, Text::Style& style) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("color", style.color);
		changed |= EditorWidget("scale", style.scale);
		changed |= EditorWidget("axis", style.axis);
		changed |= EditorWidget("linespace", style.linespace);
	}
	return changed;
}

bool EditorWidget(const cstr label, Text::Font& font) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("glyphs", font.glyphs);
		changed |= EditorWidget("glyph_atlas", font.glyph_atlas);
		changed |= EditorWidget("code_range", font.code_range);
		changed |= EditorWidget("linespace", font.linespace);
		changed |= EditorWidget("mappings", font.mappings);
	}
	return changed;
}

namespace UI {

	struct Scene {
		m4x4f32 canvas_projection;
		f32 alpha_discard;
		byte padding[12];
	};

	struct Quad {
		rtf32 rect;
		rtu32 sprite;
	};

	struct alignas(16) Sheet {
		v4f32 tint;
		u32 texture_id;
		f32 depth;
	};

	Array<const Quad> layout_text(Arena& arena, string str, rtf32 rect, const Text::Font& font, const Text::Style& style) {
		auto quads = List{ arena.push_array<Quad>(str.size()), 0 };

		//TODO fix starting point
		auto cursor = rect.min;
		cursor[!style.axis] += dim_vec(rect)[!style.axis] - style.scale * style.linespace * font.linespace[style.axis]; //* starting point

		auto advance = [&](v2f32 c, const Text::Glyph& g) -> v2f32 {
			c[style.axis] += g.orient[style.axis].advance * style.scale;
			return c;
			};

		auto next_line = [&](v2f32 c) -> v2f32 {
			c[style.axis] = rect.min[style.axis];
			c[!style.axis] -= style.scale * style.linespace * font.linespace[style.axis];
			return c;
			};

		auto new_word = [&](v2f32 c, u32 i) -> v2f32 {
			auto end_of_word = str.find_first_of("- \n\r\t\v\f\0", i);
			auto word = str.substr(i, end_of_word - i);
			auto acc_advance = [&](v2f32 a, char b) -> v2f32 { a[style.axis] += font[b].orient[style.axis].advance * style.scale; return a; };
			auto word_advance = fold(v2f32(0.f), Array<const char>(word), acc_advance);
			if (!rect.contain(c + word_advance))
				c = next_line(c);
			return c;
			};

		auto make_character_quad = [&](v2f32 c, const Text::Glyph& g) -> Quad {
			auto bbox = g.bbox(style.axis);
			bbox.min *= style.scale;
			bbox.max *= style.scale;
			return {
				.rect = {.min = c + bbox.min, .max = c + bbox.max },
				.sprite = g.sprite
			};
			};

		auto write_whitespace = [&](v2f32 c, const Text::Glyph& g, u32 i) -> v2f32 {
			c = advance(c, g);
			if (!rect.contain(c))
				c = next_line(c);
			c = new_word(c, i);
			return c;
			};

		auto write_character = [&](v2f32 c, const Text::Glyph& g) -> v2f32 {
			auto quad = make_character_quad(c, g);
			c = advance(c, g);
			if (!rect.contain(c)) {
				c = next_line(c);
				quad = make_character_quad(c, g);
				c = advance(c, g);
				if (!rect.contain(c))
					return c;
			}
			quads.push(quad);
			return c;
			};

		for (auto i : u64xrange{ 0, str.size() }) {
			if (!rect.contain(cursor))
				break;//* early break if text does not fit
			auto& glyph = font[str[i]];
			switch (str[i]) {
			case ' ': { cursor = write_whitespace(cursor, glyph, i + 1); } break;
			case '\n': { cursor = next_line(cursor); } break;
			case '\r': { cursor = new_word(cursor, i + 1); } break;
			case '\t': { cursor = new_word(cursor, i + 1); } break;//TODO implement
			case '\v': { cursor = new_word(cursor, i + 1); } break;//TODO implement
			case '\f': { cursor = new_word(cursor, i + 1); } break;//TODO implement
			default: { cursor = write_character(cursor, glyph); } break;
			}
		}

		return quads.used();
	}

	struct Batch {
		Arena* arena;
		List<Sheet> sheets;
		List<num_range<u32>> mappings;
		List<Quad> quads;
		List<GLuint> textures;
		Scene scene;

		static Batch start(Arena& arena, u64 max_sheets, u64 max_quads, const Scene& scene) {
			auto textures = List{ arena.push_array<GLuint>(get_max_textures_frag()), 0 };
			textures.push(TexBuffer::white().id);
			return {
				.arena = &arena,
				.sheets = { arena.push_array<Sheet>(max_sheets), 0 },
				.mappings = { arena.push_array<num_range<u32>>(max_sheets), 0 },
				.quads = { arena.push_array<Quad>(max_quads), 0 },
				.textures = textures,
				.scene = scene
			};
		}

		u32 next_sheet(const Sheet& sheet, Array<const Quad> sheet_quads = {}) {
			auto mindex = mappings.current;
			u32 qindex = quads.current;
			sheets.push_growing(*arena, sheet);
			mappings.push_growing(*arena, { qindex, qindex + u32(sheet_quads.size()) });
			quads.push_growing(*arena, sheet_quads);
			return mindex;
		}

		u32 push_texture(GLuint id) {
			u32 index = textures.current;
			textures.push(id);
			return index;
		}

		u32 push_quad(const Quad& quad) {
			assert(mappings.current > 0);
			quads.push_growing(*arena, quad);
			return mappings[mappings.current - 1].max++;
		}

		u32 push_lone_quad(const Quad& quad, const Sheet& sheet) {
			u32 index = quads.current;
			next_sheet(sheet, carray(&quad, 1));
			return index;
		}

		u32 push_text(string str, rtf32 rect, f32 depth, const Text::Font& font, const Text::Style& style) {
			auto idx = index_of(textures.used(), [&](GLuint id) { return id == font.glyph_atlas.texture.id; });
			if (idx < 0)
				idx = push_texture(font.glyph_atlas.texture.id);
			auto [scratch, scope] = scratch_push_scope(str.size() * sizeof(Quad), arena); defer{ scratch_pop_scope(scratch, scope); };

			return next_sheet(Sheet{
				.tint = style.color,
				.texture_id = u32(idx),
				.depth = depth
				}, layout_text(scratch, str, rect, font, style));
		}

	};

	struct Renderer {
		VertexArray vao;
		GPUBuffer ibo;
		GPUBuffer quads;
		GPUBuffer commands;
		GPUBuffer sheets;
		GPUBuffer scene;
		List<GLuint> textures;

		u32 push_texture(GLuint id) {
			u32 index = textures.current;
			textures.push(id);
			return index;
		}

		Renderer& apply_batch(const Batch& batch) {
			PROFILE_SCOPE(__PRETTY_FUNCTION__);
			reset();
			auto scratch_size = sizeof(DrawCommandElement) * batch.sheets.current + sizeof(u64) * 8;//* avoids alignements shifts during allocation pushing alloc attempt past reserved size
			auto [scratch, scope] = scratch_push_scope(scratch_size, batch.arena); defer{ scratch_pop_scope(scratch, scope); };

			textures.push(batch.textures.used());
			scene.push_one(batch.scene);
			sheets.push_as(batch.sheets.used());
			quads.push_as(batch.quads.used());
			commands.push_as(map(scratch, batch.mappings.used(),
				[&](auto qrange) { return DrawCommandElement{
					.count = 6,
					.instance_count = GLuint(qrange.size()),
					.first_index = 0,
					.base_vertex = GLint(4 * qrange.min),//* this would only matter if we had non-instanced vertex attributes, but they're all instanced here
					.base_instance = GLuint(qrange.min)
				}; }
			));
			return *this;
		}

		void reset() {
			commands.content = 0;
			quads.content = 0;
			sheets.content = 0;
			scene.content = 0;
			textures.current = 1;//* keep the white texture
		}

	};

	struct Pipeline {
		GLuint id;
		GLuint textures;
		GLuint sheets;
		union {
			struct {
				GLuint rect;
				GLuint sprite;
			};
			GLuint targets[2];
		} quads;
		GLuint scene;

		static Pipeline create(GLScope& ctx, const cstr path = "shaders/quad.glsl") {
			auto ppl = load_pipeline(ctx, path);
			return {
				.id = ppl,
				.textures = get_shader_input(ppl, "textures", R_TEX),
				.sheets = get_shader_input(ppl, "Sheets", R_SSBO),
				.quads = {
					.rect = get_shader_input(ppl, "rect", R_VERT),
					.sprite = get_shader_input(ppl, "sprite", R_VERT)
				},
				.scene = get_shader_input(ppl, "Scene", R_UBO)
			};
		}

		static constexpr auto STARTING_QUAD_COUNT = 1 << 16;
		static constexpr auto STARTING_SHEET_COUNT = 1 << 8;
		Renderer make_renderer(GLScope& ctx, u64 quad_count = STARTING_QUAD_COUNT, u64 sheet_count = STARTING_SHEET_COUNT) {
			auto indices = QuadGeo::make_indices<u32>();
			Renderer rd = {
				.vao = VertexArray::create(ctx),
				.ibo = GPUBuffer::upload(ctx, larray(indices.i)),
				.quads = GPUBuffer::create_stretchy(ctx, sizeof(Quad) * quad_count, GL_DYNAMIC_DRAW),
				.commands = GPUBuffer::create_stretchy(ctx, sizeof(DrawCommandElement) * sheet_count, GL_DYNAMIC_DRAW),
				.sheets = GPUBuffer::create_stretchy(ctx, sizeof(Sheet) * sheet_count, GL_DYNAMIC_DRAW),
				.scene = GPUBuffer::create(ctx, sizeof(Scene), GL_DYNAMIC_STORAGE_BIT),
				.textures = { ctx.arena.push_array<GLuint>(get_max_textures_frag()), 0 }
			};

			rd.textures.push(TexBuffer::white().id);

			rd.vao.conf_vattrib(quads.rect, vattr_fmt<v4f32>(offsetof(Quad, rect)));//TODO get format from shader, would only need to provide offset
			rd.vao.conf_vattrib(quads.sprite, vattr_fmt<v4u32>(offsetof(Quad, sprite)));
			return rd;
		}

		RenderCommand operator()(Arena& arena, const Renderer& rd) const {
			return {
				.pipeline = id,
				.draw_type = RenderCommand::D_MDEI,
				.draw = {.d_indirect = {
					.buffer = rd.commands.id,
					.stride = sizeof(DrawCommandElement),
					.range = {0, GLsizei(rd.commands.content_as<DrawCommandElement>())}
				}},
				.vao = rd.vao.id,
				.ibo = {
					.buffer = rd.ibo.id,
					.index_type = rd.vao.index_type,
					.primitive = rd.vao.draw_mode
				},
				.vertex_buffers = arena.push_array({VertexBinding{
					.buffer = rd.quads.id,
					.targets = arena.push_array(carray(quads.targets, sizeof(quads) / sizeof(GLuint))),
					.offset = 0,
					.stride = sizeof(Quad),
					.divisor = 1
				}}),
				.textures = arena.push_array({ TextureBinding {.textures = arena.push_array(rd.textures.used()), .target = textures} }),
				.buffers = arena.push_array({
					BufferObjectBinding{
						.buffer = rd.sheets.id,
						.type = GL_SHADER_STORAGE_BUFFER,
						.range = {0, GLuint(rd.sheets.content)},
						.target = sheets
					},
					BufferObjectBinding{
						.buffer = rd.scene.id,
						.type = GL_UNIFORM_BUFFER,
						.range = {0, sizeof(Scene)},
						.target = scene
					}
				})
			};
		}
	};
}

#endif
