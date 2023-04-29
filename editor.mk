##
## expects defined
## IMGUI
## GLM
## STB
## IMGUI
## GLFW
## GLEW
## OPENAL
## BLBLSTD
## BLBLGAME_SRC
##

ED_ROOT = editor.cpp

ED_SRC = $(ED_ROOT)
ED_SRC += $(BLBLGAME_SRC)

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
