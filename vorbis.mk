#
#! This is its own module because otherwise we have #define collisions with <windows.h> specific stuff
#

VORBIS_SRC=$(STB)/stb_vorbis.c

VORBIS_MODULE=$(BUILD_DIR)/vorbis.o

vorbis: $(VORBIS_MODULE)

$(VORBIS_MODULE): $(BUILD_DIR) $(VORBIS_SRC)
	@echo -e "Building $(COLOR)vorbis$(NOCOLOR)"
	@$(CXX) $(CFLAGS) -c $(VORBIS_SRC) $(INC:%=-I%) -o $@
