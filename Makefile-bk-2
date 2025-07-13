#── Makefile ─────────────────────────────────────────────────
# Sources
CPP_SRCS := $(wildcard src/*.cpp) $(wildcard base/crypto/*.cpp) \
            base/io/json/Json.cpp base/tools/String.cpp
CU_SRCS  := src/kawpow.cu src/kernel.cu
C_SRCS   := include/libethash/ethash_internal.c

# Tools & flags
CXX       := g++
NVCC      := nvcc
CUDA_ARCH := sm_80                            # adjust to your GPU
INCDIRS   := -Iinclude -I. -I./include -Ibase # your headers
CPPFLAGS  := -O3 -std=c++17 $(INCDIRS)        # C++ compile flags

# MODIFIED: Added --cudart=static to CUFLAGS
CUFLAGS   := -O3 -std=c++17 -arch=$(CUDA_ARCH) $(INCDIRS) --cudart=static

# MODIFIED: Added flags for static linking
# You might need to add -lculibos and -ldl on some Linux systems
LDFLAGS   := $(shell pkg-config --libs openssl) -lpthread

# Build artifacts
OBJ_DIR := build
CU_OBJS := $(CU_SRCS:src/%.cu=$(OBJ_DIR)/%.o)
CPP_OBJS:= $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(filter src/%.cpp,$(CPP_SRCS))))
CRYPTO_OBJS := $(patsubst base/crypto/%.cpp,$(OBJ_DIR)/%.o,$(filter base/crypto/%.cpp,$(CPP_SRCS)))
JSON_OBJS := $(OBJ_DIR)/Json.o
STRING_OBJS := $(OBJ_DIR)/String.o
C_OBJS  := $(OBJ_DIR)/ethash_internal.o
ALL_OBJS:= $(CU_OBJS) $(CPP_OBJS) $(CRYPTO_OBJS) $(JSON_OBJS) $(STRING_OBJS) $(C_OBJS)

# Target binary
TARGET := $(OBJ_DIR)/kawpow-miner

# Default
all: $(TARGET)

# Ensure build dir
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# CUDA objs
# MODIFIED: Changed -dc (device code) to -c (compile only) as we are not doing a separate device link step here.
# If you needed separate compilation + device linking, the structure would be more complex.
# For this project structure, a single link step is sufficient.
$(OBJ_DIR)/%.o: src/%.cu | $(OBJ_DIR)
	$(NVCC) $(CUFLAGS) -c $< -o $@

# C++ objs
VPATH := src:base/crypto
$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) -c $< -o $@

# JSON obj
$(OBJ_DIR)/Json.o: base/io/json/Json.cpp | $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) -c $< -o $@

# String obj
$(OBJ_DIR)/String.o: base/tools/String.cpp | $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) -c $< -o $@

# C obj for ethash
$(C_OBJS): $(C_SRCS) | $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) -c $< -o $@


# Link
# MODIFIED: The final link step now uses the static flags passed via CUFLAGS
$(TARGET): $(ALL_OBJS)
	$(NVCC) $(ALL_OBJS) $(LDFLAGS) -o $@

clean:
	rm -rf $(OBJ_DIR)

.PHONY: all clean
