# =============================================================================
# Configuration Variables 
# =============================================================================

# Compiler (allows g++ x86 only to test in personal environments)
CXX = riscv64-linux-gnu-g++
# CXX = g++

# Compiler flags
CXXFLAGS = -Wall -g -std=c++17 --static -Isource

# Project name
TARGET = start_car

# Directories
BUILD_DIR = build
SRC_DIR = source
SCRIPTS_DIR = scripts
LOGS_DIR = logs

# Kernel 
OS_DIR = os
KERNEL = linux-6.15.5
KERNEL_TARBALL = $(OS_DIR)/$(KERNEL).tar.xz
KERNEL_SRC_DIR = $(OS_DIR)/$(KERNEL)
KERNEL_IMAGE = $(OS_DIR)/Image
IMAGE_SRC = https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.15.5.tar.xz
JOBS = 8 # Number of parallel jobs for kernel build 			(CHOOSE ACCORDINGLY TO YOUR CPU)

# BusyBox
BUSYBOX_DIR = busybox
INSTALL_DIR = $(BUSYBOX_DIR)/_install/
BUSYBOX_REPO = https://github.com/mirror/busybox.git
BUSYBOX_CONFIG = ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu-

# Find all .cpp files
SOURCES = $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))


COMPS?=4
VM?=5

# =============================================================================
# Main targets 
# =============================================================================
.PHONY: all clean run kernel-compile busybox-compile init-script initramfs

all: clean kernel-compile busybox-compile init-script initramfs

clean:
	@echo "---------------------------------------------"
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR) $(INSTALL_DIR)/build
	@rm -rf $(OS_DIR)/$(KERNEL).tar.xz $(OS_DIR)/$(KERNEL) $(OS_DIR)/initramfs.cpio
	@if [ -d "$(LOGS_DIR)" ]; then \
		rm -rf "$(LOGS_DIR)"; \
	fi
	@echo "--> All cleaned."
	@echo "---------------------------------------------"

run: all
	@if [ -z "$(COMPS) $(VM)" ]; then \
		echo "---------------------------------------------"; \
		echo "Type: make run COMPS=<number_of_components> VM<vms>"; \
		echo "---------------------------------------------"; \
		exit 1; \
	fi

	@if [ ! -z "$(L)" ]; then \
		mkdir -p $(LOGS_DIR); \
	fi

	@mkdir -p $(LOGS_DIR)
	COMPONENTS=$(COMPS)
	VM=$(VM)

	$(eval LOG_FLAG=$(if $(L),1,0))
	LOG_FLAG=$(LOG_FLAG) ./$(SCRIPTS_DIR)/run_simulation.sh -v $(VM)

# =============================================================================
# Build process 
# =============================================================================

# Pattern rule to compile .cpp to .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "---------------------------------------------"
	@echo "--> Compiling: $<"
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) -c $< -o $@


# Rule to link the final executable
$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	@echo "---------------------------------------------"
	@echo "--> Linking: $@"
	@mkdir -p $(@D)
	@$(CXX) $(OBJECTS) -o $@ $(CXXFLAGS)


# 1. Compiling the Kernel
kernel-compile: $(KERNEL_IMAGE)

$(KERNEL_TARBALL):
	@echo "---------------------------------------------"
	@echo "Image not provided. Fetching Kernel source..."
	@mkdir -p $(OS_DIR)
	@cd $(OS_DIR) && curl -LO $(IMAGE_SRC)
	@echo "--> Successfully downloaded."

$(KERNEL_SRC_DIR): $(KERNEL_TARBALL)
	@echo "---------------------------------------------"
	@echo "Extracting Kernel source..."
	@tar -xvf $(KERNEL_TARBALL) -C $(OS_DIR)
	@echo "--> Successfully extracted."

	@echo "---------------------------------------------"
	@echo "Configuring Kernel build..."
	@$(MAKE) ARCH=riscv -C $(KERNEL_SRC_DIR) CROSS_COMPILE=riscv64-linux-gnu- defconfig
	@cd $(KERNEL_SRC_DIR) && ./scripts/config --enable CONFIG_PREEMPT_RT_FULL;
	@echo "Kernel build configuration finished."

