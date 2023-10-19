#ifndef GSPRITESHEET
# define GSPRITESHEET

#include <blblstd.hpp>
#include <animation.cpp>

using SpriteAnimation = AnimationGrid<rtu32>;

//TODO proper serialisation scheme

template<typename T> auto read_flat(FILE* file) {
	T obj;
	auto r = fread(&obj, sizeof(obj), 1, file);
	assert(r >= sizeof(obj));
	return obj;
}

template<typename T> auto write_flat(FILE* file, const T& obj) {
	auto w = fwrite(&obj, sizeof(obj), 1, file);
	assert(w >= sizeof(obj));
	return obj;
}

auto parse_array(FILE* file, Arena& arena, auto sub_parser) {
	using R = decltype(sub_parser(file, arena));
	u64 length;
	fread(&length, sizeof(length), 1, file);
	auto arr = arena.push_array<R>(length);
	for (auto& i : arr)
		i = sub_parser(file, arena);
	return arr;
}

template<typename T> usize write_array(FILE* file, Array<T> arr, auto sub_writer) {
	u64 l = arr.size();
	auto written = fwrite(&l, sizeof(l), 1, file);
	for (auto& i : arr)
		written += sub_writer(i);
	return written;
}

template<typename T> Array<T> parse_array_flat(FILE* file, Arena& arena) {
	u64 length;
	fread(&length, sizeof(length), 1, file);
	auto arr = arena.push_array<T>(length + 1);
	fread(arr.data(), sizeof(T), length, file);
	arr[length] = 0;
	return arr;
}

template<typename T> usize write_array_flat(FILE* file, Array<T> arr) {
	u64 l = arr.size();
	auto written = fwrite(&l, sizeof(l), 1, file);
	written += fwrite(arr.data(), sizeof(T), l, file);
	return written;
}

string parse_string(FILE* file, Arena& arena) { return string(parse_array_flat<char>(file, arena).data()); }
usize write_string(FILE* file, string s) { return write_array_flat(file, carray(s.data(), s.size())); }

//* incomplete SpriteAnimation
SpriteAnimation slice_spritesheet(Arena& arena, v2u32 source_dimensions, v2u32 dimensions) {
	SpriteAnimation anim;
	auto cell_dimensions = source_dimensions / dimensions;
	auto cell_count = dimensions.x * dimensions.y;
	anim.dimensions = cast<u32>(arena.push_array(carray(&dimensions, 1)));
	anim.keyframes = arena.push_array<rtu32>(cell_count);
	anim.extents = map(arena, cast<u32>(carray(&source_dimensions, 1)), [](u32 i)->f32 { return f32(i); });
	anim.config = arena.push_array({ AnimClamp, AnimClamp });
	for (auto y : u64xrange{ 0, dimensions.y }) for (auto x : u64xrange{ 0, dimensions.x })
		anim.keyframes[coord_to_index(dimensions, v2u32(x, y))] = rtu32{ v2u32(x, y) * cell_dimensions, v2u32(x + 1, y + 1) * cell_dimensions };
	return anim;
}

SpriteAnimation layout_animation(Arena& arena, SpriteAnimation source, AnimationGrid<v2u32> layout) {
	SpriteAnimation anim;
	anim.dimensions = arena.push_array(layout.dimensions);
	anim.extents = arena.push_array(layout.extents);
	anim.config = arena.push_array(layout.config);
	anim.keyframes = map(arena, layout.keyframes, [&](v2u32 cell)->rtu32 { return get_keyframe(source, cell); });
	return anim;
}

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>

struct SpritesheetLayout {
	using Entry = tuple<string, AnimationGrid<v2u32>>;
	Array<Entry> entries = {};
	string name = "";
	v2u32 dimensions = v2u32(0);
};

bool try_parse_attr_string(Arena& arena, string name, xmlDoc* doc, const xmlAttr& attr, string& out) {
	if (string((char*)attr.name) == name) {
		xmlChar* str = xmlNodeListGetString(doc, attr.children, 1); defer{ xmlFree(str); };
		out = arena.push_string((char*)str);
		return true;
	} else return false;
}

bool try_parse_attr_u32(string name, xmlDoc* doc, const xmlAttr& attr, u32& out) {
	if (string((char*)attr.name) == name) {
		xmlChar* str = xmlNodeListGetString(doc, attr.children, 1); defer{ xmlFree(str); };
		out = atoi((char*)str);
		return true;
	} else return false;
}

bool try_parse_attr_f32(string name, xmlDoc* doc, const xmlAttr& attr, f32& out) {
	if (string((char*)attr.name) == name) {
		xmlChar* str = xmlNodeListGetString(doc, attr.children, 1); defer{ xmlFree(str); };
		out = atof((char*)str);
		return true;
	} else return false;
}

