# Fixed-point MIDI ROM-wavetable synthesis

This branch was ported to ESP8266Audio by Earle F. Philhower, III <earlephilhower@yahoo.com>
from the original TinySoundFont by https://github.com/schellingb/TinySoundFont

Changes include converting a large portion of the operation to fixed-point
arithmetic and adding in a special ROM-mode where the SoundFont can be
stored inside a microcontroller ROM w/o needing a filesystem.  This saves
RAM and makes the code faster than having to page in little bits of
waveforms or regions.

## Building ROM-wavetable structures

The SoundFont is stored in ROM using the normal C structures TSF uses, so it
requires a conversion from raw SF2 file into a header with structure members.
Do NOT use `xxd` or similar on the SoundFont, it won't work.

In the `examples` directory, run `build-linux-gcc`to compile the SF2-to-H
converted.  Run it as follows:
````
$ ./dump-linux-x86_64 INPUT.SF2 OUTPUT.H
````

For example
````
$ ./# dump-linux-x86_64 Scratch2010.sf2  scratch2010.h
````

## Building the F16P16 tables

This port implements a few functions that normally require floating point
math with piecewise linear approximations.  They are hardcoded in the TSF.
file already, but to regenerate them just run `make_f16p16_tables`.

## Performance

On devices that can perform hardware single-precision FP (i.e. the Pico 2,
original ESP32, others) this code can generate up to 44.1khz sounds with
over a dozen simultaneous voices without underflow and leaving some CPU
for other work.

On devices without an FPU, sample rates of up to 22050hz are possible,
depending on the MIDI and SF2 used.
