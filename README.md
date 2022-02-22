<h1 align="center">mtest</h1>

<p align="center">
  <img alt="screenshot" src="https://raw.githubusercontent.com/codeandkey/mtest/master/screenshot.png">
</p>

## About
mtest is a minimal testing framework for C++.

## Requirements
- Windows or UNIX-like host
- A compiler supporting C++11

## Usage

To include mtest into your project, download [mtest.h](https://raw.githubusercontent.com/codeandkey/mtest/master/mtest.h), [mtest.cpp](https://raw.githubusercontent.com/codeandkey/mtest/master/mtest.cpp) and place them alongside your source files. See the [examples](https://github.com/codeandkey/mtest/tree/master/examples) directory for examples.

### CMake integration
mtest supports integration with CMake/CTest. See [cmake](https://github.com/codeandkey/mtest/tree/master/examples/cmake) for an example application. To use cmake integration you must add [mtest.cmake](https://raw.githubusercontent.com/codeandkey/mtest/master/cmake/mtest.cmake) to your project.
