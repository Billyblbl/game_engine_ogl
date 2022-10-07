#include <utils.cpp>
#include <application.cpp>
#include <playground_scene.cpp>

#include <unordered_map>
#include <string_view>


int main(int ac, char** av) {
	auto app = createApp("Test renderer", glm::uvec2(1920 / 2, 1080 / 2), 1);
	deferDo{ destroyApp(app); };
	app.focusPath = "playground";
	std::unordered_map<std::string_view, bool(*)(App&)> scenes = {
		{ "playground" , playground },
		{ "" , exitApp }
	};
	if (!initOpenGL(app)) return 1;
	while (scenes[app.focusPath] != nullptr && scenes[app.focusPath](app));
	return 0;
}
