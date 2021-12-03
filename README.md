# CG Guide

A modern OpenGL application focused on teaching computer graphics principles through user interaction.

![CG Guide Demo](https://github.com/guicattani/cgguide/blob/main/assets/cgguide1.gif)

![CG Guide Demo 2](https://github.com/guicattani/cgguide/blob/main/assets/cgguide2.gif)
# Build

# Pre Requisites

- Download GLFW3 pre-compiled binaries ([here](https://www.glfw.org/download.html)) for the **same** version as your MinGW/VS version and put it in `/lib`.
- Download FreeType ([here](https://github.com/freetype/freetype)) use CMake and skip all dependencies with
```
  mkdir build
  cd build
  cmake -G "MinGW Makefiles" -B build -D CMAKE_DISABLE_FIND_PACKAGE_ZLIB=TRUE -D CMAKE_DISABLE_FIND_PACKAGE_BZip2=TRUE  -D CMAKE_DISABLE_FIND_PACKAGE_PNG=TRUE -D CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz=TRUE -D CMAKE_DISABLE_FIND_PACKAGE_BrotliDec=TRUE ..
```
put the `libfreetype.a` file in the `/lib`

## Windows

Use VS to build for Windows (VS solution pending) or use [GNUWin Make](http://gnuwin32.sourceforge.net/packages/make.htm) and run `make`

## Linux

### Debian

In Debian run `sudo apt-get install libglfw3-dev` and then run `make`

### Arch

In Arch Linux run `yay -S glfw-x11` and run while in `bin` folder and then run `make`
