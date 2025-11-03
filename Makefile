# =============================================================================
# Configuration Variables 
# =============================================================================

# Compiler (allows g++ x86 only to test in personal environments)
CXX = riscv64-linux-gnu-g++
# CXX = g++

# Compiler flags
CXXFLAGS = -Wall -g -std=c++17 --static -Isource

# EXECUTABLE TARGETS
VEHICLE_TARGET = start_car
RSU_TARGET = start_road_site_unit

# BUILD
BUILD_DIR = build
VEHICLE_BUILD_DIR = $(BUILD_DIR)/vehicle/
RSU_BUILD_DIR = $(BUILD_DIR)/rsu/

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

LATENCY ?= 1
COMPS?=6
VM?=5

# =============================================================================
# Main targets 
# =============================================================================
.PHONY: all clean run kernel-compile busybox-compile init-script-vehicle init-script-rsu initramfs-vehicle initramfs-rsu

all: clean kernel-compile busybox-compile init-script-vehicle initramfs-vehicle init-script-rsu initramfs-rsu run

clean:
	@echo "---------------------------------------------"
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)
	@rm -rf $(OS_DIR)/$(KERNEL).tar.xz $(OS_DIR)/$(KERNEL) $(OS_DIR)/initramfs_vehicle.cpio $(OS_DIR)/initramfs_rsu.cpio
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

	@if [ "$(LATENCY)" = "1" ]; then \
		mkdir -p $(LOGS_DIR); \
	fi

	COMPONENTS=$(COMPS)
	VM=$(VM)

	$(eval LOG_FLAG=$(if $(LATENCY),1,0))
	LOG_FLAG=$(LATENCY) ./$(SCRIPTS_DIR)/run_simulation.sh -v $(VM)

# =============================================================================
# Build process 
# =============================================================================


# ------ VEHICLE ------

# Pattern rule to compile .cpp to .o
$(VEHICLE_BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "---------------------------------------------"
	@echo "--> Compiling: $<"
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) -c $< -o $@


# Rule to link the final executable
$(VEHICLE_BUILD_DIR)/$(VEHICLE_TARGET): $(OBJECTS)
	@echo "---------------------------------------------"
	@echo "--> Linking: $@"
	@mkdir -p $(@D)
	@$(CXX) $(OBJECTS) -o $@ $(CXXFLAGS)


# ------ RSU ------

# Pattern rule to compile .cpp to .o
$(RSU_BUILD_DIR)/%.0: $(SRC_DIR)/%.cpp
	@echo "---------------------------------------------"
	@echo "--> Compiling: $<"
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to link the final executable
$(RSU_BUILD_DIR)/$(RSU_TARGET): $(OBJECTS)
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
		rm -f _install/linuxrc
		mkdir -p _install/proc

		echo "--> BusyBox installation and setup finished."; \
	else \
		echo "--> BusyBox already set up. Skipping..."; \
    fi


# &. Vehicle init script
init-script-vehicle: busybox-compile
	@echo "---------------------------------------------" 
	@echo "Creating vehicle init script in $(VEHICLE_BUILD_DIR)"
	@echo '#!/bin/sh' > $(VEHICLE_BUILD_DIR)/init
	@echo 'mount -t proc proc /proc' >> $(VEHICLE_BUILD_DIR)/init
	@echo 'mount -t devtmpfs devtmpfs /dev' >> $(VEHICLE_BUILD_DIR)/init
	@echo "VEHICLE_ID=$$(cat /proc/cmdline | sed -n "s/.*vehicle_id=\([^ ]*\).*/\1/p")" >> $(VEHICLE_BUILD_DIR)/init
	@echo 'export VEHICLE_ID' >> $(VEHICLE_BUILD_DIR)/init
	@echo "echo 'Bringing up eth0...'" >> $(VEHICLE_BUILD_DIR)/init
	@echo "ip link set dev eth0 up" >> $(VEHICLE_BUILD_DIR)/init
	@echo "echo 'Network interface is up. Launching application.'" >> $(VEHICLE_BUILD_DIR)/init
	@echo "./vehicle/$(VEHICLE_TARGET) $(COMPS)" >> $(VEHICLE_BUILD_DIR)/init
	@echo 'exec /bin/sh' >> $(VEHICLE_BUILD_DIR)/init
	@chmod +x $(VEHICLE_BUILD_DIR)/init
	@echo "--> Vehicle init script successfully created."

# &. Vehicle initramfs
initramfs-vehicle: $(BUILD_DIR)/$(VEHICLE_TARGET) init-script-vehicle
	@echo "---------------------------------------------"
	@echo "Creating vehicle initramfs.cpio..."
	@cp -r $(INSTALL_DIR) $(RSU_BUILD_DIR)
	@cd $(INSTALL_DIR) && find . | cpio -o -H newc > ../../$(OS_DIR)/initramfs_vehicle.cpio
	@echo "--> initramfs_vehicle.cpio created successfully."

# &. RSU init script
init-script-vehicle: busybox-compile
	@echo "---------------------------------------------" 
	@echo "Creating RSU init script in $(RSU_BUILD_DIR)"
	@echo '#!/bin/sh' > $(RSU_BUILD_DIR)/init
	@echo 'mount -t proc proc /proc' >> $(RSU_BUILD_DIR)/init
	@echo 'mount -t devtmpfs devtmpfs /dev' >> $(RSU_BUILD_DIR)/init
	@echo "echo 'Bringing up eth0...'" >> $(RSU_BUILD_DIR)/init
	@echo "ip link set dev eth0 up" >> $(RSU_BUILD_DIR)/init
	@echo "echo 'Network interface is up. Launching application.'" >> $(RSU_BUILD_DIR)/init
	@echo "./rsu/$(RSU_TARGET) $(COMPS)" >> $(RSU_BUILD_DIR)/init
	@echo 'exec /bin/sh' >> $(RSU_BUILD_DIR)/init
	@chmod +x $(RSU_BUILD_DIR)/init
	@echo "--> RSU init script successfully created."

# &. RSU initramfs
initramfs-rsu: $(BUILD_DIR)/$(RSU_TARGET) init-script-rsu
	@echo "---------------------------------------------"
	@echo "Creating Road Site Unit initramfs.cpio..."
	@cp -r $(INSTALL_DIR) $(VEHICLE_BUILD_DIR)
	@cd $(VEHICLE_BUILD_DIR) && find . | cpio -o -H newc > ../../$(OS_DIR)/initramfs_rsu.cpio
	@echo "--> initramfs_rsu.cpio created successfully."
