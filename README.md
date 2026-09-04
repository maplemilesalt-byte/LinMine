# LinMine

Minesweeper port to Linux.

## Linux build

The current Linux executable uses SDL2 for the window, input, and 2D rendering. SDL2's BMP loader is used for the original bitmap assets. citeturn0search0turn0search1

### Dependencies

On Debian/Ubuntu:

```sh
sudo apt install build-essential pkg-config libsdl2-dev
```

On Arch Linux / CachyOS:

```sh
sudo pacman -S base-devel pkgconf sdl2
```

### Compile

From the repository root:

```sh
cd linux
make
```

This creates:

```text
linux/linmine
```

Run it with:

```sh
./linmine
```

### Controls

- Left mouse button: open a square
- Right mouse button: place/remove a flag
- Click the top panel: start a new game
- Close the window: quit

## Project structure

- `winmine/` — original NT Minesweeper sources and bitmap resources.
- `winmine/bmp/` — original bitmap assets.
- `linux/gfx.h` — platform-neutral graphics/input interface.
- `linux/gfx.c` — SDL2 implementation of that interface.
- `linux/main.c` — currently runnable Linux game loop.
- `linux/Makefile` — Linux build rules.

The Linux layer is being built so the original NT game logic can be connected without making the game logic depend directly on SDL, X11, or Wayland.

## Todo
* Compile Snap version
* Fix my kernel panic
