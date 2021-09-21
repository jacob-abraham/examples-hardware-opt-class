#!/bin/sh

# on Mac, use hvf
# on Windows, use hax (with HAXM driver installed)
# on Linux, use kvm
ACCEL=
OS=`uname`
if [[ $OS == "Darwin" ]]; then
    ACCEL="-accel hvf"
elif [[ $OS == "Linux" ]]; then
    ACCEL="-accel kvm"
fi

qemu-system-x86_64 -hda qemu-image/ubuntu20.qcow2 -m 1G -machine q35 $ACCEL -device e1000,netdev=net0 -netdev user,id=net0,hostfwd=tcp::5555-:22 -cpu Skylake-Client -smp 2 $@
