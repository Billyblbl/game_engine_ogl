##
## expects defined
## GLFW
## GLEW
## IMGUI
## GLM
## STB
## BUILD_DIR
##

CXX=g++

IMGUI_SRC += $(IMGUI)/imgui.cpp
IMGUI_SRC += $(IMGUI)/imgui_draw.cpp
IMGUI_SRC += $(IMGUI)/imgui_tables.cpp
IMGUI_SRC += $(IMGUI)/imgui_widgets.cpp
IMGUI_SRC += $(IMGUI)/imgui_demo.cpp

IMGUI_SRC += $(IMGUI)/backends/imgui_impl_opengl3.cpp
IMGUI_SRC += $(IMGUI)/backends/imgui_impl_glfw.cpp

SRC = main.cpp

INC = .
INC += $(IMGUI)
INC += $(GLM)
INC += $(STB)
INC += $(IMGUI)/backends
INC += $(GLFW)/include
INC += $(GLEW)/include
INC += $(BOX2D)/include

LIB = .
LIB += $(GLFW)/lib
LIB += $(GLEW)/lib

# LDFLAGS += -lvulkan-1
LDFLAGS += -lglfw3
LDFLAGS += -lglew32
LDFLAGS += -lopengl32
LDFLAGS += -lbox2d

CFLAGS = -g3 -std=c++20

APP_NAME=test_app

APP=$(BUILD_DIR)/test_app
EDITOR=$(BUILD_DIR)/test_editor
IMGUI_MODULE=$(BUILD_DIR)/imgui.o
APP_MODULE=$(APP:%=%.o)
EDITOR_MODULE=$(EDITOR:%=%.o)

all : app editor

$(BUILD_DIR):
	mkdir -p $@

$(IMGUI_MODULE): $(IMGUI_SRC) $(BUILD_DIR)
	cat $(IMGUI_SRC) | $(CXX) -c -x c++ $(CFLAGS) $(INC:%=-I%) -o $@ -

# $(APP_MODULE): $(SRC) $(BUILD_DIR)
# 	cat $(SRC) | $(CXX) -c -x c++ $(CFLAGS) $(INC:%=-I%) -o $@ -

$(APP_MODULE): $(SRC) $(BUILD_DIR)
	$(CXX) -c $(SRC) $(CFLAGS) $(INC:%=-I%) -o $@


$(APP): $(APP_MODULE) $(IMGUI_MODULE)
	$(CXX) $(CFLAGS) $(APP_MODULE) $(IMGUI_MODULE) $(INC:%=-I%) $(LIB:%=-L%) $(LDFLAGS) -o $@

$(EDITOR_MODULE): rect_editor.cpp $(BUILD_DIR)
	$(CXX) -c rect_editor.cpp $(CFLAGS) $(INC:%=-I%) -o $@

$(EDITOR): $(EDITOR_MODULE) $(IMGUI_MODULE)
	$(CXX) $(CFLAGS) $(EDITOR_MODULE) $(IMGUI_MODULE) $(INC:%=-I%) $(LIB:%=-L%) $(LDFLAGS) -o $@

app: $(APP)

editor: $(EDITOR)

clean:
	rm -rf $(BUILD_DIR)/*

re: clean all

.PHONY: all clean re $(APP_MODULE) editor
