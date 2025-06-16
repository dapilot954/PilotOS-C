#!/bin/bash
set -e

rm -rf build iso fatimg1.img fatimg2.img
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

# --- Create first FAT32 disk image (ATA) ---
dd if=/dev/zero of=fatimg1.img bs=1M count=64
mkfs.vfat -F 32 fatimg1.img

mkdir -p mnt1
sudo mount fatimg1.img mnt1
sudo cp -r filesystem_data1/* mnt1/ || true
sync
sudo umount mnt1
rmdir mnt1

# --- Create second FAT32 disk image (SATA via AHCI) ---
dd if=/dev/zero of=fatimg2.img bs=1M count=64
mkfs.vfat -F 32 fatimg2.img

mkdir -p mnt2
sudo mount fatimg2.img mnt2
sudo cp -r filesystem_data2/* mnt2/ || true
sync
sudo umount mnt2
rmdir mnt2

# --- Launch QEMU with ISO and two disks ---
qemu-system-i386 \
  -cdrom os_image.iso \
  -hda fatimg1.img \
  -drive id=disk2,file=fatimg2.img,format=raw,if=none \
  -device ahci,id=ahci \
  -device ide-hd,drive=disk2,bus=ahci.0 \
  -m 512 -boot d \
  -no-reboot -serial stdio
