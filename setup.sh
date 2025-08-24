#!/bin/bash

# =========================================================
# Instructions guided by Antonio Augusto Medeiros Frohlich
# =========================================================




# Additional step to Debian distro:
sudo apt install gcc make flex bison gcc-riscv64-linux-gnu


# =========================================================
# Kernel compilation. Comment if you have the pre-compiled one.
# =========================================================

curl -LO https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.15.5.tar.xz

tar -xvf linux-6.15.5.tar.xz

cd linux-6.15.5

make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- defconfig

./scripts/config --enable CONFIG_PREEMPT_RT_FULL

make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)

cd ..

# =========================================================
# BusyBox compilation
# =========================================================

git clone https://github.com/mirror/busybox.git

cd busybox/

make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc) defconfig

sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/g' .config

# Fixing compiling errors targeting RISC-V
sed -i 's/CONFIG_TC=y/CONFIG_TC=n/g' .config
sed -i 's/CONFIG_MD5SUM=y/CONFIG_MD5SUM=n/g' .config
sed -i 's/CONFIG_FEATURE_MD5_SHA1_SUM_CHECK=y/CONFIG_FEATURE_MD5_SHA1_SUM_CHECK=n/g' .config
sed -i 's/CONFIG_FEATURE_HTTPD_AUTH_MD5=y/CONFIG_FEATURE_HTTPD_AUTH_MD5=n/g' .config
sed -i 's/CONFIG_SHA1SUM=y/CONFIG_SHA1SUM=n/g' .config
sed -i 's/CONFIG_SHA1_HWACCEL=y/CONFIG_SHA1_HWACCEL=n/g' .config
# -----

make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc) install

cd _install/

rm linuxrc

mkdir proc

echo '#!/bin/sh' > init
echo 'mount -t proc proc /proc' >> init
echo 'exec /bin/sh' >> init

chmod +x init

find . | cpio -o -H newc > ../../initramfs.cpio

cd ../../

rm linux-6.15.5.tar.xz

cp linux-6.15.5/arch/riscv/boot/Image .
