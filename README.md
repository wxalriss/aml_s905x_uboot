LibreTech U-boot Tree
=====================

Board support
-------------
 - LibreTech CC/ : branch `libretech-cc`

Howto Build
-----------

 - LibreTech CC

Checkout source and prepare environment:

```
# git clone https://github.com/BayLibre/libretech-u-boot.git -b libretech-cc
# wget https://releases.linaro.org/archive/13.11/components/toolchain/binaries/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux.tar.xz
# wget https://releases.linaro.org/archive/13.11/components/toolchain/binaries/gcc-linaro-arm-none-eabi-4.8-2013.11_linux.tar.xz
# tar xvfJ gcc-linaro-aarch64-none-elf-4.8-2013.11_linux.tar.xz
# tar xvfJ gcc-linaro-arm-none-eabi-4.8-2013.11_linux.tar.xz
# export PATH=$PWD/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin:$PWD/gcc-linaro-arm-none-eabi-4.8-2013.11_linux/bin:$PATH
```

Build:

```
# cd libretech-u-boot
libretech-u-boot # make libretech_cc_defconfig
libretech-u-boot # make
```

Binaries should be available in `fip` directory :
```
fip/
├── ...
├── u-boot.bin (for eMMC)
├── u-boot.bin.sd.bin (for SDCard)
└── ...
```

Install on blank SDCard (assuming SDCard in on /dev/mmcblk0):
```
# sudo mkfs.vfat /dev/mmcblk0p1
# sudo dd if=fip/u-boot.bin.sd.bin of=/dev/mmcblk0 conv=fsync,notrunc bs=1 count=444
# sudo dd if=fip/u-boot.bin.sd.bin of=/dev/mmcblk0 conv=fsync,notrunc bs=512 skip=1 seek=1
# sync
```
