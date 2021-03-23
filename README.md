# MiniVHD - Minimalist VHD implementation in C

**MiniVHD** is a minimalist implementation of read/write/creation of VHD files. It is designed to read and write to VHD files at a sector level. It does not enable file access, or provide mounting options. Those features are left to more advanced libraries and/or the OS.

## Features
MiniVHD aims to implement as much of the VHD specification as possible. Features include:
* Implemented in pure C. Some C99 features are used (such as stdint, stdbool, array initialisation), so not compatible with strict ANSI C
* Full support for fixed, sparse (dynamic) and differencing VHD images
* Open existing VHD images
* VHD image creation
* Conversion to/from raw disk images
* Read/write sectors to VHD images
* Aims to be cross platform, although not fully there yet. Works with MinGW-w64, and presumably GCC/Clang
* Simple to include and use (I hope)

## Usage

Include `minivhd.h` in your source to get started. See `minivhd.h` for documentation of the API.  Then link the application with the library (-lminivhd for UNIX and WinGW, or minivhd.lib for Visual Studio on Windows.)

**Please note, an older version of this library can be found in the `minivhd-v1` branch of this repository, if required for some reason.**
