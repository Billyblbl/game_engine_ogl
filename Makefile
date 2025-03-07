include env.mk

CXX=$(CXX_PATH)
CC=$(CC_PATH)

export CFLAGS
export CXXFLAGS

CFLAGS += -g3
# CFLAGS += -gcodeview
# CFLAGS += -O2

CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror

CFLAGS += -Wno-misleading-indentation
CFLAGS += -Wno-unused-command-line-argument
CFLAGS += -Wno-vla-cxx-extension
CFLAGS += -Wno-error=deprecated-declarations

SAN_FLAGS += -fsanitize=address
SAN_FLAGS += -fsanitize=undefined
# SAN_FLAGS += -fsanitize-undefined-trap-on-error

# CFLAGS += -finstrument-functions
# CFLAGS += -fsanitize=leak
# CFLAGS += -fsanitize=memory
# CFLAGS += -fsanitize=thread
# CFLAGS += -fsanitize=fuzzer

ifdef SANITIZE
	CFLAGS += $(SAN_FLAGS)
endif

CXXFLAGS = $(CFLAGS)
CXXFLAGS += -std=c++23
CXXFLAGS += -fno-exceptions
#TODO check libFuzzer https://vimeo.com/855891054 -> automatic test coverage exploration

INC = $(CURDIR) $(CXX_HEADERS)

LIB = $(CURDIR)

BLBLSTD_MODULE = $(BUILD_DIR)/blblstd.o

