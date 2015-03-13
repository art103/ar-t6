# ar-t6

This project is a re-write of er9x for the FS-T6 2.4GHz TX.
Notably, the STM32 ARM processor is more capable than the Arduino in the Turnigy 9x.

Details of the project as it progresses are on [openRCforums](http://openrcforums.com/forum/viewforum.php?f=124)
and http://www2.artaylor.co.uk/ar-t6.html .

## Status

Alpha - functional/unstable, not all the desired features present. 
**USE AT YOUR OWN RISK!**

All hardware blocks are implemented in code. 
Main GUI functionality and framework is implemented.
Rudimentary mixer, limits and curves implemented providing enough functionality for flight control.

## Build (linux)
(command line from root of the project)
```
cd firmware
make
```

This produces "ar-t6.bin" that is ready to flash.

## Flash

[Program the FS-T6 with usb serial with stm32flash](http://minkbot.blogspot.com/2015/03/fs-t6-firmware-upgrade.html)

## Development Environment

Eclipse + gnu ARM toolchain
GDB (Eclipse plugin)
OOCD (Open on-chip Debugger) or stlink (http://www.github.com/texane/stlink)
STM32 Discovery board with stlink v2 (remove the jumpers for external SWD).


## Current ToDo List

Timers, flight modes, heli related functionality, custom switches, safety switches, several radion settings.


**Patches and developers welcome!**
