# LinMine

Minesweeper port to Linux.

## Linux build

The current Linux executable provides the classic Minesweeper window and input behavior.

### Dependencies

On Debian/Ubuntu:

```sh
sudo apt install build-essential pkg-config libgtk-3-dev
```

On Arch Linux / CachyOS:

```sh
sudo pacman -S base-devel pkgconf gtk3
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

## Releases

Prebuilt Linux binaries and Flatpak packages are available from the GitHub Releases page.

## Flatpak

The Flatpak manifest is located at `flatpak/io.github.maplemilesalt.LinMine.yml`.

To build it locally with Flatpak Builder:

```sh
flatpak-builder build-dir flatpak/io.github.maplemilesalt.LinMine.yml
```

### Controls

- Left mouse button: open a square
- Right mouse button: place/remove a flag
- Click the top panel: start a new game
- Close the window: quit

Touchscreen input is also supported through the normal pointer input path.

## Project structure

- `winmine/` — original NT Minesweeper sources and bitmap resources.
- `winmine/bmp/` — original bitmap assets.
- `linux/gfx.h` — platform-neutral graphics/input interface.
- `linux/gfx.c` — Linux graphics/input implementation.
- `linux/main.c` — currently runnable Linux game loop.
- `linux/Makefile` — Linux build rules.
- `flatpak/` — Flatpak packaging files.

The Linux layer is being built so the original NT game logic can be connected without making the game logic depend directly on a specific Linux windowing system.

## Todo

* Improve tablet mode
* Fix my kernel panic
