##
## TMX
## BUILD_DIR
## GXX_PATH
##

TMX_ROOT=tmx_module.c

TMX_SRC = $(TMX_ROOT)
TMX_SRC += $(TMX)/src/tmx_err.c
TMX_SRC += $(TMX)/src/tmx_hash.c
TMX_SRC += $(TMX)/src/tmx_mem.c
TMX_SRC += $(TMX)/src/tmx_utils.c
TMX_SRC += $(TMX)/src/tmx_xml.c
TMX_SRC += $(TMX)/src/tmx.c

INC += $(TMX)/src

TMX_MODULE=$(BUILD_DIR)/tmx.o

tmx: $(TMX_MODULE)

$(TMX_MODULE): $(BUILD_DIR) $(TMX_SRC)
	@echo -e "Building $(COLOR)tmx$(NOCOLOR)"
	@$(CC) $(CFLAGS) -c $(TMX_ROOT) $(INC:%=-I%) -o $@
