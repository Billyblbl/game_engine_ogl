// #include <spall/spall.h>

#include <application.cpp>
#include <playground_scene.cpp>
#include <spall/profiling.cpp>
#include <system_editor.cpp>

bool editor_test(App& app, u64 scene_id) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };
	auto playground_scene = PlaygroundScene::create(); defer{ playground_scene.release(); };

	profile_scope_begin("Frame");
	while (update(app, scene_id)) {
		defer{ profile_scope_restart("Frame"); };
		ImGui::NewFrame_OGL_GLFW();
		ImGui::DockSpaceOverViewport(ImGui::GetWindowViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
		playground_scene(true);
		ImGui::Render();
		ImGui::Draw();
	}
	return true;
}

i32 main() {
	enum SceneID : u64 {
		EXIT = App::ID_EXIT,
		PLAYGROUND,
	};
	PROFILE_PROCESS("engine_test.spall");
	PROFILE_THREAD(1024 * 1024);
	PROFILE_SCOPE("Run");
	auto app = App::create("Test editor", v2u32(1920, 1080), PLAYGROUND); defer{ app.release(); };
	if (!init_ogl(app))
		return 1;
	while (app.scene_id != App::ID_EXIT)
		editor_test(app, PLAYGROUND);
	return 0;
}