bool try_parse_attr_u32_coord(xmlDoc* doc, const xmlAttr& attr, v4u32& out) {
	return try_parse_attr_u32("x", doc, attr, out.x) ||
		try_parse_attr_u32("y", doc, attr, out.y) ||
		try_parse_attr_u32("z", doc, attr, out.z) ||
		try_parse_attr_u32("w", doc, attr, out.w) ||
		false;
}

bool try_parse_attr_f32_coord(xmlDoc* doc, const xmlAttr& attr, v4f32& out) {
	return try_parse_attr_f32("x", doc, attr, out.x) ||
		try_parse_attr_f32("y", doc, attr, out.y) ||
		try_parse_attr_f32("z", doc, attr, out.z) ||
		try_parse_attr_f32("w", doc, attr, out.w) ||
		false;
}

template<typename T> bool try_parse_attr_enum(string name, xmlDoc* doc, const xmlAttr& attr, T& out, LiteralArray<tuple<string, T>> mappings) {
	byte buffer[256];
	auto arena = Arena::from_buffer(larray(buffer));
	string s;
	if (!try_parse_attr_string(arena, name, doc, attr, s)) return false;
	for (auto& [name, value] : mappings) if (name == s) {
		out = value;
		return true;
	}
	return false;
}

//! only deals with up to 4 dimensions
SpritesheetLayout::Entry parse_grid(Arena& arena, xmlNode* node) {
	string name = "";
	v4u32 dims = v4u32(1);
	v4f32 extents = v4f32(1);
	auto config = glm::vec<4, AnimationWrap>(AnimClamp);

	for (auto& attr : traverse_by<xmlAttr, &xmlAttr::next>(node->properties))
		try_parse_attr_string(arena, "name", node->doc, attr, name) ||
		try_parse_attr_u32_coord(node->doc, attr, dims) ||
		false;

	auto cells = List{ arena.push_array<v2u32>(count<xmlNode, &xmlNode::next>(node->children)), 0 };
	for (auto& cell : traverse_by<xmlNode, &xmlNode::next>(node->children)) {
		if (string((char*)cell.name) == "dimension") for (auto& attr : traverse_by<xmlAttr, &xmlAttr::next>(cell.properties)) try_parse_attr_u32_coord(node->doc, attr, dims);
		else if (string((char*)cell.name) == "extent") for (auto& attr : traverse_by<xmlAttr, &xmlAttr::next>(cell.properties)) try_parse_attr_f32_coord(node->doc, attr, extents);
		else if (string((char*)cell.name) == "config") for (auto& attr : traverse_by<xmlAttr, &xmlAttr::next>(cell.properties))
			try_parse_attr_enum("x", node->doc, attr, config.x, { { "clamp", AnimClamp }, { "repeat", AnimRepeat }, { "mirror", AnimMirror } }) ||
			try_parse_attr_enum("y", node->doc, attr, config.y, { { "clamp", AnimClamp }, { "repeat", AnimRepeat }, { "mirror", AnimMirror } }) ||
			try_parse_attr_enum("z", node->doc, attr, config.y, { { "clamp", AnimClamp }, { "repeat", AnimRepeat }, { "mirror", AnimMirror } }) ||
			try_parse_attr_enum("w", node->doc, attr, config.y, { { "clamp", AnimClamp }, { "repeat", AnimRepeat }, { "mirror", AnimMirror } }) ||
			false;
		else if (string((char*)cell.name) == "cell") {
			v2u32 v = v2u32(0);
			for (auto& attr : traverse_by<xmlAttr, &xmlAttr::next>(cell.properties))
				try_parse_attr_u32("x", node->doc, attr, v.x) ||
				try_parse_attr_u32("y", node->doc, attr, v.y);
			cells.push(v);
		} //else assert(("unknown node type", false));
	}
	cells.shrink_to_content(arena);

	AnimationGrid<v2u32> grid;
	grid.dimensions = arena.push_array(cast<u32>(carray(&dims, 1)));
	grid.extents = arena.push_array(cast<f32>(carray(&extents, 1)));
	grid.config = arena.push_array(cast<AnimationWrap>(carray(&config, 1)));
	grid.keyframes = arena.push_array(cells.used());
	return tuple(name, grid);
}

