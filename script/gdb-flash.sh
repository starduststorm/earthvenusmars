#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Deploy which environment? v1,v2,v3?";
    exit 1
fi

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

. $SCRIPT_DIR/config

$GDB -nx --batch -ex "target extended-remote $BLACKMAGIC_PROBE" -x $SCRIPT_DIR/gdb-erase.scr
$GDB -nx --batch -ex "target extended-remote $BLACKMAGIC_PROBE" -x $SCRIPT_DIR/gdb-flash.scr $BOOTLOADER
$GDB -nx --batch -ex "target extended-remote $BLACKMAGIC_PROBE" -x $SCRIPT_DIR/gdb-flash.scr $SCRIPT_DIR/../.pio/build/$1/firmware.elf
$GDB -nx --batch -ex "target extended-remote $BLACKMAGIC_PROBE" -x $SCRIPT_DIR/gdb-kill.scr
