// #include <spall/spall.h>

#include <application.cpp>
#include <playground_scene.cpp>
#include <spall/profiling.cpp>

bool editor_test(App& app, u64 scene_id) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	profile_scope_begin("Initialisation");
	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };
	auto editor = create_editor("Editor", "Alt+X", { Input::KB::K_LEFT_ALT, Input::KB::K_X });
	auto rd = SystemEditor::create("Render", "Alt+R", { Input::KB::K_LEFT_ALT, Input::KB::K_R });
	auto au = SystemEditor::create("Audio", "Alt+O", { Input::KB::K_LEFT_ALT, Input::KB::K_O });
	auto ent = SystemEditor::create("Entities", "Alt+E", { Input::KB::K_LEFT_ALT, Input::KB::K_E });
	auto misc = SystemEditor::create("Misc", "Alt+M", { Input::KB::K_LEFT_ALT, Input::KB::K_M });
	auto ph = SystemEditor::create<Physics2D::Editor>("Physics2D", "Alt+P", { Input::KB::K_LEFT_ALT, Input::KB::K_P });
	auto scene = PlaygroundScene(); defer{ scene.release(); };
	profile_scope_end();

	while (update(app, scene_id)) {
		PROFILE_SCOPE("Frame");
		scene();
		system_keybinds({ &editor, &rd, &au, &ph, &ent, &misc });
		if (editor.show_window) {
			PROFILE_SCOPE("Editor");
			ImGui::NewFrame_OGL_GLFW(); defer{
				ImGui::Render();
				ImGui::Draw();
			};
			if (ImGui::BeginMainMenuBar()) {
				defer{ ImGui::EndMainMenuBar(); };
				system_menu("Windows", { &editor, &rd, &au, &ph, &ent, &misc });
				if (ImGui::BeginMenu("Actions")) {
					defer{ ImGui::EndMenu(); };
					if (ImGui::MenuItem("Break"))
						fprintf(stderr, "breaking!\n");
					if (ImGui::MenuItem("Restart"))
						return true;
				}
			}
			ImGui::DockSpaceOverViewport(ImGui::GetWindowViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
			scene.editor(rd, au, ph, ent, misc);
		}
	}
	return true;
}

i32 main(i32 ac, const cstrp av[]) {
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
