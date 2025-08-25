# Compiler (g++ only for test in x86)
CXX = riscv64-linux-gnu-g++
#CXX = g++

CXXFLAGS = -Wall -g -std=c++17 --static -Isource

# Project name
TARGET = test_nic

# Directories
BUILD_DIR = build
SRC_DIR = source
INSTALL_DIR = busybox/_install/


# Find all .cpp files
SOURCES = $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

# Default goal
.PHONY: all
all: $(BUILD_DIR)/$(TARGET)

	@echo "<--------------------------------------------->"
	@echo "Starting..."
	@echo "<--------------------------------------------->"

# Rule to link the final executable
$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	@echo "<--------------------------------------------->"
	@echo "Linking target: $@"
	@echo "<--------------------------------------------->"
	@mkdir -p $(@D)
	$(CXX) $(OBJECTS) -o $@ $(CXXFLAGS)

# Pattern rule to compile .cpp to .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling --> $<"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@


# Rule to create the initramfs
.PHONY: initramfs
initramfs: $(BUILD_DIR)/$(TARGET)
	@echo "<--------------------------------------------->"
	@echo "Creating initramfs.cpio..."
	@echo "<--------------------------------------------->"

	# Copy our project into BusyBox install sources
	cp -r $(BUILD_DIR) $(INSTALL_DIR)

	# Package everything into the cpio archive
	cd $(INSTALL_DIR) && find . | cpio -o -H newc > ../../initramfs.cpio

# Rule to clean up
.PHONY: clean
clean:
	@echo "<--------------------------------------------->"
	@echo "Cleaning..."
	@echo "<--------------------------------------------->"
	rm -rf $(BUILD_DIR) initramfs.cpio $(INSTALL_DIR)/$(BUILD_DIR)