# rvm_cd

An attempt to recreate the engine the Sonic CD 2011 mobile version uses. Heavily based on a decompile of the WP7 version.

## BUGS

* The ground in the special stages is low quality
* No zone card

## Controls

* Arrow Keys - Move
* J & K - Jump
* Escape - Pause
* Enter - Start

# Building

You need ffmpeg for pc and nx builds, and switch-mesa for nx

## PC:

```
cmake -B build && cmake --build build -j$(nproc)
```

## NX

```
sudo pacman -S switch-ffmpeg switch-mesa
cmake -B build-nx -DPLATFORM=NX . && cmake --build build-nx -j$(nproc)
```
