// #include <spall/spall.h>

#define PROFILE_TRACE_ON
#include <application.cpp>
#include <playground_scene.cpp>
#include <spall/profiling.cpp>
#include <system_editor.cpp>

bool engine_test(App& app, u64 scene_id) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };
	// auto playground_scene = PlaygroundScene::create(); defer{ playground_scene.release(); };
	auto scene = RefactorScene::create(GLScope::global(), app.window);

	wait_gpu();

	PROFILE_SCOPE("Frame");
	while (update(app, scene_id)) {
		defer{ profile_scope_restart("Frame"); };
		ImGui::NewFrame_OGL_GLFW();
		ImGui::DockSpaceOverViewport(ImGui::GetWindowViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
		// playground_scene(true);
		scene(app.window, true);
		ImGui::Render();
		ImGui::Draw();
	}
	return true;
}

i32 main() {
	defer { GLScope::global().release(); };
	enum SceneID : u64 {
		EXIT = App::ID_EXIT,
		PLAYGROUND,
	};
	PROFILE_PROCESS("engine_test.spall");
	PROFILE_THREAD(1024 * 1024);
	PROFILE_SCOPE("Run");
	auto app = App::create("Test engine", v2u32(1920, 1080), PLAYGROUND); defer{ app.release(); };
	if (!init_ogl())
		return 1;
	while (app.scene_id != EXIT)
		engine_test(app, PLAYGROUND);
	return 0;
}
