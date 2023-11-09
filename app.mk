##
## expects defined
## IMGUI
## GLM
## STB
## GLFW
## GLEW
## OPENAL
## BLBLSTD
## BLBLGAME_SRC
##

APP_ROOT = main.cpp

APP_SRC = $(APP_ROOT)
APP_SRC += $(BLBLGAME_SRC)
APP_SRC += playground_scene.cpp
APP_SRC += top_down_controls.cpp
APP_SRC += sidescroll_controls.cpp
APP_SRC += texture_shape_generation.cpp

APP_NAME=test_app
APP=$(BUILD_DIR)/$(APP_NAME)
APP_MODULE=$(APP:%=%.o)

include imgui.mk
include engine.mk
include vorbis.mk
include profiling.mk
include tmx.mk

app: $(APP)

$(APP_MODULE): $(BUILD_DIR) $(APP_SRC)
	@echo -e "Building $(COLOR)app module$(NOCOLOR)"
	@$(CXX) $(CFLAGS) -c $(APP_ROOT) $(INC:%=-I%) -o $@

$(APP): $(APP_MODULE) $(VORBIS_MODULE) $(IMGUI_MODULE) $(BLBLSTD_MODULE) $(PROFILING_MODULE) $(TMX_MODULE)
	@echo -e "Linking $(COLOR)app executable$(NOCOLOR)"
	@$(CXX) $(CFLAGS) $^ $(LIB:%=-L%) $(LDFLAGS) -o $@
