#ifndef GIMGUI_EXTENSION
# define GIMGUI_EXTENSION

#include <span>
#include <cstring>
#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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
