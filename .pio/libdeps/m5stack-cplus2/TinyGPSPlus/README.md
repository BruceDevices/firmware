# TinyGPSPlus

A new, customizable Arduino NMEA parsing library
A *NEW* Full-featured GPS/NMEA Parser for Arduino
TinyGPSPlus is a new Arduino library for parsing NMEA data streams provided by GPS modules.

1.1-beta update: Several pull requests incorporated (or equiv)

* [38](https://github.com/mikalhart/TinyGPSPlus/pull/38) Added Fix Quality and Fix Mode
* [66/109](https://github.com/mikalhart/TinyGPSPlus/pull/66) Fix stringop truncation warning
* [69](https://github.com/mikalhart/TinyGPSPlus/pull/69) Support for non-Arduino platforms
* [88](https://github.com/mikalhart/TinyGPSPlus/pull/88) Slight change to earth radius
* [106](https://github.com/mikalhart/TinyGPSPlus/pull/106) Support all satellite groups

Like its predecessor, TinyGPS, this library provides compact and easy-to-use methods for extracting position, date, time, altitude, speed, and course from consumer GPS devices.

However, TinyGPSPlus’s programmer interface is considerably simpler to use than TinyGPS, and the new library can extract arbitrary data from any of the myriad NMEA sentences out there, even proprietary ones.

See [Arduiniana - TinyGPSPlus](http://arduiniana.org/libraries/tinygpsplus/) for more detailed information on how to use TinyGPSPlus.
