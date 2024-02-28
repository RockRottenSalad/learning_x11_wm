# learning X11 by making a terrible WM

This is a pretty simple window manager. It exists soley for testing purposes.
<br><br>
### Controls:
- Super + Return - Spawn xterm
- Super + l - Tile windows so they all fit on screen
- Super + q - Close window manager
- Left mouse button - Drag window around
- Right mouse button - Resize window
- Middle mouse button - Close window

### Running:
Xephyr(a nested X server) is used to run this experimental WM. It is built with [wiz_build](https://github.com/RockRottenSalad/wiz-build). By default wiz_build uses clang, but it can be changed to GCC or CC by simply editing the macro definition inside wiz_build.c

```
cc ./wiz_build.c -o wiz_build
./wiz_build run
```
