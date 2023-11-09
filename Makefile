##
## expects defined
## GLFW
## GLEW
## IMGUI
## GLM
## STB
## BUILD_DIR
## GXX_PATH
## GCC_PATH
##

CXX=$(GXX_PATH)
CC=$(GCC_PATH)

CFLAGS += -g3
CFLAGS += -std=c++23
CFLAGS += -fno-exceptions
# CFLAGS += -finstrument-functions

INC = $(PWD)
LIB = $(PWD)

BLBLSTD_MODULE = $(BUILD_DIR)/blblstd.o

COLOR=\033[0;34m
NOCOLOR=\033[0m

default: app

# include editor.mk
include app.mk

$(BUILD_DIR):
	@echo -e "Init $(COLOR)build directory$(NOCOLOR)"
	@mkdir -p $@

$(BLBLSTD_MODULE): $(BUILD_DIR)
	@cd blblstd && $(MAKE)

clean:
	rm -rf $(BUILD_DIR)

re: clean default

.PHONY: app clean re $(BLBLSTD_MODULE)
