# megata
Cross platform emulator for the [Gamate](https://en.wikipedia.org/wiki/Gamate) handheld console released by Bit Corporation.

## Running

### BIOS

A valid Gamate BIOS is required to run games. The BIOS can be found on the [Internet Archive](https://archive.org) by searching for *Gamate*. Download and place the `gamate.zip` BIOS fril in the same directory as `megata.exe`.

### Game library

A collection of 58 Gamate game ROMs can be found on the [Internet Archive](https://archive.org). 

## Build Instructions

### MinGW

- Download and run the installer from https://www.msys2.org/
- Install the build dependencies from the shell
``` shell
pacman -S git mingw-w64-x86_64-toolchain make mingw-w64-x86_64-raylib
```
- Build the executable
``` shell
make
```


### MinGW (Linux cross compile)
- Ubuntu / Debian
- Install the build toolchain
``` shell
sudo apt install gcc-mingw-w64-x86-64  g++-mingw-w64-x86-64 make
```
- Install raylib
``` shell
wget -O /tmp/raylib-5.5.0_win64_mingw-w64.zip https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win32_mingw-w64.zip
sudo unzip /tmp/raylib-5.5.0_win64_mingw-w64.zip -d /opt/local/
```
- Build the executable
``` shell
CONFIG_W64=1 make
```

### Linux

- Ubuntu / Debian

- Install raylib
``` shell
sudo apt install build-essential
wget -O /tmp/5.5.0.tar.gz https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
tar xf /tmp/5.5.0.tar.gz -C /tmp/
cd /tmp/raylib-5.5/src
make clean
make PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=SHARED
sudo make install RAYLIB_LIBTYPE=SHARED
```

``` shell
make
```
