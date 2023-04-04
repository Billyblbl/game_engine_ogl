##
## expects defined
## IMGUI
## GLM
## STB
## IMGUI
## GLFW
## GLEW
## BOX2D
## BLBLSTD
##
##
##

APP_ROOT = main.cpp

APP_SRC = $(APP_ROOT)
APP_SRC += engine/application.cpp
APP_SRC += engine/b2d_debug_draw.cpp
APP_SRC += engine/buffer.cpp
APP_SRC += engine/framebuffer.cpp
APP_SRC += engine/glutils.cpp
APP_SRC += engine/imgui_extension.cpp
APP_SRC += engine/inputs.cpp
APP_SRC += engine/math.cpp
APP_SRC += engine/model.cpp
APP_SRC += engine/physics2D.cpp
APP_SRC += engine/rendering.cpp
APP_SRC += engine/textures.cpp
APP_SRC += engine/time.cpp
APP_SRC += engine/transform.cpp
APP_SRC += engine/vertex.cpp
APP_SRC += playground_scene.cpp
APP_SRC += top_down_controls.cpp
APP_SRC += entity.cpp
APP_SRC += sprite.cpp

INC += $(IMGUI)
INC += $(GLM)
INC += $(STB)
INC += $(IMGUI)/backends
INC += $(GLFW)/include
INC += $(GLEW)/include
INC += $(BOX2D)/include
INC += $(BLBLSTD)/src
INC += $(PWD)/engine

LIB += $(GLFW)/lib
LIB += $(GLEW)/lib

# LDFLAGS += -lvulkan-1
LDFLAGS += -lglfw3
LDFLAGS += -lglew32
LDFLAGS += -lopengl32
LDFLAGS += -lbox2d

APP_NAME=test_app
APP=$(BUILD_DIR)/$(APP_NAME)
APP_MODULE=$(APP:%=%.o)

app: $(APP)

$(APP_MODULE): $(BUILD_DIR) $(APP_SRC)
	@echo -e "Building $(COLOR)app module$(NOCOLOR)"
	@$(CXX) $(CFLAGS) -c $(APP_ROOT) $(INC:%=-I%) -o $@

$(APP): $(APP_MODULE) $(IMGUI_MODULE) $(BLBLSTD_MODULE)
	@echo -e "Linking $(COLOR)app executable$(NOCOLOR)"
	@$(CXX) $(CFLAGS) $^ $(INC:%=-I%) $(LIB:%=-L%) $(LDFLAGS) -o $@
