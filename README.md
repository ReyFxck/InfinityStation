# InfinityStation – SNES Frontend for PlayStation 2

[![English](https://img.shields.io/badge/English-red)](README.md)
[![Português-BR](https://img.shields.io/badge/Português--BR-blue)](README.pt-BR.md)

InfinityStation is a frontend/launcher in development for running an SNES core on the PlayStation 2, with a focus on performance, modularization, and usability on original hardware.

> **Note**
> This project is still under development (**W.I.P**). The current focus is code cleanup, modularization, and performance and usability improvements. See the **Current status and limitations** section for the current state of the project.

## Index

- [Requirements](#requirements)
- [Installation](#installation)
- [Useful commands](#useful-commands)
- [Testing methods](#testing-methods)
- [Supported ROM formats](#supported-rom-formats)
- [Project structure](#project-structure)
- [Technologies used](#technologies-used)
- [Current status and limitations](#current-status-and-limitations)
- [Progress](#progress)
- [Future plans](#future-plans)
- [How to contribute](#how-to-contribute)
- [License and legal notices](#license-and-legal-notices)
- [Credits](#credits)
- [Contact](#contact)

## Requirements

- A PlayStation 2 or emulator, for testing purposes.
- A configured PS2 build environment (`ps2sdk`, `ee-gcc` compiler, and `make`).
- Compatible SNES ROMs in `.sfc`, `.smc`, `.swc`, `.fig`, or `.zip` format.

## Installation

1. **Clone the repository**

   ```bash
   git clone https://github.com/ReyFxck/InfinityStation.git
   cd InfinityStation
   ```

2. **Build**

   Make sure `ps2sdk` and `make` are installed and available in your `PATH`.

   ```bash
   make
   ```

   The main build generates the project's ELF file.

## Useful commands

```bash
make help
make clean
make rebuild
make v
make push out=<dest_folder>
make iso [roms=<roms_folder>] [out=<dest_folder>]
```

### Quick summary

- `make help` — shows the available commands.
- `make clean` — removes leftovers from previous builds.
- `make rebuild` — cleans and rebuilds the project.
- `make v` — builds with verbose output (full compiler invocations).
- `make push out=<dest_folder>` — copies the ELF to `<dest_folder>` (works for both Android (`/sdcard/ps2`) and Windows (`/mnt/c/...`) paths).
- `make iso` — generates a test ISO.
- `make iso roms=<roms_folder>` — generates an ISO including ROMs from `<roms_folder>`.
- `make iso roms=<roms_folder> out=<dest_folder>` — generates the ISO and copies it to `<dest_folder>`.

Legacy targets (`make push-android`, `make push-win WIN_OUT_DIR=...`, `make iso-push ISO_ROMS_DIR=...`) keep working for existing scripts but are no longer shown in `make help`.

## Testing methods

### 1. ELF testing
- Build the project.
- Copy the generated ELF to a USB device or memory card.
- Run it through Free McBoot, LaunchELF, or another compatible manager.

### 2. ISO testing
- Generate a test ISO with:
  ```bash
  make iso
  ```
- To include ROMs in the image:
  ```bash
  make iso roms=/path/to/roms
  ```

### 3. Emulator testing
- The project can be tested in emulators by loading the ELF directly.
- When applicable, the ISO can also be used for quick boot flow validation.

## Supported ROM formats

The formats currently considered in the project's workflow are:

- `.sfc`
- `.smc`
- `.swc`
- `.fig`
- `.zip`

> **Note:** practical support may vary depending on the current state of the core, loader, and testing coverage.

## Project structure

```text
.github/workflows/    # CI / automated build
libretro-common/      # shared utilities
ps2boot/              # frontend, PS2 integration, and ROM loader
source/               # SNES core base code
libretro.c            # main bridge to the libretro core
Makefile              # build, push, and ISO generation
```

### Main directories inside `ps2boot/`

- `ps2boot/app/`  
  Main app flow, boot, runtime, state handling, transitions, overlay, and frontend configuration.

- `ps2boot/ui/launcher/`  
  Main launcher, browser, pages, actions, background, and fonts.

- `ps2boot/ui/select_menu/`  
  In-game menu and related pages.

- `ps2boot/audio/`, `ps2boot/video/`, `ps2boot/input/`, `ps2boot/menu/`, `ps2boot/storage/`, `ps2boot/debug/`  
  PS2-specific platform code split by subsystem.

- `ps2boot/rom_loader/`  
  ROM loader and ZIP support, including `miniz` integration.

## Technologies used

- **snes9x2005** — SNES emulation core adapted for PS2.
- **miniz** — ZIP decompression library.
- **ps2sdk** — SDK for PlayStation 2 homebrew development.
- **C / GCC** — main project language and compiler.
- **GitHub Actions** — build automation used to validate project compilation.

## Current status and limitations

The project still has several bugs, but it is already in a playable state through emulation.

Known limitations at the moment:

- Audio works but is unstable;
- FPS drops may happen even with EE headroom available;
- some menus still show incorrect behavior, such as confusion when opening the correct ROM or loading through USB;
- some animations are currently too fast;
- the launcher settings menu has not been implemented yet;
- launcher music remains disabled because of the current audio issues.

## Progress

### Menus

| Area | Optimization | Progress / Additions | Bugs / Improvements |
|---|---:|---:|---:|
| Initial launcher menu | 50% | 65% | 10% |
| Settings menu (initial launcher) | 0% | 0% | 0% |
| Credits menu (initial launcher) | 10% | 30% | 75% |
| Device selection | 60% | 10% | 95% |

### Game / Core / System

| Area | Optimization | Progress / Additions | Bugs / Status |
|---|---:|---:|---:|
| In-game audio | 60% | 50% | 100% *(no audio)* |
| General optimizations | 50% | 50% | 50% |
| GS | ? | ? | ? |
| NTSC/PAL support | ? | ? | ? |
| Core feature support | 60% | 30% | 75% |
| Screen functions (PS2) | 50% | 45% | 30% |

## Future plans

- More options in the launcher and in-game menu.
- Better visual feedback and error messages.
- More ROM compatibility testing.
- Visual polish and interface improvements.
- More complete documentation and translation.
- More real hardware testing.

## How to contribute

Contributions are welcome. To help:

1. Open an *issue* with suggestions, questions, or problems you found.
2. Fork the repository and create a branch for your change.
3. After implementing and testing it, submit a *pull request* with a clear description of the changes.
4. Feel free to help with documentation, translation, or new features.

## License and legal notices

InfinityStation as a whole is distributed under the **GNU General Public
License, version 2 or (at your option) any later version** — see the
[`LICENSE`](LICENSE) file at the root of the repository.

This repository also bundles components derived from or integrated with
third-party projects under their own (compatible) licenses. Before
redistributing, reusing, or relicensing any part of the project, check:

- the `LICENSE` file (project-wide license);
- the `copyright` file (third-party attributions);
- the repository credits;
- the headers and notices present in inherited files.

The aggregate work is GPLv2-or-later because that is the most restrictive
of the licenses combined here (`ndssfc` and the libretro frontend code).
Individual components keep their original licenses where applicable.

> **Note:** do not commit copyrighted ROM dumps to this repository or
> attach them in issues / pull requests.

## Credits

Special thanks to:

- **Iaddis** (`@iaddis`) — ideas observed in the Snesticle project files for implementing ZIP file loading.
- **Rich Geldreich** (`@richgel999`) — for creating the `miniz` library used for ZIP decompression.
- **The Snes9x community and @libretro** — for the `snes9x2005` core and related work.
- **`ps2sdk` / `ps2dev` contributors** (`@ps2dev`) — for making PS2 development possible.

## Contact

For questions, suggestions, or collaboration, use [GitHub Issues](https://github.com/ReyFxck/InfinityStation/issues) or contact the maintainer.
