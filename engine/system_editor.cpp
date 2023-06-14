#ifndef GSYSTEM_EDITOR
# define GSYSTEM_EDITOR

#include <blblstd.hpp>
#include <inputs.cpp>
#include <imgui_extension.cpp>
#include <framebuffer.cpp>

struct SystemEditor {
	string name = "";
	string shortcut_str = "";
	Input::KB::Key shortcut[2];
	bool show_window = false;
	SystemEditor() = default;
	SystemEditor(string n, string sc_s, LiteralArray<Input::KB::Key> sc) {
		name = n;
		shortcut_str = sc_s;
		shortcut[0] = *sc.begin();
		shortcut[1] = *(sc.begin() + 1);
	}
};

SystemEditor create_editor(string name, string shortcut_str, LiteralArray<Input::KB::Key> shortcut) {
	SystemEditor ed;
	ed.name = name;
	ed.shortcut_str = shortcut_str;
	ed.shortcut[0] = *shortcut.begin();
	ed.shortcut[1] = *(shortcut.begin() + 1);
	return ed;
}

bool shortcut_sub_editor(SystemEditor& ed) {
	return shortcut(larray(ed.shortcut), &ed.show_window);
}

void shortcut_sub_editors(Array<SystemEditor*> eds) {
	for (auto ed : eds) if (ed)
		shortcut_sub_editor(*ed);
}

bool sub_editor_menu_item(SystemEditor& ed) {
	return ImGui::MenuItem(ed.name.data(), ed.shortcut_str.data(), &ed.show_window);
}

bool begin_editor(SystemEditor& ed, ImGuiWindowFlags flags = 0) { return ImGui::Begin(ed.name.data(), &ed.show_window, flags); }

void end_editor() { ImGui::End(); }

void sub_editor_menu(const cstr label, Array<SystemEditor*> editors) {
	if (ImGui::BeginMenu(label)) {
		defer{ ImGui::EndMenu(); };
		for (auto e : editors) if (e)
			sub_editor_menu_item(*e);
	}
}

template<typename... T, u64... indices> void add_editors_helper(List<SystemEditor*>& list, tuple<T...>& eds, std::integer_sequence<u64, indices...>) {
	(..., list.push(&std::get<indices>(eds)));
}

template<typename... T> void add_editors(List<SystemEditor*>& list, tuple<T...>& eds) {
	add_editors_helper(list, eds, std::make_integer_sequence<u64, sizeof...(T)>{});
}

#endif
