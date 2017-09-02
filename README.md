[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
# Teensy64
Commodore C64 Emulation on a [Teensy 3.6](https://www.pjrc.com/store/teensy36.html) Microcontroller

![Teensy64](https://github.com/FrankBoesing/Teensy64/blob/master/extras/logo201707.png?raw=true)

[![Video](http://img.youtube.com/vi/CjijgL0VC6k/0.jpg)](https://www.youtube.com/watch?v=CjijgL0VC6k "C64 Emulation early demo")

Features
----------
- Supports Commodore Serial IEC Bus
- For ILI9341 SPI Display
- USB-Keyboards (wireless too)
- 31kHz reSID Audio emulation
- Audio Line-Out
- Compatible to SD2IEC
- Simple drive emulation included (load "whatever.prg" or load "$" )
- Supports original joysticks
- very small
- A custom board is available which can be used for other purposes, too (mail me - 39€/$ incl. all parts but display & T3.6).


About
----------

    10 PRINT "HELLO WORLD";    
    20 GOTO 10

Some time ago, 2015, someone in the PJRC forum asked me if it was possible to emulate the C64's SID chip on the Teensy 3.2. Since I had already had experience with the porting of audio codecs (mp3/aac/flac), I came up with the idea to search for a "finished" emulator and found the very good reSID. This is also used by VICE. [The porting](https://github.com/FrankBoesing/Teensy-reSID "Teensy-reSID") wasn't that difficult, but I've optimized some parts of it to better match the Teensy and its audio library. It is even possible to emulate a second reSID for stereo operation. Unfortunately, current reSID versions require a lot of RAM, so I had to switch to an older version - which is not necessarily worse. Soon the wish for a SID-Player came up and a little later the Teensy 3.6 was announced. With 256KB RAM, more than enough flash, SD slot and 180 MHz it seemed possible to emulate a complete C64. But before that, other hurdles had to be overcome. There was no way to control a display fast enough. However, the T3.6 has a fast SPI interface, which can be supplied via a DMA channel. The first display that crossed my mind was the well-known ILI9341, which had already done a good job for the T3.2. Some time before, a user in the forum had noticed that you can overclock its SPi interface drastically. After some attempts I had a [working video player](https://www.youtube.com/watch?v=lBLKsSEvWHM "Video Player") - even before the official release of Teensy 3.6. Paul Stoffregen thankfully provided me with an early pre-release version. 
After many preliminary considerations and deviations I came to the conclusion that the T3.6 should be capable of a C64 emulation.
Unfortunately I forgot almost everything about the C64, and I had to and still have to learn everything anew and read a lot of technical details. I must admit that I have underestimated the VIC (the video chip of the C64) enormously. But that was a good thing, otherwise I would never have started this project.
In the meantime I have rewritten the code for the VIC four times.

In the beginning my goal was to make emulation [good enough for "Boulder Dash"](https://www.youtube.com/watch?v=CjijgL0VC6k "early demo") and other of my old favorite games. This goal has been achieved and I have the ambition to make some demos executable.
"Teensy64" is compatible enough to play many games and other programs like the original. 
For the serial floppy interface the SID sound emulation has to be switched off during the accesses, because reSID requires a lot of computing time. Since the C64 operates this interface "in software" with the CPU and the signal lines have to be controlled more or less exactly in time, I couldn't find a way to keep the sound active even after many ideas and attempts.
However, a 1541 floppy disk drive that had been bought extra second-hand was damaged after a short time. It was just too old. After some searching I found the "SD2IEC" project. It seems to be a good replacement and emulates the floppy drive wonderfully. SD2IEC can be connected to the interface of the board and works wonderfully.
My plan is to port the SD2IEC software to Teensy 3.6 and integrate it into "Teensy64". I'm not sure if that's possible yet.
There is also a way to work with the Teensy64 without an external drive. I patched the emulated ROM a little bit and misused the device ID 1. The following commands are available:



- Load "$" - loads the directory (display as known with the command "LIST")


- Load "whatever. prg" - Loads the program from the Teensy SD card.


- Save "whatever. prg" - Saves the program.


Other commands such as "open" are currently NOT supported for Device 1.

Access is ALWAYS made to a subdirectory "/C64/"which must be present on the SD card.

Thanks to Bill Greiman for his SdFat library with long file names. The C64 was able to support file names of 16 characters in the 80's when Microsoft only supported 8+3 characters.

There are many more TODOs I would like to tackle little by little. The emulated VIC does not yet support the BA signal, and the emulated 6150 processor does not yet know it; -) Therefore there are some inaccuracies in the processor cycles. For example, the VIC can pause the processor or allow write access only. The latter has not yet been implemented. There's still work to be done on the internal timing of the sprites - so "Democoder"tricks like sprite stretching don't work. Also "xFLI" and "DMA Delay" is not yet possible. All things that the developers of the C64 didn't foresee at that time but were discovered by "hackers". Furthermore, only the PAL mode works satisfactorily - but this is only because I haven't found an NTSC program yet!

**This project is "Open Source" and everyone is invited to work on it.**



Board
----------
![Teensy64](https://forum.pjrc.com/attachment.php?attachmentid=10674&d=1495748304)

