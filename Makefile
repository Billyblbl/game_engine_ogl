##
## expects defined
## GLFW
## GLEW
## IMGUI
## GLM
## STB
## BUILD_DIR
## GXX_PATH
##

CXX=$(GXX_PATH)
CFLAGS = -g3 -std=c++20
INC = $(PWD)
LIB = $(PWD)

BLBLSTD_MODULE = $(BUILD_DIR)/blblstd.o

COLOR=\033[0;34m
NOCOLOR=\033[0m

default: app

include imgui.mk
include app.mk

$(BUILD_DIR):
	mkdir -p $@

$(BLBLSTD_MODULE):
	@cd blblstd && $(MAKE)

clean:
	rm -rf $(BUILD_DIR)

re: clean default

.PHONY: app clean re
