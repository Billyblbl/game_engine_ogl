##
## BUILD_DIR
## GXX_PATH
##

PROFILING_ROOT=spall/profiling.cpp

PROFILING_SRC = $(PROFILING_ROOT)

INC += spall

PROFILING_MODULE=$(BUILD_DIR)/profiling.o

profiling: $(PROFILING_MODULE)

$(PROFILING_MODULE): $(BUILD_DIR) $(PROFILING_SRC)
	@echo -e "Building $(COLOR)profiler$(NOCOLOR)"
	@$(CXX) -DPROFILING_IMPL $(CFLAGS) -c $(PROFILING_ROOT) $(INC:%=-I%) -o $@
