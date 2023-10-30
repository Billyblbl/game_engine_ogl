// #include <spall/spall.h>

#include <application.cpp>
#include <playground_scene.cpp>
#include <spall/profiling.cpp>

i32 main(i32 ac, const cstrp av[]) {
	PROFILE_PROCESS("engine_test.spall");
	PROFILE_THREAD(1024 * 1024);
	PROFILE_SCOPE("Run");
	profile_scope_begin("OGL Window creation");
	auto app = create_app("Test editor", v2u32(1920, 1080), &editor_test); defer{ destroy(app); };
	if (!init_ogl(app))
		return 1;
	profile_scope_end();
	profile_scope_begin("Scene");
	while (app.scene != nullptr && app.scene(app)) {
		profile_scope_end();
		profile_scope_begin("Scene");
	}
	profile_scope_end();
	return 0;
}
