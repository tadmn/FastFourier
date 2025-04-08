# FastFourier

A simple FFT wrapper for all the major desktop and mobile OSes. This library will always wrap the fastest (& most free)
FFT library for your platform.

This is a fork of https://github.com/hogliux/FastFourier. The original is built as a shared library (dll), whereas
this fork is built as a static library and is thus simplified somewhat (and doesn't require installation of the shared
lib alongside your binaries).

Currently, this fork only supports macOS and Windows x86_64.

## Setup

### macOS
Nothing is required. The vDSP library (via the Accelerate framework) is already installed on all macOS systems.

### Windows x86_64
You will need to download the library files for the Intel Performance Primitives (IPP). To do this, download the Intel
oneAPI installer from the Intel website. Use the installer to install the packages (I just install most everything to
make it easier). The files we need should get automatically installed to:
```
/Program Files (x86)/Intel/oneAPI/ipp/latest/lib
```