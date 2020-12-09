<br>

## GD32VF103 LISP(ish) interpreter.

<br>

This interpreter is based on [CDC_ACM Example](https://github.com/riscv-mcu/GD32VF103_Demo_Suites/tree/master/GD32VF103C_START_Demo_Suites/Projects/04_USBFS/Device/CDC_ACM) and [VirtualCOMPort](https://github.com/linusreM/GD32V-Virtual-COM-Port)

The demo is intended to be run on **Longan Nano** board, but it likely runs on other GD32V boards.

Changes from VirtualCOMPort:
* The USB drivers are moved into the project source/include directories.
* There was also a missing include in cdc_usb_core.c which was added.
* USB descriptors are slightly changed so that the device does not apear as **modem device** which activates the **Modem Manager** on Linux.
* LCD and RGB LED driver from Sipeed's [Longan_GD32VF_examples](https://github.com/sipeed/Longan_GD32VF_examples) is used to make a LCD demo (some bugs was fixed and Chinese comments translated to English)

The LISP interpreter currently supports:
* Addition, substraction, multiplication and division `(+,-,*,/) or ('ADD,'SUB,'MUL,'DIV)`
* List access and size `('LSLEN (0 2 5 6 7)) => 5`, `('LSGET (9 8 7) 1) => 8`
* Symbols are currently used for function access, using a binary tree.

My goals are to:
* Implement variables and function definition
* Implement some kind of matricial operations, to use it as a cheap though slow TPU accelerator

My Longan Nano's LCD is failed so for the time being, no graphics support. No audio/GPIO support either but that's just lazyness.

### How to test

After the demo is compiled and flashed to the Longan Nano board start some terminal emulator (minicom, putty, ...) connected to `/dev/ttyACM0` (under Linux). Write your S-Expression followed by a semicolon (;)

Example: `(+ (* 2 2) 6);` => ```token_count=9
value of evaluating ('ADD ('MUL 2 2) 6) is 10```
