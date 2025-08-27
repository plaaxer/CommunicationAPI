# =============================================================================
# Configuration Variables 
# =============================================================================
# Compiler
CXX = riscv64-linux-gnu-g++
# CXX = g++
# Compiler flags
CXXFLAGS = -Wall -g -std=c++17 --static -Isource

# Project name
TARGET = start_vehicle

# Directories
BUILD_DIR = build
SRC_DIR = source
BUSYBOX_DIR = busybox
INSTALL_DIR = $(BUSYBOX_DIR)/_install/

# BusyBox configuration
BUSYBOX_REPO = https://github.com/mirror/busybox.git
BUSYBOX_CONFIG = ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu-

# Find all .cpp files
SOURCES = $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))


# =============================================================================
# Main User Targets 
# These are the primary commands you'll run, like 'make all' or 'make run'.
# =============================================================================
.PHONY: all initramfs busybox clean run-vehicle

# The default goal. Builds everything required for the initramfs.
# This target now correctly triggers the entire build process in the right order.
all: initramfs

# Give a default ID if none is provided
ID ?= 1

# Rule to run QEMU (depends on a complete initramfs)
run-vehicle: initramfs
	@echo "<--------------------------------------------->"
	@echo "Starting QEMU for vehicle-0$(ID)..."
	@echo "<--------------------------------------------->"
	qemu-system-riscv64 \
        -machine virt \
        -nographic \
        -kernel Image \
        -initrd initramfs.cpio \
        -append "root=/dev/ram rw vehicle_id=vehicle-0$(ID)" \
        -netdev socket,id=vlan0,mcast=230.0.0.1:1234 \
        -device virtio-net,netdev=vlan0,mac=52:54:00:12:34:0$(ID)

# Rule to clean up everything
clean:
	@echo "<--------------------------------------------->"
	@echo "Cleaning..."
	@echo "<--------------------------------------------->"
	rm -rf $(BUILD_DIR) initramfs.cpio $(INSTALL_DIR)/build


# =============================================================================
# Core Build Process 
# These are the internal rules that build the components.
# =============================================================================

# Rule to create the initramfs. This is the final step.
# It correctly depends on BOTH the C++ target and the BusyBox setup.
# 'make' will ensure both are finished before this rule runs.
initramfs: $(BUILD_DIR)/$(TARGET) busybox
	@echo "<--------------------------------------------->"
	@echo "Creating initramfs.cpio..."
	@echo "<--------------------------------------------->"
	# Copy our project into BusyBox install sources
	cp -r $(BUILD_DIR) $(INSTALL_DIR)
	# Package everything into the cpio archive
	cd $(INSTALL_DIR) && find . | cpio -o -H newc > ../../initramfs.cpio

# --- C++ Project Compilation ---

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

# --- BusyBox Setup ---

# Rule to download, configure, and build BusyBox
busybox:
	@echo "<--------------------------------------------->"
	@echo "Setting up BusyBox..."
	@echo "<--------------------------------------------->"
	# Clone BusyBox if the directory doesn't exist
	@if [ ! -d "$(BUSYBOX_DIR)" ]; then \
        git clone $(BUSYBOX_REPO); \
    \
        cd $(BUSYBOX_DIR) && $(MAKE) $(BUSYBOX_CONFIG) defconfig; \
        \
        @echo "Applying patches to BusyBox config..."; \
        sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/g' $(BUSYBOX_DIR)/.config; \
        sed -i 's/CONFIG_TC=y/CONFIG_TC=n/g' $(BUSYBOX_DIR)/.config; \
        sed -i 's/CONFIG_MD5SUM=y/CONFIG_MD5SUM=n/g' $(BUSYBOX_DIR)/.config; \
        sed -i 's/CONFIG_FEATURE_MD5_SHA1_SUM_CHECK=y/CONFIG_FEATURE_MD5_SHA1_SUM_CHECK=n/g' $(BUSYBOX_DIR)/.config; \
        sed -i 's/CONFIG_FEATURE_HTTPD_AUTH_MD5=y/CONFIG_FEATURE_HTTPD_AUTH_MD5=n/g' $(BUSYBOX_DIR)/.config; \
        sed -i 's/CONFIG_SHA1SUM=y/CONFIG_SHA1SUM=n/g' $(BUSYBOX_DIR)/.config; \
        sed -i 's/CONFIG_SHA1_HWACCEL=y/CONFIG_SHA1_HWACCEL=n/g' $(BUSYBOX_DIR)/.config; \
        \
        cd $(BUSYBOX_DIR) && $(MAKE) $(BUSYBOX_CONFIG) -j$$(nproc) install; \
		\
    fi

	@echo "Creating init script in $(INSTALL_DIR)"; \
	rm -f $(INSTALL_DIR)/linuxrc; \
	mkdir -p $(INSTALL_DIR)/proc; \
	echo '#!/bin/sh' > $(INSTALL_DIR)/init; \
	echo 'mount -t proc proc /proc' >> $(INSTALL_DIR)/init; \
	echo 'mount -t devtmpfs devtmpfs /dev' >> $(INSTALL_DIR)/init; \
    echo 'VEHICLE_ID=$$(cat /proc/cmdline | sed -n "s/.*vehicle_id=\([^ ]*\).*/\1/p")' >> $(INSTALL_DIR)/init; \
    echo 'export VEHICLE_ID' >> $(INSTALL_DIR)/init; \
	echo "echo 'Bringing up eth0...'" >> $(INSTALL_DIR)/init; \
	echo "ip link set dev eth0 up" >> $(INSTALL_DIR)/init; \
	echo "echo 'Network interface is up. Launching application.'" >> $(INSTALL_DIR)/init; \
	echo "./build/$(TARGET)" >> $(INSTALL_DIR)/init; \
	echo 'exec /bin/sh' >> $(INSTALL_DIR)/init; \
	chmod +x $(INSTALL_DIR)/init; \
