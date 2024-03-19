#ifndef GSYSTEM_EDITOR
# define GSYSTEM_EDITOR

#include <blblstd.hpp>
#include <inputs.cpp>
#include <imgui_extension.cpp>
#include <framebuffer.cpp>

struct SystemEditor;
template<typename T> concept EditorType = std::derived_from<T, SystemEditor> || std::same_as<T, SystemEditor>;

struct SystemEditor {
	string name = "";
	string shortcut_str = "";
	Input::KB::Key shortcut_keys[2];
	bool show_window = true;

	template<EditorType E = SystemEditor> static E create(string name, string shortcut_str, tuple<Input::KB::Key, Input::KB::Key> shortcut_keys) {
		E ed;
		ed.name = name;
		ed.shortcut_str = shortcut_str;
		auto [s0, s1] = shortcut_keys;
		ed.shortcut_keys[0] = s0;
		ed.shortcut_keys[1] = s1;
		return ed;
	}

	bool menu_item() { return ImGui::MenuItem(name.data(), shortcut_str.data(), &show_window); }
	bool keybind() { return shortcut(larray(shortcut_keys), &show_window); }
};

void system_menu(const cstr label, LiteralArray<SystemEditor*> editors) {
	if (ImGui::BeginMenu(label)) {
		defer{ ImGui::EndMenu(); };
		for (auto ed : editors)
			ed->menu_item();
	}
}

void system_keybinds(LiteralArray<SystemEditor*> editors) {
	for (auto ed : editors)
		ed->keybind();
}

bool begin_editor(SystemEditor& ed, ImGuiWindowFlags flags = 0) { return ImGui::Begin(ed.name.data(), &ed.show_window, flags); }
void end_editor() { ImGui::End(); }

#endif
