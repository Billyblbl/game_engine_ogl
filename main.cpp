#include <application.cpp>
#include <playground_scene.cpp>

i32 main(i32 ac, const cstrp av[]) {
	auto app = create_app("Test renderer", v2u32(1920 / 2, 1080 / 2), playground);
	defer{ destroy(app); };
	if (!init_ogl(app))
		return 1;
	while (app.scene != nullptr && app.scene(app));
	return 0;
}