IMAGE_EXISTS = $(wildcard $(KERNEL_IMAGE))

ifeq ($(IMAGE_EXISTS),)
$(KERNEL_IMAGE): $(KERNEL_SRC_DIR)
	@echo "---------------------------------------------"
	@echo "Compiling from source..."
	@$(MAKE) ARCH=riscv -C $(KERNEL_SRC_DIR) CROSS_COMPILE=riscv64-linux-gnu- -j$(JOBS)
	@cp $(KERNEL_SRC_DIR)/arch/riscv/boot/Image $(OS_DIR)/Image
	@echo "--> Kernel compilation finished."
else
$(KERNEL_IMAGE):
	@echo "Using provided Kernel Image. Skipping compilation."
endif

# 2. Busy-Box setup
busybox-compile:
	@echo "---------------------------------------------"
	@echo "Setting up BusyBox..."
	@if [ ! -d "$(BUSYBOX_DIR)" ]; then \
        git clone $(BUSYBOX_REPO); \
        cd $(BUSYBOX_DIR) && $(MAKE) $(BUSYBOX_CONFIG) defconfig; \
        echo "Applying patches to BusyBox config..."; \
		sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/g' .config; \
		sed -i 's/CONFIG_TC=y/CONFIG_TC=n/g' .config; \
		sed -i 's/CONFIG_MD5SUM=y/CONFIG_MD5SUM=n/g' .config; \
		sed -i 's/CONFIG_FEATURE_MD5_SHA1_SUM_CHECK=y/CONFIG_FEATURE_MD5_SHA1_SUM_CHECK=n/g' .config; \
		sed -i 's/CONFIG_FEATURE_HTTPD_AUTH_MD5=y/CONFIG_FEATURE_HTTPD_AUTH_MD5=n/g' .config; \
		sed -i 's/CONFIG_SHA1SUM=y/CONFIG_SHA1SUM=n/g' .config; \
		sed -i 's/CONFIG_SHA1_HWACCEL=y/CONFIG_SHA1_HWACCEL=n/g' .config; \
		$(MAKE) $(BUSYBOX_CONFIG) -j$(JOBS) install; \
		echo "--> BusyBox setup and installation finished."; \
	else \
		echo "--> BusyBox already set up. Skipping..."; \
    fi

# 3. VM's init script
init-script: busybox-compile
	@echo "---------------------------------------------" 
	@echo "Creating init script in $(INSTALL_DIR)"
	@rm -f $(INSTALL_DIR)/linuxrc
	@mkdir -p $(INSTALL_DIR)/proc
	@echo '#!/bin/sh' > $(INSTALL_DIR)/init
	@echo 'mount -t proc proc /proc' >> $(INSTALL_DIR)/init
	@echo 'mount -t devtmpfs devtmpfs /dev' >> $(INSTALL_DIR)/init
	@echo "VEHICLE_ID=$$(cat /proc/cmdline | sed -n "s/.*vehicle_id=\([^ ]*\).*/\1/p")" >> $(INSTALL_DIR)/init
	@echo 'export VEHICLE_ID' >> $(INSTALL_DIR)/init
	@echo "echo 'Bringing up eth0...'" >> $(INSTALL_DIR)/init
	@echo "ip link set dev eth0 up" >> $(INSTALL_DIR)/init
	@echo "echo 'Network interface is up. Launching application.'" >> $(INSTALL_DIR)/init
	@echo "./build/$(TARGET) $(COMPS)" >> $(INSTALL_DIR)/init
	@echo 'exec /bin/sh' >> $(INSTALL_DIR)/init
	@chmod +x $(INSTALL_DIR)/init
	@echo "--> Init script successfully created."

# 4. Create initramfs
initramfs: $(BUILD_DIR)/$(TARGET) init-script
	@echo "---------------------------------------------"
	@echo "Creating initramfs.cpio..."
	@# Copy our project into BusyBox install sources
	@cp -r $(BUILD_DIR) $(INSTALL_DIR)
	@# Package everything into the cpio archive
	@cd $(INSTALL_DIR) && find . | cpio -o -H newc > ../../$(OS_DIR)/initramfs.cpio
	@echo "--> initramfs.cpio created successfully."
