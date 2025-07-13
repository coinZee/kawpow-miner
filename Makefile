# Compiler and flags
CXX       := g++
NVCC      := nvcc
TARGET    := kawpow-miner
OBJ_DIR   := build

# Include directories
INCDIRS   := -Iinclude -I. -I./include -Ibase

# Compilation flags
CPPFLAGS  := -O3 -std=c++17 $(INCDIRS) -Wall
CUFLAGS   := -O3 -std=c++17 -arch=sm_80 -arch=sm_75 $(INCDIRS) --cudart=static

# Linker flags - Tells g++ where to find the CUDA libraries
LDFLAGS   := -L/usr/local/cuda/lib64 -lcudart_static -lpthread -ldl -lrt -lssl -lcrypto

# --- Source File Discovery ---
# Automatically find all source files in their respective directories
CPP_SOURCES := $(wildcard src/*.cpp) \
               $(wildcard base/crypto/*.cpp) \
               base/io/json/Json.cpp \
               base/tools/String.cpp
CU_SOURCES  := $(wildcard src/*.cu)
C_SOURCES   := include/libethash/ethash_internal.c

# --- Object File List Generation (The Core Fix) ---
# Create a list of .o files from the source lists, placing them in the build directory
# $(notdir ...) gets the filename (e.g., main.cpp)
# $(patsubst ...) replaces the extension (e.g., main.cpp -> main.o)
# Then we prepend the build directory path.
CPP_OBJS := $(addprefix $(OBJ_DIR)/, $(patsubst %.cpp,%.o,$(notdir $(CPP_SOURCES))))
CU_OBJS  := $(addprefix $(OBJ_DIR)/, $(patsubst %.cu,%.o,$(notdir $(CU_SOURCES))))
C_OBJS   := $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(notdir $(C_SOURCES))))

ALL_OBJS := $(CPP_OBJS) $(CU_OBJS) $(C_OBJS)

# --- Build Rules ---

# Default rule: build the final executable
all: $(TARGET)

# Link the final executable from all the object files
$(TARGET): $(ALL_OBJS)
	@echo "Linking..."
	$(CXX) $(ALL_OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Generic rule to compile any .cpp file
# VPATH tells make where to look for the source files
VPATH := src base/crypto base/io/json base/tools
$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@echo "Compiling C++: $<"
	$(CXX) $(CPPFLAGS) -c $< -o $@

# Generic rule to compile any .cu file
VPATH += src
$(OBJ_DIR)/%.o: %.cu | $(OBJ_DIR)
	@echo "Compiling CUDA: $<"
	$(NVCC) $(CUFLAGS) -c $< -o $@

# Specific rule for the C file
$(OBJ_DIR)/ethash_internal.o: include/libethash/ethash_internal.c | $(OBJ_DIR)
	@echo "Compiling C: $<"
	$(CXX) $(CPPFLAGS) -c $< -o $@

# Rule to create the build directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean up build files
clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
