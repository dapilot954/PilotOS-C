#!/bin/bash
set -e

rm -rf build iso fatimg.img
mkdir -p build iso/boot/grub

# Compile all .c files
for f in $(find ./project -name '*.c'); do
    filename=$(basename "$f" .c)
    echo "Compiling $f..."
    i386-elf-gcc -m32 -ffreestanding -O2 -Wall -c "$f" -o "build/${filename}.o"
done

# Assemble all .s (assembly) files
for f in $(find ./project -name '*.s'); do
    filename=$(basename "$f" .s)
    echo "Assembling $f..."
    i386-elf-as "$f" -o "build/${filename}.o"
done

# Link all object files into kernel.elf
i386-elf-ld -m elf_i386 -T linker.ld -o build/kernel.elf build/*.o

# Create GRUB bootable ISO to load the kernel
cp build/kernel.elf iso/boot/kernel.elf
cp grub.cfg iso/boot/grub/grub.cfg
grub-mkrescue -o os_image.iso iso

# --- NEW PART: Create raw FAT32 disk image ---
# 32MB blank disk
dd if=/dev/zero of=fatimg.img bs=1M count=32

dd if=/dev/zero of=fatimg.img bs=1M count=64
mkfs.vfat -F 32 fatimg.img


# Mount and copy filesystem_data into it
mkdir -p mnt
sudo mount fatimg.img mnt
sudo cp -r filesystem_data/* mnt/
sync
sudo umount mnt
rmdir mnt

# --- Launch QEMU with ISO and ATA HDD ---
qemu-system-i386 \
  -cdrom os_image.iso \
  -hda fatimg.img \
  -m 512 -boot d \
  -no-reboot -serial stdio
