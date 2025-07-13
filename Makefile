#── Makefile ─────────────────────────────────────────────────
# Sources
CPP_SRCS := src/main.cpp src/config.cpp src/stratum.cpp \
            src/kawpow_host.cpp src/hashing.cpp
CU_SRCS  := src/kawpow.cu src/kernel.cu

# Tools & flags
CXX       := g++
NVCC      := nvcc
CUDA_ARCH := sm_80                            # adjust to your GPU
INCDIRS   := -Iinclude                        # your headers
CPPFLAGS  := -O3 -std=c++17 $(INCDIRS)        # C++ compile flags
CUFLAGS   := -O3 -std=c++17 -arch=$(CUDA_ARCH) $(INCDIRS)
LDFLAGS   := $(shell pkg-config --libs openssl) -lpthread

# Build artifacts
OBJ_DIR := build
CU_OBJS := $(CU_SRCS:src/%.cu=$(OBJ_DIR)/%.o)
CPP_OBJS:= $(CPP_SRCS:src/%.cpp=$(OBJ_DIR)/%.o)
ALL_OBJS:= $(CU_OBJS) $(CPP_OBJS)

# Target binary
TARGET := $(OBJ_DIR)/kawpow-miner

# Default
all: $(TARGET)

# Ensure build dir
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# CUDA objs
$(OBJ_DIR)/%.o: src/%.cu | $(OBJ_DIR)
	$(NVCC) $(CUFLAGS) -dc $< -o $@

# C++ objs
$(OBJ_DIR)/%.o: src/%.cpp | $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) -c $< -o $@

# Link
$(TARGET): $(ALL_OBJS)
	$(NVCC) $(CUFLAGS) $(ALL_OBJS) $(LDFLAGS) -o $@

clean:
	rm -rf $(OBJ_DIR)

.PHONY: all clean

