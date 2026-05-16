Arduino JPEGDecoder library
===========

News: October 2019 - *I have created a new Arduino compatible [Jpeg decoder library here.](https://github.com/Bodmer/TJpg_Decoder) This new library is based on TinyJpegDec, this is 60% faster on 32bit processors, it also has a much simpler interface.*

This Arduino library supports the rendering of Jpeg files stored both on SD card and in arrays within program memory (FLASH) onto a TFT display. In addition images stored in the LittleFS Flash filing system or "PROGMEM" arrays can be used with the RP2040, ESP8266 and ESP32 processors. For the ESP8266 use board Core 2.3.0 (or later) in the Arduino IDE to avoid File definition conflicts if the SPIFFS and SD libraries are used together.

The library works on the Arduino Due, ESP32 and ESP8266 (e.g. NodeMCU 1.0). Users have also reported success with the STM32 based processor boards.

Example images can be found in the "extras" folder.

Jpeg files must be in 24bit format (8 bit not supported). Jpeg files in the "Progressive" format (where image data is compressed in multiple passes with progressively higher detail) are not supported either since this would require much more memory.

High Jpeg compression ratios work best on images with smooth colour changes, however the Baboon40.jpg image at only 23.8 KBytes renders quite nicely. Typically a 480x320 image can be compressed without much degradation to less than 32 KBytes, in comparison a 24 bit BMP image would occupy 461 KBytes!  For comaprison the 480 x 320 Mouse480 image has been to compressed to a mere 6.45 Kbytes!

When storing the jpeg in a memory array bear in mind the Arduino AVR processors (e.g. Mega2560 and Xmega) have a maximum 32767 byte limit for the maximum size of an array (32 KBytes minus 1 byte).

The decompression of Jpeg images needs more RAM than an UNO provides, thus this library is targetted at processors with more RAM. The library has been tested with Arduino Due and ESP8266/ESP32 based boards.

The decompression of Jpegs involves a lot of maths, so it takes a Due about ~1.3s to render a fullscreen (480x320 pixel) image and the Mega will take ~5s to do the same. The time for smaller images will reduce roughly pro-rata with the total pixel count. An ESP8266 running at 160MHz and 40MHz SPI coupled to a 320x240 ILI9341 display can render a Jpeg in as little as 240ms.

This library supports either the SD library (and SdFat for Due). The SdFat allows a bit-bashed SPI interface to an SD Card which can be convenient for example on pins 50, 51 and 52 of a Due (on Mega these are hardware SPI).

The library has been tested with the 1.8.1 version of the Arduino IDE and may generate error messages at compile time on other versions because "#ifdef \_\_AVR\_\_" is used to distinguish between the Mega and Due and select the correct libraries.

The library has been tested with 3.2" and 3.0" displays based on the HX8357B, HX8357C and ILI9481 driver chips with a 16 bit parallel interface.  Adapting the example sketch for other TFT drivers and their graphics libraries should be quite easy if they support either setWindow() or SetAddrWindow() and pushColor() functions as found in the Adafruit_GFX library.

The Arduino Mega is not recommended as it does not reliably decode some jpeg images possibly due to a shortage of RAM.  The Due will work fine with much bigger image sets in FLASH.

The ESP8266 and ESP32 has been tested with an ILI9341 library using the SPI interface, with Jpeg images stored in SPIFFS and in Flash arrays.

SD and SPIFFS filenames can be in String or character array format. File handles can also be used.

This library has been based on the excellent picojpeg code and the Arduino library port by Makoto Kurauchi here:
https://github.com/MakotoKurauchi/JPEGDecoder


Makoto's original Readme below:
==============================

JPEG Decoder for Arduino

概要
----
Arduino 用 JPEG デコーダです。デコーダ部には [picojpeg](https://code.google.com/p/picojpeg/) を使用しています。

サンプルコード
----
###SerialCsvOut

SD カード上の JPEG ファイルをブロックごとにデコードし、シリアルから CSV を出力します。

変更履歴
----
V0.01 - 最初のリリース
