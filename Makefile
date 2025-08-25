# Compiler
CXX = riscv64-linux-gnu-g++
CXXFLAGS = -Wall -g -std=c++17 --static -Isource

# Project name
TARGET = test_nic

# Directories
BUILD_DIR = build
SRC_DIR = source
INITRAMFS_DIR = initramfs_root

# Find all .cpp files
SOURCES = $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

# Default goal
.PHONY: all
all: $(BUILD_DIR)/$(TARGET)

# Rule to link the final executable
$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	@echo "==============================================="
	@echo "Linking target: $@"
	@echo "==============================================="
	@mkdir -p $(@D)
	$(CXX) $(OBJECTS) -o $@ $(CXXFLAGS)

# Pattern rule to compile .cpp to .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "==> Compiling: $<"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to create the initramfs
.PHONY: initramfs
initramfs: $(BUILD_DIR)/$(TARGET)
	@echo "==============================================="
	@echo "Creating initramfs.cpio"
	@echo "==============================================="
	# Clean and create the directory structure
	rm -rf $(INITRAMFS_DIR)
	mkdir -p $(INITRAMFS_DIR)
	mkdir -p $(INITRAMFS_DIR)/bin
	mkdir -p $(INITRAMFS_DIR)/proc
	mkdir -p $(INITRAMFS_DIR)/sys

	# Copy our compiled program and the init script
	cp $(BUILD_DIR)/$(TARGET) $(INITRAMFS_DIR)/
	cp init.sh $(INITRAMFS_DIR)/init

	# Copy essential tools (busybox) into the initramfs
	# This requires locating them on your cross-compiler's sysroot
	# NOTE: The path below might need adjustment depending on your setup
	CROSS_SYSROOT=$(shell dirname $(shell which $(CXX)))/../riscv64-linux-gnu/libc
	cp $(CROSS_SYSROOT)/bin/busybox $(INITRAMFS_DIR)/bin/
	cd $(INITRAMFS_DIR)/bin && ln -s busybox sh
	cd $(INITRAMFS_DIR)/bin && ln -s busybox sleep
	cd $(INITRAMFS_DIR)/bin && ln -s busybox modprobe
	cd $(INITRAMFS_DIR)/bin && ln -s busybox ip

	# Package everything into the cpio archive
	cd $(INITRAMFS_DIR) && find . | cpio -o -H newc > ../initramfs.cpio

# Rule to clean up
.PHONY: clean
clean:
	@echo "==> Cleaning project"
	rm -rf $(BUILD_DIR) initramfs.cpio $(INITRAMFS_DIR)