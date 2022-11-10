#ifndef GIMGUI_EXTENSION
# define GIMGUI_EXTENSION

#include <span>
#include <cstring>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

const char* getVersion() {
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 4.6 + GLSL 130
	//TODO check if we can use more up to date glsl version
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif
	return glsl_version;
}

ImGuiConfigFlags DefaultImguiFlags = (
	ImGuiConfigFlags_NavEnableKeyboard |
	ImGuiConfigFlags_DockingEnable |
	ImGuiConfigFlags_ViewportsEnable
	);

namespace ImGui {

	void Init_OGL_GLFW(GLFWwindow* window, ImGuiConfigFlags flags = DefaultImguiFlags) {
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
		ImGui_ImplOpenGL3_Init(getVersion());
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

}

bool EditorWidget(const char* label, float& data) {
	return ImGui::DragFloat(label, &data, .1f, .0f, .0f, "%.4f");
}

bool EditorWidget(const char* label, glm::f32vec2& data) {
	return ImGui::DragFloat2(label, glm::value_ptr(data), .1f, .0f, .0f, "%.4f");
}

bool EditorWidget(const char* label, glm::f32vec3& data) {
	return ImGui::DragFloat3(label, glm::value_ptr(data), .1f, .0f, .0f, "%.4f");
}

bool EditorWidget(const char* label, glm::f32vec4& data) {
	return ImGui::DragFloat4(label, glm::value_ptr(data), .1f, .0f, .0f, "%.4f");
}

bool EditorWidget(const char* label, int32_t& data) {
	return ImGui::DragInt(label, &data);
}

bool EditorWidget(const char* label, glm::i32vec2& data) {
	return ImGui::DragInt2(label, glm::value_ptr(data));
}

bool EditorWidget(const char* label, glm::i32vec3& data) {
	return ImGui::DragInt3(label, glm::value_ptr(data));
}

bool EditorWidget(const char* label, glm::i32vec4& data) {
	return ImGui::DragInt4(label, glm::value_ptr(data));
}

bool EditorWidget(const char* label, uint32_t& data) {
	return EditorWidget(label, (int32_t&)data);
}

bool EditorWidget(const char* label, glm::u32vec2& data) {
	return EditorWidget(label, (glm::i32vec2&)data);
}

bool EditorWidget(const char* label, glm::u32vec3& data) {
	return EditorWidget(label, (glm::i32vec3&)data);
}

bool EditorWidget(const char* label, glm::u32vec4& data) {
	return EditorWidget(label, (glm::i32vec4&)data);
}

template<typename T> bool EditorWidget(const char* label, std::span<T> data) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		for (uint32_t i = 0; i < data.size(); i++) {
			char labelBuff[32];
			snprintf(labelBuff, 32, "%u", i);
			ImGui::PushID(i);
			auto& element = data[i];
			changed |= EditorWidget((const char*)labelBuff, element);
			ImGui::PopID();
		}
		ImGui::TreePop();
	}
	return changed;
}

template<> bool EditorWidget<char>(const char* label, std::span<char> data) {
	return ImGui::InputText(label, data.data(), data.size());
}

#endif
