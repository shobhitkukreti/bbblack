#sudo ./qemu/bin/qemu-system-aarch64 -M virt -cpu cortex-a57 -kernel ./Images/kernel -append "root=/dev/root rootfstype=9p rootflags=trans=virtio,version=9p2000.L rw  console=ttyAMA0,$Baud ip=dhcp" -semihosting -fsdev local,id=r,path=../fs1,security_model=none -device virtio-9p-pci,fsdev=r,mount_tag=/dev/root -nographic

QBIN=BootQEMU
VID=0x4e8
PID=0xa101
rpath=../ubuntu-exynos-rootfs/fs1/

sudo ./$QBIN//qemu/bin/qemu-system-aarch64 -M virt -cpu cortex-a57 -smp 4 -m 1G -kernel ./$QBIN/Images/kernel -append "root=/dev/root rw rootfstype=9p rootflags=trans=virtio,version=9p2000.L rw  console=ttyAMA0,$Baud ip=dhcp" -semihosting -fsdev local,id=r,path=$rpath,security_model=none,writeout=immediate -device virtio-9p-pci,fsdev=r,mount_tag=/dev/root -netdev type=user,id=me,hostfwd=tcp::5555-:22 -device driver=virtio-net,netdev=me -nographic -device usb-ehci,id=ehci -device usb-host,id=ETH_DONGLE,bus=ehci.0,vendorid=$VID,productid=$PID

