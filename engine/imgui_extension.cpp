#ifndef GIMGUI_EXTENSION
# define GIMGUI_EXTENSION

#include <cstring>

#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <math.cpp>
#include <blblstd.hpp>
#include <typeinfo>

ImGuiConfigFlags DefaultImguiFlags = (
	ImGuiConfigFlags_NavEnableKeyboard |
	ImGuiConfigFlags_DockingEnable |
	ImGuiConfigFlags_ViewportsEnable
	);

namespace ImGui {

	void init_ogl_glfw(GLFWwindow* window, ImGuiConfigFlags flags = DefaultImguiFlags);
	void init_ogl_glfw(GLFWwindow* window, ImGuiConfigFlags flags) {
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = flags;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 450");
	}

	inline void shutdown_ogl_glfw() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	inline void Draw(ImDrawData* data = null) {
		if (data == null) {
			ImGui::Render();
			data = ImGui::GetDrawData();
		}
		ImGui_ImplOpenGL3_RenderDrawData(data);
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}


	template<typename T> bool bit_flags(const cstr label, T& flags, Array<const string> bit_names, bool same_line = true) {
		bool changed = false;
		if (same_line) {
			ImGui::Text("%s", label);
			ImGui::SameLine();
		}
		if (same_line || ImGui::TreeNode(label)) {
			for (u64 bit_idx : u64xrange{ 0, bit_names.size() }) {
				bool checked = flags & mask<T>(bit_idx);
				if (ImGui::Checkbox(bit_names[bit_idx].cbegin(), &checked)) {
					flags ^= mask<T>(bit_idx);
					changed = true;
				}
				if (same_line && bit_idx < bit_names.size() - 1)
					ImGui::SameLine();
			}
			if (!same_line)
				ImGui::TreePop();
		}
		return changed;
	}

	template<typename T> bool mask_flags(const cstr label, T& flags, Array<const tuple<const string, T>> masks) {
		bool changed = false;
		if (ImGui::BeginCombo(label, "", ImGuiComboFlags_NoPreview)) {
			defer { ImGui::EndCombo(); };
			for (auto& [name, mask] : masks) {
				bool checked = (flags & mask) == mask;
				if (ImGui::Selectable(name.cbegin(), &checked, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_AllowDoubleClick)) {
					if (checked)
						flags |= mask;
					else
						flags &= ~mask;
					changed = true;
				}
			}
		}
		return changed;
	}

	template<typename T> inline bool bit_flags(const cstr label, T& flags, LiteralArray<string> bit_names) {
		return bit_flags(label, flags, larray(bit_names));
	}

	template<typename T> inline bool mask_flags(const cstr label, T& flags, LiteralArray<tuple<const string, T>> masks) {
		return mask_flags(label, flags, larray(masks));
	}

	inline void NewFrame_OGL_GLFW() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	template<typename F> inline ImDrawData* RenderNewFrame(F func) {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		func();
		ImGui::Render();
		return ImGui::GetDrawData();
	}

	inline v2f32 to_glm(ImVec2 in) { return { in.x, in.y }; }
	inline ImVec2 from_glm(v2f32 in) { return { in.x, in.y }; }

	inline ImVec2 fit_to_window(ImVec2 dimensions) {
		auto window = ImGui::GetContentRegionAvail();
		auto ratios = ImVec2(window.x / dimensions.x, window.y / dimensions.y);
		return ImVec2(
			dimensions.x * min(ratios.x, ratios.y),
			dimensions.y * min(ratios.x, ratios.y)
		);
	}
}

inline bool EditorWidget(const cstr label, auto& unimplemented, bool foldable = false) {
	(void)foldable;
	ImGui::Text("%s : Unimplemented Widget for type %s", label, typeid(decltype(unimplemented)).name());
	return false;
}

inline bool EditorWidget(const cstr label, f32& data) {
	return ImGui::DragFloat(label, &data, .01f, .0f, .0f, "%.4f");
}

inline bool EditorWidget(const cstr label, v2f32& data) {
	return ImGui::DragFloat2(label, glm::value_ptr(data), .01f, .0f, .0f, "%.4f");
}

