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
	FT_Face face;
	Atlas2D glyph_atlas;
	axu32 glyph_indices;
	Array<Glyph> glyphs;
	Array<rtu32> glyph_views;
	f32 linespace[2];

	static Font load(
		Arena& arena,
		FT_Library lib,
		const cstr path,
		u32 index = 0,
		v2u32 font_size = v2u32(64),
		v2u32 atlas_size = v2u32(1024),
		num_range<u32> character_codes = num_range<u32>{ 32, 127 }//* default printable
	) {
		//TODO replace constants with import parameters
		//TODO fix SDF bitmap mapping (bitmap rect in atlas is different from what metrics indicate)
		Font f;
		printf("Loading Font %s:%u\n", path, index);
		if (auto err = FT_New_Face(lib, path, index, &f.face))
			return (pfterror(__FILE__, __LINE__, err), Font{});

		if (auto err = FT_Set_Pixel_Sizes(f.face, font_size.x, font_size.y))
			pfterror(__FILE__, __LINE__, err);
		f.linespace[Text::H] = f.face->size->metrics.height / 64.f;
		f.linespace[Text::V] = f.face->size->metrics.max_advance / 64.f;

		struct { FT_ULong code; FT_UInt gindex; } available_glyphs_buffer[character_codes.size() + 1];
		memset(available_glyphs_buffer, 0, sizeof(available_glyphs_buffer[0]) * (character_codes.size() + 1));
		auto available_glyphs = List{ carray(available_glyphs_buffer, character_codes.size()), 0 };

		{ //* fetch available glyphs
			f.glyph_indices = axu32{ v1u32(-(1lu)), v1u32(0) };
			FT_UInt gindex;
			for (auto c = FT_Get_First_Char(f.face, &gindex); gindex != 0; c = FT_Get_Next_Char(f.face, c, &gindex)) if (character_codes.contains_inc(c)) {
				available_glyphs.push({ c, gindex });
				f.glyph_indices = combined_aabb(f.glyph_indices, axu32{ v1u32(gindex), v1u32(gindex) });
			}
		}

		auto mipmaps = 4;

		{ //* Allocate ressources
			f.glyphs = arena.push_array<Glyph>(width(f.glyph_indices) + 1, true);
			f.glyph_views = arena.push_array<rtu32>(width(f.glyph_indices) + 1, true);
			f.glyph_atlas = Atlas2D::create(atlas_size, R32F, mipmaps);
			f.glyph_atlas.texture
				.conf_border_color(v4f32(1.0, 0, 1.0, 1.0))
				.conf_wrap({ ClampToBorder, ClampToBorder, ClampToBorder })
				.conf_sampling({ LinearMipmapLinear, Linear })
				.conf_max_sample_count(8);
		}

		//* load glyphs
		for (auto [c, gindex] : available_glyphs.used()) if (u32 err = 0;
			(err = FT_Load_Glyph(f.face, gindex, 0)) ||
			(err = FT_Render_Glyph(f.face->glyph, FT_RENDER_MODE_NORMAL))
			// (err = FT_Render_Glyph(f.face->glyph, FT_RENDER_MODE_SDF))
			) {
			pfterror(__FILE__, __LINE__, err);
		} else {
			Glyph glyph;
			glyph.atlas_view_index = gindex - f.glyph_indices.min.x;
			glyph.code = c;
			glyph.index = gindex;
			glyph.orient[Text::H].bearing = v2f32(f.face->glyph->metrics.horiBearingX, f.face->glyph->metrics.horiBearingY) / 64.f;
			glyph.orient[Text::H].advance = f.face->glyph->metrics.horiAdvance / 64.f;
			glyph.orient[Text::V].bearing = v2f32(f.face->glyph->metrics.vertBearingX, f.face->glyph->metrics.vertBearingY) / 64.f;
			glyph.orient[Text::V].advance = f.face->glyph->metrics.vertAdvance / 64.f;
			glyph.size = v2u32(f.face->glyph->metrics.width / 64.f, f.face->glyph->metrics.height / 64.f);
			f32 buffer[f.face->glyph->bitmap.rows * f.face->glyph->bitmap.pitch + 1];
			auto scratch = Arena::from_array(carray(buffer, f.face->glyph->bitmap.rows * f.face->glyph->bitmap.pitch));
			f.glyph_views[glyph.atlas_view_index] = f.glyph_atlas.push(
				make_bitmap_image_alpha(scratch, f.face->glyph->bitmap),
				// make_bitmap_image_sdf(scratch, f.face->glyph->bitmap),
				v2u32(mipmaps * 2)
			);
			f.glyphs[glyph.atlas_view_index] = glyph;
		}


		f.glyph_atlas.texture.generate_mipmaps(); //* auto generation since the manual lower res font is broken (cf below)
		//TODO fix this so that it can be usable (offset problem with bitmap of the same glyph being different)
		// {//* generate lower res glyphs in mipmaps
		// 	u32 div = 1;
		// 	for (auto i : u64xrange{ 1, mipmaps }) {
		// 		div *= 2;
		// 		if (auto err = FT_Set_Pixel_Sizes(f.face, font_size.x / div, font_size.y / div)) {
		// 			pfterror(__FILE__, __LINE__, err);
		// 			continue;
		// 		}
		// 		for (auto [c, gindex] : available_glyphs.used()) if (u32 err = 0;
		// 			(err = FT_Load_Glyph(f.face, gindex, 0)) ||
		// 			(err = FT_Render_Glyph(f.face->glyph, FT_RENDER_MODE_NORMAL))
		// 			) {
		// 			pfterror(__FILE__, __LINE__, err);
		// 		} else {
		// 			auto atlas_view_index = gindex - f.glyph_indices.min.x;
		// 			auto base_rect = f.glyph_views[atlas_view_index];
		// 			f32 buffer[f.face->glyph->bitmap.rows * f.face->glyph->bitmap.pitch];
		// 			auto scratch = Arena::from_array(carray(buffer, f.face->glyph->bitmap.rows * f.face->glyph->bitmap.pitch));
		// 			auto img = make_bitmap_image_alpha(scratch, f.face->glyph->bitmap);
		// 			auto rect = rtu32{ base_rect.min / div, base_rect.min / div + img.dimensions };
		// 			upload_texture_data(f.glyph_atlas.texture, img.data, img.format, slice_to_area<2>(rect, 0), i);
		// 		}
		// 	}
		// }

		return f;
	}

	void release() {
		glyph_atlas.release();
		FT_Done_Face(face);
	}

	u32 glyph_index_of(u64 character_code) const {
		auto index_in_face = FT_Get_Char_Index(face, character_code);
		if (index_in_face == 0 || glyph_indices.min.x > index_in_face || glyph_indices.max.x < index_in_face) {
			return 0;
		}
		return index_in_face - glyph_indices.min.x;
	}

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

