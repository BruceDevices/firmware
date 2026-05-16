# Contributing guidelines

These are the current requirements for getting your code included in RF24:

- Try your best to follow the rest of the code, if you're unsure then [the NASA C style guide](https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19950022400.pdf) can help as it's closest to the current style.

- Definitely follow [PEP-8](https://www.python.org/dev/peps/pep-0008/) if it's Python code.

- Follow the [Arduino example formatting style](https://docs.arduino.cc/learn/contributions/arduino-writing-style-guide) for Arduino examples

- Add [doxygen-compatible documentation](https://www.doxygen.nl/manual/docblocks.html) to any new functions you add, or update existing documentation if you change behavior

- CMake modules and CMakeLists.txt files should also have a uniform syntax.
  - Indentation is a mandatory 4 spaces (not a `\t` character).
  - Closing parenthesis for multi-line commands should have the same indentation as the line that opened the parenthesis.
  - For other useful CMake syntax convention, please see [CMake docs for developers](https://cmake.org/cmake/help/v3.20/manual/cmake-developer.7.html) and [this useful best CMake practices article](https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1). The qiBuild project has some [well-reasoned "Dos & Don'ts" guideline](http://doc.aldebaran.com/qibuild/hacking/contrib/cmake/coding_guide.html), but beware that the nRF24 organization is not related to the qiBuild project in any way.
