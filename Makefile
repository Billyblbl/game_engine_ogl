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
CFLAGS = -g3 -std=c++20 -fno-exceptions
INC = $(PWD)
LIB = $(PWD)

BLBLSTD_MODULE = $(BUILD_DIR)/blblstd.o

INC = .

COLOR=\033[0;34m
NOCOLOR=\033[0m

default: app

include imgui.mk
include engine.mk
include editor.mk
include vorbis.mk
include app.mk

$(BUILD_DIR):
	mkdir -p $@

$(BLBLSTD_MODULE):
	@cd blblstd && $(MAKE)

clean:
	rm -rf $(BUILD_DIR)

re: clean default

.PHONY: app clean re $(BLBLSTD_MODULE)
