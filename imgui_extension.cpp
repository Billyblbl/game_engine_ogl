#ifndef GIMGUI_EXTENSION
# define GIMGUI_EXTENSION

#include <span>
#include <cstring>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <math.cpp>

ImGuiConfigFlags DefaultImguiFlags = (
	ImGuiConfigFlags_NavEnableKeyboard |
	ImGuiConfigFlags_DockingEnable |
	ImGuiConfigFlags_ViewportsEnable
);

namespace ImGui {

	void init_OGL_GLFW(GLFWwindow* window, ImGuiConfigFlags flags = DefaultImguiFlags) {
		printf("Initializing DearImgui\n");
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
		ImGui_ImplOpenGL3_Init("#version 430");
	}

	void Shutdown_OGL_GLFW() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void Draw() {
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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


}

bool EditorWidget(const cstr label, float& data) {
	return ImGui::DragFloat(label, &data, .1f, .0f, .0f, "%.4f");
}

bool EditorWidget(const cstr label, v2f32& data) {
	return ImGui::DragFloat2(label, glm::value_ptr(data), .1f, .0f, .0f, "%.4f");
}

bool EditorWidget(const cstr label, v3f32& data) {
	return ImGui::DragFloat3(label, glm::value_ptr(data), .1f, .0f, .0f, "%.4f");
}

bool EditorWidget(const cstr label, v4f32& data) {
	return ImGui::DragFloat4(label, glm::value_ptr(data), .1f, .0f, .0f, "%.4f");
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

bool EditorWidget(const cstr label, v2u32& data) {
	return EditorWidget(label, (v2i32&)data);
}

bool EditorWidget(const cstr label, v3u32& data) {
	return EditorWidget(label, (v3i32&)data);
}

bool EditorWidget(const cstr label, v4u32& data) {
	return EditorWidget(label, (v4i32&)data);
}

template<typename T> bool EditorWidget(const cstr label, Array<T> data) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		for (u32 i = 0; i < data.size(); i++) {
			char label_buff[32];
			snprintf(label_buff, 32, "%u", i);
			ImGui::PushID(i);
			changed |= EditorWidget((const cstr)label_buff, data[i]);
			ImGui::PopID();
		}
		ImGui::TreePop();
	}
	return changed;
}

template<> bool EditorWidget<char>(const cstr label, Array<char> data) {
	return ImGui::InputText(label, data.data(), data.size());
}

#endif
