#ifndef GRECT_EDITOR
# define GRECT_EDITOR

#include <application.cpp>
#include <imgui_extension.cpp>
#include <utils.cpp>
#include <physics2d.cpp>
#include <transform.cpp>
#include <textures.cpp>
#include <framebuffer.cpp>
#include <blblstd.hpp>

i32 main(i32 ac, const cstrp av[]) {
	auto app = create_app("Test renderer", v2u32(1920 / 2, 1080 / 2), null);
	defer{ destroy(app); };
	if (!init_ogl(app))
		return 1;

	ImGui::init_OGL_GLFW(app.window);
	defer{ ImGui::Shutdown_OGL_GLFW(); };

	struct {
		OrthoCamera ortho = { v3f32(16, 9, 10) };
		Transform2D transform;
	} camera;

	struct {
		Textures::Texture texture;
		Framebuffer framebuffer;
	} scene_panel;
	scene_panel.texture = Textures::allocate(Textures::ImageFormatFromPixelType<v4f32>, app.pixel_dimensions);
	scene_panel.framebuffer = create_framebuffer(scene_panel.texture);

	while (update(app, null)) {
		Input::poll(app.inputs);
		GL_GUARD(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		render(scene_panel.framebuffer.id, { v2u32(0), scene_panel.texture.dimensions }, GL_COLOR_BUFFER_BIT, []() {});

		ImGui::BeginFrame_OGL_GLFW();

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New", "Ctrl+N")) {}
				if (ImGui::MenuItem("Open", "Ctrl+O")) {}
				if (ImGui::MenuItem("Save as", "Ctrl+Shift+S")) {}
				if (ImGui::MenuItem("Save", "Ctrl+S")) {}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		ImGui::Begin("Scene");
		ImGui::Image((ImTextureID)scene_panel.texture.id, ImGui::GetWindowContentSize());
		auto scenePanelSize = ImGui::GetWindowSize();
		camera.ortho.dimensions.x = scenePanelSize.x;
		camera.ortho.dimensions.y = scenePanelSize.y;
		ImGui::End();

		ImGui::Begin("Data");
		if (ImGui::TreeNode("Camera")) {
			EditorWidget("Format", camera.ortho);
			EditorWidget("Transform", camera.transform);
			ImGui::TreePop();
		}
		ImGui::End();

		ImGui::EndFrame_OGL_GLFW();

		render(0, { v2u32(0), app.pixel_dimensions }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, ImGui::Draw);
	}
	return 0;
}

#endif