COLOR=\033[0;34m
NOCOLOR=\033[0m

default:
	@$(MAKE) -j8 app

configure:
	@echo -e "fetching $(COLOR)submodules$(NOCOLOR) dependencies"
	@git submodule update --init --recursive
	@echo -e "fetching $(COLOR)packages$(NOCOLOR) dependencies"
	@$(call install_dependencies)

#* imgui

IMGUI_ROOT=imgui_module.cpp

IMGUI_SRC = $(IMGUI_ROOT)
IMGUI_SRC += $(IMGUI)/imgui.cpp
IMGUI_SRC += $(IMGUI)/imgui_draw.cpp
IMGUI_SRC += $(IMGUI)/imgui_tables.cpp
IMGUI_SRC += $(IMGUI)/imgui_widgets.cpp
IMGUI_SRC += $(IMGUI)/imgui_demo.cpp
IMGUI_SRC += $(IMGUI)/backends/imgui_impl_opengl3.cpp
IMGUI_SRC += $(IMGUI)/backends/imgui_impl_glfw.cpp

INC += $(IMGUI)
INC += $(IMGUI)/backends

IMGUI_MODULE=$(BUILD_DIR)/imgui.o

imgui:
	@echo -e "Rebuilding $(COLOR)imgui$(NOCOLOR)"
	@rm -f $(IMGUI_MODULE)
	@$(MAKE) $(IMGUI_MODULE)

$(IMGUI_MODULE): $(BUILD_DIR) $(IMGUI_SRC)
	@echo -e "Building $(COLOR)imgui$(NOCOLOR)"
	@$(CXX) $(CXXFLAGS) -c $(IMGUI_ROOT) $(INC:%=-I%) -o $@

#* /imgui

#* engine

#* core

CORE_ROOT=engine/core_module.cpp

CORE_SRC += engine/math.cpp
CORE_SRC += engine/imgui_extension.cpp
CORE_SRC += engine/inputs.cpp
CORE_SRC += engine/time.cpp
CORE_SRC += engine/system_editor.cpp # TODO reevaluate the value of the stuff in there
CORE_SRC += engine/entity.cpp # TODO reevaluate the value of the stuff in there
CORE_SRC += engine/transform.cpp

BLBLGAME_SRC += $(CORE_SRC)

INC += $(GLM)
INC += $(STB)

CORE_MODULE=$(BUILD_DIR)/core.o

core:
	@echo -e "Rebuilding $(COLOR)core$(NOCOLOR)"
	@rm -f $(CORE_MODULE)
	@$(MAKE) $(CORE_MODULE)


$(CORE_MODULE): $(BUILD_DIR) $(CORE_SRC)
	@echo -e "Building $(COLOR)core$(NOCOLOR)"
	@$(CXX) $(CXXFLAGS) -c $(CORE_ROOT) $(INC:%=-I%) -o $@

#* /core

#* gfx

GFX_ROOT=engine/gfx_module.cpp

GFX_SRC += engine/application.cpp
GFX_SRC += engine/buffer.cpp
GFX_SRC += engine/pipeline.cpp
GFX_SRC += engine/framebuffer.cpp
GFX_SRC += engine/glutils.cpp
GFX_SRC += engine/glresource.cpp
GFX_SRC += engine/model.cpp
GFX_SRC += engine/rendering.cpp
GFX_SRC += engine/textures.cpp
GFX_SRC += engine/vertex.cpp
GFX_SRC += engine/atlas.cpp

BLBLGAME_SRC += $(GFX_SRC)

INC += $(GLFW)/include
LIB += $(GLFW)/lib
LDFLAGS += -lglfw3

INC += $(GLEW)/include
LIB += $(GLEW)/lib
LDFLAGS += -lglew32
LDFLAGS += -lopengl32

GFX_MODULE=$(BUILD_DIR)/gfx.o

gfx:
	@echo -e "Rebuilding $(COLOR)gfx$(NOCOLOR)"
	@rm -f $(GFX_MODULE)
	@$(MAKE) $(GFX_MODULE)


$(GFX_MODULE): $(BUILD_DIR) $(GFX_SRC)
	@echo -e "Building $(COLOR)gfx$(NOCOLOR)"
	@$(CXX) $(CXXFLAGS) -c $(GFX_ROOT) $(INC:%=-I%) -o $@

#* /gfx

#* audio

AUDIO_ROOT=engine/audio_module.cpp

AUDIO_SRC += engine/alutils.cpp
AUDIO_SRC += engine/audio.cpp
AUDIO_SRC += engine/audio_source.cpp
AUDIO_SRC += engine/audio_buffer.cpp

BLBLGAME_SRC += $(AUDIO_SRC)

INC += $(OPENAL)
LDFLAGS += -lopenal

AUDIO_MODULE=$(BUILD_DIR)/audio.o

audio:
	@echo -e "Rebuilding $(COLOR)audio$(NOCOLOR)"
	@rm -f $(AUDIO_MODULE)
	@$(MAKE) $(AUDIO_MODULE)

$(AUDIO_MODULE): $(BUILD_DIR) $(AUDIO_SRC)
	@echo -e "Building $(COLOR)audio$(NOCOLOR)"
	@$(CXX) $(CXXFLAGS) -c $(AUDIO_ROOT) $(INC:%=-I%) -o $@

#* /audio

#* physics

PHYSICS_ROOT=engine/physics_module.cpp

PHYSICS_SRC += engine/polygon.cpp
PHYSICS_SRC += engine/physics_2d.cpp
PHYSICS_SRC += engine/shape_2d.cpp
PHYSICS_SRC += engine/physics_2d_debug.cpp

BLBLGAME_SRC += $(PHYSICS_SRC)

PHYSICS_MODULE=$(BUILD_DIR)/physics.o

physics:
	@echo -e "Rebuilding $(COLOR)physics$(NOCOLOR)"
	@rm -f $(PHYSICS_MODULE)
	@$(MAKE) $(PHYSICS_MODULE)


$(PHYSICS_MODULE): $(BUILD_DIR) $(PHYSICS_SRC)
	@echo -e "Building $(COLOR)physics$(NOCOLOR)"
	@$(CXX) $(CXXFLAGS) -c $(PHYSICS_ROOT) $(INC:%=-I%) -o $@

#* /physics

#* misc

MISC_ROOT=engine/misc_module.cpp

BLBLGAME_SRC += engine/animation.cpp
BLBLGAME_SRC += engine/sprite.cpp
BLBLGAME_SRC += engine/text.cpp
BLBLGAME_SRC += engine/tilemap.cpp

INC += $(XML2)/include/libxml2
LIB += $(XML2)/lib
LDFLAGS += -lxml2

INC += $(FREETYPE)/include/freetype2
LIB += $(FREETYPE)/lib
LDFLAGS += -lfreetype

MISC_MODULE=$(BUILD_DIR)/misc.o

misc:
	@echo -e "Rebuilding $(COLOR)misc$(NOCOLOR)"
	@rm -f $(MISC_MODULE)
	@$(MAKE) $(MISC_MODULE)


$(MISC_MODULE): $(BUILD_DIR) $(BLBLGAME_SRC)
	@echo -e "Building $(COLOR)misc$(NOCOLOR)"
	@$(CXX) $(CXXFLAGS) -c $(MISC_ROOT) $(INC:%=-I%) -o $@

#* /misc

INC += $(CURDIR)/engine
INC += $(BLBLSTD)/src
#*/ engine

#* vorbis
#! This is its own module because otherwise we have #define collisions with <windows.h> specific stuff

VORBIS_SRC=$(STB)/stb_vorbis.c

VORBIS_MODULE=$(BUILD_DIR)/vorbis.o

vorbis:
	@echo -e "Rebuilding $(COLOR)vorbis$(NOCOLOR)"
	@rm -f $(VORBIS_MODULE)
	@$(MAKE) $(VORBIS_MODULE)


$(VORBIS_MODULE): $(BUILD_DIR) $(VORBIS_SRC)
	@echo -e "Building $(COLOR)vorbis$(NOCOLOR)"
	@$(CC) $(CFLAGS) -c $(VORBIS_SRC) $(INC:%=-I%) -o $@

#*/ vorbis

#* profiling

PROFILING_ROOT=spall/profiling.cpp

PROFILING_SRC = $(PROFILING_ROOT)

INC += spall
INC += $(DLFCN)/include
LIB += $(DLFCN)/lib
LDFLAGS += -ldl

PROFILING_MODULE=$(BUILD_DIR)/profiling.o

profiling:
	@echo -e "Rebuilding $(COLOR)profiling$(NOCOLOR)"
	@rm -f $(PROFILING_MODULE)
	@$(MAKE) $(PROFILING_MODULE)


$(PROFILING_MODULE): $(BUILD_DIR) $(PROFILING_SRC)
	@echo -e "Building $(COLOR)profiler$(NOCOLOR)"
	@$(CXX) -DPROFILING_IMPL $(CXXFLAGS) -c $(PROFILING_ROOT) $(INC:%=-I%) -o $@

#*/ profiling

#* tmx

TMX_ROOT=tmx_module.c

TMX_SRC = $(TMX_ROOT)
TMX_SRC += $(TMX)/src/tmx_err.c
TMX_SRC += $(TMX)/src/tmx_hash.c
TMX_SRC += $(TMX)/src/tmx_mem.c
TMX_SRC += $(TMX)/src/tmx_utils.c
TMX_SRC += $(TMX)/src/tmx_xml.c
TMX_SRC += $(TMX)/src/tmx.c

INC += $(TMX)/src
INC += $(XML2)/include/libxml2
LIB += $(XML2)/lib
LDFLAGS += -lxml2

TMX_MODULE=$(BUILD_DIR)/tmx.o

tmx:
	@echo -e "Rebuilding $(COLOR)tmx$(NOCOLOR)"
	@rm -f $(TMX_MODULE)
	@$(MAKE) $(TMX_MODULE)


$(TMX_MODULE): $(BUILD_DIR) $(TMX_SRC)
	@echo -e "Building $(COLOR)tmx$(NOCOLOR)"
	@$(CC) $(CFLAGS) -c $(TMX_ROOT) $(INC:%=-I%) -o $@

#*/ tmx

#* app module

APP_ROOT = game/main.cpp

APP_SRC = $(APP_ROOT)
APP_SRC += $(BLBLGAME_SRC)
APP_SRC += game/playground_scene.cpp
APP_SRC += game/top_down_controls.cpp
APP_SRC += game/sidescroll_controls.cpp
APP_SRC += game/texture_shape_generation.cpp

INC += game

APP_NAME=test_app
APP=$(BUILD_DIR)/$(APP_NAME)
APP_MODULE=$(APP:%=%.o)

app_module:
	@echo -e "Rebuilding $(COLOR)app_module$(NOCOLOR)"
	@rm -f $(APP_MODULE)
	@$(MAKE) $(APP_MODULE)

$(APP_MODULE): $(BUILD_DIR) $(APP_SRC)
	@echo -e "Building $(COLOR)app module$(NOCOLOR)"
	@$(CXX) $(CXXFLAGS) -c $(APP_ROOT) $(INC:%=-I%) -o $@

#* /app module

#* app

app: $(APP)

$(APP): $(APP_MODULE) $(VORBIS_MODULE) $(IMGUI_MODULE) $(BLBLSTD_MODULE) $(PROFILING_MODULE) $(TMX_MODULE)
	@echo -e "Linking $(COLOR)app executable$(NOCOLOR)"
	@$(CXX) $(CXXFLAGS) $^ $(LIB:%=-L%) $(LDFLAGS) -o $@

#*/ app

$(BUILD_DIR):
	@echo -e "Init $(COLOR)build directory$(NOCOLOR)"
	@mkdir -p $@

$(BLBLSTD_MODULE): $(BUILD_DIR)
	@cd blblstd && $(MAKE)

blblstd:
	@echo -e "Rebuilding $(COLOR)blblstd$(NOCOLOR)"
	@rm -f $(BLBLSTD_MODULE)
	@$(MAKE) $(BLBLSTD_MODULE)

clean:
	rm -rf $(BUILD_DIR)

re: clean
	$(MAKE) default

.PHONY: app clean re default tmx imgui vorbis profiling app_module blblstd core gfx misc audio physics $(BLBLSTD_MODULE) dep