inline bool EditorWidget(const cstr label, v3f32& data) {
	return ImGui::DragFloat3(label, glm::value_ptr(data), .01f, .0f, .0f, "%.4f");
}

inline bool EditorWidget(const cstr label, v4f32& data) {
	return ImGui::DragFloat4(label, glm::value_ptr(data), .01f, .0f, .0f, "%.4f");
}

inline bool EditorWidget(const cstr label, i32& data) {
	return ImGui::DragInt(label, &data);
}

inline bool EditorWidget(const cstr label, v2i32& data) {
	return ImGui::DragInt2(label, glm::value_ptr(data));
}

inline bool EditorWidget(const cstr label, v3i32& data) {
	return ImGui::DragInt3(label, glm::value_ptr(data));
}

inline bool EditorWidget(const cstr label, v4i32& data) {
	return ImGui::DragInt4(label, glm::value_ptr(data));
}

inline bool EditorWidget(const cstr label, u32& data) {
	return EditorWidget(label, (i32&)data);
}

inline bool EditorWidget(const cstr label, u8& data) {
	u32 tmp = data;
	auto changed = EditorWidget(label, tmp);
	if (changed) data = tmp;
	return changed;
}

inline bool EditorWidget(const cstr label, u64& data) {
	u32 tmp = data;
	auto changed = EditorWidget(label, tmp);
	if (changed) data = tmp;
	return changed;
}

inline bool EditorWidget(const cstr label, v2u32& data) {
	return EditorWidget(label, (v2i32&)data);
}

inline bool EditorWidget(const cstr label, v3u32& data) {
	return EditorWidget(label, (v3i32&)data);
}

inline bool EditorWidget(const cstr label, v4u32& data) {
	return EditorWidget(label, (v4i32&)data);
}

inline bool EditorWidget(const cstr label, bool& data) {
	return ImGui::Checkbox(label, &data);
}

template<typename T> inline bool EditorWidget(const cstr label, reg_polytope<T>& data, bool foldable = true) {
	bool changed = false;
	if (!foldable)
		ImGui::Text("%s", label);
	if (!foldable || ImGui::TreeNode(label)) {
		changed |= EditorWidget("Min corner", data.min);
		changed |= EditorWidget("Max corner", data.max);
		if (foldable)
			ImGui::TreePop();
	}
	return changed;
}

inline bool EditorWidget(const cstr label, Segment<v2f32>& data) {
	bool changed = false;
	ImGui::Text("%s", label);
	changed |= EditorWidget("A", data.A);
	changed |= EditorWidget("B", data.B);
	return changed;
}

inline bool EditorWidget(const cstr label, string data) {
	ImGui::Text("%s", label); ImGui::SameLine(); ImGui::Text("%s", data.data());
	return false;
}

template <typename T, typename U = int> struct has_name : std::false_type {};
template <typename T> struct has_name<T, decltype((void)T::name, 0)> : std::true_type {};

template<typename T> inline bool EditorWidgetArray(const cstr label, Array<T> data, auto widget, bool foldable = true) {
	bool changed = false;
	if (!foldable || ImGui::TreeNode(label)) {
		for (u32 i = 0; i < data.size(); i++) {
			if constexpr (has_name<T>::value) {
				string name = data[i].name;
				ImGui::PushID(i);
				changed |= widget(name.data(), data[i]);
				ImGui::PopID();
			} else {
				char label_buff[32];
				snprintf(label_buff, 32, "[%u]", i);
				ImGui::PushID(i);
				changed |= widget(label_buff, data[i]);
				ImGui::PopID();
			}
		}
		if (foldable)
			ImGui::TreePop();
	}
	return changed;
}

inline bool EditorWidget(const cstr label, Array<char> data) {
	return ImGui::InputText(label, data.data(), data.size());
}

template<typename T> inline bool EditorWidgetPtr(const cstr label, T* data, auto widget) {
	auto buff_size = snprintf(nullptr, 0, "%s : %p", label, data);
	char buffer[buff_size + 1];
	snprintf(buffer, buff_size, "%s : %p", label, data);
	return widget(buffer, *data);
}

#endif