struct UIRenderer {

	GLuint pipeline;
	GPUGeometry rect_buffer;

	struct {
		GLint textures;
		BufferBinding fonts;
		BufferBinding characters;
		BufferBinding	texts;
		BufferBinding	glyphs;
		BufferBinding	scene;
		GPUBuffer commands;
	} inputs;

	struct TextInstance {
		m4x4f32 transform;
		v4f32 color;
	};

	struct CharacterInstance {
		u32 glyph_index;
		u32 text_index;
	};

	struct AtlasInstance {
		v2u32 size;
		v2u32 glyph_range;
		u32 texture_index;
	};

	struct Scene {
		m4x4f32 project;
		f32 alpha_discard;
	};

	static UIRenderer load(const cstr path, u32 max_texts = 256, u32 max_characters = (1 << 16), u32 max_glyphs = 512) {
		UIRenderer rd;
		rd.pipeline = load_pipeline(path);
		// describe(rd.pipeline);

		rd.rect_buffer = GPUGeometry::allocate(vertexAttributesOf<v2f32>);

		rd.inputs.textures = init_binding_texture(rd.pipeline, "textures");

		BufferBinding::Sequencer bindings = { rd.pipeline };

		rd.inputs.fonts = bindings.next(BufferBinding::SSBO, "Fonts", sizeof(AtlasInstance) * 16);
		rd.inputs.characters = bindings.next(BufferBinding::SSBO, "Characters", sizeof(CharacterInstance) * max_characters);
		rd.inputs.texts = bindings.next(BufferBinding::SSBO, "Texts", sizeof(TextInstance) * max_texts);
		rd.inputs.glyphs = bindings.next(BufferBinding::SSBO, "Glyphs", sizeof(v4u32) * max_glyphs);
		rd.inputs.scene = bindings.next(BufferBinding::UBO, "Scene", sizeof(Scene));
		rd.inputs.commands = GPUBuffer::create(sizeof(DrawCommandElement) * 128, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

		printf(
			"UIRenderer:\n"
			"\ttextures : = %i\n"
			"\tFonts : %s = %i,%llu\n"
			"\tCharacters : %s = %i,%llu\n"
			"\tTexts : %s = %i,%llu\n"
			"\tGlyphs : %s = %i,%llu\n"
			"\tScene : %s = %i,%llu\n",
			rd.inputs.textures,
			GLtoString(rd.inputs.fonts.target).data(), rd.inputs.fonts.index, rd.inputs.fonts.buffer.size,
			GLtoString(rd.inputs.characters.target).data(), rd.inputs.characters.index, rd.inputs.characters.buffer.size,
			GLtoString(rd.inputs.texts.target).data(), rd.inputs.texts.index, rd.inputs.texts.buffer.size,
			GLtoString(rd.inputs.glyphs.target).data(), rd.inputs.glyphs.index, rd.inputs.glyphs.buffer.size,
			GLtoString(rd.inputs.scene.target).data(), rd.inputs.scene.index, rd.inputs.scene.buffer.size
		);
		return rd;
	}

	void release() {
		inputs.fonts.buffer.release();
		inputs.texts.buffer.release();
		inputs.characters.buffer.release();
		inputs.scene.buffer.release();
		inputs.glyphs.buffer.release();
		rect_buffer.release();
		destroy_pipeline(pipeline);
	}

	void operator()(Array<Text> texts, v2f32 canvas = v2f32(1920, 1080)) {
		if (texts.size() == 0)
			return;

		auto [scratch, scope] = scratch_push_scope(sizeof(void*) * get_max_textures_frag()); defer{ scratch_pop_scope(scratch, scope); };

		auto textures = List { scratch.push_array<TexBuffer*>(get_max_textures_frag()), 0 };
		auto fonts = List { cast<AtlasInstance>(inputs.fonts.buffer.map()), 0 };
		auto characters = List { cast<CharacterInstance>(inputs.characters.buffer.map()), 0 };
		auto texts_instances = List { cast<TextInstance>(inputs.texts.buffer.map()), 0 };
		auto glyphs = List { cast<rtu32>(inputs.glyphs.buffer.map()), 0 };
		auto vertices = List { cast<v2f32>(rect_buffer.vbo.map()), 0 };
		auto indices = List { cast<u32>(rect_buffer.ibo.map()), 0 };
		auto commands = List { cast<DrawCommandElement>(inputs.commands.map()), 0 };
		{
			defer {
				inputs.commands.unmap({0, commands.current});
				rect_buffer.ibo.unmap({0, indices.current});
				rect_buffer.vbo.unmap({0, vertices.current});
				inputs.glyphs.buffer.unmap({0, glyphs.current});
				inputs.texts.buffer.unmap({0, texts_instances.current});
				inputs.characters.buffer.unmap({0, characters.current});
				inputs.fonts.buffer.unmap({0, fonts.current});
			};
			Font* font = null;
			for (i32 batch = 0; batch >= 0; batch = linear_search(texts.subspan(batch), [=](Text& t) { return t.str.size() > 0 && t.font != font; })) {
				auto mesh_start_vertex = vertices.current;
				auto mesh_start_index = indices.current;
				font = texts[batch].font;

				v2u32 glyph_range = { glyphs.current, font->glyphs.size() };
				glyphs.push(font->glyph_views);
				fonts.push(AtlasInstance{
					.size = font->glyph_atlas.texture.dimensions,
					.glyph_range = glyph_range,
					.texture_index = u32(textures.current)
				});
				textures.push(&font->glyph_atlas.texture);

				for (auto& text : texts.subspan(batch)) if (text.font == font && text.str.size() > 0) {
					auto text_index = u32(texts_instances.current);
					texts_instances.push({
						.transform = glm::translate(v3f32(text.rect.min, f32(text_index))),
						.color = text.color
					});

					auto cursor = v2f32(0);
					cursor[!text.orient] = dim_vec(text.rect)[!text.orient] - text.scale * text.linespace * font->linespace[text.orient]; //* starting point

					auto advance = (
						[&](v2f32 c, const Glyph& g) -> v2f32 {
							c[text.orient] += g.orient[text.orient].advance * text.scale;
							return c;
						}
					);

					auto next_line = (
						[&](v2f32 c) -> v2f32 {
							c[text.orient] = 0;
							c[!text.orient] -= text.scale * text.linespace * font->linespace[text.orient];
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

					struct CharacterData {
						CharacterInstance instance;
						v2f32 vertices[4];
						u32 indices[6];
					};

					auto make_character_data = (
						[&](v2f32& c, const Glyph& g) -> CharacterData {
							auto bbox = g.bbox(text.orient);
							bbox.min *= text.scale;
							bbox.max *= text.scale;
							auto quad_start_in_mesh = vertices.current - mesh_start_vertex;//* might be wrong but i managed to confused myself about why
							return {
								.instance = {
									.glyph_index = g.atlas_view_index,
									.text_index = text_index
								},
								.vertices = {
									c + bbox.min,
									c + v2f32(bbox.max.x, bbox.min.y),
									c + v2f32(bbox.min.x, bbox.max.y),
									c + bbox.max
								},
								.indices = {
									u32(quad_start_in_mesh + 0),
									u32(quad_start_in_mesh + 1),
									u32(quad_start_in_mesh + 2),
									u32(quad_start_in_mesh + 2),
									u32(quad_start_in_mesh + 1),
									u32(quad_start_in_mesh + 3)
								}
							};
						}
					);

					for (auto i : u64xrange{ 0, text.str.size() }) {
						auto& glyph = (*text.font)[text.str[i]];
						switch (text.str[i]) {
						case ' ': {
							cursor = advance(cursor, glyph);
							if (!text.rect.contain(cursor))
								cursor = next_line(cursor);
							cursor = new_word(cursor, i + 1);
						} break;
						case '\n': { cursor = next_line(cursor); } break;
						case '\r': { cursor = new_word(cursor, i + 1); } break;
						case '\t': { cursor = new_word(cursor, i + 1); } break;//TODO implement
						case '\v': { cursor = new_word(cursor, i + 1); } break;//TODO implement
						case '\f': { cursor = new_word(cursor, i + 1); } break;//TODO implement
						default: { //* Printable characters
							auto data = make_character_data(cursor, glyph);
							cursor = advance(cursor, glyph);
							if (!text.rect.contain(cursor)) {
								cursor = next_line(cursor);
								data = make_character_data(cursor, glyph);
								cursor = advance(cursor, glyph);
								if (!text.rect.contain(cursor))
									//TODO make this break out of the loop and not just the switch
									break;//* early break if text does not fit
							}
							vertices.push(data.vertices);
							indices.push(data.indices);
							characters.push(data.instance);
						} break;
						}
					}
				}
				commands.push(DrawCommandElement{
					.count = u32(indices.current - mesh_start_index),
					.instance_count = 1,
					.first_index = u32(mesh_start_index),
					.base_vertex = i32(mesh_start_vertex),
					.base_instance = 0
				});
			}
		}
		if (commands.current == 0 || texts_instances.current == 0 || characters.current == 0)
			return;
		// printf("Drawing %llu commands, %llu texts, %llu characters\n", commands.current, texts_instances.current, characters.current);
		OrthoCamera camera = {
			.dimensions = v3f32(canvas, texts.size()),
			.center = v3f32(-canvas, texts.size()) / 2.f
		};

		//* submitting
		//? whats the overhead difference between writing to mappings and writing directly to the buffers ?
		GL_GUARD(glUseProgram(pipeline)); defer{ GL_GUARD(glUseProgram(0)); };
		GL_GUARD(glBindVertexArray(rect_buffer.vao.id)); defer{ GL_GUARD(glBindVertexArray(0)); };

		push_texture_array(inputs.textures, 0, textures.used());
		inputs.fonts.bind({0, fonts.used().size_bytes()}); defer{ inputs.fonts.unbind(); };
		inputs.texts.bind({0, texts_instances.used().size_bytes()}); defer{ inputs.texts.unbind(); };
		inputs.characters.bind({0, characters.used().size_bytes()}); defer{ inputs.characters.unbind(); };
		inputs.glyphs.bind({0, glyphs.used().size_bytes()}); defer{ inputs.glyphs.unbind(); };
		inputs.scene.push(Scene{ .project = m4x4f32(camera), .alpha_discard = 0.05f }); defer{ inputs.scene.unbind(); };

		inputs.commands.bind(GL_DRAW_INDIRECT_BUFFER); defer{ inputs.commands.unbind(GL_DRAW_INDIRECT_BUFFER); };
		GL_GUARD(glMultiDrawElementsIndirect(rect_buffer.vao.draw_mode, rect_buffer.vao.index_type, 0, commands.current, 0));
	}

};
#endif
