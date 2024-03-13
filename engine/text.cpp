#ifndef GTEXT
# define GTEXT

#include <blblstd.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BBOX_H
#include FT_GLYPH_H

#include <rendering.cpp>
#include <atlas.cpp>

struct Font;
struct Text {
	string str;
	rtu32 rect;
	v4f32 color;
	Font* font;
	f32 scale;
	f32 linespace;
	enum Orientation : u32 { H = 0, V = 1 };
	Orientation orient;
};

struct Glyph {
	u64 code;
	u64 index;
	v2f32 size;
	struct {
		v2f32 bearing;
		f32 advance;
	} orient[2];
	u32 atlas_view_index;

	rtf32 bbox(Text::Orientation ori = Text::H) const {
		rtf32 rt;
		rt.min.x = orient[ori].bearing.x;
		rt.min.y = orient[ori].bearing.y - size.y;
		rt.max.x = orient[ori].bearing.x + size.x;
		rt.max.y = orient[ori].bearing.y;
		return rt;
	}
};

Image make_bitmap_image(Arena& arena, const FT_Bitmap& bitmap) {
	//TODO handle other formats
	if (bitmap.pitch == 0 || bitmap.rows == 0) return make_image<u8>({}, v2u32(0), 1);
	switch (bitmap.pixel_mode) {
	case FT_PIXEL_MODE_NONE: { panic(); } break;
	case FT_PIXEL_MODE_MONO: { panic(); } break;
	case FT_PIXEL_MODE_GRAY: {
		auto true_pitch = abs(bitmap.pitch);
		auto bitmap_buffer = cast<u8>(carray(bitmap.buffer, bitmap.rows * true_pitch));
		auto image = List{ arena.push_array<u8>(bitmap.rows * bitmap.width), 0 };
		for (auto i : u64xrange{ 0, bitmap.rows }) if (bitmap.pitch < 0) {
			image.push(bitmap_buffer.subspan(bitmap_buffer.size() - (1 + i) * true_pitch, bitmap.width));
		} else {
			image.push(bitmap_buffer.subspan(i * true_pitch, bitmap.width));
		}
		return make_image<u8>(image.used(), v2u32(bitmap.width, bitmap.rows), 1);
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

struct Font {
	FT_Face face;
	Atlas2D glyph_atlas;
	axu32 glyph_indices;
	Array<Glyph> glyphs;
	Array<rtu32> glyph_views;
	f32 linespace;

	static Font load(
		Arena& arena,
		FT_Library lib,
		const cstr path,
		u32 index = 0,
		v2u32 font_size = v2u32(64),
		v2u32 atlas_size = v2u32(1024),
		num_range<u32> character_codes = num_range<u32>{ 32, 127 }//* default printable
	) {
		Font f;
		printf("Loading Font %s:%u\n", path, index);
		if (auto err = FT_New_Face(lib, path, index, &f.face))
			return (pfterror(__FILE__, __LINE__, err), Font{});

		if (auto err = FT_Set_Pixel_Sizes(f.face, font_size.x, font_size.y))
			pfterror(__FILE__, __LINE__, err);

		//TODO vertical
		// f.linespace = f.face. f.face->available_sizes->height;
		f.linespace = f.face->size->metrics.height / 64.f;

		struct { FT_ULong code; FT_UInt gindex; } available_glyphs_buffer[character_codes.size()];
		auto available_glyphs = List{ carray(available_glyphs_buffer, character_codes.size()), 0 };

		{ //* fetch available glyphs
			f.glyph_indices = axu32{ v1u32(-(1lu)), v1u32(0) };
			FT_UInt gindex;
			for (auto c = FT_Get_First_Char(f.face, &gindex); gindex != 0; c = FT_Get_Next_Char(f.face, c, &gindex)) if (character_codes.contains_inc(c)) {
				available_glyphs.push({ c, gindex });
				f.glyph_indices = combined_aabb(f.glyph_indices, axu32{ v1u32(gindex), v1u32(gindex) });
			}
		}

		{ //* Allocate ressources
			f.glyphs = arena.push_array<Glyph>(width(f.glyph_indices), true);
			f.glyph_views = arena.push_array<rtu32>(width(f.glyph_indices), true);
			//TODO R32F atlas (requires R8UI to R32F conversion in make_bitmap_image) to allow for linear sampling
			f.glyph_atlas = Atlas2D::create(atlas_size, R8UI);
			f.glyph_atlas.texture.conf_border_color(v3f32(1.0, 0, 1.0));
			f.glyph_atlas.texture.conf_wrap({ ClampToBorder, ClampToBorder, ClampToBorder });
		}

		//* load glyphs
		for (auto [c, gindex] : available_glyphs.used()) if (auto err = FT_Load_Glyph(f.face, gindex, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT)) {
			pfterror(__FILE__, __LINE__, err);
		} else {
			Glyph glyph;
			glyph.code = c;
			glyph.index = gindex;
			glyph.orient[Text::H].bearing = v2f32(f.face->glyph->metrics.horiBearingX, f.face->glyph->metrics.horiBearingY) / 64.f;
			glyph.orient[Text::H].advance = f.face->glyph->metrics.horiAdvance / 64.f;
			glyph.orient[Text::V].bearing = v2f32(f.face->glyph->metrics.vertBearingX, f.face->glyph->metrics.vertBearingY) / 64.f;
			glyph.orient[Text::V].advance = f.face->glyph->metrics.vertAdvance / 64.f;
			glyph.size = v2u32(f.face->glyph->metrics.width / 64.f, f.face->glyph->metrics.height / 64.f);
			glyph.atlas_view_index = gindex - f.glyph_indices.min.x;
			byte buffer[f.face->glyph->bitmap.rows * f.face->glyph->bitmap.pitch];
			auto scratch = Arena::from_buffer(carray(buffer, f.face->glyph->bitmap.rows * f.face->glyph->bitmap.pitch));
			f.glyph_views[glyph.atlas_view_index] = f.glyph_atlas.push(make_bitmap_image(scratch, f.face->glyph->bitmap));
			f.glyphs[glyph.atlas_view_index] = glyph;
		}

		return f;
	}

	void release() {
		glyph_atlas.release();
		FT_Done_Face(face);
	}

	u32 glyph_index_of(u64 character_code) const { return FT_Get_Char_Index(face, character_code) - glyph_indices.min.x; }
	Glyph& operator[](u64 character_code) { return glyphs[glyph_index_of(character_code)]; }
	const Glyph& operator[](u64 character_code) const { return glyphs[glyph_index_of(character_code)]; }
};

bool EditorWidget(const cstr label, Font& font) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("face", font.face);
		changed |= EditorWidget("glyph_indices", font.glyph_indices);
		changed |= EditorWidget("glyphs", font.glyphs);
		changed |= EditorWidget("glyph_views", font.glyph_views, true);
		changed |= EditorWidget("glyph_atlas", font.glyph_atlas);
	}
	return changed;
}

struct TextRenderer {
	FT_Library lib;
	GLuint pipeline;
	RenderMesh rect;

	struct {
		ShaderInput font_atlas;
		ShaderInput characters;
		ShaderInput texts;
		ShaderInput glyphs;
		ShaderInput scene;
	} inputs;

	struct TextInstance {
		m4x4f32 transform;
		v4f32 color;
	};

	struct Instance {
		rtf32 rect;
		u32 glyph;
		u32 text;
		u32 padding[2];
	};

	struct Scene {
		m4x4f32 project;
		v2u32 font_atlas_size;
		f32 alpha_discard;
		u32 padding;
	};

	static TextRenderer load(const cstr path, u32 max_texts = 256, u32 max_characters = (1 << 16), u32 max_glyphs = 512) {
		TextRenderer rd;
		if (auto err = FT_Init_FreeType(&rd.lib))
			pfterror(__FILE__, __LINE__, err);
		else {
			rd.pipeline = load_pipeline(path);
			// describe(rd.pipeline);
			rd.rect = create_rect_mesh(v2f32(1));
			rd.inputs.font_atlas = ShaderInput::create_slot(rd.pipeline, ShaderInput::Texture, "font");
			rd.inputs.characters = ShaderInput::create_slot(rd.pipeline, ShaderInput::SSBO, "Characters", sizeof(Instance) * max_characters);
			rd.inputs.texts = ShaderInput::create_slot(rd.pipeline, ShaderInput::SSBO, "Texts", sizeof(TextInstance) * max_texts);
			rd.inputs.glyphs = ShaderInput::create_slot(rd.pipeline, ShaderInput::SSBO, "Glyphs", sizeof(v4u32) * max_glyphs);
			rd.inputs.scene = ShaderInput::create_slot(rd.pipeline, ShaderInput::UBO, "Scene", sizeof(Scene));
		}
		return rd;
	}

	void release() {
		inputs.font_atlas.release();
		inputs.texts.release();
		inputs.characters.release();
		inputs.scene.release();
		inputs.glyphs.release();
		rect.release();
		destroy_pipeline(pipeline);
		FT_Done_FreeType(lib);
	}

	void operator()(Array<Text> texts, v2f32 canvas = v2f32(1920, 1080)) {
		if (texts.size() == 0)
			return;

		Font* font = null;
		for (i32 batch = 0; batch >= 0; batch = linear_search(texts.subspan(batch), [=](Text& t) { return t.font != font; })) {
			font = texts[batch].font;
			auto [scratch, scope] = scratch_push_scope(texts.size_bytes() + inputs.characters.backing_buffer.size); defer{ scratch_pop_scope(scratch, scope); };
			List<TextInstance> text_instances = { cast<TextInstance>(scratch.push(texts.size() * sizeof(TextInstance))), 0 };
			List<Instance> instances = { cast<Instance>(scratch.push(inputs.characters.backing_buffer.size)), 0 };

			for (auto& text : texts.subspan(batch)) if (text.font == font) {
				auto cursor = v2f32(0);
				cursor[!text.orient] = dim_vec(text.rect)[!text.orient] - text.scale * text.linespace * font->linespace; //* starting point

				auto advance = (
					[&](v2f32 c, const Glyph& g) -> v2f32 {
						c[text.orient] += g.orient[text.orient].advance * text.scale;
						return c;
					}
				);

				auto next_line = (
					[&](v2f32 c) -> v2f32 {
						c[text.orient] = 0;
						c[!text.orient] -= text.scale * text.linespace * font->linespace;
						return c;
					}
				);

				auto new_word = (
					[&](v2f32 c, u32 i) -> v2f32 {
						auto end_of_word = text.str.find_first_of("- \n\r\t\v\f\0", i);
						auto word = text.str.substr(i, end_of_word - i);
						auto acc_advance = [&](v2f32 a, char b) -> v2f32 { a[text.orient] += (*text.font)[b].orient[text.orient].advance * text.scale; return a; };
						auto word_advance = fold(v2f32(0.f), Array<const char>(word), acc_advance);
						if (!text.rect.contain(c + word_advance))
							c = next_line(c);
						return c;
					}
				);

				auto make_character_instance = (
					[&](v2f32& c, const Glyph& g, u32 text_index) -> Instance {
						auto bbox = g.bbox(text.orient);
						bbox.min *= text.scale;
						bbox.max *= text.scale;
						Instance instance = {};
						instance.glyph = g.atlas_view_index;
						instance.text = text_index;
						instance.rect = { c + bbox.min, c + bbox.max };
						return instance;
					}
				);

				auto make_text_instance = (
					[&](Text& t, u32 index) -> TextInstance {
						TextInstance tinstance;
						tinstance.color = t.color;
						tinstance.transform = glm::translate(v3f32(t.rect.min, f32(text_instances.current)));
						for (auto i : u64xrange{ 0, t.str.size() }) {
							auto& glyph = (*t.font)[t.str[i]];
							switch (t.str[i]) {
							case ' ': {
								cursor = advance(cursor, glyph);
								if (!t.rect.contain(cursor))
									cursor = next_line(cursor);
								cursor = new_word(cursor, i + 1);
							} break;
							case '\n': { cursor = next_line(cursor); } break;
							case '\r': { cursor = new_word(cursor, i + 1); } break;
							case '\t': { cursor = new_word(cursor, i + 1); } break;//TODO implement
							case '\v': { cursor = new_word(cursor, i + 1); } break;//TODO implement
							case '\f': { cursor = new_word(cursor, i + 1); } break;//TODO implement
							default: { //* Printable characters
								auto instance = make_character_instance(cursor, glyph, index);
								cursor = advance(cursor, glyph);
								if (!t.rect.contain(cursor)) {
									cursor = next_line(cursor);
									instance = make_character_instance(cursor, glyph, index);
									cursor = advance(cursor, glyph);
									if (!t.rect.contain(cursor))
										return tinstance; //* early return if text does not fit
								}
								instances.push(instance);
							} break;
							}
						}
						return tinstance;
					}
				);

				text_instances.push(make_text_instance(text, text_instances.current));
			}

			OrthoCamera camera = {};
			camera.dimensions = v3f32(canvas, texts.size());
			camera.center = v3f32(-canvas, texts.size()) / 2.f;

			GL_GUARD(glUseProgram(pipeline)); defer{ GL_GUARD(glUseProgram(0)); };
			GL_GUARD(glBindVertexArray(rect.vao.id)); defer{ GL_GUARD(glBindVertexArray(0)); };
			inputs.font_atlas.bind_texture(font->glyph_atlas.texture.id); defer{ inputs.font_atlas.unbind(); };
			inputs.glyphs.bind_content(font->glyph_views); defer{ inputs.glyphs.unbind(); };
			inputs.texts.bind_content(text_instances.used());defer{ inputs.texts.unbind(); };
			inputs.characters.bind_content(instances.used());defer{ inputs.characters.unbind(); };
			inputs.scene.bind_object(Scene{ camera, font->glyph_atlas.texture.dimensions, 0.01f }); defer{ inputs.scene.unbind(); };
			GL_GUARD(glDrawElementsInstanced(rect.vao.draw_mode, rect.vao.element_count, rect.vao.index_type, null, instances.current));
		}
	}

};

#endif
