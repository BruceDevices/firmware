# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
<!-- markdownlint-disable MD024 -->

## [1.4.11] - 2025-01-10

### <!-- 1 --> 🚀 Added

- Use enum to return precise FIFO state by \@2bndy5 in [#1009](https://github.com/nRF24/RF24/pull/1009)
- Make `available(pipe)` depend on `available(void)` by \@2bndy5 in [#1007](https://github.com/nRF24/RF24/pull/1007)

### <!-- 4 --> 🛠️ Fixed

- Fixes for STM32 by \@2bndy5 in [#1011](https://github.com/nRF24/RF24/pull/1011)
- Init `failureDetected` by \@2bndy5 in [#1016](https://github.com/nRF24/RF24/pull/1016)

### <!-- 9 --> 🗨️ Changed

- Adjustments to CHANGELOG by \@2bndy5 in [`8aa7103`](https://github.com/nRF24/RF24/commit/8aa71035ae3dd10de35865a894082f10a9c51027)
- Change comment in example by \@2bndy5 in [`80e7f13`](https://github.com/nRF24/RF24/commit/80e7f137ae4caf0df4889b5499c22857019a328a)

[1.4.11]: https://github.com/nRF24/RF24/compare/v1.4.10...v1.4.11

Full commit diff: [`v1.4.10...v1.4.11`][1.4.11]

## [1.4.10] - 2024-10-06

### <!-- 1 --> 🚀 Added

- Add ebyte manual for ML01DP5 to datasheets by \@2bndy5 in [#996](https://github.com/nRF24/RF24/pull/996)
- Add pyproject.toml and update docs by \@2bndy5 in [#1001](https://github.com/nRF24/RF24/pull/1001)

### <!-- 3 --> 🗑️ Removed

- Remove useless `ce()` call by \@2bndy5 in [#987](https://github.com/nRF24/RF24/pull/987)
- Remove sphinx sources and update RTD config by \@2bndy5 in [#1003](https://github.com/nRF24/RF24/pull/1003)

### <!-- 4 --> 🛠️ Fixed

- Fix doc typo by \@2bndy5 in [`81348ad`](https://github.com/nRF24/RF24/commit/81348ad9b100abf252de6f287a5855c18c0c9cc8)
- Fix spelling by \@2bndy5 in [#998](https://github.com/nRF24/RF24/pull/998)
- Fix some linting errors noted by clang-tidy by \@2bndy5 in [#999](https://github.com/nRF24/RF24/pull/999)

### <!-- 9 --> 🗨️ Changed

- Change SERIAL_DEBUG into RF24_DEBUG by \@2bndy5 in [#981](https://github.com/nRF24/RF24/pull/981)
- Update py examples' comment about pin numbers by \@2bndy5 in [`2344282`](https://github.com/nRF24/RF24/commit/234428209ae9cc90d38a2288c2d5645a7ead4cef)
- Update doxygen CSS by \@2bndy5 in [#982](https://github.com/nRF24/RF24/pull/982)
- Separate 2 problems' answers in COMMON_ISSUES.md by \@2bndy5 in [#984](https://github.com/nRF24/RF24/pull/984)
- Migrate issue templates into forms by \@2bndy5 in [#985](https://github.com/nRF24/RF24/pull/985)
- Refactor some SPI transactions by \@2bndy5 in [#988](https://github.com/nRF24/RF24/pull/988)
- Bump version to v1.4.10 by \@2bndy5 in [`5bb22e2`](https://github.com/nRF24/RF24/commit/5bb22e2dbab31dedd677defc0007c2f0e7cd7f11)

[1.4.10]: https://github.com/nRF24/RF24/compare/v1.4.9...v1.4.10

Full commit diff: [`v1.4.9...v1.4.10`][1.4.10]

## [1.4.9] - 2024-06-11

### <!-- 1 --> 🚀 Added

- Add delay to manualAcknowledgements example by \@TMRh20 in [#953](https://github.com/nRF24/RF24/pull/953)
- Separate ISR handling from IRQ processing by \@2bndy5 in [#956](https://github.com/nRF24/RF24/pull/956)
- Promiscuous scanners by \@2bndy5 in [#955](https://github.com/nRF24/RF24/pull/955)
- Update SPIDEV driver for Character Device v2 ABI; cache FDs by \@2bndy5 in [#959](https://github.com/nRF24/RF24/pull/959)
- IRQ support using Linux kernel Character Device + Posix threads by \@2bndy5 in [#961](https://github.com/nRF24/RF24/pull/961)
- Add link to RF24G library by \@TMRh20 in [#964](https://github.com/nRF24/RF24/pull/964)
- Revise MRAA's GPIO implementation by \@2bndy5 in [#966](https://github.com/nRF24/RF24/pull/966)
- Update python wrapper/examples/doc by \@2bndy5 in [#967](https://github.com/nRF24/RF24/pull/967)

### <!-- 2 --> 🚫 Deprecated

- Update linux installer docs by \@TMRh20 in [#957](https://github.com/nRF24/RF24/pull/957)

### <!-- 3 --> 🗑️ Removed

- Wrap MRAA lib's `Gpio::isr()` for IRQ support by \@2bndy5 in [#970](https://github.com/nRF24/RF24/pull/970)
- Disable auto-detect driver and default to SPIDEV by \@2bndy5 in [#973](https://github.com/nRF24/RF24/pull/973)

### <!-- 4 --> 🛠️ Fixed

- GPIO changes (RPi5 support) by \@TMRh20 in [#942](https://github.com/nRF24/RF24/pull/942)
- Fix for SPIDEV & MRAA: delayMicroseconds() by \@TMRh20 in [#952](https://github.com/nRF24/RF24/pull/952)
- Fix for pigpio delayMicroseconds by \@TMRh20 in [#954](https://github.com/nRF24/RF24/pull/954)
- Fix scanner.py; draw 1 space (not 2) after channel label by \@2bndy5 in [`9d0aa8d`](https://github.com/nRF24/RF24/commit/9d0aa8d3b0f46a99105d15e0ef7e3284d6b5bdf3)

### <!-- 6 --> 📦 Dependency updates

- Bump actions/checkout from 3 to 4 by \@dependabot[bot] in [#941](https://github.com/nRF24/RF24/pull/941)

### <!-- 8 --> 📝 Documentation

- Modify Main docs page by \@TMRh20 in [#951](https://github.com/nRF24/RF24/pull/951)

### <!-- 9 --> 🗨️ Changed

- Update clang format by \@2bndy5 in [#940](https://github.com/nRF24/RF24/pull/940)
- Byte alignment/packing mismatches by \@TMRh20 in [#945](https://github.com/nRF24/RF24/pull/945)
- Update COMMON_ISSUES re: write() function by \@TMRh20 in [#947](https://github.com/nRF24/RF24/pull/947)
- TestRPD by \@TMRh20 in [#948](https://github.com/nRF24/RF24/pull/948)
- Update MRAA timer functions by \@TMRh20 in [#950](https://github.com/nRF24/RF24/pull/950)
- Cache gpio chip detection when properly initialized by \@2bndy5 in [#962](https://github.com/nRF24/RF24/pull/962)
- Use quotes for linux/gpio.h include by \@2bndy5 in [#963](https://github.com/nRF24/RF24/pull/963)
- Use BCM2835 lib's Edge Detection Status for IRQ support by \@2bndy5 in [#969](https://github.com/nRF24/RF24/pull/969)
- Wrap wiringPiISRStop() in driver API by \@2bndy5 in [#968](https://github.com/nRF24/RF24/pull/968)
- Bump version to v1.4.9 by \@2bndy5 in [`29231f2`](https://github.com/nRF24/RF24/commit/29231f22d5739b8fb4d3a4d43c9a8ae07a7f1ce4)

[1.4.9]: https://github.com/nRF24/RF24/compare/v1.4.8...v1.4.9

Full commit diff: [`v1.4.8...v1.4.9`][1.4.9]

## New Contributors
* \@dependabot[bot] made their first contribution in [#941](https://github.com/nRF24/RF24/pull/941)
## [1.4.8] - 2023-09-27

### <!-- 8 --> 📝 Documentation

- Mention RPi Pico board's 3v regulator in Pico SDK doc by \@2bndy5 in [#915](https://github.com/nRF24/RF24/pull/915)

### <!-- 9 --> 🗨️ Changed

- Revert available() algorithm by \@2bndy5 in [#914](https://github.com/nRF24/RF24/pull/914)
- Update COMMON_ISSUES.md by \@TMRh20 in [#918](https://github.com/nRF24/RF24/pull/918)
- Bump patch version by \@2bndy5 in [`638095e`](https://github.com/nRF24/RF24/commit/638095e0d18b86726116cdf6e87425ebe8b232ec)

[1.4.8]: https://github.com/nRF24/RF24/compare/v1.4.7...v1.4.8

Full commit diff: [`v1.4.7...v1.4.8`][1.4.8]

## [1.4.7] - 2023-06-06

### <!-- 1 --> 🚀 Added

- Add `deploy-release` arg to PIO reusable CI call by \@2bndy5 in [`2ff3f2b`](https://github.com/nRF24/RF24/commit/2ff3f2b9d6c72dd274d8fa5bdbc92e404f6f80b0)

### <!-- 3 --> 🗑️ Removed

- Reworked the utility/rp2 SPI class and removed static members by \@TheGarkine in [#892](https://github.com/nRF24/RF24/pull/892)

### <!-- 4 --> 🛠️ Fixed

- [OrangePi Zero] Linux chip enable conditional statement by \@fertinator in [#904](https://github.com/nRF24/RF24/pull/904)
- Fix for potential memory issue by \@TMRh20 in [`457e036`](https://github.com/nRF24/RF24/commit/457e03694f6e3923b2425bb1e7be3feeed630996)

### <!-- 8 --> 📝 Documentation

- Doc tweaks and reusable CI by \@2bndy5 in [#887](https://github.com/nRF24/RF24/pull/887)
- Alias gpio pin datatype by \@2bndy5 in [#898](https://github.com/nRF24/RF24/pull/898)

### <!-- 9 --> 🗨️ Changed

- Solve \#894 by \@2bndy5 in [#895](https://github.com/nRF24/RF24/pull/895)
- Resolves \#899 by \@2bndy5 in [`557d165`](https://github.com/nRF24/RF24/commit/557d165c12e72329b7573b380eb1e7f132cfbfe6)
- Update pico_sdk.md to reflect changes to SPI by \@matt-walker0 in [#900](https://github.com/nRF24/RF24/pull/900)
- Update version for release by \@TMRh20 in [`a028813`](https://github.com/nRF24/RF24/commit/a028813b285d6cec3090d2c96984bd67a399c52e)

[1.4.7]: https://github.com/nRF24/RF24/compare/v1.4.6...v1.4.7

Full commit diff: [`v1.4.6...v1.4.7`][1.4.7]

## New Contributors
* \@fertinator made their first contribution in [#904](https://github.com/nRF24/RF24/pull/904)
* \@TheGarkine made their first contribution in [#892](https://github.com/nRF24/RF24/pull/892)
## [1.4.6] - 2022-09-18

### <!-- 1 --> 🚀 Added

- Add manual trigger to docs CI by \@2bndy5 in [#868](https://github.com/nRF24/RF24/pull/868)
- Add printf support to ESP32/8266 and arduino-pico cores by \@2bndy5 in [#873](https://github.com/nRF24/RF24/pull/873)

### <!-- 2 --> 🚫 Deprecated

- Fix `pgm_read_byte/word()` definitions by \@2bndy5 in [#871](https://github.com/nRF24/RF24/pull/871)

### <!-- 4 --> 🛠️ Fixed

- Fix Raspberry Pi company name by \@lurch in [#858](https://github.com/nRF24/RF24/pull/858)
- Fix `pgm_read_ptr()` (fixes bblanchon/ArduinoJson\#1790) by \@bblanchon in [#864](https://github.com/nRF24/RF24/pull/864)

### <!-- 8 --> 📝 Documentation

- Adjust writeFast docs by \@2bndy5 in [#866](https://github.com/nRF24/RF24/pull/866)

### <!-- 9 --> 🗨️ Changed

- Make utilities/CMakeLists.txt respect RF24_NO_IRQ by \@2bndy5 in [#859](https://github.com/nRF24/RF24/pull/859)
- Do not set PA level on begin by \@TMRh20 in [`8ad2886`](https://github.com/nRF24/RF24/commit/8ad2886b846375fca5cfcda74da784d3452d9148)
- Bump version (patch) by \@2bndy5 in [`d7c4041`](https://github.com/nRF24/RF24/commit/d7c4041f001a7fbc880535f5151f89a56a520072)

[1.4.6]: https://github.com/nRF24/RF24/compare/v1.4.5...v1.4.6

Full commit diff: [`v1.4.5...v1.4.6`][1.4.6]

## New Contributors
* \@bblanchon made their first contribution in [#864](https://github.com/nRF24/RF24/pull/864)
* \@lurch made their first contribution in [#858](https://github.com/nRF24/RF24/pull/858)
## [1.4.5] - 2022-07-19

### <!-- 3 --> 🗑️ Removed

- Remove trailing whitespace and add a blank line by \@2bndy5 in [`3f786bf`](https://github.com/nRF24/RF24/commit/3f786bffd5e27bd67bbe518bcc3249885fde05ec)

### <!-- 4 --> 🛠️ Fixed

- All single line loops and conditions use curly brackets by \@2bndy5 in [#855](https://github.com/nRF24/RF24/pull/855)

### <!-- 9 --> 🗨️ Changed

- Bump version by \@2bndy5 in [`a04f32b`](https://github.com/nRF24/RF24/commit/a04f32b1fb0b2aa3070179d6abcde0aacaf60ebb)

[1.4.5]: https://github.com/nRF24/RF24/compare/v1.4.4...v1.4.5

Full commit diff: [`v1.4.4...v1.4.5`][1.4.5]

## [1.4.4] - 2022-07-18

### <!-- 4 --> 🛠️ Fixed

- Fix for data corruption issues by \@TMRh20 in [`399d219`](https://github.com/nRF24/RF24/commit/399d219d6db589df449a56534d6327048915af87)

### <!-- 9 --> 🗨️ Changed

- Update version by \@TMRh20 in [`d7ba9c2`](https://github.com/nRF24/RF24/commit/d7ba9c2fb83f7c2fbb6336f76b607c630f3ebfc2)

[1.4.4]: https://github.com/nRF24/RF24/compare/v1.4.3...v1.4.4

Full commit diff: [`v1.4.3...v1.4.4`][1.4.4]

## [1.4.3] - 2022-07-09

### <!-- 1 --> 🚀 Added

- Add support for sprintf by \@dstroy0 in [#821](https://github.com/nRF24/RF24/pull/821)
- Add new feature encodeRadioDetails() and optimize sprintfPrettyDetails() by \@dstroy0 in [#825](https://github.com/nRF24/RF24/pull/825)
- Add isChipConnected() to py wrapper by \@2bndy5 in [`6c5993f`](https://github.com/nRF24/RF24/commit/6c5993f1130d396677d63f7bd0623e491f833160)
- Add isFifo() methods by \@2bndy5 in [#849](https://github.com/nRF24/RF24/pull/849)
- Add model variant info to printPrettyDetails() by \@2bndy5 in [#850](https://github.com/nRF24/RF24/pull/850)

### <!-- 3 --> 🗑️ Removed

- Various improvements to docs by \@2bndy5 in [#844](https://github.com/nRF24/RF24/pull/844)
- Clang format by \@2bndy5 in [#824](https://github.com/nRF24/RF24/pull/824)

### <!-- 4 --> 🛠️ Fixed

- Fix logic in disableAckPayloads() by \@2bndy5 in [`f9e5075`](https://github.com/nRF24/RF24/commit/f9e507544686af23bcfe9578a1558bbb08d382c9)
- Fix \#832 typo in printDetails() by \@2bndy5 in [`d4c3c35`](https://github.com/nRF24/RF24/commit/d4c3c355af0ed5d326ec79b5bbe56ff0f2a3d146)
- Patch for \#834 by \@2bndy5 in [#839](https://github.com/nRF24/RF24/pull/839)

### <!-- 8 --> 📝 Documentation

- Update docs README by \@2bndy5 in [`e4e7156`](https://github.com/nRF24/RF24/commit/e4e7156f00418cb9e18d340fb7319d96145f2302)
- [docs] fix recieved -> received  typos by \@prawnpie in [#817](https://github.com/nRF24/RF24/pull/817)
- Pigpio Testing by \@TMRh20 in [#819](https://github.com/nRF24/RF24/pull/819)
- Revert changes to install doc by \@TMRh20 in [`8ccbb9d`](https://github.com/nRF24/RF24/commit/8ccbb9dc747cc6a65a5684d791fb118b54c31536)
- Update sphinx doc config by \@2bndy5 in [#848](https://github.com/nRF24/RF24/pull/848)

### <!-- 9 --> 🗨️ Changed

- Only run linux CI on "created release" events by \@2bndy5 in [`8d25675`](https://github.com/nRF24/RF24/commit/8d256751d6dbb2523bf3b3376071faef56d073cc)
- Update actions/checkout to v2 by \@2bndy5 in [`f584685`](https://github.com/nRF24/RF24/commit/f584685c34b6a948384c9512140ad90fd0661770)
- Updated common issues added issues and fixes by \@matt-walker0 in [#820](https://github.com/nRF24/RF24/pull/820)
- Undo changes to CI triggers in \#821 by \@2bndy5 in [`b2cf576`](https://github.com/nRF24/RF24/commit/b2cf5769ffbc59c84b9b99a2933d74634f08efab)
- Ran black on print_dets.py by \@2bndy5 in [`aaf0f53`](https://github.com/nRF24/RF24/commit/aaf0f53cfa4484d9bc0d3f6c2d5be91d8cdfe931)
- Don't create PDFs on RTD by \@2bndy5 in [`7e69d7d`](https://github.com/nRF24/RF24/commit/7e69d7d1fa10c52a3bbfba1fecec89915995f90e)
- Update printf.h to include megaAVR by \@Cobalt6700 in [#843](https://github.com/nRF24/RF24/pull/843)
- Update linux_install.md by \@TMRh20 in [`c327568`](https://github.com/nRF24/RF24/commit/c3275683e62fcb184cfca225f5fb972faad01ad6)
- Follow up \#824 by \@2bndy5 in [#846](https://github.com/nRF24/RF24/pull/846)
- Update py examples by \@2bndy5 in [#851](https://github.com/nRF24/RF24/pull/851)
- Minor version bump by \@2bndy5 in [`54ce53a`](https://github.com/nRF24/RF24/commit/54ce53a960a00d1f64dbd601dfa852936ba4be57)

[1.4.3]: https://github.com/nRF24/RF24/compare/v1.4.2...v1.4.3

Full commit diff: [`v1.4.2...v1.4.3`][1.4.3]

## New Contributors
* \@Cobalt6700 made their first contribution in [#843](https://github.com/nRF24/RF24/pull/843)
* \@dstroy0 made their first contribution in [#825](https://github.com/nRF24/RF24/pull/825)
* \@matt-walker0 made their first contribution in [#820](https://github.com/nRF24/RF24/pull/820)
* \@prawnpie made their first contribution in [#817](https://github.com/nRF24/RF24/pull/817)
## [1.4.2] - 2021-11-27

### <!-- 1 --> 🚀 Added

- Pico SDK support, Sphinx, documentation updates and CMake build system for Linux by \@2bndy5 in [#772](https://github.com/nRF24/RF24/pull/772)
- Add pico examples to sphinx docs by \@2bndy5 in [`6741869`](https://github.com/nRF24/RF24/commit/67418691dadcfc11bbbd30c9bab7ef10205bce8c)

### <!-- 2 --> 🚫 Deprecated

- Document deprecated stuff in sphinx by \@2bndy5 in [`6ec52c7`](https://github.com/nRF24/RF24/commit/6ec52c782fff5b5c88d8ab6778b276836d215772)

### <!-- 8 --> 📝 Documentation

- Sphinx docs updates by \@2bndy5 in [#803](https://github.com/nRF24/RF24/pull/803)
- Resolve \#779 and augment docs by \@2bndy5 in [`04b103e`](https://github.com/nRF24/RF24/commit/04b103e81a7d21437a3ec0958a9d39bbd87a6c98)
- Small docs update by \@2bndy5 in [`f9447d9`](https://github.com/nRF24/RF24/commit/f9447d9c81a11862a27eb6fdf34edbac2f6ee489)
- [docs] fix API sig; add badge; fix dup heading by \@2bndy5 in [`3befead`](https://github.com/nRF24/RF24/commit/3befead0495ac4d92228632e7d49a0c2d4f17ddc)
- Reverse Engineering tricks by \@2bndy5 in [#804](https://github.com/nRF24/RF24/pull/804)

### <!-- 9 --> 🗨️ Changed

- Update pyRF24.cpp by \@razerraz in [#704](https://github.com/nRF24/RF24/pull/704)
- Save tag file to doxygen html output by \@2bndy5 in [`2a03328`](https://github.com/nRF24/RF24/commit/2a033285ed22a24dd391a209a49b976032801c93)
- Revert "save tag file to doxygen html output" by \@2bndy5 in [`741a508`](https://github.com/nRF24/RF24/commit/741a5085938008dc97136298d3e9f3cebd1e76cd)
- Trigger rp2xxx CI (picoSDK v1.3.0) by \@2bndy5 in [`496f5da`](https://github.com/nRF24/RF24/commit/496f5da29ba96cb40f1cb761d278211ee807f4b3)

[1.4.2]: https://github.com/nRF24/RF24/compare/v1.4.1...v1.4.2

Full commit diff: [`v1.4.1...v1.4.2`][1.4.2]

## New Contributors
* \@razerraz made their first contribution in [#704](https://github.com/nRF24/RF24/pull/704)
## [1.4.1] - 2021-06-02

### <!-- 4 --> 🛠️ Fixed

- Undo latest changes to SoftSPI & add patches by \@2bndy5 in [#771](https://github.com/nRF24/RF24/pull/771)

### <!-- 8 --> 📝 Documentation

- Use angle brackets in a docs code block by \@2bndy5 in [`7c3d6ec`](https://github.com/nRF24/RF24/commit/7c3d6ecfc55e52ab1cd968ea71d5c86ab8c7ffd4)

[1.4.1]: https://github.com/nRF24/RF24/compare/v1.4.0...v1.4.1

Full commit diff: [`v1.4.0...v1.4.1`][1.4.1]

## [1.4.0] - 2021-04-11

### <!-- 1 --> 🚀 Added

- Add printf.h fix from \@TonioChingon in \#739 by \@2bndy5 in [`c0279c8`](https://github.com/nRF24/RF24/commit/c0279c881f36d5be556564c5750014f89667f235)
- Add Due to ArduinoCLI action by \@2bndy5 in [`41484dc`](https://github.com/nRF24/RF24/commit/41484dc781a002e4ec67b68fdae7799b92afca7c)
- Add MRAA to Linux CI action by \@2bndy5 in [`8e7d470`](https://github.com/nRF24/RF24/commit/8e7d4709371608222e582930462670fc951d5011)
- Add example code snippets to Arduino support pg by \@2bndy5 in [`4770050`](https://github.com/nRF24/RF24/commit/477005031ff0473d2f2999a82a07daf8aec56b16)
- Add msg about COMMON_ISSUES to README by \@2bndy5 in [`e26f202`](https://github.com/nRF24/RF24/commit/e26f202d74d57a3143df1c93a1964fe5f163ccdd)
- Add estimated mbed example to arduino.md by \@2bndy5 in [`2ea8cbf`](https://github.com/nRF24/RF24/commit/2ea8cbf80a0389847046b549569126df6b34d8a4)
- Add pinout img from RF24Audio repo by \@2bndy5 in [`d159954`](https://github.com/nRF24/RF24/commit/d159954b54add6e7318532aed6f0bdc9e009726d)
- Docs review by \@2bndy5 in [#750](https://github.com/nRF24/RF24/pull/750)

### <!-- 2 --> 🚫 Deprecated

- Doxygen v1.9.1 deprecated COLS_IN_ALPHA_INDEX tag by \@2bndy5 in [`d4e1e0d`](https://github.com/nRF24/RF24/commit/d4e1e0dda8091041e0a988dfa98d76dedbac9152)

### <!-- 3 --> 🗑️ Removed

- Remove useless Due config file \#752 by \@2bndy5 in [`25d5530`](https://github.com/nRF24/RF24/commit/25d5530a42a77e53a864c08a18d755e55bff6950)

### <!-- 4 --> 🛠️ Fixed

- Fixed platforms list in library.json by \@dagix5 in [#742](https://github.com/nRF24/RF24/pull/742)
- Fix faulty \#ifdef logic by \@2bndy5 in [`416cc35`](https://github.com/nRF24/RF24/commit/416cc35d69dd7cbe5874cee5775c70081b73b319)
- Fix teensy support; add PIO CI for teensy by \@2bndy5 in [`6935766`](https://github.com/nRF24/RF24/commit/693576613efc503c4932910bbe533e1f0ebf7f73)
- Fix \#414; note need mbed example in arduino.md by \@2bndy5 in [`5a28733`](https://github.com/nRF24/RF24/commit/5a28733542997d8295a6206cadc9f7c49ce1ae7b)
- Fix printf() only for adafruit/ArduinoCore-SAMD by \@2bndy5 in [`27710e2`](https://github.com/nRF24/RF24/commit/27710e2cc41146fbaca5af600f3e38ee9f416a49)
- Fix indent by \@2bndy5 in [`1e974ce`](https://github.com/nRF24/RF24/commit/1e974ce2f2550d756e6ca5af3da55f5077adb037)

### <!-- 8 --> 📝 Documentation

- Overload begin() and amended docs about begin() by \@2bndy5 in [`fee50f6`](https://github.com/nRF24/RF24/commit/fee50f69196bb0f7a16e9097127ee6af99317104)
- Doxygen action shouldn't create a docs folder by \@2bndy5 in [`715ed75`](https://github.com/nRF24/RF24/commit/715ed754facdf0e59904c0cd15e9216110c27b59)
- Doc typos by \@2bndy5 in [`36449a5`](https://github.com/nRF24/RF24/commit/36449a5581429cecf51a00e8ad32b854c68829ae)
- Due can't use SPI_PTR; delete ATXMEGA/readme.md by \@2bndy5 in [`a54668e`](https://github.com/nRF24/RF24/commit/a54668ee0efb9607d7a8ea342f77ae54801b5785)
- Boost.py docs suck by \@2bndy5 in [`79ce0cd`](https://github.com/nRF24/RF24/commit/79ce0cd7098ec8b25d4bc2ec5745c507255eef87)

### <!-- 9 --> 🗨️ Changed

- Endorse COMMON_ISSUES.md in issue templates by \@2bndy5 in [#747](https://github.com/nRF24/RF24/pull/747)
- Abstract related pages from header by \@2bndy5 in [`e971e5b`](https://github.com/nRF24/RF24/commit/e971e5bc1fe650f571f87ece096bd9af46706581)
- Convert _SPI calls to pointers where applicable by \@2bndy5 in [`5b0b0f9`](https://github.com/nRF24/RF24/commit/5b0b0f9237fad9d41b02bacfe50e3fe880ce4aa2)
- Typo made BCM driver fail by \@2bndy5 in [`b0f81d3`](https://github.com/nRF24/RF24/commit/b0f81d30647bd4b297146594d1a83b593e23694f)
- Bump version to v1.4.0 by \@2bndy5 in [`9702367`](https://github.com/nRF24/RF24/commit/9702367f45c818ef5c249589a58f95fae7ec724f)
- Exclude only RF24_LINUX by \@2bndy5 in [`4e7a548`](https://github.com/nRF24/RF24/commit/4e7a5487e8a99897ba4bdf24174a68ce61848ee3)
- Apply ptr treatment to Teensy, LittleWire, & Due by \@2bndy5 in [`46f22f0`](https://github.com/nRF24/RF24/commit/46f22f0c29f17e01acd255bb9c4ddeac241610fe)
- Trigger ArduinoCLI action by \@2bndy5 in [`4d68464`](https://github.com/nRF24/RF24/commit/4d684648dcc3fd60f3d3b5f9cc1d3a7e67b43ac4)
- No need to specify output dir in doxygen action by \@2bndy5 in [`55a3312`](https://github.com/nRF24/RF24/commit/55a3312e4996c5c741b41f5951a2f185e8902834)
- Make my life simpler: use new RF24_SPI_PTR macro by \@2bndy5 in [`5bda5c9`](https://github.com/nRF24/RF24/commit/5bda5c96109c978c869eeb2c628d0d1c1601cf8e)
- Forgot about c'tor; fix 2-space indent in begin() by \@2bndy5 in [`f537882`](https://github.com/nRF24/RF24/commit/f537882b684237a59bb87f836b6c54110da1dc9b)
- Abstract SPI::begin() from _init_pins() by \@2bndy5 in [`19b9c32`](https://github.com/nRF24/RF24/commit/19b9c32e9ac90b1e14e9e77c3dbcfdc4f63c0054)
- Adjust csn_pin before SPI.begin() on BCM driver by \@2bndy5 in [`4b4bd71`](https://github.com/nRF24/RF24/commit/4b4bd7108f081107dc7160884abf18afbccfdf79)
- Conforming whitespace by \@2bndy5 in [`ff8f8fb`](https://github.com/nRF24/RF24/commit/ff8f8fb8b310e66613f8c86233123fe7581abd37)
- Rename atxmega.md & add link to supported product by \@2bndy5 in [`d0bdfd8`](https://github.com/nRF24/RF24/commit/d0bdfd8b816bcc41ae7cf56c59f2fe582eb44ab9)
- [PIO CI] use quoted matrix entries by \@2bndy5 in [`94d1441`](https://github.com/nRF24/RF24/commit/94d1441f20614dab1e1e0884ca05b76d8efc42e8)
- [PIO CI] use single quotes? by \@2bndy5 in [`1f65995`](https://github.com/nRF24/RF24/commit/1f659958592aaf1871c587cbbc9abc99b34d7930)
- I hate YML by \@2bndy5 in [`7826e86`](https://github.com/nRF24/RF24/commit/7826e86a7b26cfec45ecd0bd3ef3a4ba1388a32b)
- Gimme that badge by \@2bndy5 in [`101aa2a`](https://github.com/nRF24/RF24/commit/101aa2ac45eb5d6ee5cb49de61d5188ce1c14f15)
- Typo by \@2bndy5 in [`8fb76c8`](https://github.com/nRF24/RF24/commit/8fb76c8b216db7449f86e4e666679a3b44d1ff3e)
- [Linux CI] MRAA install needs special permission by \@2bndy5 in [`fb62fbd`](https://github.com/nRF24/RF24/commit/fb62fbd9d52189aa6b072360d7e6473b4fcc906b)
- Try with "sudo bash -c" by \@2bndy5 in [`ea855d7`](https://github.com/nRF24/RF24/commit/ea855d725d65d100f488dec2d5ded46283e33be7)
- [Linux CI] MRAA not cross compiling (I think) by \@2bndy5 in [`cc95a77`](https://github.com/nRF24/RF24/commit/cc95a77a508dc19bcd7f28a013e6fd3535282e94)
- Avr/pgmspace.h was added to Due core in 2013 by \@2bndy5 in [`22995fd`](https://github.com/nRF24/RF24/commit/22995fdc34a91f064a749821a570f3a25343ace3)
- Oops, undo my testing change by \@2bndy5 in [`54e4886`](https://github.com/nRF24/RF24/commit/54e4886aa0378ec9fceaf69b4ac8451a93cd19b9)
- Ammend support for SPI_UART in csn() by \@2bndy5 in [`9989c20`](https://github.com/nRF24/RF24/commit/9989c205b57d53ac0425dd683f013538ad2a23ac)
- Confirm working 2nd SPI bus on esp8266 \#648 by \@2bndy5 in [`02ba92f`](https://github.com/nRF24/RF24/commit/02ba92f346f180898587b0c4beeaef94fef08ced)
- C-n-p artifact; more indent inconcsistencies by \@2bndy5 in [`0a6aae3`](https://github.com/nRF24/RF24/commit/0a6aae3bfc632bbe1568bca03d215f1023f5829e)
- Comment out theroretical example snippets by \@2bndy5 in [`4b6fb8d`](https://github.com/nRF24/RF24/commit/4b6fb8d4e99b95511df334f8a5a6e06f8f44ee1f)
- Doxygen sux at reading imgs by \@2bndy5 in [`55cc2dd`](https://github.com/nRF24/RF24/commit/55cc2ddf22acd03499103222a00b2a130bc19706)
- Implement \#539 by \@2bndy5 in [`0c433f3`](https://github.com/nRF24/RF24/commit/0c433f3bc30e682eb5d3086609132465007e1d5c)
- My CnP skills are lacking by \@2bndy5 in [`befe021`](https://github.com/nRF24/RF24/commit/befe02112fe989ee082b271e26745a28898725e7)
- Need to test py wrapper by \@2bndy5 in [`1f0335b`](https://github.com/nRF24/RF24/commit/1f0335b4d56a1842dae043101d553213d94f7167)
- [py_wrap] begin fails to compile by \@2bndy5 in [`76f6048`](https://github.com/nRF24/RF24/commit/76f6048963df6252d05ef90cb9ae7fed896a74de)
- [py wrap] oops left the old begin still in there by \@2bndy5 in [`768be5b`](https://github.com/nRF24/RF24/commit/768be5b4405abcbcf07e845b3aaddac7111f416f)
- [py wrap] use thin wrapper for begin_w/_pins by \@2bndy5 in [`fffd6bc`](https://github.com/nRF24/RF24/commit/fffd6bc3d8d48b0a8ca4f1ca3f4d8037d6135006)
- [pr wrap] try explicit begin(void) by \@2bndy5 in [`458e638`](https://github.com/nRF24/RF24/commit/458e63838ca6fadb32fa26027160ef141d016506)
- [py wrap] try using same name for begin(pin, pin) by \@2bndy5 in [`fe87b19`](https://github.com/nRF24/RF24/commit/fe87b19113fd83b6f38df7776682010402cd7cb9)
- [py wrap] c'tor overload is bad by \@2bndy5 in [`bc64e6f`](https://github.com/nRF24/RF24/commit/bc64e6f764f4d0647ed06ba06178a4ce49e5c880)
- [py wrap] "optional<>" template didn't take by \@2bndy5 in [`bba718c`](https://github.com/nRF24/RF24/commit/bba718c46aecf3cab23d86d0db156addb3ae3c4e)
- [py wrap] unmatched parenthesis by \@2bndy5 in [`93817ca`](https://github.com/nRF24/RF24/commit/93817cacbcad82b3b6f45c420418d56b9e2be67f)
- Advise that teensy doesn't need overloaded begin() by \@2bndy5 in [`4e431a6`](https://github.com/nRF24/RF24/commit/4e431a6501c900b76341b38ed4b1cf3a786988e8)

[1.4.0]: https://github.com/nRF24/RF24/compare/v1.3.12...v1.4.0

Full commit diff: [`v1.3.12...v1.4.0`][1.4.0]

## New Contributors
* \@dagix5 made their first contribution in [#742](https://github.com/nRF24/RF24/pull/742)
## [1.3.12] - 2021-02-27

### <!-- 1 --> 🚀 Added

- Add STM32 to supported by \@TMRh20 in [`02ff620`](https://github.com/nRF24/RF24/commit/02ff620a2fc8de3769442a74988efd0a30f1476e)

### <!-- 3 --> 🗑️ Removed

- Clamp setRetries() args & doc main page fixes by \@2bndy5 in [#709](https://github.com/nRF24/RF24/pull/709)

### <!-- 4 --> 🛠️ Fixed

- Fix links to datasheets by \@2bndy5 in [#699](https://github.com/nRF24/RF24/pull/699)
- Fix millis() functionality by \@TMRh20 in [`61a3ffc`](https://github.com/nRF24/RF24/commit/61a3ffc16c6cbad580da79f48ab6c6511001d254)
- Fix print pretty by \@2bndy5 in [#707](https://github.com/nRF24/RF24/pull/707)

### <!-- 9 --> 🗨️ Changed

- Wrapped setters/getters of channel & payloadSize by \@2bndy5 in [#701](https://github.com/nRF24/RF24/pull/701)
- Update version for release by \@TMRh20 in [`11298d5`](https://github.com/nRF24/RF24/commit/11298d52113c3f109928b81043810456b0a809bf)
- Update MulticeiverDemo.ino by \@2bndy5 in [#733](https://github.com/nRF24/RF24/pull/733)
- Community appreciation by \@2bndy5 in [#735](https://github.com/nRF24/RF24/pull/735)
- Conclude work on a small branch by \@2bndy5 in [#737](https://github.com/nRF24/RF24/pull/737)
- Update COMMON_ISSUES.md by \@2bndy5 in [#738](https://github.com/nRF24/RF24/pull/738)
- Merge branch 'master' of https://github.com/nRF24/RF24 by \@TMRh20 in [`2af44fc`](https://github.com/nRF24/RF24/commit/2af44fcac5b0022780a18fddde593359194c634d)

[1.3.12]: https://github.com/nRF24/RF24/compare/v1.3.11...v1.3.12

Full commit diff: [`v1.3.11...v1.3.12`][1.3.12]

## [1.3.11] - 2020-12-18

### <!-- 2 --> 🚫 Deprecated

- New examples and more by \@2bndy5 in [#691](https://github.com/nRF24/RF24/pull/691)

### <!-- 3 --> 🗑️ Removed

- Follow-up fixes from \#691 by \@2bndy5 in [#697](https://github.com/nRF24/RF24/pull/697)

### <!-- 9 --> 🗨️ Changed

- Update library.json by \@TMRh20 in [`a965e6a`](https://github.com/nRF24/RF24/commit/a965e6ae68f0294fcd628700c9675cf8ea75a00c)
- Update library.properties by \@TMRh20 in [`045a689`](https://github.com/nRF24/RF24/commit/045a6898d666a820cd8b995c7fff94d5af268d72)

[1.3.11]: https://github.com/nRF24/RF24/compare/v1.3.10...v1.3.11

Full commit diff: [`v1.3.10...v1.3.11`][1.3.11]

## [1.3.10] - 2020-12-14

### <!-- 1 --> 🚀 Added

- Addressed some Doxygen warnings and issues by \@2bndy5 in [#639](https://github.com/nRF24/RF24/pull/639)
- Added a link to doxygen documentation to CONTRIBUTING.md by \@2bndy5 in [#637](https://github.com/nRF24/RF24/pull/637)
- Add ARC to printDetails() by \@jscrane in [#647](https://github.com/nRF24/RF24/pull/647)
- Add changes to stopListening by \@TMRh20 in [`a0cd707`](https://github.com/nRF24/RF24/commit/a0cd707137607842f0ef782f12f2e40409a12d38)

### <!-- 3 --> 🗑️ Removed

- Remove SPI class for ATTiny: provided by ATTinyCore by \@jscrane in [#646](https://github.com/nRF24/RF24/pull/646)

### <!-- 4 --> 🛠️ Fixed

- Fix for wiringPi by \@TMRh20 in [`34b4892`](https://github.com/nRF24/RF24/commit/34b4892a7293287a2f36fee4e8f391314412a188)

### <!-- 8 --> 📝 Documentation

- Improved CONTRIBUTING.md and README.md by \@2bndy5 in [#635](https://github.com/nRF24/RF24/pull/635)
- Make RF24_POWERUP_DELAY configurable in RF24_config.h by \@jscrane in [#690](https://github.com/nRF24/RF24/pull/690)

### <!-- 9 --> 🗨️ Changed

- Update python wrapper by \@TMRh20 in [`4fd92d1`](https://github.com/nRF24/RF24/commit/4fd92d113359902b8e7b8ea00dd42ed65e676155)
- Update CONTRIBUTING.md by \@Avamander in [`d062771`](https://github.com/nRF24/RF24/commit/d0627715a2e9ff626e7bfbb4d89215f94c9bc8cd)
- Available() could be slightly faster \#631 by \@jscrane in [#650](https://github.com/nRF24/RF24/pull/650)
- Update library.properties by \@TMRh20 in [`7debc6f`](https://github.com/nRF24/RF24/commit/7debc6f6202e6807401a135fd8c91a9ad4c594d5)
- Update library.json by \@TMRh20 in [`c7cfbea`](https://github.com/nRF24/RF24/commit/c7cfbea636dbb552fcf93829b3b397b62dc9814d)

[1.3.10]: https://github.com/nRF24/RF24/compare/v1.3.9...v1.3.10

Full commit diff: [`v1.3.9...v1.3.10`][1.3.10]

## New Contributors
* \@jscrane made their first contribution in [#690](https://github.com/nRF24/RF24/pull/690)
* \@2bndy5 made their first contribution in [#637](https://github.com/nRF24/RF24/pull/637)
## [1.3.9] - 2020-08-22

### <!-- 4 --> 🛠️ Fixed

- Fix compilation errors by \@TMRh20 in [`c0440e0`](https://github.com/nRF24/RF24/commit/c0440e0b7fc425d1c0b9819c9f9bfeac731ec069)

[1.3.9]: https://github.com/nRF24/RF24/compare/v1.3.8...v1.3.9

Full commit diff: [`v1.3.8...v1.3.9`][1.3.9]

## [1.3.8] - 2020-08-17

### <!-- 3 --> 🗑️ Removed

- Improve switching from TX->RX by \@TMRh20 in [`a779828`](https://github.com/nRF24/RF24/commit/a779828026b243585a3407d4895c6ca947d37b89)
- Remove warning in configure by \@TMRh20 in [`59123c6`](https://github.com/nRF24/RF24/commit/59123c6860b668a587aeaa338567129aa3923afa)

### <!-- 8 --> 📝 Documentation

- Adjustment for prev change, doc update by \@TMRh20 in [`19c3357`](https://github.com/nRF24/RF24/commit/19c3357bee57afa006005616aa1d72d24a674874)

### <!-- 9 --> 🗨️ Changed

- Clean up begin() function by \@TMRh20 in [`3272fac`](https://github.com/nRF24/RF24/commit/3272facf4db3a02f7d2b3e8fd56dbf36e568aec6)
- Update for relese by \@TMRh20 in [`bb71b13`](https://github.com/nRF24/RF24/commit/bb71b13d600631f71df4de35df6f5ca5fe363377)

[1.3.8]: https://github.com/nRF24/RF24/compare/v1.3.7...v1.3.8

Full commit diff: [`v1.3.7...v1.3.8`][1.3.8]

## [1.3.7] - 2020-08-01

### <!-- 1 --> 🚀 Added

- Added Constant Carrier Wave by \@waltbar in [#609](https://github.com/nRF24/RF24/pull/609)
- Add CCW to scanner example by \@TMRh20 in [`92b15c7`](https://github.com/nRF24/RF24/commit/92b15c74f0790a89554229eae4a86e0d10578037)

### <!-- 3 --> 🗑️ Removed

- Update printDetails() by \@TMRh20 in [`ed35bd9`](https://github.com/nRF24/RF24/commit/ed35bd93cc981ca1e118556a2e530954cdb80125)
- Cleanup examples by \@TMRh20 in [`6ab44fc`](https://github.com/nRF24/RF24/commit/6ab44fccda97f3a81a664108d4a6432d2297594c)
- Update RF24 docs by \@TMRh20 in [`399f1a9`](https://github.com/nRF24/RF24/commit/399f1a91f50261f9c4ad0828fd8895cf8fd8040e)
- Update python wrapper by \@TMRh20 in [`7bd5f7f`](https://github.com/nRF24/RF24/commit/7bd5f7f99f07e0d3ad2bc5b0f28994c5624b6e02)
- Removed ancient lefovers by \@Avamander in [`4751968`](https://github.com/nRF24/RF24/commit/47519686ccf46545d2fa0b02e0c604fb1646d6d7)
- Removed ancient leftovers by \@Avamander in [`1f83dd7`](https://github.com/nRF24/RF24/commit/1f83dd7e4cb79326a6659311b760f60570170bc7)
- Cleanup of warnings/errors by \@TMRh20 in [`5d6a02a`](https://github.com/nRF24/RF24/commit/5d6a02af52d0bf4e9a9baabb8fe22a4db43b4030)

### <!-- 4 --> 🛠️ Fixed

- Fix bug with RPi timer code by \@TMRh20 in [`13ea68e`](https://github.com/nRF24/RF24/commit/13ea68e06016cc8d164267cd199d4845b115325b)

### <!-- 9 --> 🗨️ Changed

- Merge branch 'master' of https://github.com/nRF24/RF24 by \@TMRh20 in [`e4b5377`](https://github.com/nRF24/RF24/commit/e4b53771c3da9585f1da253a9051c8dbb083d1b6)
- Unify Aduino & Linux Constructor by \@TMRh20 in [`914db31`](https://github.com/nRF24/RF24/commit/914db3115a2d0c06f2fbbe2fa81d7d408f65b003)
- Update RF24.h by \@TMRh20 in [`72f5316`](https://github.com/nRF24/RF24/commit/72f5316ea493f7167c11c7b9a8de4c960f127617)
- Update BCM driver etc by \@TMRh20 in [`1543ad6`](https://github.com/nRF24/RF24/commit/1543ad6e7ecd0d1112541031b0759b8691c0c997)
- Get rid of compilation warning by \@TMRh20 in [`9231656`](https://github.com/nRF24/RF24/commit/92316566c7e6a40343e659efe47f7c101ad346a6)
- En/Disable LNA Gain by \@TMRh20 in [`8c081b3`](https://github.com/nRF24/RF24/commit/8c081b3a934cde8eb3f3b5f68e670a195b39070d)
- Bug Fix! Super Awesome Bug Fix!!!!! by \@TMRh20 in [`3af80f8`](https://github.com/nRF24/RF24/commit/3af80f8fa4ddc741ef9d2f1b9793da0ab5047b1a)
- Update version & supported architectures by \@TMRh20 in [`64b5103`](https://github.com/nRF24/RF24/commit/64b510384b84cc7d13bb5403480db510451d6d0e)
- Update version for release by \@TMRh20 in [`4c6c106`](https://github.com/nRF24/RF24/commit/4c6c106f2dcf09e6a31830ccba32c57a8f749939)

[1.3.7]: https://github.com/nRF24/RF24/compare/v1.3.6...v1.3.7

Full commit diff: [`v1.3.6...v1.3.7`][1.3.7]

## New Contributors
* \@waltbar made their first contribution in [#609](https://github.com/nRF24/RF24/pull/609)
## [1.3.6] - 2020-07-14

### <!-- 4 --> 🛠️ Fixed

- Fix issues with SPI Transactions detection by \@TMRh20 in [`da16f57`](https://github.com/nRF24/RF24/commit/da16f57a1b3d2d1c15628ab1e935e09c6dd06d26)

### <!-- 9 --> 🗨️ Changed

- Linux SPI speed cfg by \@TMRh20 in [`a62aa4a`](https://github.com/nRF24/RF24/commit/a62aa4af0d90c9ca121fb9298b1594e10a936545)
- RF24 Doc Update by \@TMRh20 in [`b25413e`](https://github.com/nRF24/RF24/commit/b25413e40f66d1f0e5a29c05c10569d92f2f14ff)
- Update version for release by \@TMRh20 in [`8f214c1`](https://github.com/nRF24/RF24/commit/8f214c153a2cd8d921570cc14e407574e1a14cbe)

[1.3.6]: https://github.com/nRF24/RF24/compare/v1.3.5...v1.3.6

Full commit diff: [`v1.3.5...v1.3.6`][1.3.6]

## [1.3.5] - 2020-07-08

### <!-- 1 --> 🚀 Added

- Added a few missing keywords for syntax highlighting by \@Avamander in [`939ca40`](https://github.com/nRF24/RF24/commit/939ca409e55cdab14ad24162544c551b7e9acb46)

### <!-- 3 --> 🗑️ Removed

- Revert digitalWrite compatibility changes by \@TMRh20 in [`5e1ac02`](https://github.com/nRF24/RF24/commit/5e1ac023b0101cca3d7277d198d7b87ec8e1e241)

### <!-- 4 --> 🛠️ Fixed

- Fix for creating multiple instances with SPIDEV by \@krzysztof9nowak in [#585](https://github.com/nRF24/RF24/pull/585)
- Fix documentation for build on linux/RPi by \@TMRh20 in [`b323f53`](https://github.com/nRF24/RF24/commit/b323f535aa9644d988370fc6989a2ef9f85aa469)
- Fix for ESP32 program memory error/warning by \@TMRh20 in [`3667a54`](https://github.com/nRF24/RF24/commit/3667a54ed75d6c0b38fc6037710ef2e095189bfe)
- Fixed a compiler warning in Linux example by \@gherlein in [#384](https://github.com/nRF24/RF24/pull/384)
- Fixed undefined behavior in openReadingPipe by \@azais-corentin in [#604](https://github.com/nRF24/RF24/pull/604)

### <!-- 8 --> 📝 Documentation

- Update constructor info to reference docs by \@TMRh20 in [`2e75fc2`](https://github.com/nRF24/RF24/commit/2e75fc2a6b30060d4279a141a8348bd97e5090e9)
- Cleaned up the formatting and improved documentation by \@Avamander in [`e456503`](https://github.com/nRF24/RF24/commit/e456503554529011c4223286a6703fdf411b7733)

### <!-- 9 --> 🗨️ Changed

- Update Doxyfile by \@Avamander in [`85ac82a`](https://github.com/nRF24/RF24/commit/85ac82ab1ffbfcb83a5a0ecf1a05059d3964aad8)
- Documentation update, python & news by \@TMRh20 in [`35c51ff`](https://github.com/nRF24/RF24/commit/35c51ff0ad4f105c3efe9902a2d5fb80457be9f5)
- Cleaned up the style of of the pingpair_ack example by \@Avamander in [`c49c7e9`](https://github.com/nRF24/RF24/commit/c49c7e984806bb00bfa7fc5d77d8a6f266bc7ddd)
- Made sure that printf exists when printDetails is used by \@Avamander in [`53b588f`](https://github.com/nRF24/RF24/commit/53b588f2226284f435db88e3557da6a3be19ee0c)
- Made sure that printf is included when printDetails is used by \@Avamander in [`fcbc804`](https://github.com/nRF24/RF24/commit/fcbc804d2af432848fe7d6a54b8b18e2b25ff5b9)
- Update pingpair_irq_simple.ino by \@Avamander in [`aa8511f`](https://github.com/nRF24/RF24/commit/aa8511f46995698174262c5f2b1f82f726aed3cc)
- Clarified printf usage a bit by \@Avamander in [`7bf5dce`](https://github.com/nRF24/RF24/commit/7bf5dceb21e00ce1b0ec0cbb6f95c9cc5c5fe4ce)
- Increment version for release by \@TMRh20 in [`43f55df`](https://github.com/nRF24/RF24/commit/43f55dfdef228f9455633e3403b386ac0f2ea1bc)

[1.3.5]: https://github.com/nRF24/RF24/compare/v1.3.4...v1.3.5

Full commit diff: [`v1.3.4...v1.3.5`][1.3.5]

## New Contributors
* \@azais-corentin made their first contribution in [#604](https://github.com/nRF24/RF24/pull/604)
* \@gherlein made their first contribution in [#384](https://github.com/nRF24/RF24/pull/384)
* \@krzysztof9nowak made their first contribution in [#585](https://github.com/nRF24/RF24/pull/585)
## [1.3.4] - 2020-02-15

### <!-- 1 --> 🚀 Added

- Added Dolphin files to gitignore just in case by \@Avamander in [`4b5ba72`](https://github.com/nRF24/RF24/commit/4b5ba72751d8a5463cf0a32b95aef7af89186eb5)
- Add function to get automatic retransmission count by \@Avamander in [#558](https://github.com/nRF24/RF24/pull/558)
- Added GitHub actions configuration by \@CoRfr in [#566](https://github.com/nRF24/RF24/pull/566)

### <!-- 3 --> 🗑️ Removed

- Forced braces also on while and for loops, removed unneccesary spaces by \@Avamander in [`71ec8a1`](https://github.com/nRF24/RF24/commit/71ec8a1820f02a5b2b18981251bfb30aad4aeb03)

### <!-- 4 --> 🛠️ Fixed

- Fix in RF24::getDynamicPayloadSize by \@jonnykl in [`ea5dee4`](https://github.com/nRF24/RF24/commit/ea5dee4624d6913708a51c71cf3ae082d2d470f7)
- Fixes incorrect preprocessor statement being used by \@Avamander in [`147ef4d`](https://github.com/nRF24/RF24/commit/147ef4d72fd7f1ddc457a23ee133a66a99b9fc4b)

### <!-- 8 --> 📝 Documentation

- Overloaded digitalWrite to accept boolean by \@Avamander in [`9f55c0c`](https://github.com/nRF24/RF24/commit/9f55c0c577b229f553bca4c73e3045cc89bf73c1)

### <!-- 9 --> 🗨️ Changed

- Update .gitignore by \@Avamander in [`bf25900`](https://github.com/nRF24/RF24/commit/bf259002b2eb0b44df812d7c2d332f70b50525b4)
- Update to latest BCM 2835 driver for RPI4 support by \@TMRh20 in [`e221acd`](https://github.com/nRF24/RF24/commit/e221acdc4315a39016b31c28143639b27421d75a)
- Merge pull request \#520 from jonnykl/fix-get-dyn-pld-size-spi by \@TMRh20 in [#520](https://github.com/nRF24/RF24/pull/520)
- Mod to run RF24 on Raspberry SPI1 by \@Crkk in [`569639a`](https://github.com/nRF24/RF24/commit/569639a5cfedf37239619e29c888bf25395bc0eb)
- Merge pull request \#530 from Crkk/patch-1 by \@TMRh20 in [#530](https://github.com/nRF24/RF24/pull/530)
- Reformatted the entire project using a single code style by \@Avamander in [`0d36fca`](https://github.com/nRF24/RF24/commit/0d36fca8406aaf46558c171b8d85d27da2d4d97e)
- Update CONTRIBUTING.md by \@Avamander in [`d1a5d5e`](https://github.com/nRF24/RF24/commit/d1a5d5e6c5db8f7d2c75289adaba13ec244ee8ad)
- Improved Github templates by \@olback in [#543](https://github.com/nRF24/RF24/pull/543)
- Update -march definition on RPi (BCM2835) to fix compilation by \@CoRfr in [#565](https://github.com/nRF24/RF24/pull/565)
- Improved exception when not running as root by \@CoRfr in [#564](https://github.com/nRF24/RF24/pull/564)
- Improved indentation and formatting by \@Avamander in [`42cf6dd`](https://github.com/nRF24/RF24/commit/42cf6dd201f961bf2b4891d25b16c400bce90198)
- Marked MegaAVR as a supported arch by \@Avamander in [`ddc8c62`](https://github.com/nRF24/RF24/commit/ddc8c62c530d0ba71866d38ba30eac8a8a25b203)
- Marked megaavr as supported by \@Avamander in [`80c456a`](https://github.com/nRF24/RF24/commit/80c456ae3a70c5405d8a5691910c2b3d6489a59b)

[1.3.4]: https://github.com/nRF24/RF24/compare/v1.3.3...v1.3.4

Full commit diff: [`v1.3.3...v1.3.4`][1.3.4]

## New Contributors
* \@CoRfr made their first contribution in [#564](https://github.com/nRF24/RF24/pull/564)
* \@olback made their first contribution in [#543](https://github.com/nRF24/RF24/pull/543)
* \@Crkk made their first contribution
* \@jonnykl made their first contribution
## [1.3.3] - 2019-06-18

### <!-- 1 --> 🚀 Added

- Addi delay to SPI intializer if device already up by \@TMRh20 in [`db6a99b`](https://github.com/nRF24/RF24/commit/db6a99bc0ab98d240bd147c59ff3ff05f1ff5aa0)

### <!-- 4 --> 🛠️ Fixed

- Fix broken stuff after using pgm_read_ptr by \@TMRh20 in [`4220bf5`](https://github.com/nRF24/RF24/commit/4220bf5ce6d804a7ea8a3056102636dda70203de)

### <!-- 8 --> 📝 Documentation

- Update docs for write() per \#500 by \@TMRh20 in [`bd88f9e`](https://github.com/nRF24/RF24/commit/bd88f9e48ba74119ebd9ebd8cdbe6a69ecc4b963)

### <!-- 9 --> 🗨️ Changed

- Clean up printf.h by \@TMRh20 in [`0fabd62`](https://github.com/nRF24/RF24/commit/0fabd62dda48ce621121199078f0ee6790809d30)
- Modify printDetails to work with ESP8266 by \@TMRh20 in [`c25d840`](https://github.com/nRF24/RF24/commit/c25d84089e91c7e206365fe04b80dcbebf48b640)
- One more change for pgm_read_ptr by \@TMRh20 in [`2fa6e8c`](https://github.com/nRF24/RF24/commit/2fa6e8cc6c434e3979c24f5149bd725cb8bffb55)
- Protect against calling bcm_init() more than once by \@TMRh20 in [`b19c3d8`](https://github.com/nRF24/RF24/commit/b19c3d88b99d0c111082d462c4ce203948939323)
- Protect against initializing SPI more than once by \@TMRh20 in [`304d2bb`](https://github.com/nRF24/RF24/commit/304d2bb4c11d0d9b94f7f08170184ee7f89a0863)
- Revert to using initialization protection for SPI by \@TMRh20 in [`9f19b40`](https://github.com/nRF24/RF24/commit/9f19b4054fb196d57259d7bb2524640af950e0f7)
- Improve accuracy of millis() function in SPIDEV by \@TMRh20 in [`3b6391a`](https://github.com/nRF24/RF24/commit/3b6391a06732ad114f5edb7ecc0527bee7811820)
- Enable failure handling, add example by \@TMRh20 in [`13cbad9`](https://github.com/nRF24/RF24/commit/13cbad91df8136b9ca92eb6e76f14432a8a33f19)
- Edit example - forgot to add 'break;' by \@TMRh20 in [`6821e42`](https://github.com/nRF24/RF24/commit/6821e42f2781e81c4b95a52ef0b27b1dd8b634c8)
- Edit RF24_config.h for ESP32 by \@TMRh20 in [`0b24667`](https://github.com/nRF24/RF24/commit/0b2466772ada73b9cb7843128c8aba3a62dafaf0)
- Make attiny SPI code independent from different Arduino pins definitions by \@troffmo5 in [`41866a3`](https://github.com/nRF24/RF24/commit/41866a3354e82cedaf451453a9f1d5225b30ba88)
- Merge pull request \#326 from troffmo5/attiny by \@TMRh20 in [#326](https://github.com/nRF24/RF24/pull/326)
- Update RF24_config.h by \@IkpeohaGodson in [`0015498`](https://github.com/nRF24/RF24/commit/0015498c06b31d524e0471848f6b0891ea56b234)
- Cleaned up formatting of RF24_config.h by \@Avamander in [`279c67b`](https://github.com/nRF24/RF24/commit/279c67b76ed1fcaa3fd94571f8ab46ef0dfe514d)
- Update RF24_config.h by \@Avamander in [`df3f821`](https://github.com/nRF24/RF24/commit/df3f8211976c4c457128f7a2dab606c2bea76728)
- Update RF24_config.h by \@Avamander in [`f3b2fb0`](https://github.com/nRF24/RF24/commit/f3b2fb002386cf9f714b6dec04f959402995ca43)
- Merge pull request \#502 from IkpeohaGodson/patch-1 and clean up RF24_config.h by \@Avamander in [#502](https://github.com/nRF24/RF24/pull/502)
- Update library.json by \@Avamander in [`5f9814f`](https://github.com/nRF24/RF24/commit/5f9814f19731595d39464695cc7d1524b8d1cdf4)
- Update library.properties by \@Avamander in [`fdb4366`](https://github.com/nRF24/RF24/commit/fdb43667485b155269923017668a7650238f075e)

[1.3.3]: https://github.com/nRF24/RF24/compare/v1.3.2...v1.3.3

Full commit diff: [`v1.3.2...v1.3.3`][1.3.3]

## New Contributors
* \@IkpeohaGodson made their first contribution
* \@troffmo5 made their first contribution
## [1.3.2] - 2019-03-22

### <!-- 1 --> 🚀 Added

- Merge pull request \#413 from haseebehsan/pythonWrappers by \@TMRh20 in [#413](https://github.com/nRF24/RF24/pull/413)
- Added an exception if non-superuser uses the library by \@akatran in [#428](https://github.com/nRF24/RF24/pull/428)
- Fixed misleading indent warning in bcm2835.c in [#444](https://github.com/nRF24/RF24/pull/444)

### <!-- 3 --> 🗑️ Removed

- Change perror/abort in SPIDEV driver c++ std exception by \@ludiazv in [`6f74bad`](https://github.com/nRF24/RF24/commit/6f74badb05567bd83b957f5b879060efdcfc085c)

### <!-- 9 --> 🗨️ Changed

- Update pingpair_dyn.py by \@TMRh20 in [`57d3eef`](https://github.com/nRF24/RF24/commit/57d3eef5909dbb15f00afec558514000f24951f3)
- Merge remote-tracking branch 'upstream/master' by \@ludiazv in [`5b09d19`](https://github.com/nRF24/RF24/commit/5b09d193369313946bd28d527f17ca93638d1cb6)
- Merge pull request \#411 from ludiazv/master by \@TMRh20 in [#411](https://github.com/nRF24/RF24/pull/411)
- PyRF24 moved to its specific folder by \@haseebehsan in [`8eaa26d`](https://github.com/nRF24/RF24/commit/8eaa26de6066941a32c68fe0aa33b04ff271143f)
- PyRF24Network python wrapper created by \@haseebehsan in [`570bd1e`](https://github.com/nRF24/RF24/commit/570bd1eafcf545c69a373df3771f223c55a96564)
- PyRF24Mesh python Wrapper created by \@haseebehsan in [`ba335de`](https://github.com/nRF24/RF24/commit/ba335de7751084a68da8f85908b8c185424ea501)
- Keeping previous RF24 boost wrapper as it is by \@haseebehsan in [`2e29f9d`](https://github.com/nRF24/RF24/commit/2e29f9d0661218de6d159aef5645237b3e9af00e)
- Revert "keeping previous RF24 boost wrapper as it is" by \@haseebehsan in [`bbc6b1d`](https://github.com/nRF24/RF24/commit/bbc6b1d452884768bf480e481fb3756552ead2ff)
- Revert "Revert "keeping previous RF24 boost wrapper as it is"" by \@haseebehsan in [`a476c42`](https://github.com/nRF24/RF24/commit/a476c42a7a1b6394817413f8eab26a95cadda102)
- Update RPi BCM driver to latest by \@TMRh20 in [`20b9d77`](https://github.com/nRF24/RF24/commit/20b9d77716e9e75993dc7994e82ec3fb6ad40080)
- SPIDEV compatibility layer improvement and patch for 64bits by \@ludiazv in [#455](https://github.com/nRF24/RF24/pull/455)
- Reorder libs in linker arguments. by \@wamonite in [`4dcf86c`](https://github.com/nRF24/RF24/commit/4dcf86ca47461ac0793bd47acbddf4e36ddcf6d4)
- Ensure pyRF24 constructors match C++ conditionals. by \@wamonite in [`ae4c03b`](https://github.com/nRF24/RF24/commit/ae4c03bc49eef915cbd389fc5339396d7849bb90)
- Merge pull request \#402 from wamonite/dev_up2 by \@TMRh20 in [#402](https://github.com/nRF24/RF24/pull/402)
- Update Transfer.ino by \@HannHank in [`ba5c39f`](https://github.com/nRF24/RF24/commit/ba5c39faeec42795ce65d4310a3b0ce41b031259)
- Merge pull request \#457 from HannHank/patch-1 by \@TMRh20 in [#457](https://github.com/nRF24/RF24/pull/457)
- Update Transfer.ino by \@Avamander in [`a2b35d6`](https://github.com/nRF24/RF24/commit/a2b35d6594160263a5a8c460a9754dd71fba6d95)
- Automatically choose correct SPI clock divider. by \@herm in [`5b1be4e`](https://github.com/nRF24/RF24/commit/5b1be4ef63b26fea8cbbc88a2beffd7198bc47c1)
- Merge pull request \#472 from herm/spi_clock by \@TMRh20 in [#472](https://github.com/nRF24/RF24/pull/472)
- Update delay functions for SPIDEV by \@TMRh20 in [`23d1ca9`](https://github.com/nRF24/RF24/commit/23d1ca92e27490da2b0e98f35dbb2479575cb40f)
- Set delays by CPU speed by \@TMRh20 in [`a3ab7dd`](https://github.com/nRF24/RF24/commit/a3ab7ddb075360deac54fe2c8e2fab286793d21c)
- Update library.properties by \@Avamander in [`bba722a`](https://github.com/nRF24/RF24/commit/bba722a01b7be4c9fbb7431c2af1a686fee753e4)
- Update library.json by \@Avamander in [`56f9bf4`](https://github.com/nRF24/RF24/commit/56f9bf41bd34721308aacf15df607ef7f1fb559c)

[1.3.2]: https://github.com/nRF24/RF24/compare/v1.3.1...v1.3.2

Full commit diff: [`v1.3.1...v1.3.2`][1.3.2]

## New Contributors
* \@herm made their first contribution
* \@HannHank made their first contribution
* \@ludiazv made their first contribution in [#455](https://github.com/nRF24/RF24/pull/455)
* \@haseebehsan made their first contribution
* \@wamonite made their first contribution
## [1.3.1] - 2018-01-28

### <!-- 1 --> 🚀 Added

- Added implementation for wiringPi driver by \@aurelian17 in [#349](https://github.com/nRF24/RF24/pull/349)

### <!-- 3 --> 🗑️ Removed

- Remove progmem macro overrides (ESP8266 Arduino now supports them) by \@sidoh in [#375](https://github.com/nRF24/RF24/pull/375)

### <!-- 4 --> 🛠️ Fixed

- Fix configure for Raspberry PI 2 B+ Raspberian Jessie by \@MathiasMoog in [#366](https://github.com/nRF24/RF24/pull/366)
- Use correct separator in keywords.txt by \@per1234 in [#381](https://github.com/nRF24/RF24/pull/381)
- Fix build issues against ESP32 (ESP-WROOM-32). by \@anders-liu in [#409](https://github.com/nRF24/RF24/pull/409)

### <!-- 9 --> 🗨️ Changed

- Allow redefining SOFTSPI pins with preprocessor flags by \@trojanc in [#372](https://github.com/nRF24/RF24/pull/372)
- New library release and additional architectures by \@Avamander in [#410](https://github.com/nRF24/RF24/pull/410)

[1.3.1]: https://github.com/nRF24/RF24/compare/v1.3.0...v1.3.1

Full commit diff: [`v1.3.0...v1.3.1`][1.3.1]

## New Contributors
* \@anders-liu made their first contribution in [#409](https://github.com/nRF24/RF24/pull/409)
* \@trojanc made their first contribution in [#372](https://github.com/nRF24/RF24/pull/372)
* \@sidoh made their first contribution in [#375](https://github.com/nRF24/RF24/pull/375)
* \@per1234 made their first contribution in [#381](https://github.com/nRF24/RF24/pull/381)
* \@aurelian17 made their first contribution in [#349](https://github.com/nRF24/RF24/pull/349)
* \@MathiasMoog made their first contribution in [#366](https://github.com/nRF24/RF24/pull/366)
## [1.3.0] - 2017-06-01

### <!-- 1 --> 🚀 Added

- Add static initialization for pthread mutexes in [`e204656`](https://github.com/nRF24/RF24/commit/e204656cbdb361a983ee76a7e343ecdb1d22bd54)
- Add to RF24: Arduino Due supported in SPI_UART by \@TMRh20 in [`3b5fc3e`](https://github.com/nRF24/RF24/commit/3b5fc3e85bd11f075f703b1cbe0709c3cada27dd)
- Merge pull request \#247 from corcrash/master in [#247](https://github.com/nRF24/RF24/pull/247)
- Added disableDynamicPayloads() by \@soligen2010 in [#318](https://github.com/nRF24/RF24/pull/318)
- Added isChipConnected(). by \@wmarkow in [#291](https://github.com/nRF24/RF24/pull/291)

### <!-- 4 --> 🛠️ Fixed

- Fixed output and made it consistent with output from Ardunio version. by \@campenr in [`efb8ed1`](https://github.com/nRF24/RF24/commit/efb8ed1f8964ca398184b5130c46819dcf2262c3)
- Fix clamping to a single hex digit by \@nickmooney in [`b37514b`](https://github.com/nRF24/RF24/commit/b37514b9b7199f4d8e8d8fbef11f599eca1e02fa)
- Fix SPIDEV interface by \@plasticassius in [#315](https://github.com/nRF24/RF24/pull/315)

### <!-- 9 --> 🗨️ Changed

- Update to match new RPi scanner example. by \@campenr in [`b233ea1`](https://github.com/nRF24/RF24/commit/b233ea17316a8a37dab77880e8c779e722795b73)
- Merge pull request \#301 from campenr/master by \@TMRh20 in [#301](https://github.com/nRF24/RF24/pull/301)
- Per \#242 change binary to hex by \@TMRh20 in [`aaa09e1`](https://github.com/nRF24/RF24/commit/aaa09e1833dc5bb4705f642c9b003af05a27fd25)
- Merge pull request \#273 from marceloaqno/fix-mutex-init by \@TMRh20 in [#273](https://github.com/nRF24/RF24/pull/273)
- :begin: reset dynamic_payloads_enabled to false by \@derf in [`17e29ea`](https://github.com/nRF24/RF24/commit/17e29ea1fc8480353305c4434a61788acc4e7c02)
- Merge pull request \#319 from derf/master by \@derf in [#319](https://github.com/nRF24/RF24/pull/319)
- Merge branch 'master' into master in [`3c4f842`](https://github.com/nRF24/RF24/commit/3c4f8420dc8c6399837abe937df1799c3ed47506)
- Merge pull request \#304 from nickmooney/patch-1 by \@nickmooney in [#304](https://github.com/nRF24/RF24/pull/304)
- Create CONTRIBUTING.md by \@Avamander in [`ea3b7a6`](https://github.com/nRF24/RF24/commit/ea3b7a66918c86d18081736927f86fdda8c5693f)
- Create LICENSE by \@Avamander in [`706c1d6`](https://github.com/nRF24/RF24/commit/706c1d6eba9082c1221212f270feb4a365642122)
- Simplified logic of disableDynamicPayloads() by \@mattura in [#332](https://github.com/nRF24/RF24/pull/332)
- Make flush_rx public by \@soligen2010 in [#352](https://github.com/nRF24/RF24/pull/352)
- Update library.properties by \@Avamander in [`177bdc5`](https://github.com/nRF24/RF24/commit/177bdc5e3bbdd3a748f3eaca9d9587ca1125ca59)
- Update library.json by \@Avamander in [`4001548`](https://github.com/nRF24/RF24/commit/40015481ebe81cd8ddac33c05194cc41d1a6138a)
- Update library.json by \@Avamander in [`7f14e7f`](https://github.com/nRF24/RF24/commit/7f14e7f1fa0dadfe5a68d1234e51094d3eb6287c)

[1.3.0]: https://github.com/nRF24/RF24/compare/RF24v1.2.0...v1.3.0

Full commit diff: [`RF24v1.2.0...v1.3.0`][1.3.0]

## New Contributors
* \@soligen2010 made their first contribution in [#352](https://github.com/nRF24/RF24/pull/352)
* \@mattura made their first contribution in [#332](https://github.com/nRF24/RF24/pull/332)
* \@plasticassius made their first contribution in [#315](https://github.com/nRF24/RF24/pull/315)
* \@nickmooney made their first contribution in [#304](https://github.com/nRF24/RF24/pull/304)
* \@derf made their first contribution in [#319](https://github.com/nRF24/RF24/pull/319)
* \@campenr made their first contribution
## [RF24v1.2.0] - 2016-11-05

### <!-- 1 --> 🚀 Added

- Add link to RPi/Linux Blog Post in documentation by \@TMRh20 in [`63c83d4`](https://github.com/nRF24/RF24/commit/63c83d4f4e913185e510d2ef291ca608605bfd20)

### <!-- 4 --> 🛠️ Fixed

- Fix bad documentation link from last commit by \@TMRh20 in [`c94f877`](https://github.com/nRF24/RF24/commit/c94f87746552a3c662c35fd11e772b747f19d8ee)

### <!-- 8 --> 📝 Documentation

- Minor documentation correction - RF24_CRC_DISABLED by \@pfeerick in [`3766eeb`](https://github.com/nRF24/RF24/commit/3766eebf139ed83fb5129ce06521c6168d2749c2)
- Merge pull request \#286 from pfeerick/patch-1 by \@TMRh20 in [#286](https://github.com/nRF24/RF24/pull/286)

### <!-- 9 --> 🗨️ Changed

- Update PlatformIO library manifest by \@valeros in [`aa54114`](https://github.com/nRF24/RF24/commit/aa54114b6c29e8dcae221d90c73288b07042033e)
- Merge pull request \#280 from valeros/patch-1 by \@Avamander in [#280](https://github.com/nRF24/RF24/pull/280)
- Let's PlatformIO decide how to build Arduino v1.0 library by \@ivankravets in [`903fc55`](https://github.com/nRF24/RF24/commit/903fc55e364cfe9fe87c63e512bc8bb05152de85)
- Merge pull request \#284 from ivankravets/patch-2 by \@TMRh20 in [#284](https://github.com/nRF24/RF24/pull/284)
- Make internal delays configurable by \@TMRh20 in [`195f5ac`](https://github.com/nRF24/RF24/commit/195f5ac1d7451d72f320de3c050c20a1ccd3ff0f)
- Update Library Version 1.2 by \@TMRh20 in [`8ea5127`](https://github.com/nRF24/RF24/commit/8ea5127078394591a0af5b409c38fa11cb0677d9)

[RF24v1.2.0]: https://github.com/nRF24/RF24/compare/v1.1.7...RF24v1.2.0

Full commit diff: [`v1.1.7...RF24v1.2.0`][RF24v1.2.0]

## New Contributors
* \@pfeerick made their first contribution
* \@valeros made their first contribution
## [1.1.7] - 2016-08-12

### <!-- 1 --> 🚀 Added

- Add interrupts to SPIDEV by \@TMRh20 in [`d705abd`](https://github.com/nRF24/RF24/commit/d705abd4073defa0a5d338f6d58a604597f2aff7)
- Add sudo when linking for old lib name via remote by \@TMRh20 in [`87f3d11`](https://github.com/nRF24/RF24/commit/87f3d11033c70e9ca5032295ca5ffd5cdc665f7f)
- Support for ATXMega256D3 by \@akatran in [`990f3f7`](https://github.com/nRF24/RF24/commit/990f3f739ece5b5f6c25dc25779a9e2c7d7b014b)
- Merge pull request \#238 from akatran/master by \@TMRh20 in [#238](https://github.com/nRF24/RF24/pull/238)
- Merge pull request \#267 from mz-fuzzy/doc_update by \@TMRh20 in [#267](https://github.com/nRF24/RF24/pull/267)

### <!-- 3 --> 🗑️ Removed

- Remove the custom PROGRAM SPACE definitions and use avr/pgmspace! by \@akatran in [`a8f100b`](https://github.com/nRF24/RF24/commit/a8f100b27c902019c852c20c8999ee5195d0a2e4)
- Documentation update: by \@martin-mat in [`3f54a0a`](https://github.com/nRF24/RF24/commit/3f54a0aec951426cc66046ccd8f4dccaab657635)

### <!-- 4 --> 🛠️ Fixed

- Fix progmem inclusion \#252 by \@TMRh20 in [`372562a`](https://github.com/nRF24/RF24/commit/372562a2ffb81f43504ff35fdd17ec98fbca26e8)
- Fix path to platform specific files in library.json for platformio by \@rnovacek in [`13ce2bb`](https://github.com/nRF24/RF24/commit/13ce2bb58b1fdfa5e55d1b32d69ef8c31582e3b6)
- Merge pull request \#263 from rnovacek/master by \@TMRh20 in [#263](https://github.com/nRF24/RF24/pull/263)

### <!-- 9 --> 🗨️ Changed

- \#234 by \@TMRh20 in [`2b558d0`](https://github.com/nRF24/RF24/commit/2b558d05c8656df26fa3d4aab87c54785e94b526)
- Change CONFIG to NRF_CONFIG per \#212 by \@TMRh20 in [`b250082`](https://github.com/nRF24/RF24/commit/b250082605a8fafbc866f61e21a50c351698a819)
- Duplicate read NRF_CONFIG per \#203 in powerUp() by \@TMRh20 in [`9708f72`](https://github.com/nRF24/RF24/commit/9708f720ebaf372f4d90104577be5ee481ab1cc0)
- ATTiny 861 support per \#180 by \@TMRh20 in [`970309d`](https://github.com/nRF24/RF24/commit/970309d18ca0ebcb7d23cedc27dc398529476e34)
- Makefile and project structure improvements by \@martin-mat in [`3b13827`](https://github.com/nRF24/RF24/commit/3b138276e919c96d7d578273f5b113e21d30e985)
- Configure - cli parameters by \@martin-mat in [`891becd`](https://github.com/nRF24/RF24/commit/891becdf89cd660db865b9ae8f5acaddc713e891)
- Python wrapper cross-compilation refinements by \@martin-mat in [`2c6d28a`](https://github.com/nRF24/RF24/commit/2c6d28af43654736df9ebdccb3388c648406889b)
- Configured compiler binary to be respected in setup.py by \@martin-mat in [`4d94f59`](https://github.com/nRF24/RF24/commit/4d94f5931338f873c327692b77063ef4df12cf0c)
- Makefile improvemets rebasing leftovers - BBB -> SPIDEV by \@martin-mat in [`87b5545`](https://github.com/nRF24/RF24/commit/87b554518b04a2990566ae61c61c79c134eb09f5)
- Makefile: 'configure' target added by \@martin-mat in [`46824af`](https://github.com/nRF24/RF24/commit/46824af345f4c567348b31d0708b6fe7d5f0be7e)
- Linux library name backward compatibilty using simlink by \@martin-mat in [`ef1914b`](https://github.com/nRF24/RF24/commit/ef1914b47506750d26ddbd89110700b689fea1c2)
- Pthread switch added to compile phase by \@martin-mat in [`0f928c0`](https://github.com/nRF24/RF24/commit/0f928c043c6cb3b8f939723c301059ff41329938)
- SPIDEV fix non-root user segfault after startup by \@TMRh20 in [`874fee8`](https://github.com/nRF24/RF24/commit/874fee84452fa6ee09da99139a3746754d28c276)
- RPi Bugfix: False Hardware Failure indication by \@TMRh20 in [`5d49fd6`](https://github.com/nRF24/RF24/commit/5d49fd6bfeb8682415bfda23e97b9d1c12b9b5e3)
- RPi Interrupts w/SPIDEV Bugfix: Enable mutex lock by \@TMRh20 in [`540e10d`](https://github.com/nRF24/RF24/commit/540e10d588f630ca74bf715d3542d65dee72156d)
- Revert NRF_CONFIG in printDetails to CONFIG by \@TMRh20 in [`406265e`](https://github.com/nRF24/RF24/commit/406265e317716f7cd8aaf98b23f1f6f744199ac6)
- Spidev fix: don't close spidev if not open before by \@martin-mat in [`4c52feb`](https://github.com/nRF24/RF24/commit/4c52feb23cdbf12a5e6fc82c10156825255ff802)
- Avoid memory corruption when calling RF24::read(0) on linux machines by \@martin-mat in [`2f2cb2b`](https://github.com/nRF24/RF24/commit/2f2cb2bbbb404a26e4c21bd5b0a5e8b22128df68)
- Merge branch 'makefile_improvements' of https://github.com/TMRh20/RF24 into master by \@martin-mat in [`4507fd1`](https://github.com/nRF24/RF24/commit/4507fd15c67b277e18cb953112cf5c885e7f5dec)
- Linux_examples makefile: move libs after source files by \@martin-mat in [`061341e`](https://github.com/nRF24/RF24/commit/061341ed5945d7808fd82ab840c6d673bee12389)
- Makefile/configure - fix of recursive 'make clean' call when using 'make -B' by \@martin-mat in [`6c8e302`](https://github.com/nRF24/RF24/commit/6c8e302ad0abf1a90053246ce7434216fbd17696)
- Changed ce_pin and csn_pin to uint16_t so the library supports GPIO identifiers higher than 255 (Minnowboard) in [`eb05670`](https://github.com/nRF24/RF24/commit/eb056706e9fba23fe8f2c97c8b46727f15ef8d8c)
- Change back to default the user setting FAILURE_HANDLING by \@akatran in [`f6e5ee7`](https://github.com/nRF24/RF24/commit/f6e5ee7a69ed5ae2cdd5e33a817e95995c30dc8b)
- Merge remote-tracking branch 'refs/remotes/TMRh20/master' by \@akatran in [`a9255c8`](https://github.com/nRF24/RF24/commit/a9255c808259138d3b53df5e7db71379e694c000)
- Merge branch 'master' of https://github.com/TMRh20/RF24 by \@akatran in [`9e71772`](https://github.com/nRF24/RF24/commit/9e717722d83dd74554ff424b7e817409f5df0efa)
- Update from TMRh20/master and merge with local. by \@akatran in [`ef70a4f`](https://github.com/nRF24/RF24/commit/ef70a4fe7fc1a2c8ebcfab4189a2187c7e891f3d)
- Update README.md by \@akatran in [`a466b36`](https://github.com/nRF24/RF24/commit/a466b36e6c8497a59f8a52a22f63cb1af415484b)
- Update compatibility.c by \@akatran in [`4d52e67`](https://github.com/nRF24/RF24/commit/4d52e67a1ffa6b386494ae27c6c8e26bc5ba6610)
- Update RF24.h - Add comments for github.io by \@akatran in [`22a719f`](https://github.com/nRF24/RF24/commit/22a719fee33e4f9aa0d9caa4150299b2c5d4ad4d)
- Merge remote-tracking branch 'refs/remotes/TMRh20/master' by \@akatran in [`c4dd23b`](https://github.com/nRF24/RF24/commit/c4dd23b38a14f094fa765368780561e8d0a5eb7f)
- Revert back (correctly) the README.md by \@akatran in [`db715ad`](https://github.com/nRF24/RF24/commit/db715ad34eb9d82bcf27992c7eb96ceeb0224ed7)
- Change back the RF24_config.h to be in sync with TMRh20/master by \@akatran in [`a2c7047`](https://github.com/nRF24/RF24/commit/a2c7047be4aa1a5b366ee1da092a17163b69a943)
- Configure - fix of wrong soname to respect major library version by \@martin-mat in [`a296ce6`](https://github.com/nRF24/RF24/commit/a296ce68af965152d91324f79256905aba0c887a)
- Linux config: H3 detection and gcc switches. by \@martin-mat in [`244dfb1`](https://github.com/nRF24/RF24/commit/244dfb128ba3ea5010f7d40cea03b8dfd7ccf77b)
- Change examples_RPi to examples_linux by \@gitgrimbo in [`a69535d`](https://github.com/nRF24/RF24/commit/a69535d9b310c93fde884e4385d17406c89af7d0)
- Merge pull request \#254 from gitgrimbo/fix-examples-folder-name by \@martin-mat in [#254](https://github.com/nRF24/RF24/pull/254)
- Change detection for ATXMega256D3 by \@akatran in [`abf4f29`](https://github.com/nRF24/RF24/commit/abf4f29926a8f5461199148e820effad2b96bc5d)
- Changes to include XMEGA D3 family. by \@akatran in [`bdc7446`](https://github.com/nRF24/RF24/commit/bdc74464249313958c910f66ff9ca957580fb6b4)
- Merge pull request \#251 from akatran/master by \@Avamander in [#251](https://github.com/nRF24/RF24/pull/251)
- Update library.properties by \@Avamander in [`4e9f1fb`](https://github.com/nRF24/RF24/commit/4e9f1fb992c73d720cbebdd3755cd64001ffe3e2)

[1.1.7]: https://github.com/nRF24/RF24/compare/v1.1.6...v1.1.7

Full commit diff: [`v1.1.6...v1.1.7`][1.1.7]

## New Contributors
* \@rnovacek made their first contribution
* \@gitgrimbo made their first contribution
* \@akatran made their first contribution
## [1.1.6] - 2016-02-20

### <!-- 1 --> 🚀 Added

- Added possibility/demonstration of IRQ pin usage for RPi/Python. by \@martin-mat in [`031d388`](https://github.com/nRF24/RF24/commit/031d3888e837fa2ef23e92e3923ddc10defbcbf8)

### <!-- 4 --> 🛠️ Fixed

- Fix GettingStarted sample by \@SaintGimp in [`81f1c0d`](https://github.com/nRF24/RF24/commit/81f1c0d50365e612fe96f32de573d43f69983584)
- Fix warning: C++11 requires a space between literal and identifier by \@flavio-fernandes in [`c3a0996`](https://github.com/nRF24/RF24/commit/c3a0996b20fa17aa7f4f58630c3e516b189a6aa9)
- Fix for \#217 GettingStarted enhancement. by \@wmarkow in [`d0bbfaa`](https://github.com/nRF24/RF24/commit/d0bbfaa28fe8114a46e72d76475763cc20f7e10e)
- Fix for \#213. by \@wmarkow in [`5a0ec58`](https://github.com/nRF24/RF24/commit/5a0ec58516a85d77163241a824623e96a4602d6a)

### <!-- 9 --> 🗨️ Changed

- Changed serial baudrate by \@Avamander in [`842d048`](https://github.com/nRF24/RF24/commit/842d0489a67d1f2692b731133df7555e49d521a7)
- Updated serial baudrate by \@Avamander in [`14b8fb0`](https://github.com/nRF24/RF24/commit/14b8fb0b5acdbd68da0e01e87970ca019adfde7b)
- Merge branch 'master' of https://github.com/TMRh20/RF24 by \@TMRh20 in [`c9feaf5`](https://github.com/nRF24/RF24/commit/c9feaf5161b5aa928a12a45f0314b81c880ec1cb)
- Checking for wrong return value in detachInterrupt of pthread_cancel and close. by \@Oitzu in [`72fe373`](https://github.com/nRF24/RF24/commit/72fe373e312e5f1a44f12476fd6648494ae7418f)
- Python wrapper now respects RPI/SPIDEV/MRAA RF24 configuration. by \@martin-mat in [`95926b2`](https://github.com/nRF24/RF24/commit/95926b2c9aeedd1cdae011a1bb0f54f40b9a7726)
- Merge pull request \#200 from SaintGimp/FixGettingStarted by \@Avamander in [`58f6d8d`](https://github.com/nRF24/RF24/commit/58f6d8dbc5d6f0e8a25a5a6dd680aa62a520fab3)
- Merge pull request \#210 from flavio-fernandes/topic/fix_warning by \@Avamander in [`47435fd`](https://github.com/nRF24/RF24/commit/47435fdd3489e5550aa31487f4556da832955bfc)
- Fixed function to clear interrupt bits first. by \@ab316 in [`0242486`](https://github.com/nRF24/RF24/commit/0242486985504f34c11a4fb945c641bc04878b96)
- Merge pull request \#206 from ab316/master by \@Avamander in [`f043cb7`](https://github.com/nRF24/RF24/commit/f043cb780db1ea519b387dbbc7339836ec314497)
- Update library.properties by \@Avamander in [`50cf850`](https://github.com/nRF24/RF24/commit/50cf850a87ddbf422ce1f527e901601d4d15f5ec)
- Merge pull request \#218 from wmarkow/217_fix by \@Avamander in [`8c7adbc`](https://github.com/nRF24/RF24/commit/8c7adbcc4cb0de89060325ac854b2e75cdfee2b3)
- Merge pull request \#216 from wmarkow/213_fix by \@Avamander in [`99ada7b`](https://github.com/nRF24/RF24/commit/99ada7ba2dd8790080d3cdb319a240d80da93523)
- Revert "Fix for \#217 GettingStarted enhancement." by \@Avamander in [`df225d6`](https://github.com/nRF24/RF24/commit/df225d641b0fc563b3eae15af96dc741c42092a6)
- Merge pull request \#219 from TMRh20/revert-218-217_fix by \@Avamander in [`fd89aae`](https://github.com/nRF24/RF24/commit/fd89aae4f0c65874453478d3d65deff70f384196)

[1.1.6]: https://github.com/nRF24/RF24/compare/v1.1.5...v1.1.6

Full commit diff: [`v1.1.5...v1.1.6`][1.1.6]

## New Contributors
* \@wmarkow made their first contribution
* \@flavio-fernandes made their first contribution
* \@ab316 made their first contribution
* \@SaintGimp made their first contribution
## [1.1.5] - 2016-01-07

### <!-- 1 --> 🚀 Added

- Add bcm2835 lib v1.48 by \@TMRh20 in [`18daa4c`](https://github.com/nRF24/RF24/commit/18daa4c4ddaef1c5bb3048c0a891c45030955c46)
- Added comments to RPi interrupt files. by \@Oitzu in [`2bb6f32`](https://github.com/nRF24/RF24/commit/2bb6f325f00b3427a0dc850c8e23798c05a1ffe9)
- Add mutex locking to SPI functions for RPi by \@TMRh20 in [`715edc5`](https://github.com/nRF24/RF24/commit/715edc5e39ca04e352e8d6c26784972c13b9a1e8)
- Add 'static' to void in spi.h by \@TMRh20 in [`ed9ae08`](https://github.com/nRF24/RF24/commit/ed9ae08dbab6036fd42b1376462d74a94cf90bf4)

### <!-- 3 --> 🗑️ Removed

- Update RPi call_response_int2 example by \@TMRh20 in [`bf06db9`](https://github.com/nRF24/RF24/commit/bf06db96b94c582360cee454a1210f5e4851b5ad)

### <!-- 4 --> 🛠️ Fixed

- Fix Warning. ‘void* interruptHandler(void*)’ declared ‘static’ but never defined. by \@Oitzu in [`409cbf7`](https://github.com/nRF24/RF24/commit/409cbf78ee5e779f3abb2c2cc6a3f22abc809fba)
- Fixes for last commit by \@TMRh20 in [`a57fafe`](https://github.com/nRF24/RF24/commit/a57fafe017961b8cfdef2ccac3d764721c42e62d)
- Fix per \#183 by \@TMRh20 in [`9e68f9e`](https://github.com/nRF24/RF24/commit/9e68f9e133f426322d9b512239fc952b2dd3b1b9)

### <!-- 9 --> 🗨️ Changed

- Initial commit for interrupts based on wiringPi. by \@Oitzu in [`67d0285`](https://github.com/nRF24/RF24/commit/67d0285fc5ceae2b6f50f453ee9926490e629bd4)
- Merged TMRh20's changes. by \@Oitzu in [`f4a6073`](https://github.com/nRF24/RF24/commit/f4a6073bc9a34717d7c09d07bbb466bf6d32c6c9)
- Moving pthread to shared linker flags. by \@Oitzu in [`e83bc9a`](https://github.com/nRF24/RF24/commit/e83bc9affb30c6a1c90bf63de72d19aabcd47162)
- Merge pull request \#173 from Oitzu/master by \@TMRh20 in [`a381194`](https://github.com/nRF24/RF24/commit/a381194bf0a5e1e1ecb06a7f0c502791e049d77c)
- Mutex handling for interrupts on RPi by \@TMRh20 in [`243e25f`](https://github.com/nRF24/RF24/commit/243e25f23a6d3ff4f3d57ec2f89419ac84d36c16)
- Deadlocks due to RPi SPI mutex code by \@TMRh20 in [`9a5ea80`](https://github.com/nRF24/RF24/commit/9a5ea80cacf9f50cce0b1600da4b5a55596b1011)
- Move interrupt examples, add new int examples by \@TMRh20 in [`2c4d199`](https://github.com/nRF24/RF24/commit/2c4d1991b1a314e7f0ad5a3d808761e36002c2b5)
- Initial commit RPI-SPI-Transactions. by \@Oitzu in [`e1fe438`](https://github.com/nRF24/RF24/commit/e1fe43814cca7af57b7515f999f70356b51fd3b1)
- Implement detachInterrupt method. See \#173 by \@Oitzu in [`b615951`](https://github.com/nRF24/RF24/commit/b615951a3d59ed258b61daa40f3a090b02cb6a00)
- Minor chgs to mirror Arduino SPI transaction API by \@TMRh20 in [`f285fde`](https://github.com/nRF24/RF24/commit/f285fde257438e7fe482ff4c6dc03f0de185b7e5)
- One more per \#183 by \@TMRh20 in [`d37d030`](https://github.com/nRF24/RF24/commit/d37d030495483a2c6522b2f259b46974d3d0f623)
- Merge remote-tracking branch 'refs/remotes/origin/Interrupts' by \@TMRh20 in [`3e8147a`](https://github.com/nRF24/RF24/commit/3e8147a1a150f59908d3e2bb43719193d54a54fc)
- RPI2 autodetection by \@yozik04 in [`d7cacdc`](https://github.com/nRF24/RF24/commit/d7cacdc20bbb89632f8b9ee1a1bba439cf80de79)
- RPI2 autodetection in examples by \@yozik04 in [`b417d58`](https://github.com/nRF24/RF24/commit/b417d58ce10cf4f18bcffa8b6869c8b53d729e4f)
- Merge pull request \#179 from yozik04/master by \@TMRh20 in [`1c78dd0`](https://github.com/nRF24/RF24/commit/1c78dd007220687f6c262b970c7b7e65819ddece)
- Update library.properties by \@Avamander in [`8115a34`](https://github.com/nRF24/RF24/commit/8115a3436f37685e83f4ddde1eb26ba1962829ea)

[1.1.5]: https://github.com/nRF24/RF24/compare/v1.1.4...v1.1.5

Full commit diff: [`v1.1.4...v1.1.5`][1.1.5]

## New Contributors
* \@yozik04 made their first contribution
* \@Oitzu made their first contribution
## [1.1.4] - 2015-12-14

### <!-- 3 --> 🗑️ Removed

- Doc Update: Add current info to news by \@TMRh20 in [`f6fa0be`](https://github.com/nRF24/RF24/commit/f6fa0be4931149050d340695a15ddaa15afcf124)

### <!-- 4 --> 🛠️ Fixed

- Fixed inconsistent documentation, added Tinyx313 pin map by \@zador-blood-stained in [`b5228f3`](https://github.com/nRF24/RF24/commit/b5228f3775bb520e90fe40aff49b1f28b47cf7f7)
- Fix broken transfer example by \@TMRh20 in [`13c069c`](https://github.com/nRF24/RF24/commit/13c069caf1e74021da0517d349741d3cb2fb1e7d)

### <!-- 8 --> 📝 Documentation

- Merge pull request \#155 from zador-blood-stained/master by \@TMRh20 in [`4554f73`](https://github.com/nRF24/RF24/commit/4554f739e7087852a7375b29172df7e5df03838c)
- Update .h file per doc changes by \@TMRh20 in [`e192580`](https://github.com/nRF24/RF24/commit/e192580383d332bb7e2398a7ae057c41b1918d87)

### <!-- 9 --> 🗨️ Changed

- Python3 support added by \@martin-mat in [`cda3e18`](https://github.com/nRF24/RF24/commit/cda3e18b0d011fca4e65100918ab6bb3477655b1)
- Merge pull request \#164 from mz-fuzzy/python3 by \@TMRh20 in [`90dd53e`](https://github.com/nRF24/RF24/commit/90dd53e1abb68c72bf5035a8c9497d83189fb680)
- Update library.properties by \@Avamander in [`e888f55`](https://github.com/nRF24/RF24/commit/e888f55864485f4d919f7753ddb479702264fb1f)

[1.1.4]: https://github.com/nRF24/RF24/compare/v1.1.3...v1.1.4

Full commit diff: [`v1.1.3...v1.1.4`][1.1.4]

## [1.1.3] - 2015-10-31

### <!-- 3 --> 🗑️ Removed

- ESP8266 printDetails, Docs by \@TMRh20 in [`aeb8b42`](https://github.com/nRF24/RF24/commit/aeb8b427304af7c5d45e03330a013ae98f895040)
- Removed usage of hardcoded SS pins by \@zador-blood-stained in [`1376fbd`](https://github.com/nRF24/RF24/commit/1376fbd8ce63c730068d2b6c7d160b990b497a3d)

### <!-- 9 --> 🗨️ Changed

- Update gettingstarted.cpp by \@cguzmanceti in [`f3cc137`](https://github.com/nRF24/RF24/commit/f3cc137cb669f5ae61ecd45c89d7e4946c87d5d8)
- Merge pull request \#153 from cguzmanceti/patch-1 by \@TMRh20 in [`0bf4998`](https://github.com/nRF24/RF24/commit/0bf499884f6c1a2cd6e251b70c211cd596a478ea)
- Update gettingstarted_call_response.cpp by \@cguzmanceti in [`c5fdf1b`](https://github.com/nRF24/RF24/commit/c5fdf1be64606b11dd5f07ba05204d38c4768102)
- Merge pull request \#154 from cguzmanceti/patch-2 by \@TMRh20 in [`dfb2395`](https://github.com/nRF24/RF24/commit/dfb2395557ad93d659018dea5efd91e0bfeb46e4)
- ATTiny2313/4313 support by \@zador-blood-stained in [`ac0ff44`](https://github.com/nRF24/RF24/commit/ac0ff44a90d80d632aad3fba983c5020b5773535)
- Merge pull request \#152 from zador-blood-stained/master by \@TMRh20 in [`07e8362`](https://github.com/nRF24/RF24/commit/07e83623032c5aeb78b961cad4fe5491092860ef)
- Change printf to cout in RPi examples by \@TMRh20 in [`3652890`](https://github.com/nRF24/RF24/commit/365289022b60a951d99442f138daebfe330924bf)

[1.1.3]: https://github.com/nRF24/RF24/compare/v1.1.2...v1.1.3

Full commit diff: [`v1.1.2...v1.1.3`][1.1.3]

## New Contributors
* \@cguzmanceti made their first contribution
* \@zador-blood-stained made their first contribution
## [1.1.2] - 2015-10-20

### <!-- 9 --> 🗨️ Changed

- Delete FAQ by \@Avamander in [`69c1f3c`](https://github.com/nRF24/RF24/commit/69c1f3cc402e51e632e456f592dfeb3351dd6c6c)
- Update library.properties by \@Avamander in [`a045c5a`](https://github.com/nRF24/RF24/commit/a045c5ac3c3f3304b793d8baeffa066d2a88e536)
- Made possible many examples to run in 512 bytes ram chips, fixed few typos, missing F() macro calls and other minor improvements by \@fab672000 in [`0483d36`](https://github.com/nRF24/RF24/commit/0483d3604116f0ae79cc17ba0d522f58b968333c)
- Restored back multicast pingpair example from previous maniacbug  fork by \@fab672000 in [`aab3a52`](https://github.com/nRF24/RF24/commit/aab3a529f898ec40e3d62510cd535d5e2d8a5ac4)
- Merge pull request \#141 from fab672000/master by \@Avamander in [`46ccceb`](https://github.com/nRF24/RF24/commit/46ccceb65f98dd505b9eb34ce4d5f4fd56c84d5d)
- Update library.properties by \@Avamander in [`7a44681`](https://github.com/nRF24/RF24/commit/7a446813de8278117920aa8c0ef06da599a94280)

[1.1.2]: https://github.com/nRF24/RF24/compare/v1.1.1...v1.1.2

Full commit diff: [`v1.1.1...v1.1.2`][1.1.2]

## New Contributors
* \@fab672000 made their first contribution
## [1.1.1] - 2015-10-01

### <!-- 9 --> 🗨️ Changed

- Increment version - test by \@TMRh20 in [`b305bb4`](https://github.com/nRF24/RF24/commit/b305bb4bba4c2a3db65f07f5c21d67923508d9df)

[1.1.1]: https://github.com/nRF24/RF24/compare/v1.0.1...v1.1.1

Full commit diff: [`v1.0.1...v1.1.1`][1.1.1]

## [1.0.1] - 2015-10-01

### <!-- 9 --> 🗨️ Changed

- Increment library.properties by \@TMRh20 in [`c98f396`](https://github.com/nRF24/RF24/commit/c98f39633070d02c3a1142094114390c93b6eb14)
- Merge branch 'Updates' by \@TMRh20 in [`a4f412c`](https://github.com/nRF24/RF24/commit/a4f412c37f9178f3bee449a92d985398c53ae492)

[1.0.1]: https://github.com/nRF24/RF24/compare/v1.0...v1.0.1

Full commit diff: [`v1.0...v1.0.1`][1.0.1]

## [1.0] - 2015-10-01

### <!-- 1 --> 🚀 Added

- Added by \@maniacbug in [`de083c9`](https://github.com/nRF24/RF24/commit/de083c964d9aeeb9fda7485c39fed27443cd617c)
- Added a small documentation build system by \@maniacbug in [`0b89d37`](https://github.com/nRF24/RF24/commit/0b89d37f19176f14fbbb8e35bced7c54b5452729)
- Added new star-topology example by \@maniacbug in [`37d8e8b`](https://github.com/nRF24/RF24/commit/37d8e8b17d4672d297f3ffe1d031008f862a2ef4)
- Added an indication of which pipe a message came from.  Fixed a bug where pipes 2+ were not by \@maniacbug in [`c8ed556`](https://github.com/nRF24/RF24/commit/c8ed556aba2259a2c658eecf9ef38494918647ce)
- Added new sample, based on pingpair by \@maniacbug in [`997140e`](https://github.com/nRF24/RF24/commit/997140e0d46b6ec98da293a6a280bfd413bdb050)
- Added ack payload.  Added a sample using pingpair as a base with minimal changes. by \@maniacbug in [`79628e7`](https://github.com/nRF24/RF24/commit/79628e7849cc3b9f92bc7203ed3d8fbe49d8e537)
- Added powerDown(). by \@maniacbug in [`b697fd4`](https://github.com/nRF24/RF24/commit/b697fd4e79453c2450b88a441e2e1a36e78442fc)
- Added server time to printout.  Power disable/enable before/after sleeping.  Changed timer to 4s. by \@maniacbug in [`ee4568e`](https://github.com/nRF24/RF24/commit/ee4568ede4108e19cfd5426841a7dd20c489e3c3)
- Added a full constructor for payload_t by \@maniacbug in [`4f6c4c9`](https://github.com/nRF24/RF24/commit/4f6c4c909e3534dcf63051d49fcee1a3237d13f8)
- Added relay role, and tweaked base to work with it by \@maniacbug in [`2e7ea67`](https://github.com/nRF24/RF24/commit/2e7ea6709325e7a517ff5dadbdf993e4aa7d302e)
- Added self-healing ping offset to leaf node.  Made timeout more parallelized by \@maniacbug in [`7b71380`](https://github.com/nRF24/RF24/commit/7b7138027893b1e31ba49398e314a64890a908ad)
- Added more documentation by \@maniacbug in [`6ec7640`](https://github.com/nRF24/RF24/commit/6ec7640a7a6f9cccacb313a44a6516fef6457148)
- Added extra debugging readability by \@maniacbug in [`badc86c`](https://github.com/nRF24/RF24/commit/badc86cfe0bc247fab6a0bec2874413b9dc7e443)
- Added carrier detect and auto-ack disable, and an example to use it by \@maniacbug in [`bee284f`](https://github.com/nRF24/RF24/commit/bee284f9c5478880a78c019dd1f565e4fc1ffd2c)
- Added carrier detect and auto ack enable/disable by \@maniacbug in [`a6e7444`](https://github.com/nRF24/RF24/commit/a6e7444059c516115a31f988e657be5672a1ed71)
- Added setDataRate() by \@maniacbug in [`4131420`](https://github.com/nRF24/RF24/commit/4131420de79456be80e218523bf1c3c0ab36b2a1)
- Added 3rd FTDI port for my own use by \@maniacbug in [`af9261c`](https://github.com/nRF24/RF24/commit/af9261cf930da1733c43466a2e88a34556ed281f)
- Added setCRCLength by \@maniacbug in [`34efd5a`](https://github.com/nRF24/RF24/commit/34efd5ac94a5c5fd173a26fd11d5ef9b50c25b65)
- Added a new example by \@maniacbug in [`ad99581`](https://github.com/nRF24/RF24/commit/ad99581285b9097fd90de288e73a97843a1091ee)
- Added led_remote to docs by \@maniacbug in [`a62338f`](https://github.com/nRF24/RF24/commit/a62338f3a7a855aca9f8af7cd7d3479faa7f149e)
- Add RF_SETUP to printDetails by \@maniacbug in [`748fde2`](https://github.com/nRF24/RF24/commit/748fde239b8b363bad0509bcf8a04ab2c7c53c81)
- Added Jamfile for led_remote by \@maniacbug in [`8d5f020`](https://github.com/nRF24/RF24/commit/8d5f020c78d61a118c3e00be2f1dea43a727d667)
- Add dynpd/feature to printDetails by \@maniacbug in [`04e7277`](https://github.com/nRF24/RF24/commit/04e72775bb4d92e39ea51bc63b0018dd6eaff5bb)
- Added setRetries() by \@maniacbug in [`a22906f`](https://github.com/nRF24/RF24/commit/a22906f9617215f0448daa3e9c6c871e0ab0c9b1)
- Adding new example pingpair_dyn for dynamic payloads by \@maniacbug in [`98fbd44`](https://github.com/nRF24/RF24/commit/98fbd44111f11063b4c4cfbcf642dbaac00159d0)
- Added working test by \@maniacbug in [`77411b4`](https://github.com/nRF24/RF24/commit/77411b4ec57839b55b0dfb25c0296e0a8c4b3f9f)
- Added a test script to control the test, using python. by \@maniacbug in [`22b4c55`](https://github.com/nRF24/RF24/commit/22b4c558cd9c5c922ca84d7781e058d53d47bc9c)
- Added a second test configuration by \@maniacbug in [`c13db8f`](https://github.com/nRF24/RF24/commit/c13db8f42db76a78ba79fa244271d6beba1c425c)
- Added 3rd test case, static payload by \@maniacbug in [`d436895`](https://github.com/nRF24/RF24/commit/d436895f7c97a4c862299431bbad29d5f939c6f2)
- Adds constantness to method declarations and arguments where possible. by \@gcopeland in [`b1a7dc7`](https://github.com/nRF24/RF24/commit/b1a7dc7d54d51d5ffa7291ecbd6d9dd0f2e53b64)
- Added two default values to the RF24 constructor; speed and channel. by \@gcopeland in [`aac2c3d`](https://github.com/nRF24/RF24/commit/aac2c3dba33b163521dc6a00bdb7d81761af1fbc)
- Adds explicit CRC width and PA level. Changes hardware initialization by \@gcopeland in [`1581647`](https://github.com/nRF24/RF24/commit/158164746b59d32eb10a8017e1ad03b1f858df86)
- Added another test of a simpler calling pattern.  No IRQ, no dynamic payload or ack packet. by \@maniacbug in [`89180f2`](https://github.com/nRF24/RF24/commit/89180f284404e0d598675b8c0ccc5edd301f89b7)
- Add getDataRate, add DISABLED to data rate enum by \@maniacbug in [`8f17e63`](https://github.com/nRF24/RF24/commit/8f17e63a22ac01f03a3923b139799baec8256443)
- Added back in gcopeland's additional debugging info, with readable values. by \@maniacbug in [`4c21ab6`](https://github.com/nRF24/RF24/commit/4c21ab6ad099c193f1d47822c4e13fc9c77be9d0)
- Added FAQ page to docs by \@maniacbug in [`7aeaf0d`](https://github.com/nRF24/RF24/commit/7aeaf0d37329db116e8e363098f61b0b27bbf132)
- Added more information about whether the write was successful. by \@maniacbug in [`dceef7b`](https://github.com/nRF24/RF24/commit/dceef7b45ec2ae29025089fea2fa9dcdf411aea0)
- Added new example to go along with Getting Started tutorial blog post. by \@maniacbug in [`3a2d94b`](https://github.com/nRF24/RF24/commit/3a2d94b698c463f0b3168d7080ea1398bc7a6303)
- Added new example to documentation by \@maniacbug in [`d437163`](https://github.com/nRF24/RF24/commit/d4371639e612e8c74a9dcfb3148118439a6a9dac)
- Added example for Maple by \@maniacbug in [`a6bf911`](https://github.com/nRF24/RF24/commit/a6bf911f5a99253c7b718d4c119f5a65d4f59d59)
- Added new jamfile to this example by \@maniacbug in [`19fa395`](https://github.com/nRF24/RF24/commit/19fa3954c0d452f43ef9b8e4b8afca47d8b98a41)
- Added \#error to printf for examples that require Arduino by \@maniacbug in [`26ea458`](https://github.com/nRF24/RF24/commit/26ea4583cdebbc219b5f4460096044d911874e9d)
- Add isValid by \@maniacbug in [`ebcd0d1`](https://github.com/nRF24/RF24/commit/ebcd0d1d0b3061fcb57444e1dbe5829ef25705cd)
- Added writeFast() cmd - fill FIFO or return 0 by \@TMRh20 in [`281ace4`](https://github.com/nRF24/RF24/commit/281ace4ba24fb9c0cf814572c72752eebd3b8d95)
- Add powerUp to begin(), increase powerUp delay by \@TMRh20 in [`3455e48`](https://github.com/nRF24/RF24/commit/3455e489f4c88504bd65b6b5edf10282d5a47ab3)
- Add optional write timeout/failure handling by \@TMRh20 in [`327f060`](https://github.com/nRF24/RF24/commit/327f0609cf7b52b70accccc175b5809dd5074704)
- Add ATTiny example, change Minimal define by \@TMRh20 in [`d9c7df3`](https://github.com/nRF24/RF24/commit/d9c7df3ac9f4fb40376f6e299d833ae6add6802a)
- Add rxFifoFull() function, Only flush tx w ack_plds by \@TMRh20 in [`82629d4`](https://github.com/nRF24/RF24/commit/82629d4d3716f05be677323fd73c7a1df3308fd9)
- Add RPi B+ info to main examples and readme by \@TMRh20 in [`63b672c`](https://github.com/nRF24/RF24/commit/63b672c92963426377f722bfbc0e1c30038d6b62)
- Add readme for python wrapper by \@TMRh20 in [`cd59648`](https://github.com/nRF24/RF24/commit/cd596482156afff4f2f5e8a65d3ae0d897015a22)
- Add new RPi installer info to readme by \@TMRh20 in [`6bf37cd`](https://github.com/nRF24/RF24/commit/6bf37cdf5523c61d91a3ee09260f94ef26034b09)
- Adding SPI enum to RPi python wrapper by \@martin-mat in [`dcdc35d`](https://github.com/nRF24/RF24/commit/dcdc35d70deaeba951769492213636d15d9581ec)
- Add changes from doc update by \@TMRh20 in [`0b2d8dd`](https://github.com/nRF24/RF24/commit/0b2d8ddb2dbe996b60716482860a67407de6537b)
- Adding LittleWire support to RF24 by \@lnxbil in [`9b9a573`](https://github.com/nRF24/RF24/commit/9b9a57348d6fe3b9a1c238bc5598c5f283e524e7)
- Address fix for attiny example sketch by \@ax7884 in [`ba161a3`](https://github.com/nRF24/RF24/commit/ba161a3b25388988b178570cc859894df2d1d186)
- Support RPi 2 w/bcm2835.c version 1.39 by \@mannkind in [`25d379c`](https://github.com/nRF24/RF24/commit/25d379c187d673c775a7d68702f6f811405f1bdc)
- Add updated makefile for latest changes by \@TMRh20 in [`f420557`](https://github.com/nRF24/RF24/commit/f4205573b4c40907318e9ddbbabf094c55b3270d)
- Add include for nanosleep \#69 by \@TMRh20 in [`30ef8dc`](https://github.com/nRF24/RF24/commit/30ef8dc58aa19b0d1ba0819b7e4e18bd8b5a3071)
- Add a few links to RF24 docs for related projects by \@TMRh20 in [`c83f0f2`](https://github.com/nRF24/RF24/commit/c83f0f26ff65c66754037ce959a3485c55404e02)
- Added ```busNo``` passing to mraa::Spi() object creation call so that SPI bus number now can be defined by user when instatiating RF24 object by \@prophet-roshak in [`dd4b86a`](https://github.com/nRF24/RF24/commit/dd4b86aaac26b3c59912ebd93e8912ce208aee88)
- Add generic pin config for RPi, minor doc changes by \@TMRh20 in [`4cb2c5c`](https://github.com/nRF24/RF24/commit/4cb2c5c5a737221ed8855c20c495143c0da0bddb)
- Add RPi pin config to main page per \#89 by \@TMRh20 in [`7fdc708`](https://github.com/nRF24/RF24/commit/7fdc708b4ff912825b513210142c8b4bd387bd3f)
- Add SPI transaction code from Ralph Doncaster by \@TMRh20 in [`b1b8cad`](https://github.com/nRF24/RF24/commit/b1b8caddfe02b0876fc928fbadab2e6f20db4e2c)
- Add transaction code to remaining functions by \@TMRh20 in [`64da088`](https://github.com/nRF24/RF24/commit/64da08852c1537eab7e10314870e9943fe5172fa)
- Added getChannel Method to allow for verification of the configured channel by \@komby in [`c4d222c`](https://github.com/nRF24/RF24/commit/c4d222ce0db216b7e1c372c587e9b568c6af4f79)

### <!-- 3 --> 🗑️ Removed

- Removed the line that set the payload size to 8 in begin().  Reduced some ints to uint8_t.  Added some more debugging info. by \@maniacbug in [`dfb2faa`](https://github.com/nRF24/RF24/commit/dfb2faa8412ac7c64abd4da8409301c622c64ed1)
- Removed ping/pong.  Now tx just sends and rx just sends ack payload by \@maniacbug in [`38392fa`](https://github.com/nRF24/RF24/commit/38392fa5f46e945afbfe7e7962a7d94702e9a727)
- Removed 'ackpacket' example.  It was never working by \@maniacbug in [`f9b664c`](https://github.com/nRF24/RF24/commit/f9b664cd34834885eb383fd166921239e39f065e)
- Removed power_all/enable/disable, useless.  Added a multiples of the wakeup. by \@maniacbug in [`4ef1f4b`](https://github.com/nRF24/RF24/commit/4ef1f4b8e656c1e7b8b98f455517879a56c6a9a2)
- Removed unused file by \@maniacbug in [`ce881ba`](https://github.com/nRF24/RF24/commit/ce881ba0dcd5f06fc302c55cd02e4bdc52b87051)
- Removed method argument const qualifiers. by \@gcopeland in [`85c0f57`](https://github.com/nRF24/RF24/commit/85c0f57e3fa180cf047704cd1ed34eecb82f8b51)
- Merge branch 'p_enable' into constremove by \@gcopeland in [`542c46c`](https://github.com/nRF24/RF24/commit/542c46c0c68eb67a5e9e1f3f970cc27f4e416ac1)
- Merge gcopeland/constremove by \@maniacbug in [`602d962`](https://github.com/nRF24/RF24/commit/602d96264adcf6bc003cd7a7902ce0ae200cffae)
- SPI bus speed divider to 4, suggested by Greg Copeland.  Also removed duplicated spi setup out of begin(), as it's all in csn() by \@maniacbug in [`a28e236`](https://github.com/nRF24/RF24/commit/a28e2361fc1bab48117cff68d07ab9ee9951948f)
- Refactor pingpair_test to remove all serial io from within the irq handler, as necessitated by Arduino 1.0 by \@maniacbug in [`dc48235`](https://github.com/nRF24/RF24/commit/dc48235343acae2e23a16278b24aa96665fea04b)
- Remove prog_char, for compliance with gcc 4.6.1 by \@maniacbug in [`f6e59b9`](https://github.com/nRF24/RF24/commit/f6e59b9de8d8424c3a1daecfbdafdcbfcd85ccac)
- High Speed Data Transfers/Optimization by \@TMRh20 in [`4cf0fb0`](https://github.com/nRF24/RF24/commit/4cf0fb0c5adc305d40547f42366161e809122b94)
- - Slight modification to reUseTX cmd by \@TMRh20 in [`77b1e2c`](https://github.com/nRF24/RF24/commit/77b1e2c1400e755d13bfd9345bd235075b056ad4)
- Ext. timeout hang, examples, board support by \@TMRh20 in [`2f7e448`](https://github.com/nRF24/RF24/commit/2f7e4489bad411ee79f068b593c83c59c1f8c515)
- RPi SPI functionality and RPi fix for startWrite by \@TMRh20 in [`6530f95`](https://github.com/nRF24/RF24/commit/6530f954a18731e213efeb967265b612b1087ac5)
- Due & RPi bugfix - \#9 - root cause resolved by \@TMRh20 in [`2869663`](https://github.com/nRF24/RF24/commit/2869663f7fcdb5389a2c717b704a413944de1b8a)
- Modify write,startWrite, remove rx buffer flushes by \@TMRh20 in [`1d18015`](https://github.com/nRF24/RF24/commit/1d1801500dc6135eede3921008c4f3d381a80252)
- Removed wrong flush() on RPi by \@TMRh20 in [`589b2c7`](https://github.com/nRF24/RF24/commit/589b2c76fd7c2cc81f91780c776d996c269e9896)
- Update the pin layout table and consolidate the RPi readme files. by \@reixd in [`1e8de6e`](https://github.com/nRF24/RF24/commit/1e8de6e88f98a68868f23565ddc6cc524f5eca00)
- Remove unnecessary delay by \@TMRh20 in [`a12cdca`](https://github.com/nRF24/RF24/commit/a12cdca388a38066eca7cc92d7d4ea7c72fc6bf6)
- Update readme for installer by \@TMRh20 in [`91c6d21`](https://github.com/nRF24/RF24/commit/91c6d214b8030555efb326b555391a46eba30cab)
- Fixes and Improvements from large round of testing by \@TMRh20 in [`4d5f014`](https://github.com/nRF24/RF24/commit/4d5f014c0efabe2d638b928eb95b07894531b569)
- Fix 'features'? per https://github.com/dsbaha \#56 by \@TMRh20 in [`39a6385`](https://github.com/nRF24/RF24/commit/39a63859b0f83b5128eb8377a1d6c9abfcbb20ce)
- - reset config register to default by \@dsbaha in [`8d5e869`](https://github.com/nRF24/RF24/commit/8d5e8695ceae08d742472c66eb04333cfb89295d)
- Fix bugs in transfer example by \@TMRh20 in [`0066eec`](https://github.com/nRF24/RF24/commit/0066eece027f84d4478b4d7aff40af439097eb22)
- Remove trailing slash from docs/Prevent warning by \@TMRh20 in [`e900a9c`](https://github.com/nRF24/RF24/commit/e900a9cc5b097f669e31159f6e964c4b2a9c37dd)
- SPI/GPIO fixes/changes for MRAA and SPIDEV by \@TMRh20 in [`c1bb29c`](https://github.com/nRF24/RF24/commit/c1bb29c74b4cb2c1f13658665160fb8f247bcef9)
- Destructor for Linux only, replace bell with null by \@TMRh20 in [`e3f4a6b`](https://github.com/nRF24/RF24/commit/e3f4a6ba1879b8bb30e9b54d57c353d0fa42a34d)
- Fix/Changes for \#111 & \#112 by \@TMRh20 in [`5acbcbd`](https://github.com/nRF24/RF24/commit/5acbcbd2d1d297804aecdf4f42e82ae2fde33526)

### <!-- 4 --> 🛠️ Fixed

- Fixed a bug where it was writing to the wrong pipe by \@maniacbug in [`93f5cfa`](https://github.com/nRF24/RF24/commit/93f5cface0ebc0b82801ab7741b952578af37f7c)
- Fixed relay writing pipe. Added id to payload. Made Ping timing a consistent gap by \@maniacbug in [`52ab9e0`](https://github.com/nRF24/RF24/commit/52ab9e0081752725fa2c2aa6b8a823a76c59bc9e)
- Fixed a bug in setDataRate by \@maniacbug in [`731c472`](https://github.com/nRF24/RF24/commit/731c472abc08540a7cb85cd72819ed3930f1f95f)
- Fixed led_remote in docs by \@maniacbug in [`bec84a7`](https://github.com/nRF24/RF24/commit/bec84a79075b86c71695e668c5cc281c81d4ec96)
- Fix bug in openReadingPipe which disallowed pipe 5 by \@maniacbug in [`6e6264b`](https://github.com/nRF24/RF24/commit/6e6264be43e9e95c9b25838ea16377650e8fc215)
- Fixed unused parameter warning by \@maniacbug in [`14dcc34`](https://github.com/nRF24/RF24/commit/14dcc343dc457fa4b6752969f5b7505acab0b907)
- Fixed bug of not including name.  Also added extra tab on short strings. by \@maniacbug in [`91df6f6`](https://github.com/nRF24/RF24/commit/91df6f6df5d6d9b2b181e486b1a6141ed48e5033)
- Fix bug in setDataRate by \@maniacbug in [`8701d82`](https://github.com/nRF24/RF24/commit/8701d82615a4e4c138b4b6dcc0dbe9d50efc04a5)
- Fixed missing includes, thanks to Spoiler for reporting the problem. by \@maniacbug in [`a783ed3`](https://github.com/nRF24/RF24/commit/a783ed38da19d781a3863215d47a2e59a0b2754e)
- Fixed bug in setcrclength and setdatarate which was mishandling the bits by \@maniacbug in [`1517f12`](https://github.com/nRF24/RF24/commit/1517f1249de1713d0ec26c37476cbf2bf4291b58)
- Fix up some doc bugs by \@maniacbug in [`fcc3dfb`](https://github.com/nRF24/RF24/commit/fcc3dfbedb2e7047cd1f7f5931bea553c2ef1980)
- Fix bugs.  Ack payload available was broken since interrupts went in.  stREtries was missing since the gcopeland merge. by \@maniacbug in [`df32840`](https://github.com/nRF24/RF24/commit/df328408f79962adaed44194fef99e4c7426f41c)
- Fix bug in which static payloads were broken by \@maniacbug in [`af00a61`](https://github.com/nRF24/RF24/commit/af00a6155596b7728884a2759d11a02b9e54901e)
- Fixed a bug when pipe 0 was NOT being used for writing. by \@maniacbug in [`cc7e0a2`](https://github.com/nRF24/RF24/commit/cc7e0a2f19264852847edd42c4231584da8ec104)
- Fixed bug in scanner display, thanks to David Bath.  Also upgraded to latest Jam toolchain and moved pins over to the new GettingStarted defaults. by \@maniacbug in [`5726c16`](https://github.com/nRF24/RF24/commit/5726c1622a1dc4d12d9a71d90e3dce167dba5e27)
- Fix bug in pingpair Jamfile by \@maniacbug in [`76f79e6`](https://github.com/nRF24/RF24/commit/76f79e62ed1fa28887019aca9505196926bc9a3e)
- Fix Jamfile for proper ordering of libraries by \@maniacbug in [`33e59af`](https://github.com/nRF24/RF24/commit/33e59af725074afd19e0e70f08c0711779f38b4c)
- Fix bug introduced in the move to PROGMEM.  Props to \@basilrx for finding this.  Also added a test case to demonstrate the bug, and guard against more like it in the future. by \@maniacbug in [`2c97186`](https://github.com/nRF24/RF24/commit/2c9718632983e4ca563f559558cdede2358ccb35)
- Standby after regular write cmd by \@TMRh20 in [`bcd48f2`](https://github.com/nRF24/RF24/commit/bcd48f2f471e0c9536b30b0be7ead0f62f163f5a)
- Fixed link by \@TMRh20 in [`4d8da2f`](https://github.com/nRF24/RF24/commit/4d8da2f01de1d5e4ed2354d28f46c0ffec2a49d7)
- TxStandBy hang. Updated Transfer example by \@TMRh20 in [`f3b3fb1`](https://github.com/nRF24/RF24/commit/f3b3fb1f444f0e0c6d32d4731860d966d8779272)
- Missing byte with due RX by \@TMRh20 in [`d28323a`](https://github.com/nRF24/RF24/commit/d28323aadca8fa626315a9748c370d7e5a05dc64)
- Due write() issues. by \@TMRh20 in [`afdb084`](https://github.com/nRF24/RF24/commit/afdb084daf9fce9c8792281c7fd17683ce2ce1e2)
- Return value of write, writefast by \@TMRh20 in [`5fabf26`](https://github.com/nRF24/RF24/commit/5fabf2608a64572876e5b7b94a0e7e66b392cd8c)
- Fix typo in readme by \@TMRh20 in [`cd8bb19`](https://github.com/nRF24/RF24/commit/cd8bb19dc3aa63d30b4e474ce95b8224f0fa71c2)
- Fix for https://github.com/TMRh20/RF24/issues/8 by \@TMRh20 in [`57521fb`](https://github.com/nRF24/RF24/commit/57521fbcc43e0a13cd40c2eff897cf5394441348)
- Fix for \#12 by \@TMRh20 in [`ecda0fa`](https://github.com/nRF24/RF24/commit/ecda0fad53ba897089efb46c045505b2f0fa749c)
- Static payload size bug prev introduced by \@TMRh20 in [`e8a63c0`](https://github.com/nRF24/RF24/commit/e8a63c03712ae99b671718c6dfee62e7afd5c106)
- RPi dynamic ACK issue by \@TMRh20 in [`9e12c23`](https://github.com/nRF24/RF24/commit/9e12c23683b57a5979572d9bf8e8386f14d7f928)
- Multicast pipe0 address handling issue by \@TMRh20 in [`9da3eac`](https://github.com/nRF24/RF24/commit/9da3eac0941563d9e9828c8377c9a88a161cd855)
- Fix display of pin info for ATTiny by \@TMRh20 in [`23532cf`](https://github.com/nRF24/RF24/commit/23532cfe4a8b728f77d4e3eca624868a2b757dcb)
- Fix "warning: binary constants are a GCC extension". by \@reixd in [`6f70a3c`](https://github.com/nRF24/RF24/commit/6f70a3cc5a74dab00d834a31a3ad3b820056e8be)
- Fix mistake in new code by \@TMRh20 in [`d3e5c4e`](https://github.com/nRF24/RF24/commit/d3e5c4e54d74e675d617a010ae0c741e80d6a614)
- Fix wrong pin layout by \@reixd in [`51a71bc`](https://github.com/nRF24/RF24/commit/51a71bca0b1c1d9a025e58978bfc72981f3661f8)
- Fix broken ATTiny support in updates branch by \@TMRh20 in [`3e6ca45`](https://github.com/nRF24/RF24/commit/3e6ca45f9ebab5729862d93989f1b378c55f99f7)
- Broke SPI for Due and Teensy last commit by \@TMRh20 in [`c077f70`](https://github.com/nRF24/RF24/commit/c077f709de98fb174e98fa49b8db689981bc7377)
- Fix for last commit (code merge) by \@TMRh20 in [`8503804`](https://github.com/nRF24/RF24/commit/85038040a56ed242b726a029493ebba4ee8df433)
- Fix include for RF24_config.h by \@TMRh20 in [`bb88f4a`](https://github.com/nRF24/RF24/commit/bb88f4a78e702e83b3644e2fe01238281047bcd1)
- Fix header file install for RPi in Makefile by \@TMRh20 in [`603f4cc`](https://github.com/nRF24/RF24/commit/603f4cc81489dfb0f23a3ac3109e8309b6da133d)
- Fix for last pull request by \@TMRh20 in [`bfd62ea`](https://github.com/nRF24/RF24/commit/bfd62ead172060ac707ef3988d374e442ff77e30)
- RPi memory corruption by \@TMRh20 in [`73cf59c`](https://github.com/nRF24/RF24/commit/73cf59c10af95bd99b843c920cb4aae40a2b02b8)
- Fix mistake in example update by \@TMRh20 in [`a802136`](https://github.com/nRF24/RF24/commit/a802136a0487f7c5450c4d99b2caae93e67add9e)
- Fix build for python wrapper by \@TMRh20 in [`6354cac`](https://github.com/nRF24/RF24/commit/6354cacc6f5456a536ae1002c3bb033b5ed85d2b)
- Dual SPI devices with RF24 by \@TMRh20 in [`9b48595`](https://github.com/nRF24/RF24/commit/9b48595943a77ee78dba4b6e8d0d9c19c2144c94)
- Fixed issue with ARDUINO not issuing CS for radio by \@prophet-roshak in [`c0848cb`](https://github.com/nRF24/RF24/commit/c0848cb1fad0c3ab45bb31813e2a8ffb17d97d47)
- Fix isAckPayloadAvailable() per \@jorihardman by \@TMRh20 in [`f57b383`](https://github.com/nRF24/RF24/commit/f57b383039c7085888eb026a8944443c3a842444)
- Fix operator precedence for isAckPayloadAvailable. by \@jorihardman in [`8bd5289`](https://github.com/nRF24/RF24/commit/8bd5289514f609c9a29353077217f2a251465576)
- Fix IDE version detection for SPI transactions by \@TMRh20 in [`f6e1f37`](https://github.com/nRF24/RF24/commit/f6e1f37b3b46ba7ae92c617106ce6b9f0796724e)
- Fix issue \#101 found by \@mz-fuzzy by \@TMRh20 in [`f691c45`](https://github.com/nRF24/RF24/commit/f691c45a69064c132dd11c52973e6fe011bded3a)
- Fix the rest of issue \#101 per mz-fuzzy by \@TMRh20 in [`90d85be`](https://github.com/nRF24/RF24/commit/90d85bef4bf370f54404e00805431f77a49607e7)
- Fix for SPI transaction code by \@TMRh20 in [`c79ccc7`](https://github.com/nRF24/RF24/commit/c79ccc70bb744f91b9d325d5b21b028fead3bd48)
- Fix example per \#114 by \@TMRh20 in [`357fcb9`](https://github.com/nRF24/RF24/commit/357fcb95a5bc89704846679fa667dbcafe68a5ea)
- Fix the last fix by \@TMRh20 in [`869a2c2`](https://github.com/nRF24/RF24/commit/869a2c2f7329967bc600e66d0b0422e959e22e2d)

### <!-- 7 -->🚦 Tests

- Prepare tests to run with configuration parameters by \@maniacbug in [`7640852`](https://github.com/nRF24/RF24/commit/7640852edf6e3cf139608fe70505cffa93a8c650)
- Treat warning as errors in tests by \@maniacbug in [`18ebe7a`](https://github.com/nRF24/RF24/commit/18ebe7a0aac86bf90aa08e0fd8d48d6929651383)
- Runtests now hups between each test instead of uploading by \@maniacbug in [`94635e8`](https://github.com/nRF24/RF24/commit/94635e8cc50843db3603971d9c8978f1942a2d5e)
- Tune timing and upload rate of tests by \@maniacbug in [`205d4b5`](https://github.com/nRF24/RF24/commit/205d4b5cc12d119149e75e62e62b98fce0f38ac7)
- Lib now passes all tests by \@TMRh20 in [`4340d93`](https://github.com/nRF24/RF24/commit/4340d9339a333c28c416406ada828cd3a465018e)
- Update examples, minimize code size by \@TMRh20 in [`e83a107`](https://github.com/nRF24/RF24/commit/e83a107bdf8e7430457975b1d9251cc5c6ba7107)
- Performance tweaks to address \#9 by \@TMRh20 in [`5ec5b06`](https://github.com/nRF24/RF24/commit/5ec5b06bdc4f3b58eac3f34b3c4355a51b659ae6)

### <!-- 8 --> 📝 Documentation

- Updated documentation by \@maniacbug in [`0503017`](https://github.com/nRF24/RF24/commit/0503017d1a03dd5163d32db2b399e33fdfdf4caa)
- Simplified example to use less calls. in [`e97e023`](https://github.com/nRF24/RF24/commit/e97e0239d7861c27b08bb7541a1966b9c89a2120)
- Tweaked doc paramters by \@maniacbug in [`117678c`](https://github.com/nRF24/RF24/commit/117678c46f0d0cf82377953a399bb9f6740fb705)
- Cleaned up documentation. by \@maniacbug in [`21057e4`](https://github.com/nRF24/RF24/commit/21057e4608e755ef606e33041df3f18290572638)
- Revised documentation by \@maniacbug in [`1c65c7f`](https://github.com/nRF24/RF24/commit/1c65c7f181f8e6040e3f37d4302574eb5ebea147)
- Updating docs by \@maniacbug in [`f3369a5`](https://github.com/nRF24/RF24/commit/f3369a552cd49d8e57cbc1e11a48a985082026ef)
- Docupdate by \@maniacbug in [`1c6cc96`](https://github.com/nRF24/RF24/commit/1c6cc96fb74a406786bab4f9841d82f65a65c861)
- More doc tweaks by \@maniacbug in [`c9d64fd`](https://github.com/nRF24/RF24/commit/c9d64fd0888d7f7610c8bded105a787086fda62e)
- Update docs for 1.0 by \@maniacbug in [`8ea84e7`](https://github.com/nRF24/RF24/commit/8ea84e7f77fc4122d77b1bd035d78d18eaab0bda)
- Update docs and comments for new nordic fob blog post by \@maniacbug in [`e31f8c8`](https://github.com/nRF24/RF24/commit/e31f8c8211c82a8984bc7a1e2dfa098d99108a71)
- Update readme temporarily by \@TMRh20 in [`dfc75db`](https://github.com/nRF24/RF24/commit/dfc75db8e61276e9920e1cbc6be59939966ca4d9)
- Updated readme and h file for doxygen by \@TMRh20 in [`14187cd`](https://github.com/nRF24/RF24/commit/14187cd1a1257ee889fc41b5986ce75324f5500d)
- Corrected part of the documentation by \@TMRh20 in [`63ee0ce`](https://github.com/nRF24/RF24/commit/63ee0ce2c077373e4d25cc4ef30fac1036df6854)
- Update doxyfile & css to match tmrh20.github.io by \@TMRh20 in [`04165e4`](https://github.com/nRF24/RF24/commit/04165e4dd1ce9aa5b46b4b0f7b2705a6c74e53b2)
- Update RF24.h with documentation info by \@TMRh20 in [`9ddbe8b`](https://github.com/nRF24/RF24/commit/9ddbe8b168c93c09e8b7d7eb42e3a3e4903bbd9b)
- Documentation and File Structure Changes by \@TMRh20 in [`8a0f5ab`](https://github.com/nRF24/RF24/commit/8a0f5ab57a778c468a90b3e6ed6164fe5daf6e54)
- Replace readme with link to tmrh20.github.io/RF24 by \@TMRh20 in [`5333b4e`](https://github.com/nRF24/RF24/commit/5333b4efa8cfe43ce1e4ad80e645865e6c624785)
- Reorganize Optional and Advanced parameters in docs by \@TMRh20 in [`7741224`](https://github.com/nRF24/RF24/commit/7741224e9678db941068d498c44a7bc0750b4c90)
- More doc updates for coming changes by \@TMRh20 in [`8e15310`](https://github.com/nRF24/RF24/commit/8e1531014fe34b13aab168bf9b03106ef2ddfbc5)
- Update docs in 'updates' branch by \@TMRh20 in [`b320f9e`](https://github.com/nRF24/RF24/commit/b320f9e69c130ebe5b75ef15bb6b350fe71a68ea)
- Update *.cpp examples pin config by \@TMRh20 in [`b342d97`](https://github.com/nRF24/RF24/commit/b342d97f3693f89f06739ccf351281a95ac21805)
- \@PlatformIO Library Registry manifest file by \@ivankravets in [`ea960e0`](https://github.com/nRF24/RF24/commit/ea960e057b3360094a153ca1ab9c6fa984c3d68a)

### <!-- 9 --> 🗨️ Changed

- Type by \@maniacbug in [`ea69f9d`](https://github.com/nRF24/RF24/commit/ea69f9d1a6e085ce4df7ff125ca3af30ea94e2b3)
- Changed name of print_details to comply with the overall naming convention by \@maniacbug in [`0dc43ab`](https://github.com/nRF24/RF24/commit/0dc43ab8722c74c8efd9a960014219805407ebcf)
- Tuned the text a bit by \@maniacbug in [`c5c4537`](https://github.com/nRF24/RF24/commit/c5c4537cdc94a2d50b4fec5bd61aa15b738da04e)
- Flipped the logic on the role pin. by \@maniacbug in [`c2dc356`](https://github.com/nRF24/RF24/commit/c2dc35652e500e58aa11b44e6f1344c5e8868a26)
- Make the makefile be smart about which OS it's running, and make by \@maniacbug in [`e1db440`](https://github.com/nRF24/RF24/commit/e1db4401ca01fd5a113e49acdbe8b0148c45c0e7)
- Trying to get the formatting just right by \@maniacbug in [`0668494`](https://github.com/nRF24/RF24/commit/0668494aee5536d43fb23806d9f0c9bdfb113dee)
- Now sets spi data mode before interacting with the chip.  needed to interoperate with other spi devices by \@maniacbug in [`1045cec`](https://github.com/nRF24/RF24/commit/1045cec9b13d079102c92488854c74b1aa0d66c9)
- Updated vim modeline by \@maniacbug in [`5b632f3`](https://github.com/nRF24/RF24/commit/5b632f374085b17d8614fb3c13fb3d79bae9a9ff)
- Enabled warnings, and cleaned up warnings.  Updated pingpair makefile for recent makefile improvements by \@maniacbug in [`e188242`](https://github.com/nRF24/RF24/commit/e188242a8687c2880dbb60b394308a6dd25c254e)
- Merge branch 'master' of github.com:maniacbug/RF24 by \@maniacbug in [`e005ec7`](https://github.com/nRF24/RF24/commit/e005ec75f140db665c889727c55d2c234f28e940)
- Moved printf format strings to progmem. by \@maniacbug in [`33b8973`](https://github.com/nRF24/RF24/commit/33b897318fd04f33c91fc5adf0aa67cb2461938e)
- Split out upload target into two by \@maniacbug in [`e7254a3`](https://github.com/nRF24/RF24/commit/e7254a3b3a0f85b5c168057cdf4336681dbe92bb)
- Merge branch 'master' of 207.97.227.239:maniacbug/RF24 by \@maniacbug in [`a88e78a`](https://github.com/nRF24/RF24/commit/a88e78a0da96fe2335c6249b1c5dfa7216f75c3c)
- Stripped down extraneous pipe opens, and other un-needed things. by \@maniacbug in [`39b15c8`](https://github.com/nRF24/RF24/commit/39b15c8b55f8e86ac0295cf430123818eb9db349)
- Got ack payloads working.  Renamed from 'ackpacket' by \@maniacbug in [`ffc7d09`](https://github.com/nRF24/RF24/commit/ffc7d099ed05e48724b6471b4d75ce98d8a05075)
- Cleaned up comments, variables names, enums by \@maniacbug in [`f3356f3`](https://github.com/nRF24/RF24/commit/f3356f31a281dc5895b16c7842440a32979738bf)
- Created a new example to show sleeping in between sends.  This isn't done yet.  It's just a copy of pingpair for now. by \@maniacbug in [`72dc774`](https://github.com/nRF24/RF24/commit/72dc77467e0d42a0e0ccc2c5d20e2506074330e0)
- First complete compile/link with sleep functionality.  Haven't run it yet. by \@maniacbug in [`3179382`](https://github.com/nRF24/RF24/commit/31793823eaefee67246638621e6cdc126e3bea5b)
- Turned off serial debug by \@maniacbug in [`0d20f83`](https://github.com/nRF24/RF24/commit/0d20f831d02958f78f1c759288adc71de4a6ae0f)
- Created new example, copied from starping by \@maniacbug in [`1fd8ee7`](https://github.com/nRF24/RF24/commit/1fd8ee72c0b9a32d754ece211b1790bea0265b4e)
- Rewired everything, re-tested with ping/pong nodes only by \@maniacbug in [`9868642`](https://github.com/nRF24/RF24/commit/986864236c51b70692e6ca143ddcf6a6a3bba79d)
- Allow opening pipe 0 for reading.  Useful if it's on a different subnet from others by \@maniacbug in [`d8b2512`](https://github.com/nRF24/RF24/commit/d8b2512e0c391ef218e941a52dd554d0f288a199)
- Moved base node code, to make it more readable by \@maniacbug in [`33edb1f`](https://github.com/nRF24/RF24/commit/33edb1f18acee3f45db56ce7785e70febccc6c57)
- Leaf only starts hunting for a new slot after 3 timeouts.  Makes the system more stable. by \@maniacbug in [`e86c6b0`](https://github.com/nRF24/RF24/commit/e86c6b0f15f7171adf57f6aaacdef72cf15a07c0)
- Now caches the reading pipe 0 and re-sets it when starting to listen. by \@maniacbug in [`1d28d28`](https://github.com/nRF24/RF24/commit/1d28d28a29d452d79b42dd94732e550721b727be)
- Refactored common code from leaf/relay nodes into static functions.  Improved comments. by \@maniacbug in [`7e65824`](https://github.com/nRF24/RF24/commit/7e658242e2e22c313ecbb06c55a49be93f50a507)
- Comment bug by \@maniacbug in [`77a4c95`](https://github.com/nRF24/RF24/commit/77a4c953193c8c18cce7fcd09ff53e12c46d5c41)
- Renamed ping delay to ping interval and added some comments by \@maniacbug in [`937805d`](https://github.com/nRF24/RF24/commit/937805d6a2acde22bf685e5fdbc0905c937004f1)
- Do clock divider on chip select. Should improve SPI interop with other devices by \@maniacbug in [`8f80038`](https://github.com/nRF24/RF24/commit/8f8003822760e76bf1e43cd6000031e804de1168)
- Avoid spurious warnings by \@maniacbug in [`40eea86`](https://github.com/nRF24/RF24/commit/40eea86ffe6993eecb3ef97652009821f5b316a7)
- Environment-specific changes for me.  Testing out latest arduino from git, and added a 3rd upload option by \@maniacbug in [`1062f9d`](https://github.com/nRF24/RF24/commit/1062f9d879c6c6a1ca77c315b7a9bc39a31cd9bd)
- Made all CONFIG register changes incremental not absolute by \@maniacbug in [`ec33f03`](https://github.com/nRF24/RF24/commit/ec33f0336880ac530623f4e94ba32234140e94b0)
- Astyle -A1 by \@maniacbug in [`26c45e8`](https://github.com/nRF24/RF24/commit/26c45e85a9dab5586d56fa649657c3a383bc9753)
- Changed the logic.  Button press now toggles the LED. by \@maniacbug in [`3cc856a`](https://github.com/nRF24/RF24/commit/3cc856aa952840e67f1d09290bde0635d570195a)
- Cleaned up some reinterpret casts to use new-style casts by \@maniacbug in [`aa7a7c6`](https://github.com/nRF24/RF24/commit/aa7a7c647ae5700d7a3e47f2fe713fd87e6f477b)
- Refactor repeated parts of printDetails into own methods by \@maniacbug in [`92373ba`](https://github.com/nRF24/RF24/commit/92373ba7f53f451f2e60bb557817982caa782463)
- Move examples to 57600 baud by \@maniacbug in [`fea8c0e`](https://github.com/nRF24/RF24/commit/fea8c0e82f831d0442c38e84b4ae2fea1614deae)
- Moved completely over to jam.  bye bye make\! by \@maniacbug in [`e7c30f2`](https://github.com/nRF24/RF24/commit/e7c30f265fca9933710c99ef636a1faa948d6ba5)
- Deleted starping relay.  Superseded by RF24Network. by \@maniacbug in [`549d405`](https://github.com/nRF24/RF24/commit/549d4054fba01f1b3d63ea1761aa136ad86b218b)
- Purge useless gitignores by \@maniacbug in [`86798b6`](https://github.com/nRF24/RF24/commit/86798b65e4407764f2d69eac7168ca5331d9c7af)
- Prettied up printDetails some more by \@maniacbug in [`81f85bd`](https://github.com/nRF24/RF24/commit/81f85bd5215704269e11a31619d41ba20c58420c)
- Tidied up EN_RXADDR setting by \@maniacbug in [`c151ffb`](https://github.com/nRF24/RF24/commit/c151ffbf67c5b64baef1852ea9e7ec83ba7dc70f)
- Documented some questionable behaviour as pointed out by mdenzien by \@maniacbug in [`ca4820c`](https://github.com/nRF24/RF24/commit/ca4820cd27fb5466b73c96dfaf23d6c6d0899a65)
- Made timeouts more explicit in write() by \@maniacbug in [`c5823c2`](https://github.com/nRF24/RF24/commit/c5823c2605e80ffd73e31d6a5b6a76249be13228)
- More comments, factored out powerdown from write() by \@maniacbug in [`c62224f`](https://github.com/nRF24/RF24/commit/c62224ff8636f259dcbab79d544f3ea8b444ae0c)
- Factored out status-checking from write() into a public method by \@maniacbug in [`fbae441`](https://github.com/nRF24/RF24/commit/fbae441249f11a3f15b856881c71899182b42ca3)
- Initial checking of new pingpair_irq example.  Right now this is just the pingpair example but renamed. by \@maniacbug in [`bb37e88`](https://github.com/nRF24/RF24/commit/bb37e88094c6ed4193b1de45d56d6fece84518d3)
- Created a non-blocking write and an example for how to use it. by \@maniacbug in [`d3fff68`](https://github.com/nRF24/RF24/commit/d3fff68a7cd3aaddc75ec989b50ab79a2704fe5c)
- Simplified printing, ok to do just from the irq handler by \@maniacbug in [`8549566`](https://github.com/nRF24/RF24/commit/854956687f85bdb98ec2f50092bde885066eed81)
- Now both roles use the IRQ.  Thanks to Mike Denzien for testing & troubleshooting. by \@maniacbug in [`062d833`](https://github.com/nRF24/RF24/commit/062d83346b27c03bcc4789c6b94f12ee3ed6be9b)
- Astyle -A1 -s2 by \@maniacbug in [`d5a3e79`](https://github.com/nRF24/RF24/commit/d5a3e79c665f29bca7940b2889773f0165accc55)
- Optimization of whatHappened, suggested by Mike Denzien by \@maniacbug in [`b000a87`](https://github.com/nRF24/RF24/commit/b000a87aa1f9582a887569591fbd8edbfacf1f8f)
- Implement dynamic payloads, with an example by \@maniacbug in [`63eb62e`](https://github.com/nRF24/RF24/commit/63eb62ebc3a69f4f549532c493ce48cdfa0a8928)
- Clean up read() to use new-style read_register, which reduces it to two lines of code by \@maniacbug in [`9566c82`](https://github.com/nRF24/RF24/commit/9566c82367a735b1db7b603880dc657352a1793a)
- Reduce example payload size. Does this help reliability? by \@maniacbug in [`774f53d`](https://github.com/nRF24/RF24/commit/774f53d6c5ff7cc635b2a5740b8044c3d9132b68)
- Comments by \@maniacbug in [`ccf3289`](https://github.com/nRF24/RF24/commit/ccf32891f02f07564e7a4114cd5468572cd76103)
- Purge 'boolean' type, use 'bool' instead by \@maniacbug in [`42bf3e8`](https://github.com/nRF24/RF24/commit/42bf3e8cd2acb68ca346779b4479495563929568)
- Created skeleton of new test by \@maniacbug in [`0946538`](https://github.com/nRF24/RF24/commit/0946538f3a4df9000005ed470a1a53983bae8a4a)
- Bugfix by \@maniacbug in [`c624815`](https://github.com/nRF24/RF24/commit/c6248157046fca8ee8a358d71eba3eb6c8a98664)
- Created an all-up script runner by \@maniacbug in [`af37c34`](https://github.com/nRF24/RF24/commit/af37c345e87e7bbf67196bc667ee1156a0ae3af1)
- Simplify specification of test configs slightly by \@maniacbug in [`c408c5e`](https://github.com/nRF24/RF24/commit/c408c5ef90928c679e4533183dbe6f19008d39e1)
- New P variant feature support with corresponding defines. RF_DR is now broken into two bits to support P variant and is now obsolete. RF_DR is now RF_DR_HIGH and RF_DR_LOW; with RF_DR_HIGH corresponding to the original RF_DR bit. Data rate can now select 250kbs mode on supporting hardware. by \@gcopeland in [`1a37c1c`](https://github.com/nRF24/RF24/commit/1a37c1cd0ff12675ea983f921b445771b0e699da)
- Moved the wide_band variable and initializer cleanup. by \@gcopeland in [`634d0ef`](https://github.com/nRF24/RF24/commit/634d0efde5930e79d1bc2a51972f83e942394d62)
- Re-organized nRF24L01.h file to place 'P' features in one by \@gcopeland in [`059efa5`](https://github.com/nRF24/RF24/commit/059efa5ffa03d544c80d0101c6c6de14a1e5f582)
- Reverted constructor changes. Changed SPI bus speed. Fixed setAutoAck by \@gcopeland in [`b491480`](https://github.com/nRF24/RF24/commit/b491480dde5669590996f8b9db510d03ea57f3d6)
- As a just in case I misunderstood some of the constant-ness changes, I by \@gcopeland in [`0c2515d`](https://github.com/nRF24/RF24/commit/0c2515df19d180c82303bcda074f06a239f6ff2c)
- Sed s/boolean/bool/g by \@maniacbug in [`956b63a`](https://github.com/nRF24/RF24/commit/956b63a4dce35afb73bdd4847f20512fbbb38ea5)
- Merge cleanups by \@maniacbug in [`cd3b0b3`](https://github.com/nRF24/RF24/commit/cd3b0b3ccbb55c09d9b4b7beb8ca001b86cf6f41)
- Changed test to use new 250kbps mode by \@maniacbug in [`77a798b`](https://github.com/nRF24/RF24/commit/77a798b1a91d213b0fb65734afc5a89cda24ce70)
- Improve pingpair timing.  Should lead to less timeouts. by \@maniacbug in [`7b401d7`](https://github.com/nRF24/RF24/commit/7b401d7281d783b4a156ad4b9c4a41a18aec9fd0)
- Experimental changes to help in debugging by \@maniacbug in [`2f33bdb`](https://github.com/nRF24/RF24/commit/2f33bdbebd697919e936fcf825dd62e66afebdda)
- Merge fix for static payloads by \@maniacbug in [`4542464`](https://github.com/nRF24/RF24/commit/4542464e0b88946d74ac44cf3c9dfcaf48b82cee)
- Static and dynamic payloads now fully fixed and interoperable by \@maniacbug in [`a31cafc`](https://github.com/nRF24/RF24/commit/a31cafcc887f1809fb63ac23ea3881e1bcda3e17)
- Ignore .swp files by \@maniacbug in [`39ca6b9`](https://github.com/nRF24/RF24/commit/39ca6b95dd127340869445d3e746c8f01c7c623b)
- Merge master with static payload fixes by \@maniacbug in [`3aab74a`](https://github.com/nRF24/RF24/commit/3aab74ae8f0338c27dba97749a3386987e6460b6)
- Comment cleanups by \@maniacbug in [`a261710`](https://github.com/nRF24/RF24/commit/a261710761b885915c5b8c27af453bf3fef5bb57)
- Test refinement.  IRQ test times out after not receiving anything, warning that you probably don't have interrupts hooked up. by \@maniacbug in [`d967f77`](https://github.com/nRF24/RF24/commit/d967f7708f0cb2a560f17cb96a60e53608ea565b)
- Reorganize header file to better group the now very large list of methods by \@maniacbug in [`4833610`](https://github.com/nRF24/RF24/commit/483361029e812f22d8fd66e431d4720fe04a5928)
- CHanged default data rate to 1MBps and default channel to 100 for better performance by \@maniacbug in [`5344c5f`](https://github.com/nRF24/RF24/commit/5344c5fe8cefaafa1a267267e7edefec631603d9)
- Small tweaks and new test for native target by \@maniacbug in [`e1527af`](https://github.com/nRF24/RF24/commit/e1527afc31bb46fff3929df579aec5e81eb9d078)
- Avert warnings on native platform by \@maniacbug in [`19dd5c7`](https://github.com/nRF24/RF24/commit/19dd5c77265bd61d6c84df8bd9c4278f7c449c5e)
- More debugging information. by \@gcopeland in [`7ad1b40`](https://github.com/nRF24/RF24/commit/7ad1b409fe180b7c37aeecefb08bc4f93c503e5d)
- Yet more merge cleanup. Changes default channel to be by \@gcopeland in [`a4cabc6`](https://github.com/nRF24/RF24/commit/a4cabc66a2b15c45182718c9302385d7378b4447)
- Flush all buffers on start and stop listening by \@maniacbug in [`066d79f`](https://github.com/nRF24/RF24/commit/066d79f21e5570251f4b7e56e14a3248200c0f05)
- Using Arduino 21 by \@maniacbug in [`f5c0ba2`](https://github.com/nRF24/RF24/commit/f5c0ba25ee04dd4689f90261d533e5040ed888ce)
- Compatibility with Arduino 1.0 by \@maniacbug in [`d92ac14`](https://github.com/nRF24/RF24/commit/d92ac14f8685f01a3ea2a1d94083b0ca99851f60)
- Move default pins over to Getting Started defaults by \@maniacbug in [`9f806a5`](https://github.com/nRF24/RF24/commit/9f806a5b00ee2809a530b2695661431970267db9)
- Changes to work on Maple by \@maniacbug in [`f393408`](https://github.com/nRF24/RF24/commit/f3934084a1368a2e1683ce9b7739a4099edf8c69)
- Moved environment-specific checking out to RF24_config.h by \@maniacbug in [`53cb579`](https://github.com/nRF24/RF24/commit/53cb579400a76db6583ee973332c6c6d17a7f7d5)
- Clean up pin assignments by \@maniacbug in [`840ad1c`](https://github.com/nRF24/RF24/commit/840ad1cd3dc5ce1d23c0d070ef14343cd4ddaf40)
- Handle SPI.h better in Maple by \@maniacbug in [`5cb32c9`](https://github.com/nRF24/RF24/commit/5cb32c92cc709c2fc86508864fbdb014f5ab80c5)
- Reduce delays in startWrite down to only required delays by \@maniacbug in [`50b18a9`](https://github.com/nRF24/RF24/commit/50b18a9c7dc39bd27db7405567b7d27f338100ff)
- Correct the comment for pin #'s by \@maniacbug in [`8493b9a`](https://github.com/nRF24/RF24/commit/8493b9a60e694d9824bf3a91c30c5d9e9e2094f8)
- Documentation update.  1.0 fully supported, Maple works, more examples, links and pictures for proejcts by \@maniacbug in [`fc06760`](https://github.com/nRF24/RF24/commit/fc067607364c02b38474bb7a2f2ae778cd86fb1a)
- Use latest copyright by \@maniacbug in [`07a4bcf`](https://github.com/nRF24/RF24/commit/07a4bcf425d91c99105dbdbad0226296c7cd3a93)
- New 'nordic_fob' example started, based on pingpair by \@maniacbug in [`36167a9`](https://github.com/nRF24/RF24/commit/36167a9e02fecc3d45f863083b43a619953cb98e)
- Nordic_fob example fully working. by \@maniacbug in [`d1653e8`](https://github.com/nRF24/RF24/commit/d1653e8cad74daf6fa4fa60a5646804592ee8290)
- Really...Pins 9 & 10 this time.  I mean it\! by \@maniacbug in [`cb6613d`](https://github.com/nRF24/RF24/commit/cb6613d62fdc7f68327811d5cb2e36821b55c695)
- Reduce RAM footprint by moving static const data to progmem, and refactoring out switch statements. by \@maniacbug in [`eb3b2d8`](https://github.com/nRF24/RF24/commit/eb3b2d800a240ce097e45423816dc0db9b423ef2)
- Use latest Jamfile by \@maniacbug in [`c2e9564`](https://github.com/nRF24/RF24/commit/c2e956467895ffe73d705afd1b84c5c68bf65775)
- Pingpair_blocking test now uses expect and runs on linux by \@maniacbug in [`dec61af`](https://github.com/nRF24/RF24/commit/dec61af85fa5316ade24168006797c547dc8bbc4)
- FAQ- How much RAM?? by \@maniacbug in [`c6262ed`](https://github.com/nRF24/RF24/commit/c6262edbce48bb839d5fed4412da0fad39e0250a)
- Latest jamfiles by \@maniacbug in [`84e464d`](https://github.com/nRF24/RF24/commit/84e464def4a52a3a697ee95d26eb66997db0ea76)
- Update examples/led_remote/led_remote.pde in [`49f22ea`](https://github.com/nRF24/RF24/commit/49f22eae21c2b7a872897a6a627bd255369ff2b2)
- Merge pull request \#16 from RWSDev/master by \@maniacbug in [`8eaad77`](https://github.com/nRF24/RF24/commit/8eaad77b2cb6d7491d5bc757d1bdfb78e6bebad1)
- Changed writing modes by \@TMRh20 in [`d0ec0c4`](https://github.com/nRF24/RF24/commit/d0ec0c461ce6c2e1f15b334344330cdf8f993417)
- Inspired idea to change TX standby code by \@TMRh20 in [`1e2f41c`](https://github.com/nRF24/RF24/commit/1e2f41c3f8143c9be0ae4aaa165cf98cb3052d84)
- Updated some examples to show the new lib usage by \@TMRh20 in [`99571d6`](https://github.com/nRF24/RF24/commit/99571d653001b143d8305425e2eab193136b38fe)
- TxStandby() function modified completely by \@TMRh20 in [`1cc7454`](https://github.com/nRF24/RF24/commit/1cc7454d97d0995e443745c788e37e1f9192d62e)
- Forgot to move startFastWrite to public by \@TMRh20 in [`0ddec52`](https://github.com/nRF24/RF24/commit/0ddec5256343186aafef9d4c653d12ace6a2e712)
- Extended timeouts, txStandBy changes by \@TMRh20 in [`6263bcc`](https://github.com/nRF24/RF24/commit/6263bccd0241aaf0c0e45106d514ea34516f39e9)
- Full Due support, maskIRQ function by \@TMRh20 in [`e1aaa57`](https://github.com/nRF24/RF24/commit/e1aaa571721d48584c43cfb287f55efc17ed97dd)
- Revamped write_payload() and read_payload() by \@zephyrr in [`84146a1`](https://github.com/nRF24/RF24/commit/84146a17f301d4e341f12db9613bff44cc285ae0)
- Merge pull request \#1 from zephyrr/master by \@TMRh20 in [`8077f56`](https://github.com/nRF24/RF24/commit/8077f56a7f1d9ca0e0bd078b1077215d89e52c3d)
- ATTiny support fr jscrane fork tested and working by \@TMRh20 in [`07eb9f8`](https://github.com/nRF24/RF24/commit/07eb9f874999c2d862a99d452a29d08a0798760d)
- EnableDynamicAck(), Single NOACK, default RT, by \@TMRh20 in [`92a19af`](https://github.com/nRF24/RF24/commit/92a19af8f808967aa03cf4e18d8e4c9fd3cc27fc)
- Revert powerUp() delay to 5ms delay by \@TMRh20 in [`1858312`](https://github.com/nRF24/RF24/commit/185831233868b07310b69daefa63b169f1bf8ed4)
- FIFO rd change (Thanks Zephyrr), Initial SI24R1 support by \@TMRh20 in [`c965f73`](https://github.com/nRF24/RF24/commit/c965f73838383f7d11d15670ddc371f98b5be8b3)
- Small fix before pushing last commit by \@TMRh20 in [`8ff9fb6`](https://github.com/nRF24/RF24/commit/8ff9fb6f155d263adc5671e7b1c28f6265a73f15)
- Raspberry Pi support by \@TMRh20 in [`2406e06`](https://github.com/nRF24/RF24/commit/2406e06a777d1ff0e2feb4c0b62b42f27ee21a59)
- Restore closeReadingPipe by \@a-lurker in [`1289f85`](https://github.com/nRF24/RF24/commit/1289f85c4c25e1dbe4d85a7c2b7e09fba1063e75)
- Merge pull request \#4 from a-lurker/master by \@TMRh20 in [`e682b7d`](https://github.com/nRF24/RF24/commit/e682b7dd0c9677b6e214353896fb68124f803575)
- Install header files and separate install task by \@cornet in [`9895101`](https://github.com/nRF24/RF24/commit/9895101a43015bb1352f86977106567c1a5bacf2)
- Bring readme.md inline with new Makefile by \@cornet in [`dc9ccbf`](https://github.com/nRF24/RF24/commit/dc9ccbff92b1229ff75fec3824595c461f03982d)
- Cleanup readme syntax by \@cornet in [`8e8c111`](https://github.com/nRF24/RF24/commit/8e8c1115eb110a722d623e2490e9caecc0dc9ffd)
- Cleanup table in readme by \@cornet in [`ddd6a0d`](https://github.com/nRF24/RF24/commit/ddd6a0dc1f1e3adda16f5691356e3062669d93bf)
- Use spaces, not tabs, in readme.md by \@cornet in [`ac3962c`](https://github.com/nRF24/RF24/commit/ac3962cca58567886be310af33c6aaae564d3d27)
- Make install should build first by \@cornet in [`c004c55`](https://github.com/nRF24/RF24/commit/c004c5545040d3df83cdce0a9aec1c5dc6278016)
- Merge pull request \#2 from cornet/fix-rpi-build-scripts by \@TMRh20 in [`ee7f603`](https://github.com/nRF24/RF24/commit/ee7f6035cf6753864c14da739fa4c60ba7716ef1)
- Update readme by \@TMRh20 in [`01cac5c`](https://github.com/nRF24/RF24/commit/01cac5c3bfa29c9b0d67efab971c16135d1cb8d8)
- De-Activate features by default with RPi by \@TMRh20 in [`b7e4a09`](https://github.com/nRF24/RF24/commit/b7e4a09abee35c4284b3fa93abfb7617cd81eb42)
- Updated address assignment by \@TMRh20 in [`b7c24a8`](https://github.com/nRF24/RF24/commit/b7c24a81792d1c28aa30de1854c503a747f41bb6)
- Updated examples per issue \#6 by \@TMRh20 in [`a009487`](https://github.com/nRF24/RF24/commit/a009487abdbf2d3d00cd60f1f8e0a979d749b3bb)
- Update rpi readme by \@TMRh20 in [`db1c051`](https://github.com/nRF24/RF24/commit/db1c0510798e8226373cbb303844ca336aed344d)
- Initial Teensy3x support by \@TMRh20 in [`5e63e3a`](https://github.com/nRF24/RF24/commit/5e63e3aeba3403290bd6b5fad5cef6e03f1e8537)
- Updated defines, fixed missing due define by \@TMRh20 in [`b8312b5`](https://github.com/nRF24/RF24/commit/b8312b5382c047d704565e157e10be5273c4c457)
- Further tweaks per \#9 by \@TMRh20 in [`b62e179`](https://github.com/nRF24/RF24/commit/b62e179ab2866a05d14bd8e86db74fe652b5d917)
- - Fixes and modifications for Due & Teensy3.x by \@TMRh20 in [`aac1650`](https://github.com/nRF24/RF24/commit/aac1650646b2c440c8822deb2f99514f4e2e4ddd)
- Final tweaks to address \#9 by \@TMRh20 in [`b8d97f5`](https://github.com/nRF24/RF24/commit/b8d97f53c138a23f6a0d262d508f3f29b5b5b659)
- RPi - Example updates, minor bmc driver change by \@TMRh20 in [`ea98781`](https://github.com/nRF24/RF24/commit/ea9878107267a4c7fa3cf3c2fed405c224e6b56a)
- Adj delay to accomodate 250kbps rates better by \@TMRh20 in [`cb60afe`](https://github.com/nRF24/RF24/commit/cb60afe1dfb1d2cdc6ddf5bc73646b93207a50d4)
- Delay for dyn_Pld_length corruption by \@TMRh20 in [`4e250cb`](https://github.com/nRF24/RF24/commit/4e250cbb07740dfa7e7358c4ab64c4eec44a4874)
- Reduce error rate in call response scenarios by \@TMRh20 in [`9e28a60`](https://github.com/nRF24/RF24/commit/9e28a60236cca121eb3d746ea53b9d362f06b381)
- Merge pull request \#19 from reixd/master by \@TMRh20 in [`f5341bf`](https://github.com/nRF24/RF24/commit/f5341bfc5010f0c87c285510e01f2a7f53bec4b2)
- Select a standard pin layout for the examples. by \@reixd in [`e16c99e`](https://github.com/nRF24/RF24/commit/e16c99e1f0d1c3976a249f52221075cae97681e0)
- Merge pull request \#21 from reixd/examples by \@TMRh20 in [`8c0e9f8`](https://github.com/nRF24/RF24/commit/8c0e9f80b70fbcd192728de4c0f9e89fee89e656)
- RPi B+ Update to BCM2835 V1.37 by \@TMRh20 in [`a2c3977`](https://github.com/nRF24/RF24/commit/a2c3977ac1d60f901887a2dfa2ee5ba28672630f)
- RPi B+ Update to BCM2835 V1.37 by \@TMRh20 in [`1732332`](https://github.com/nRF24/RF24/commit/173233231a12e502ed655544367fee7489772680)
- Merge code per \#17 by \@TMRh20 in [`0153c71`](https://github.com/nRF24/RF24/commit/0153c713d991dc73d35684379f2fb8ef7eab56f2)
- Merge remote-tracking branch 'origin/Updates' by \@TMRh20 in [`1c89b94`](https://github.com/nRF24/RF24/commit/1c89b946cf050b1f13829a7d7f4051926dc1e296)
- Teensy Support by \@TMRh20 in [`5ff3088`](https://github.com/nRF24/RF24/commit/5ff3088ee39376ecb42192bb6366eba1e8aaf92a)
- \#24 Enable UART as 2ndary SPI BUS by \@TMRh20 in [`3f4a1cb`](https://github.com/nRF24/RF24/commit/3f4a1cbcadf3bb2854810b1ef39e077102af94a5)
- \#24 Add softSPI supp, fix SPI_UART by \@TMRh20 in [`56b7711`](https://github.com/nRF24/RF24/commit/56b7711ed09dac3264c96b2ed8a40a16eb599151)
- \#25 Fix CRC status, revert to default SPI lib by \@TMRh20 in [`5238ca2`](https://github.com/nRF24/RF24/commit/5238ca2576562e20c8aca13302f8723b1f1364fe)
- \#24 Fix: SoftSPI or UartSPI working with std SPI by \@TMRh20 in [`2d79e75`](https://github.com/nRF24/RF24/commit/2d79e75e3a468f9c2dfec4e475912c08d7502f83)
- \#24 lol Fix: Spelling and ; on std spi define by \@TMRh20 in [`80e942c`](https://github.com/nRF24/RF24/commit/80e942ce6892e47727552241aa12f16522471690)
- Merge branch 'Updates' of https://github.com/TMRh20/RF24 into Updates by \@TMRh20 in [`ecc5135`](https://github.com/nRF24/RF24/commit/ecc51358c839341328cb5102d4cabe044c00108c)
- Merge remote-tracking branch 'origin/master' into Updates by \@TMRh20 in [`8be1be0`](https://github.com/nRF24/RF24/commit/8be1be0cfa3c88e5ec8e4eeb930946a4092a42ad)
- Built-in support multiple radios on RPi by \@TMRh20 in [`d6a9c3d`](https://github.com/nRF24/RF24/commit/d6a9c3d3cf8309b95b441cb29b39f2541354f55f)
- Character fix by \@lnxbil in [`422495a`](https://github.com/nRF24/RF24/commit/422495ade0b55a7bdd8d159c2735c727fb3ac0d5)
- Merge pull request \#28 from lnxbil/printDetails-fix by \@TMRh20 in [`79eb410`](https://github.com/nRF24/RF24/commit/79eb4107a34578db20e90f58ba43cf9176a40cbc)
- Merge RPi and Arduino code by \@TMRh20 in [`435e4db`](https://github.com/nRF24/RF24/commit/435e4db883811700d4688b9d5c86bf93528d31fa)
- Replace old examples with the current ones for RPi by \@TMRh20 in [`c51049d`](https://github.com/nRF24/RF24/commit/c51049de7c3c3c65967d03f1e201c0b113db0b5d)
- Moved RPi SPI driver to RPi folder from root by \@TMRh20 in [`eabd46a`](https://github.com/nRF24/RF24/commit/eabd46aec6d1313bba1e38ddfd5f3b7db60912d2)
- Incr delay in stopListening(); Fix transfer example by \@TMRh20 in [`8814d39`](https://github.com/nRF24/RF24/commit/8814d39c4505e2ff7b0600f28eaa52735bcfc85f)
- Slight fix for makefile and RF24.h define by \@TMRh20 in [`1da112a`](https://github.com/nRF24/RF24/commit/1da112a6d574f72a142f9136f469505ff18f7f7c)
- Minor change to readme by \@TMRh20 in [`663f41b`](https://github.com/nRF24/RF24/commit/663f41bb02027846bc818a28d5adacc1814f9bc6)
- Another correction RPi code merge (install info) by \@TMRh20 in [`b007fb4`](https://github.com/nRF24/RF24/commit/b007fb4a123eed1e1434dd07bd67410964fe6143)
- Reliability improvements for RPi by \@TMRh20 in [`d41d47a`](https://github.com/nRF24/RF24/commit/d41d47ab20cb794b8250560154076b1b2b06feae)
- Revert last commit, if defined... delay by \@TMRh20 in [`deef592`](https://github.com/nRF24/RF24/commit/deef592bb04729aae5bbd61bce4b31629496b7cd)
- Python wrapper for RPi added by \@martin-mat in [`567ca69`](https://github.com/nRF24/RF24/commit/567ca69234c5c51bbe6ae7f68b3fc6e350c3a6bb)
- Merge pull request \#35 from mz-fuzzy/master by \@TMRh20 in [`a2954f8`](https://github.com/nRF24/RF24/commit/a2954f8f42b97d84f64c5753201cc8a1b3b482fc)
- Revert change to delay from last commit by \@TMRh20 in [`58293d6`](https://github.com/nRF24/RF24/commit/58293d6fe48c13f3e4ddc35bd7ac2767fd0660ea)
- Non-conflicting names by \@jfktrey in [`1650b6b`](https://github.com/nRF24/RF24/commit/1650b6bd58baded2e53e0267bd9268e55a60b675)
- Merge pull request \#38 from jfktrey/master by \@TMRh20 in [`50450a9`](https://github.com/nRF24/RF24/commit/50450a9fe363d7f549497137b1d9587d715e73a1)
- Intel Gallileo support added in [`83dccd2`](https://github.com/nRF24/RF24/commit/83dccd282d75018a7970240b366db768035a1df5)
- Optimization for Galileo Delay by \@spaniakos in [`7574bf5`](https://github.com/nRF24/RF24/commit/7574bf51ca5330beecd83879eaeb35ee53049362)
- Galileo Optimizations and FIFO Fix by \@spaniakos in [`0969010`](https://github.com/nRF24/RF24/commit/0969010b6d3a2f2da956a352e60eef009dfe7515)
- Galileo Optimizations by \@spaniakos in [`e13b238`](https://github.com/nRF24/RF24/commit/e13b238be108fa0a5ed1a2bb829242ed0edd3203)
- Merge pull request \#40 from spaniakos/master by \@TMRh20 in [`aace3b4`](https://github.com/nRF24/RF24/commit/aace3b4bd7ca157303fa964bbc9ade7ac20056a9)
- Updated led_example.pde to work with newer version of library. by \@k2OS in [`65d7fc1`](https://github.com/nRF24/RF24/commit/65d7fc1606dcb4cdd1ecbf0e4eef0f5dc6f9d983)
- Merge pull request \#45 from k2OS/master by \@TMRh20 in [`a0c8087`](https://github.com/nRF24/RF24/commit/a0c808751c4239d5c692588b6730f5fbda7c6954)
- Lower default SPI clock speed by \@TMRh20 in [`3515538`](https://github.com/nRF24/RF24/commit/3515538781ea714a9f4bb5f7313cd2abe676b601)
- Merge pull request \#50 from mz-fuzzy/pyRF24fix by \@TMRh20 in [`350e00a`](https://github.com/nRF24/RF24/commit/350e00a0d4ef8e47224c241f1167b3ce1c375581)
- Update many of the example files by \@TMRh20 in [`d05a356`](https://github.com/nRF24/RF24/commit/d05a356270650adc9d4e98f56362cd6deb373a0e)
- Enable hardware warnings by default on linux by \@TMRh20 in [`831a424`](https://github.com/nRF24/RF24/commit/831a424d3ba1e75dbb7bbcdc4f68b65165201601)
- Update python readme and example by \@TMRh20 in [`714999c`](https://github.com/nRF24/RF24/commit/714999c186466a97a74a151e3713b7e19caae015)
- Attempt to correct markdown formatting in readme by \@TMRh20 in [`3c28f13`](https://github.com/nRF24/RF24/commit/3c28f1307ce492e5110426146a4ebb9ccb34e860)
- Attempt 2 to fix python readme formatting by \@TMRh20 in [`54daf66`](https://github.com/nRF24/RF24/commit/54daf66fc129f38fd52a09ebc58047b21697f52c)
- Updated for using LittleWire by \@lnxbil in [`36fc1f1`](https://github.com/nRF24/RF24/commit/36fc1f10aeaa4d366e7efe4c533589b495453d3e)
- Ignoring DS_Store files on MacOS by \@lnxbil in [`6d6f185`](https://github.com/nRF24/RF24/commit/6d6f185365853822368e15213464b5922533fb5c)
- Slight mods to LittleWire code changes by \@TMRh20 in [`ba4b8a3`](https://github.com/nRF24/RF24/commit/ba4b8a34ea4112aca9f5d1c14536f0e3d6d79698)
- Merge branch 'lnxbil-littlewire' by \@TMRh20 in [`1e05097`](https://github.com/nRF24/RF24/commit/1e050979899096e7528e122595e37b99d87897e1)
- Enable buffered writes/transmissions by \@TMRh20 in [`08480a3`](https://github.com/nRF24/RF24/commit/08480a3b3750e2c44263631c2008682122b9b029)
- Adapted python wrapper for RF24 interface changes in txStandBy and startFastWrite by \@martin-mat in [`8d68102`](https://github.com/nRF24/RF24/commit/8d681027a8ffcf1318ec85c0df5cf920202a6965)
- Merge remote-tracking branch 'TMRh20/master' into pyRF24fix by \@martin-mat in [`b2e40bc`](https://github.com/nRF24/RF24/commit/b2e40bcceaa4250976e9ceab31091c89d10f8ad6)
- Merge pull request \#65 from mz-fuzzy/pyRF24fix by \@TMRh20 in [`a46779f`](https://github.com/nRF24/RF24/commit/a46779fad0f52ffffd1e43a4fd1e3214aed17011)
- Merge pull request \#66 from ax7884/master by \@TMRh20 in [`4b435c9`](https://github.com/nRF24/RF24/commit/4b435c99fdc39de0f31d3798aad667cbf5a48a4e)
- Tweak CONFIG register on radio startup. by \@dsbaha in [`a27b840`](https://github.com/nRF24/RF24/commit/a27b84063269385249f46958f85181ddf2894d69)
- Merge pull request \#67 from dsbaha/master by \@TMRh20 in [`e3f9f9a`](https://github.com/nRF24/RF24/commit/e3f9f9aa395441bd42adf74c527cc9904b590d17)
- Merge remote-tracking branch 'origin/master' into Updates by \@TMRh20 in [`3c652ed`](https://github.com/nRF24/RF24/commit/3c652ed62b2007012b39d4c4bc4190a0b721371c)
- Rough port for BBB \#69 by \@TMRh20 in [`09269a3`](https://github.com/nRF24/RF24/commit/09269a355759c49854ae844a903ac74143ac7fe8)
- \#69 - Forgot to include the main config changes by \@TMRh20 in [`2254bbf`](https://github.com/nRF24/RF24/commit/2254bbf7d5e90b4fe2b619d621f0045b421fa83a)
- Move Due and ATTiny to new template format by \@TMRh20 in [`e7deb68`](https://github.com/nRF24/RF24/commit/e7deb6843828ea729e5fea121166bbb9f83ec4eb)
- Merge pull request \#72 from mannkind/Updates by \@TMRh20 in [`f06ca7c`](https://github.com/nRF24/RF24/commit/f06ca7cb86ce7a0ec5e22174bf8c8b551e2b9ebe)
- Transitioning to new hardware support template by \@TMRh20 in [`e2abd64`](https://github.com/nRF24/RF24/commit/e2abd640a296e1be9b2abc0d27bbd5caa1affa78)
- Restore bcm updates from \#72 by \@TMRh20 in [`2ca76c6`](https://github.com/nRF24/RF24/commit/2ca76c6b24c660a81e9cdbb279ca9576b83bbf39)
- Update for generic spidev/Linux support by \@TMRh20 in [`84f4590`](https://github.com/nRF24/RF24/commit/84f4590dff3c4e4ff0daf7af446d1f25fb21f6f9)
- Update Makefile for LittleWire by \@TMRh20 in [`02b9b16`](https://github.com/nRF24/RF24/commit/02b9b1630366bed83e267d806f0e382c2ebce0c5)
- MRAA support tweaks by \@TMRh20 in [`3d049ab`](https://github.com/nRF24/RF24/commit/3d049abfe890225bb8232f2a025e7415e731e6e2)
- Merge pull request \#75 from prophet-roshak/mraa by \@TMRh20 in [`350cafb`](https://github.com/nRF24/RF24/commit/350cafbce914598917397473dec93ebe728a7284)
- Memory handling with MRAA device contexts by \@prophet-roshak in [`e78893b`](https://github.com/nRF24/RF24/commit/e78893bf7189d534323ecd6fd05bcdf446da9410)
- Memory handling with MRAA device contexts, minor optimizations by \@prophet-roshak in [`2eb660f`](https://github.com/nRF24/RF24/commit/2eb660f64a31b82ffc77f63733092900896e8b6a)
- Merge pull request \#76 from prophet-roshak/mraa by \@TMRh20 in [`225052a`](https://github.com/nRF24/RF24/commit/225052a3c06af786e0085c22680bb4b610bd3a10)
- Update note in gettingstarted example by \@TMRh20 in [`18e6775`](https://github.com/nRF24/RF24/commit/18e67756e4b014d7828592eef5cb15c88cf48903)
- BBB/SPIDEV changes by \@TMRh20 in [`40b907c`](https://github.com/nRF24/RF24/commit/40b907c318b2046edcae761084242838bdfb0d96)
- Cleanup RF24_config.h, add Teensy to arch/ support by \@TMRh20 in [`30a01d7`](https://github.com/nRF24/RF24/commit/30a01d74190d726e3a1bed9da8dbb961e107f4a5)
- Merge pull request \#78 from prophet-roshak/mraa by \@TMRh20 in [`71337bd`](https://github.com/nRF24/RF24/commit/71337bd48e69e77d75a6aeac7c54f4021a25071c)
- Unify RF24 constructor API for all Linux platforms by \@TMRh20 in [`bd5ad6b`](https://github.com/nRF24/RF24/commit/bd5ad6b90c325a1bde6b1be015611ea8e673aeb1)
- Update Makefile with better RPi detection by \@TMRh20 in [`4120b18`](https://github.com/nRF24/RF24/commit/4120b18361723607590be7b967df48ddaa76f38e)
- Merge pull request \#83 from jorihardman/master by \@TMRh20 in [`d28cfcb`](https://github.com/nRF24/RF24/commit/d28cfcbf19b9d132d4135c0f311bff58184a8830)
- Merge branch 'master' of https://github.com/TMRh20/RF24 by \@TMRh20 in [`d577379`](https://github.com/nRF24/RF24/commit/d57737978da8456e28e765a9676bee91934cb510)
- Transactions beta by \@nerdralph in [`c02a51c`](https://github.com/nRF24/RF24/commit/c02a51c25b1a85f025f8174355412cdabf6e7c8c)
- Merge branch 'Updates' of https://github.com/nerdralph/RF24 into Updates by \@TMRh20 in [`3cc10bb`](https://github.com/nRF24/RF24/commit/3cc10bb6093d7c234d23cf0d80bb70a4a6247a4c)
- Initialize setup variable by \@TMRh20 in [`4445842`](https://github.com/nRF24/RF24/commit/444584238db1f76439cb184d8ed859f47ac946ed)
- Change printf to serial.print in transfer example by \@TMRh20 in [`da0c594`](https://github.com/nRF24/RF24/commit/da0c5943c040f29d3957947cc46a4f94dd35492c)
- Use SPI_HAS_TRANSACTION to detect transaction API by \@TMRh20 in [`28c363f`](https://github.com/nRF24/RF24/commit/28c363f4965ad4059e108a0a4921a64e4e737cac)
- Adjust spi transaction detection again by \@TMRh20 in [`dceeabb`](https://github.com/nRF24/RF24/commit/dceeabb3423044e008906ba4a95940271da983bc)
- Update GettingStarted.ino by \@Interneedus in [`6f5e954`](https://github.com/nRF24/RF24/commit/6f5e95494c34b827c803b29c5c342ee96c71c45a)
- Merge pull request \#103 from Interneedus/patch-1 by \@TMRh20 in [`638b199`](https://github.com/nRF24/RF24/commit/638b199b173f771902020445f42bedfed3fdc6a8)
- Update Transfer.ino by \@Interneedus in [`51600d6`](https://github.com/nRF24/RF24/commit/51600d69034b5f74944adedcbb4fdce6d122e4b4)
- Merge pull request \#104 from Interneedus/patch-2 by \@TMRh20 in [`5896d62`](https://github.com/nRF24/RF24/commit/5896d62c2859811fb42d38debf4ba7d7b3359258)
- Update scanner.ino by \@Interneedus in [`7ad1c2f`](https://github.com/nRF24/RF24/commit/7ad1c2f0c621c0cf1c87112459673dbbfdd7e883)
- Merge pull request \#105 from Interneedus/patch-3 by \@TMRh20 in [`a75fd1f`](https://github.com/nRF24/RF24/commit/a75fd1f4cac3eb2b9c82288f8575735840deb649)
- Merge pull request \#122 from ivankravets/patch-1 by \@TMRh20 in [`417e6e5`](https://github.com/nRF24/RF24/commit/417e6e523bd34b64b2e39d13d9d883488cd51f57)
- Modify printDetails() per \#121 by \@TMRh20 in [`594a949`](https://github.com/nRF24/RF24/commit/594a949b4be911430ccdc91d0d2d8a60d288afbf)
- ESP8266 Support per \@crcastle by \@TMRh20 in [`cd52d61`](https://github.com/nRF24/RF24/commit/cd52d61bc08c3c3e46b5164a056b29e61fdf2689)
- Merge branch 'master' of https://github.com/TMRh20/RF24 by \@komby in [`0a8b1d3`](https://github.com/nRF24/RF24/commit/0a8b1d3df28a50bb189b3939da78c20a9b8ed0da)
- Merge pull request \#129 from komby/master by \@TMRh20 in [`16c49cb`](https://github.com/nRF24/RF24/commit/16c49cbbf33cc4ad31ffeb09e835768385880665)
- Change %S to %s for ESP8266 by \@TMRh20 in [`ded7e1b`](https://github.com/nRF24/RF24/commit/ded7e1bc7dbe83490452106bd2677ca4eafd37c1)
- Merge branch 'master' into Updates by \@TMRh20 in [`2db0f2e`](https://github.com/nRF24/RF24/commit/2db0f2e659894ba82f32fa7400639170d6c6b79d)
- Redefine PROGMEM variables for ESP8266 by \@TMRh20 in [`14ed863`](https://github.com/nRF24/RF24/commit/14ed8638de5815fb38e88fe5d4a8daeaacbf6f20)
- Fix for SPIDEV on 4.1.7+ by \@TMRh20 in [`ded2157`](https://github.com/nRF24/RF24/commit/ded2157c0ba68e74e304e2e968e72c406f008875)
- Merge branch 'Updates' by \@TMRh20 in [`5296c16`](https://github.com/nRF24/RF24/commit/5296c16664f1ac22218fdec741a8d67ee3e8b58c)
- Create library.properties by \@Avamander in [`f824302`](https://github.com/nRF24/RF24/commit/f8243024523a14b8216b3a3df97359302028ee94)
- Merge pull request \#140 from Avamander/patch-1 by \@TMRh20 in [`45d085b`](https://github.com/nRF24/RF24/commit/45d085bdc2b9fc3794e59fc297fcf95ccf4936b5)
- Update library.properties by \@Avamander in [`b4e1fa7`](https://github.com/nRF24/RF24/commit/b4e1fa7b13c1c5cd92fc806bc070f2298f34b8ea)
- Merge pull request \#142 from Avamander/patch-1 by \@TMRh20 in [`2e5b360`](https://github.com/nRF24/RF24/commit/2e5b36006a44df481dac64c8b6fb3ab540434734)
- Rename arch folder to utility by \@TMRh20 in [`55b034e`](https://github.com/nRF24/RF24/commit/55b034eeb20c5bc838b7dc5d433162b8180860b8)
- Reverting library properties and releases by \@TMRh20 in [`517326e`](https://github.com/nRF24/RF24/commit/517326e164feeb35e7e54440a8c4b71c042d3f14)

[1.0]: https://github.com/nRF24/RF24/compare/de083c964d9aeeb9fda7485c39fed27443cd617c...v1.0

Full commit diff: [`de083c9...v1.0`][1.0]

## New Contributors
* \@TMRh20 made their first contribution
* \@Avamander made their first contribution
* \@komby made their first contribution
* \@ivankravets made their first contribution
* \@Interneedus made their first contribution
* \@nerdralph made their first contribution
* \@jorihardman made their first contribution
* \@prophet-roshak made their first contribution
* \@mannkind made their first contribution
* \@dsbaha made their first contribution
* \@ax7884 made their first contribution
* \@martin-mat made their first contribution
* \@lnxbil made their first contribution
* \@k2OS made their first contribution
* \@spaniakos made their first contribution
* \@ made their first contribution
* \@jfktrey made their first contribution
* \@reixd made their first contribution
* \@a-lurker made their first contribution
* \@cornet made their first contribution
* \@zephyrr made their first contribution
* \@maniacbug made their first contribution
* \@gcopeland made their first contribution
<!-- generated by git-cliff -->
