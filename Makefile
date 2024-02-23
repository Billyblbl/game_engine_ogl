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
CFLAGS += -O2
CFLAGS += -std=c++23
CFLAGS += -fno-exceptions
# CFLAGS += -finstrument-functions
# CFLAGS += -fsanitize=address
# CFLAGS += -fsanitize=memory
# CFLAGS += -fsanitize=undefined
# CFLAGS += -fsanitize=leak
# CFLAGS += -fsanitize=thread

#TODO check libFuzzer https://vimeo.com/855891054 -> automatic test coverage exploration
#CFLAGS += -fsanitize=fuzzer,address,memory,undefined,leak,thread

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
