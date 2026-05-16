![https://avatars.githubusercontent.com/u/195753706?s=96&v=4](https://avatars.githubusercontent.com/u/195753706?s=96&v=4)

# AsyncTCP

[![License: LGPL 3.0](https://img.shields.io/badge/License-LGPL%203.0-yellow.svg)](https://opensource.org/license/lgpl-3-0/)
[![Continuous Integration](https://github.com/ESP32Async/AsyncTCP/actions/workflows/ci.yml/badge.svg)](https://github.com/ESP32Async/AsyncTCP/actions/workflows/ci.yml)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/ESP32Async/library/AsyncTCP.svg)](https://registry.platformio.org/libraries/ESP32Async/AsyncTCP)

Discord Server: [https://discord.gg/X7zpGdyUcY](https://discord.gg/X7zpGdyUcY)

## Async TCP Library for ESP32 Arduino

This is a fully asynchronous TCP library, aimed at enabling trouble-free, multi-connection network environment for Espressif's ESP32 MCUs.

This library is the base for [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer)

## How to install

The library can be downloaded from the releases page at [https://github.com/ESP32Async/AsyncTCP/releases](https://github.com/ESP32Async/AsyncTCP/releases).

It is also deployed in these registries:

- Arduino Library Registry: [https://github.com/arduino/library-registry](https://github.com/arduino/library-registry)

- ESP Component Registry [https://components.espressif.com/components/esp32async/asynctcp/](https://components.espressif.com/components/esp32async/asynctcp/)

- PlatformIO Registry: [https://registry.platformio.org/libraries/esp32async/AsyncTCP](https://registry.platformio.org/libraries/esp32async/AsyncTCP)

  - Use: `lib_deps=ESP32Async/AsyncTCP` to point to latest version
  - Use: `lib_deps=ESP32Async/AsyncTCP @ ^<x.y.z>` to point to latest version with the same major version
  - Use: `lib_deps=ESP32Async/AsyncTCP @ <x.y.z>` to always point to the same version (reproductible build)

## AsyncClient and AsyncServer

The base classes on which everything else is built. They expose all possible scenarios, but are really raw and require more skills to use.

## Important recommendations

Most of the crashes are caused by improper configuration of the library for the project.
Here are some recommendations to avoid them.

I personally use the following configuration in my projects:

```c++
  -D CONFIG_ASYNC_TCP_MAX_ACK_TIME=5000 // (keep default)
  -D CONFIG_ASYNC_TCP_PRIORITY=10 // (keep default)
  -D CONFIG_ASYNC_TCP_QUEUE_SIZE=64 // (keep default)
  -D CONFIG_ASYNC_TCP_RUNNING_CORE=1 // force async_tcp task to be on same core as the app (default is core 0)
  -D CONFIG_ASYNC_TCP_STACK_SIZE=4096 // reduce the stack size (default is 16K)
```

## Compatibility

- ESP32
- Arduino Core 2.x and 3.x
