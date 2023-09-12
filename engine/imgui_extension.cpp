#ifndef GIMGUI_EXTENSION
# define GIMGUI_EXTENSION

#include <cstring>

#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <math.cpp>
#include <blblstd.hpp>

ImGuiConfigFlags DefaultImguiFlags = (
	ImGuiConfigFlags_NavEnableKeyboard |
	ImGuiConfigFlags_DockingEnable |
	ImGuiConfigFlags_ViewportsEnable
	);

namespace ImGui {

	void init_ogl_glfw(GLFWwindow* window, ImGuiConfigFlags flags = DefaultImguiFlags) {
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

	void shutdown_ogl_glfw() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void Draw(ImDrawData* data = null) {
		ImGui_ImplOpenGL3_RenderDrawData(data == null ? ImGui::GetDrawData() : data);
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void BeginFrame_OGL_GLFW() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void EndFrame_OGL_GLFW() {
		ImGui::Render();
	}

	auto GetWindowContentSize() {
		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();
		return ImVec2(max.x - min.x, max.y - min.y);
	}

	template<typename T> bool bit_flags(const cstr label, T& flags, Array<const string> bit_names, bool same_line = true) {
		bool changed = false;
		if (same_line) {
			ImGui::Text(label);
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

	template<typename T> bool bit_flags(const cstr label, T& flags, LiteralArray<string> bit_names) {
		return bit_flags(label, flags, larray(bit_names));
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
		auto window = GetWindowContentSize();
		auto ratios = ImVec2(window.x / dimensions.x, window.y / dimensions.y);
		return ImVec2(
			dimensions.x * min(ratios.x, ratios.y),
			dimensions.y * min(ratios.x, ratios.y)
		);
	}
}

bool EditorWidget(const cstr label, f32& data) {
	return ImGui::DragFloat(label, &data, .01f, .0f, .0f, "%.4f");
}

bool EditorWidget(const cstr label, v2f32& data) {
	return ImGui::DragFloat2(label, glm::value_ptr(data), .01f, .0f, .0f, "%.4f");
}

bool EditorWidget(const cstr label, v3f32& data) {
	return ImGui::DragFloat3(label, glm::value_ptr(data), .01f, .0f, .0f, "%.4f");
}

bool EditorWidget(const cstr label, v4f32& data) {
	return ImGui::DragFloat4(label, glm::value_ptr(data), .01f, .0f, .0f, "%.4f");
}

bool EditorWidget(const cstr label, i32& data) {
	return ImGui::DragInt(label, &data);
}

bool EditorWidget(const cstr label, v2i32& data) {
	return ImGui::DragInt2(label, glm::value_ptr(data));
}

bool EditorWidget(const cstr label, v3i32& data) {
	return ImGui::DragInt3(label, glm::value_ptr(data));
}

bool EditorWidget(const cstr label, v4i32& data) {
	return ImGui::DragInt4(label, glm::value_ptr(data));
}

bool EditorWidget(const cstr label, u32& data) {
	return EditorWidget(label, (i32&)data);
}

bool EditorWidget(const cstr label, u8& data) {
	u32 tmp = data;
	auto changed = EditorWidget(label, tmp);
	if (changed) data = tmp;
	return changed;
}

bool EditorWidget(const cstr label, u64& data) {
	u32 tmp = data;
	auto changed = EditorWidget(label, tmp);
	if (changed) data = tmp;
	return changed;
}

bool EditorWidget(const cstr label, v2u32& data) {
	return EditorWidget(label, (v2i32&)data);
}

bool EditorWidget(const cstr label, v3u32& data) {
	return EditorWidget(label, (v3i32&)data);
}

bool EditorWidget(const cstr label, v4u32& data) {
	return EditorWidget(label, (v4i32&)data);
}

bool EditorWidget(const cstr label, bool& data) {
	return ImGui::Checkbox(label, &data);
}

template<typename T> bool EditorWidget(const cstr label, reg_polytope<T>& data, bool embed = true) {
	bool changed = false;
	if (embed)
		ImGui::Text(label);
	if (embed || ImGui::TreeNode(label)) {
		changed |= EditorWidget("Min corner", data.min);
		changed |= EditorWidget("Max corner", data.max);
		if (!embed)
			ImGui::TreePop();
	}
	return changed;
}

bool EditorWidget(const cstr label, Segment<v2f32>& data) {
	bool changed = false;
	ImGui::Text(label);
	changed |= EditorWidget("A", data.A);
	changed |= EditorWidget("B", data.B);
	return changed;
}

bool EditorWidget(const cstr label, string data) {
	ImGui::Text(label); ImGui::SameLine(); ImGui::Text(data.data());
	return false;
}

template <typename T, typename U = int> struct has_name : std::false_type {};
template <typename T> struct has_name<T, decltype((void)T::name, 0)> : std::true_type {};

template<typename T> bool EditorWidget(const cstr label, Array<T> data, bool foldable = true) {
	bool changed = false;
	if (!foldable || ImGui::TreeNode(label)) {
		for (u32 i = 0; i < data.size(); i++) {
			if constexpr (has_name<T>::value) {
				string name = data[i].name;
				ImGui::PushID(i);
				changed |= EditorWidget((const cstrp)name.data(), data[i]);
				ImGui::PopID();
			} else {
				char label_buff[32];
				snprintf(label_buff, 32, "%u", i);
				ImGui::PushID(i);
				changed |= EditorWidget((const cstrp)label_buff, data[i]);
				ImGui::PopID();
			}
		}
		if (foldable)
			ImGui::TreePop();
	}
	return changed;
}

bool EditorWidget(const cstr label, Array<char> data) {
	return ImGui::InputText(label, data.data(), data.size());
}

template<typename T> bool EditorWidget(const cstr label, T* data) {
	auto change = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		ImGui::Text("ptr : %p", data);
		change |= EditorWidget("value", *data);
	}
	return change;
}

bool EditorWidget(const cstrp label, auto& unimplemented) {
	ImGui::Text("Unimplemented Widget for type %s", typeid(decltype(unimplemented)).name());
	return false;
}

#endif
