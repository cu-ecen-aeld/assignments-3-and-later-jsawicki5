#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

export PATH=$PATH:~/install-lnx/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    #make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p "$OUTDIR/rootfs"
cd "$OUTDIR/rootfs"
mkdir -p bin dev etc home lib lib64 proc sys sbin tmp usr var
mkdir -p usr/bin usr/sbin usr/lib
mkdir -p var/log


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

cd "${OUTDIR}/rootfs"
echo "Library dependencies"
prog_interpreter=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter" | awk -F: '{print $2}' | awk '{$1=$1;print}' | awk -F']' '{print $1}')
echo "Program interpreter: ${prog_interpreter}"
libraries=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library" | awk -F'[' '{print $2}' | awk -F']' '{print $1}')
echo "shared libraries: ${libraries}"

# TODO: Add library dependencies to rootfs
if [ -n "$prog_interpreter" ]
then
	interpretor_path=$(find /home -name "$(basename $prog_interpreter)" 2>/dev/null)
	echo "Copying dependency ${interpretor_path} to lib"
	cp ${interpretor_path} ${OUTDIR}/rootfs/lib
fi

if [ -n "$libraries" ]
then
	for lib in $libraries
	do
		lib_path=$(find /home -name "$lib" 2>/dev/null)
		echo "Copying shared lib ${lib_path} to lib64"
		cp ${lib_path} ${OUTDIR}/rootfs/lib64
	done
fi
	
# TODO: Make device nodes
echo "Creating device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1
sudo mknod -m 666 dev/ttyAMA0 c 204 64

# TODO: Clean and build the writer utility
echo "Cleaning and building the writer utility"
cd ~/Documents/assignments/assignment-1-jsawicki5/finder-app
make CROSS_COMPILE=aarch64-none-linux-gnu- clean
make CROSS_COMPILE=aarch64-none-linux-gnu- all


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "Copying files into the home directory"
cp finder.sh ${OUTDIR}/rootfs/home
cp writer ${OUTDIR}/rootfs/home
cp autorun-qemu.sh ${OUTDIR}/rootfs/home
cp finder-test.sh ${OUTDIR}/rootfs/home

mkdir -p ${OUTDIR}/rootfs/home/conf
cd conf
cp -r assignment.txt ${OUTDIR}/rootfs/home/conf
cp -r username.txt ${OUTDIR}/rootfs/home/conf

# TODO: Chown the root directory
echo "creating the cpio file"
sudo chown -R root:root ${OUTDIR}/rootfs
cd "$OUTDIR/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio

# TODO: Create initramfs.cpio.gz
echo "Compressing the cpio file"
gzip -f ${OUTDIR}/initramfs.cpio
