// #include <spall/spall.h>

#define PROFILE_TRACE_ON
#include <application.cpp>
#include <playground_scene.cpp>
#include <spall/profiling.cpp>
#include <system_editor.cpp>

bool engine_test(App& app) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };
	auto scene = RefactorScene::create(GLScope::global());

	glFinish();

	PROFILE_SCOPE("Frame");
	while (app.update()) {
		defer{ profile_scope_restart("Frame"); };
		ImGui::NewFrame_OGL_GLFW();
		ImGui::DockSpaceOverViewport(0, ImGui::GetWindowViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
		auto [drawn, target] = scene(true);

		start_render_pass(window_renderpass(app.window)); {
			clear({
				.attachements = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
				.color = { 0.0f, 0.0f, 0.0f, 1.0f },
				.depth = 1.0f,
				.stencil = 0
			});
			blit_fb(target, window_render_target(app.window), drawn, flex_viewport(app.pixel_dimensions, drawn.size()));
			ImGui::Draw();
		}
	}
	return false;
}

i32 main() {
	PROFILE_PROCESS("engine_test.spall");
	PROFILE_THREAD(1024 * 1024);
	PROFILE_SCOPE("Run");
	defer { GLScope::global().release(); };
	auto app = App::create("Test engine", v2u32(1920, 1080)); defer{ app.release(); };
	if (!init_ogl())
		return 1;
	while (engine_test(app));
	return 0;
}
