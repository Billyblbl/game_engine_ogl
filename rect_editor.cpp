#ifndef GRECT_EDITOR
# define GRECT_EDITOR

#include <application.cpp>
#include <imgui_extension.cpp>
#include <utils.cpp>
#include <physics2d.cpp>
#include <transform.cpp>

bool rect_editor(App& app) {

	ImGui::Init_OGL_GLFW(app.window);
	deferDo{ ImGui::Shutdown_OGL_GLFW(); };

	struct {
		OrthoCamera ortho;
		Transform2D transform;
	} camera = { OrthoCamera{ glm::vec3(app.pixelDimensions, -1) }, {} };

	auto world = b2World(b2Vec2(0.f, 0.f));
	auto debugDraw = B2dDebugDraw();
	world.SetDebugDraw(&debugDraw);
	debugDraw.SetFlags(b2Draw::e_shapeBit);

	while (update(app, __func__)) {

		Input::poll(app.inputs);
		GL_GUARD(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		{
			ImGui::BeginFrame_OGL_GLFW();

			ImGui::Begin("Test window");



			ImGui::End();

			ImGui::EndFrame_OGL_GLFW();
			ImGui::Draw();
		}

		debugDraw.viewTransform = camera.ortho.matrix() * glm::inverse(camera.transform.matrix());
		world.DebugDraw();

	}
	return false;
}


int main(int ac, char** av) {
	auto app = createApp("Test renderer", glm::uvec2(1920 / 2, 1080 / 2), 1);
	deferDo{ destroyApp(app); };
	app.focusPath = "rect_editor";
	if (!initOpenGL(app)) return 1;
	rect_editor(app);
	return 0;
}

#endif
