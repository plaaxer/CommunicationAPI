# Compiler
CXX = riscv64-linux-gnu-g++

# Compiler flags
CXXFLAGS = -Wall -g -std=c++17 --static -Isource

# Project name
TARGET = test_nic

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

# Default goal: build the main C++ executable
.PHONY: all
all: $(BUILD_DIR)/$(TARGET)

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

# Rule to download, configure, and build BusyBox
.PHONY: busybox
busybox:
	@echo "<--------------------------------------------->"
	@echo "Setting up BusyBox..."
	@echo "<--------------------------------------------->"
	# Clone BusyBox if the directory doesn't exist
	@if [ ! -d "$(BUSYBOX_DIR)" ]; then \
		git clone $(BUSYBOX_REPO); \
	fi
	# Configure BusyBox
	cd $(BUSYBOX_DIR) && $(MAKE) $(BUSYBOX_CONFIG) defconfig
	# Apply required patches for static linking and RISC-V compatibility
	@echo "Applying patches to BusyBox config..."
	cd $(BUSYBOX_DIR) && sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/g' .config
	cd $(BUSYBOX_DIR) && sed -i 's/CONFIG_TC=y/CONFIG_TC=n/g' .config
	cd $(BUSYBOX_DIR) && sed -i 's/CONFIG_MD5SUM=y/CONFIG_MD5SUM=n/g' .config
	cd $(BUSYBOX_DIR) && sed -i 's/CONFIG_FEATURE_MD5_SHA1_SUM_CHECK=y/CONFIG_FEATURE_MD5_SHA1_SUM_CHECK=n/g' .config
	cd $(BUSYBOX_DIR) && sed -i 's/CONFIG_FEATURE_HTTPD_AUTH_MD5=y/CONFIG_FEATURE_HTTPD_AUTH_MD5=n/g' .config
	cd $(BUSYBOX_DIR) && sed -i 's/CONFIG_SHA1SUM=y/CONFIG_SHA1SUM=n/g' .config
	cd $(BUSYBOX_DIR) && sed -i 's/CONFIG_SHA1_HWACCEL=y/CONFIG_SHA1_HWACCEL=n/g' .config
	# Compile and install BusyBox
	cd $(BUSYBOX_DIR) && $(MAKE) $(BUSYBOX_CONFIG) -j$$(nproc) install
	# Create a basic init script
	@echo "Creating init script in $(INSTALL_DIR)"
	@rm -f $(INSTALL_DIR)/linuxrc
	@mkdir -p $(INSTALL_DIR)/proc
	@echo '#!/bin/sh' > $(INSTALL_DIR)/init
	@echo 'mount -t proc proc /proc' >> $(INSTALL_DIR)/init
	@echo 'mount -t devtmpfs devtmpfs /dev' >> $(INSTALL_DIR)/init
	@echo '/build/$(TARGET) &' >> $(INSTALL_DIR)/init
	@echo 'exec /bin/sh' >> $(INSTALL_DIR)/init
	@chmod +x $(INSTALL_DIR)/init

# Rule to create the initramfs (depends on C++ target and BusyBox)
.PHONY: initramfs
initramfs: $(BUILD_DIR)/$(TARGET) busybox
	@echo "<--------------------------------------------->"
	@echo "Creating initramfs.cpio..."
	@echo "<--------------------------------------------->"
	# Copy our project into BusyBox install sources
	cp -r $(BUILD_DIR) $(INSTALL_DIR)
	# Package everything into the cpio archive
	cd $(INSTALL_DIR) && find . | cpio -o -H newc > ../../initramfs.cpio

# Rule to run QEMU (depends on initramfs)
.PHONY: run
run: initramfs
	@echo "<--------------------------------------------->"
	@echo "Starting QEMU..."
	@echo "<--------------------------------------------->"
	qemu-system-riscv64 \
		-machine virt \
		-nographic \
		-kernel Image \
		-initrd initramfs.cpio \
		-append "root=/dev/ram rw" \
		-netdev socket,id=vlan0,mcast=230.0.0.1:1234 \
		-device virtio-net,netdev=vlan0,mac=52:54:00:12:34:00

# Rule to clean up everything
.PHONY: clean
clean:
	@echo "<--------------------------------------------->"
	@echo "Cleaning..."
	@echo "<--------------------------------------------->"
	rm -rf $(BUILD_DIR) initramfs.cpio $(BUSYBOX_DIR)