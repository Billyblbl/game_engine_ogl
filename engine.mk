##
## expects defined
## IMGUI
## GLM
## STB
## OPENAL
## GLFW
## GLEW
## BLBLSTD
##

BLBLGAME_SRC += engine/application.cpp
BLBLGAME_SRC += engine/buffer.cpp
BLBLGAME_SRC += engine/framebuffer.cpp
BLBLGAME_SRC += engine/glutils.cpp
BLBLGAME_SRC += engine/imgui_extension.cpp
BLBLGAME_SRC += engine/inputs.cpp
BLBLGAME_SRC += engine/math.cpp
BLBLGAME_SRC += engine/model.cpp
BLBLGAME_SRC += engine/physics_2d.cpp
BLBLGAME_SRC += engine/physics_2d_debug.cpp
BLBLGAME_SRC += engine/rendering.cpp
BLBLGAME_SRC += engine/textures.cpp
BLBLGAME_SRC += engine/time.cpp
BLBLGAME_SRC += engine/transform.cpp
BLBLGAME_SRC += engine/vertex.cpp
BLBLGAME_SRC += engine/sprite.cpp
BLBLGAME_SRC += engine/animation.cpp
BLBLGAME_SRC += engine/alutils.cpp
BLBLGAME_SRC += engine/audio.cpp
BLBLGAME_SRC += engine/audio_source.cpp
BLBLGAME_SRC += engine/audio_buffer.cpp
BLBLGAME_SRC += engine/entity.cpp
BLBLGAME_SRC += engine/system_editor.cpp

INC += $(PWD)/engine
INC += $(IMGUI)
INC += $(GLM)
INC += $(STB)
INC += $(OPENAL)
INC += $(IMGUI)/backends
INC += $(GLFW)/include
INC += $(GLEW)/include
INC += $(BLBLSTD)/src

LIB += $(GLFW)/lib
LIB += $(GLEW)/lib

LDFLAGS += -lglfw3
LDFLAGS += -lglew32
LDFLAGS += -lopengl32
LDFLAGS += -lopenal
