<div align="center">

# PS2 SNES Concept

[![English](https://img.shields.io/badge/English-red)](README.md)
[![Português-BR](https://img.shields.io/badge/Português--BR-blue)](README.pt-BR.md)

A PlayStation 2 frontend/boot project for running a SNES core with a custom launcher, in-game menu, ZIP ROM loading, and PS2-specific platform code.

</div>

> [!NOTE]
> This project is currently **W.I.P**.
>
> It is focused on cleanup, modularization, PS2 usability improvements, and making future work easier.

## Inspired By

This project is heavily inspired by:
- **PGEN**
- **SNESticle**
- **SNESStation**

Not as a direct copy of any one of them, but as part of the general idea, style, and direction for building a more usable SNES frontend/experience on PlayStation 2.

## Current Features

- Embedded ROM boot option
- USB browser/launcher
- In-game menu
- Display position adjustment
- Aspect ratio options
- FPS overlay
- Frame limit options
- ZIP ROM loading support
- Modularized app, launcher, menu, and PS2 video code

## Progress

Recent work includes:
- splitting large launcher files into smaller modules
- splitting select menu actions and page rendering
- splitting PS2 video code into separate parts
- splitting app startup, runtime, and callback responsibilities
- adding modular ROM loading
- adding ZIP support through `miniz`
- cleaning and consolidating the `Makefile`

## Planned Improvements

- more launcher options
- better UI feedback and error messages
- more ROM compatibility testing
- more visual polish
- more documentation
- more real hardware testing

## Credits

- **Iaddis** (**@iaddis**)  
  Some ideas about how ZIP file loading could be implemented.

- **Rich Geldreich** (**@richgel999**)  
  `miniz`, used for ZIP decompression and support.

- **Snes9x and community**, together with **@libretro**  
  For the `snes9x2005` core and related work used in this project.

- **ps2sdk / ps2dev contributors** (**@ps2dev**)  
  Thank you for the effort that makes it easier to build applications for the PlayStation 2.

## Notes

This does not represent a final polished release.

The project is currently focused on:
- code cleanup
- modularization
- PS2 usability improvements
- making future work easier

## License

See the `LICENSE` file in this repository.
