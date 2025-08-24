# =============================================================================
# Variables
# =============================================================================

# Compiler (following the instructions)
CXX = riscv64-linux-gnu-g++

# Compiler flags
# -Wall: Turn on all warnings
# -g:    Generate debugging information
# -std=c++17: Use the C++17 standard (good practice)
# --static: Link statically
CXXFLAGS = -Wall -g -std=c++17 --static

# Include paths for header files (-I<directory>)
# This tells the compiler where to look for .hpp
INCLUDE_PATHS = -Isource/api -Isource/api/observer -Isource/api/network -Isource/car-components

# Adds the source folder in the compiler list of includes path (to use relative paths from source/)
CXXFLAGS += -Isource

# Project name
TARGET = test_nic

# Directories
BUILD_DIR = build
SRC_DIR = source

# =============================================================================
# File Discovery
# =============================================================================

# Find all .cpp files recursively in the source directory
SOURCES = $(shell find $(SRC_DIR) -name '*.cpp')

# Create a list of object files that will be placed in the BUILD_DIR
# e.g., source/test_nic.cpp -> build/test_nic.o
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

# =============================================================================
# Rules
# =============================================================================

# Default goal when run "make"
# It depends on the final executable file.
.PHONY: all
all: $(BUILD_DIR)/$(TARGET)

# Rule to link all object files into the final executable
$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	@echo "==============================================="
	@echo "Linking target: $@"
	@echo "==============================================="
	@mkdir -p $(@D) # Ensure the build directory exists
	$(CXX) $(OBJECTS) -o $@ $(CXXFLAGS)

# Pattern rule to compile a .cpp file from the source directory
# into a corresponding .o file in the build directory.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "==> Compiling: $<"
	@mkdir -p $(@D) # Ensure the object file's directory exists
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATHS) -c $< -o $@

# make initramfs
.PHONY: initramfs
initramfs: $(BUILD_DIR)/$(TARGET)
	@echo "==============================================="
	@echo "Creating initramfs.cpio"
	@echo "==============================================="

	# Create a temporary directory for the cpio archive structure
	rm -rf initramfs_root && mkdir -p initramfs_root
	cp $(BUILD_DIR)/$(TARGET) initramfs_root/
	cd initramfs_root && find . | cpio -o -H newc > ../initramfs.cpio
	rm -rf initramfs_root

# make clean
.PHONY: clean
clean:
	@echo "==> Cleaning project"
	rm -rf $(BUILD_DIR) initramfs.cpio
