##
## expects defined
## IMGUI
## GLM
## STB
## IMGUI
## GLFW
## GLEW
## BOX2D
## OPENAL
## BLBLSTD
##
##
##

ED_ROOT = editor.cpp

ED_SRC = $(ED_ROOT)
ED_SRC += engine/application.cpp
ED_SRC += engine/b2d_debug_draw.cpp
ED_SRC += engine/buffer.cpp
ED_SRC += engine/framebuffer.cpp
ED_SRC += engine/glutils.cpp
ED_SRC += engine/imgui_extension.cpp
ED_SRC += engine/inputs.cpp
ED_SRC += engine/math.cpp
ED_SRC += engine/model.cpp
ED_SRC += engine/physics2D.cpp
ED_SRC += engine/rendering.cpp
ED_SRC += engine/textures.cpp
ED_SRC += engine/time.cpp
ED_SRC += engine/transform.cpp
ED_SRC += engine/vertex.cpp
ED_SRC += engine/sprite.cpp
ED_SRC += engine/animation.cpp
ED_SRC += engine/alutils.cpp
ED_SRC += engine/audio.cpp
ED_SRC += engine/audio_source.cpp
ED_SRC += engine/audio_buffer.cpp

INC += $(IMGUI)
INC += $(GLM)
INC += $(STB)
INC += $(OPENAL)
INC += $(IMGUI)/backends
INC += $(GLFW)/include
INC += $(GLEW)/include
INC += $(BOX2D)/include
INC += $(BLBLSTD)/src
INC += $(PWD)/engine

LDFLAGS += -lglfw3
LDFLAGS += -lglew32
LDFLAGS += -lopengl32
LDFLAGS += -lopenal
LDFLAGS += -lbox2d

ED_NAME=blbled
ED=$(BUILD_DIR)/$(ED_NAME)
ED_MODULE=$(ED:%=%.o)

ed: $(ED)

$(ED_MODULE): $(BUILD_DIR) $(ED_SRC)
	@echo -e "Building $(COLOR)editor module$(NOCOLOR)"
	@$(CXX) $(CFLAGS) -c $(ED_ROOT) $(INC:%=-I%) -o $@

$(ED): $(ED_MODULE) $(IMGUI_MODULE) $(BLBLSTD_MODULE)
	@echo -e "Linking $(COLOR)editor executable$(NOCOLOR)"
	@$(CXX) $(CFLAGS) $^ $(INC:%=-I%) $(LIB:%=-L%) $(LDFLAGS) -o $@
