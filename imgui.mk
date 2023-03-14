##
## IMGUI
## BUILD_DIR
## GXX_PATH
##

IMGUI_ROOT=imgui_module.cpp

IMGUI_SRC = $(IMGUI_ROOT)
IMGUI_SRC += $(IMGUI)/imgui.cpp
IMGUI_SRC += $(IMGUI)/imgui_draw.cpp
IMGUI_SRC += $(IMGUI)/imgui_tables.cpp
IMGUI_SRC += $(IMGUI)/imgui_widgets.cpp
IMGUI_SRC += $(IMGUI)/imgui_demo.cpp
IMGUI_SRC += $(IMGUI)/backends/imgui_impl_opengl3.cpp
IMGUI_SRC += $(IMGUI)/backends/imgui_impl_glfw.cpp

INC += .
INC += $(IMGUI)
INC += $(IMGUI)/backends

imgui: $(IMGUI_MODULE)

IMGUI_MODULE=$(BUILD_DIR)/imgui.o

$(IMGUI_MODULE): $(BUILD_DIR) $(IMGUI_SRC)
	@echo -e "Building $(COLOR)imgui$(NOCOLOR)"
	@$(CXX) $(CFLAGS) -c $(IMGUI_ROOT) $(INC:%=-I%) -o $@