SpritesheetLayout::Entry parse_sequence(Arena& arena, xmlNode* node) {
	string name = "";
	f32 duration = 1;
	AnimationWrap w = AnimClamp;

	for (auto& attr : traverse_by<xmlAttr, &xmlAttr::next>(node->properties))
		try_parse_attr_string(arena, "name", node->doc, attr, name) ||
		try_parse_attr_f32("duration", node->doc, attr, duration) ||
		try_parse_attr_enum("wrap", node->doc, attr, w, { { "clamp", AnimClamp }, { "repeat", AnimRepeat }, { "mirror", AnimMirror } }) ||
		false;

	auto cells = List{ arena.push_array<v2u32>(count<xmlNode, &xmlNode::next>(node->children)), 0 };
	for (auto& cell : traverse_by<xmlNode, &xmlNode::next>(node->children)) if (string((char*)cell.name) == "cell") {
		v4u32 v = v4u32(0);
		for (auto& attr : traverse_by<xmlAttr, &xmlAttr::next>(cell.properties))
			try_parse_attr_u32("x", node->doc, attr, v.x) ||
			try_parse_attr_u32("y", node->doc, attr, v.y);
		cells.push(v);
	}
	u32 size = cells.current;

	AnimationGrid<v2u32> grid;
	grid.dimensions = arena.push_array(carray(&size, 1));
	grid.extents = arena.push_array(carray(&duration, 1));
	grid.config = arena.push_array(carray(&w, 1));
	grid.keyframes = arena.push_array(cells.used());

	return tuple(name, grid);
}

SpritesheetLayout build_layout(Arena& arena, const cstr recipe_path) {
	SpritesheetLayout layout;
	auto doc = xmlParseFile(recipe_path); assert(("TODO implement parse failure handling", doc)); defer{ xmlFreeDoc(doc); };
	auto root = xmlDocGetRootElement(doc); assert(string((char*)root->name) == "spritesheet");

	for (auto& attr : traverse_by<xmlAttr, &xmlAttr::next>(root->properties))
		try_parse_attr_u32("x", doc, attr, layout.dimensions.x) ||
		try_parse_attr_u32("y", doc, attr, layout.dimensions.y) ||
		try_parse_attr_string(arena, "name", doc, attr, layout.name) ||
		false;

	assert(layout.dimensions.x > 0); assert(layout.dimensions.y > 0);

	{
		auto entries = List{ arena.push_array<SpritesheetLayout::Entry>(count<xmlNode, &xmlNode::next>(root->children)), 0 };
		for (auto& anim : traverse_by<xmlNode, &xmlNode::next>(root->children)) {
			if (string((char*)anim.name) == "grid") {
				entries.push(parse_grid(arena, &anim));
			} else if (string((char*)anim.name) == "sequence") {
				entries.push(parse_sequence(arena, &anim));
			} //else {
			// 	fprintf(stderr, "Unknown animation entry type '%s'\n", anim.name);
			// }
		}
		layout.entries = entries.used();
	}
	return layout;
}

SpritesheetLayout parse_spritesheet_layout(FILE* file, Arena& arena) {
	auto name = parse_string(file, arena);
	v2u32 dims;
	fread(&dims, sizeof(dims), 1, file);
	auto entries = parse_array(file, arena, [](FILE* file, Arena& arena) { return tuple(parse_string(file, arena), parse_animation_grid<v2u32>(file, arena)); });
	SpritesheetLayout layout;
	layout.entries = entries;
	layout.name = name;
	layout.dimensions = dims;
	return layout;
}

usize write_spritesheet_layout(FILE* file, SpritesheetLayout layout) {
	auto w = write_string(file, layout.name);
	w += fwrite(&layout.dimensions, sizeof(layout.dimensions), 1, file);
	return w + write_array(file, layout.entries,
		[&](SpritesheetLayout::Entry entry) {
			auto& [name, anim] = entry;
			return write_string(file, name) + write_animation_grid(file, anim);
		}
	);
}

i32 get_entry(const SpritesheetLayout& layout, string id) {
	auto i = linear_search(layout.entries, [=](const SpritesheetLayout::Entry& entry) { return std::get<0>(entry) == id; });
	assert(i >= 0);
	return i;
}

Array<SpriteAnimation> build_animations(Arena& arena, const SpritesheetLayout& layout, v2u32 texture_dimensions, Array<const string> animations) {
	using Entry = SpritesheetLayout::Entry;
	auto [scratch, scope] = scratch_push_scope(layout.dimensions.x * layout.dimensions.y * sizeof(rtu32) * 2); defer { scratch_pop_scope(scratch, scope); };
	auto sheet = slice_spritesheet(scratch, texture_dimensions, layout.dimensions);
	return map(arena, animations, (
		[&](string id) -> SpriteAnimation {
			auto& [_, anim] = layout.entries[get_entry(layout, id)];
			return layout_animation(arena, sheet, anim);
		})
	);
}

auto build_animations(Arena& a, const SpritesheetLayout& l, v2u32 t, LiteralArray<string> o) { return build_animations(a, l, t, larray(o)); }

#endif
