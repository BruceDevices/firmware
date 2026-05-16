PNGdec
------
Copyright (c) 2021 BitBank Software, Inc.<br>
Written by Larry Bank<br>
bitbank@pobox.com<br>

What is it?
------------
An 'embedded-friendly' (aka Arduino) PNG image decoding library<br>
<br>

Why did you write it?
---------------------
Starting in the late 80's I wrote my own imaging codecs for the existing standards (CCITT G3/G4 was the first). I soon added GIF, JPEG and not long after that, the PNG specification was ratified. All of this code was "clean room" - written just from the specification. I used my imaging library in many projects and products over the years and recently decided that some of my codecs could get a new lease on life as open source, embedded-friendly libraries for microcontrollers.<br>
<br>

What's special about it?<br>
------------------------
The PNG image specification was written at a time when computers had megabytes of RAM and conserving memory wasn't a big priority. The memory allocated for decoding the compressed data (zlib) and for holding the uncompressed image can be quite a bit more than is available on modern microcontrollers (usually measured in K bytes). Three goals for this project are: easy to compile+use on all embedded systems, use a minimal amount of RAM and be self-contained. One of the dependencies I like to remove when working on embedded software is malloc/free. When compiling on a system with a tiny amount of RAM, heap memory management might not even exist.<br>

Feature summary:<br>
----------------<br>
- *New* Added PNGDisplay helper class to simplify displaying images on displays supported by my bb_spi_lcd library<br>
- Runs on any MCU with at least 48K of free RAM<br>
- No external dependencies (including malloc/free)<br>
- Decode an image line by line with a callback function<br>
- Decode an image to a user supplied buffer (no callback needed)<br>
- Supports all standard options except interlacing (too much RAM needed)<br>
- Function provided to turn any pixel format into RGB565 for LCD displays<br>
- Optionally disable zlib's internal CRC check - improves speed by 10-30%
- Arduino-style C++ library class with simple API<br>
- Can by built as straight C as well<br>
<br>

How fast is it?<br>
---------------<br>
For most PNG images, the time is split about 50/50 between inflate and de-filtering. Indexed images usually don't enable filtering, so it becomes nearly 100% the inflator. Converting the output to RGB565 can take significant cycles if doing alpha blending. The examples folder contains a sketch to measure the performance of decoding a 240x200 image of varying bit depths. Here's the results when run on a few common MCUs:<br>

<br>
<p align="center">
  <img width="770" height="201" src="https://github.com/bitbank2/PNGdec/blob/master/perf_small.png?raw=true">
</p>

Documentation:<br>
---------------<br>
Detailed information about the API is in the [Wiki](https://github.com/bitbank2/PNGdec/wiki)<br>
See the examples folder for easy starting points<br>
<br>

## Funding

This project received funding through [NGI Zero Core](https://nlnet.nl/core), a fund established by [NLnet](https://nlnet.nl) with financial support from the European Commission's [Next Generation Internet](https://ngi.eu) program. Learn more at the [NLnet project page](https://nlnet.nl/project/ImageCodes-Optimised).

[<img src="https://nlnet.nl/logo/banner.png" alt="NLnet foundation logo" width="20%" />](https://nlnet.nl)
[<img src="https://nlnet.nl/image/logos/NGI0_tag.svg" alt="NGI Zero Logo" width="20%" />](https://nlnet.nl/core)

