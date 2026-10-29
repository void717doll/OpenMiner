# OpenMiner

[![Build Status](https://travis-ci.com/Unarelith/OpenMiner.svg?branch=master)](https://travis-ci.com/Unarelith/OpenMiner)
[![Documentation Status](https://readthedocs.org/projects/openminer/badge/?version=latest)](https://openminer.readthedocs.io/en/latest/?badge=latest)
[![License](https://img.shields.io/badge/license-LGPLv2.1%2B-blue.svg)](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html)
[![Discord](https://img.shields.io/discord/527527086756200458.svg?style=popout)](https://discord.gg/eN8k8wt)
[![Donate PayPal](https://img.shields.io/badge/donate-paypal-brightgreen.svg)](https://www.paypal.me/Unarelith)
[![Donate Patreon](https://img.shields.io/badge/donate-patreon-brightgreen.svg)](https://www.patreon.com/unarelith)

- [Project goal](#project-goal)
- [Wiki](#wiki)
- [Keys](#keys)
- [How to compile](#how-to-compile)
    - [Using Windows](#using-windows)
    - [Using Linux](#using-linux)
    - [Using MacOS](#using-macos)
- [Discussion](#discussion)
- [Project status](#project-status)
- [Screenshots](#screenshots)
- [Credits](#credits)

## Project goal

The long-term goal of this project is to provide a viable alternative to Minecraft modding.

## Wiki

- [Getting started](https://github.com/Unarelith/OpenMiner/wiki/Getting-Started)
    - [Contributing](https://github.com/Unarelith/OpenMiner/blob/master/CONTRIBUTING.md)
    - [How to use git](https://github.com/Unarelith/OpenMiner/wiki/How-to-use-git)
    - [Roadmap](https://github.com/Unarelith/OpenMiner/wiki/Roadmap)
    - [Coding style](https://github.com/Unarelith/OpenMiner/wiki/Coding-Style)
- [Lua API](https://openminer.readthedocs.io/en/latest/)
    - [Core API](https://openminer.readthedocs.io/en/latest/lua-api-core/)
    - [Mod API](https://openminer.readthedocs.io/en/latest/lua-api-mod/)
    - [GUI API](https://openminer.readthedocs.io/en/latest/lua-api-gui/)
    - [C++ classes in Lua](https://openminer.readthedocs.io/en/latest/lua-api-cpp/)
- [local world runtime](docs/local-world.md)

## Keys

- Movement: <kbd>W</kbd><kbd>A</kbd><kbd>S</kbd><kbd>D</kbd>
- Inventory: <kbd>E</kbd>
- Chat: <kbd>T</kbd>
- Jump: <kbd>Space</kbd>
- Sprint: <kbd>Ctrl</kbd>
- Jetpack: <kbd>X</kbd>
- Exit: <kbd>Escape</kbd>
- Debug: <kbd>F3</kbd>
- Fullscreen: <kbd>F11</kbd>

## How to compile

### Using Windows

- [Wiki: Compiling on Windows using CMake and MinGW-w64](https://github.com/Unarelith/OpenMiner/wiki/Compiling-on-Windows-with-MinGW-w64)
- [Wiki: Compiling on Windows using CMake and Visual Studio 2019](https://github.com/Unarelith/OpenMiner/wiki/Compiling-on-Windows-with-Visual-Studio-2019)

### Using Linux

- [Wiki: Compiling on Linux](https://github.com/Unarelith/OpenMiner/wiki/Compiling-on-Linux)

### Using MacOS

- Dependencies:
    - A compiler with C++17 support (GCC >= 7.0 or clang >= 5.0)
    - `git` and [CMake](http://www.cmake.org/download/) (>= 3.12.4)
    - **Optional:** `ninja` (much faster builds)
    - _Check Homebrew for packages._
- Run `cmake -B build . && cmake --build build`; if you installed `ninja` above, add `-GNinja` before `-B` in the previous command.
- Run the client with `./build/openminer`
- choose play to create or load a local world. `./build/openminer --world MyWorld` opens a named save directly.

## Discussion

- Discord: [join server](https://discord.gg/eN8k8wt)
- IRC: irc.freenode.net `#openminer`

## Project status

This list is not complete.

See also the roadmap for 1.0.0 [here](https://github.com/Unarelith/OpenMiner/wiki/Roadmap).

### Implemented features

- Menus (title screen, world selection, settings, etc...)
- Basic worldgen (biomes, lakes, trees, flowers, pseudo-caves)
- Pseudo-infinite world in X/Y/Z axis (2^32 * 2^32 * 2^32 blocks) ([#25](https://github.com/Unarelith/OpenMiner/pull/25))
- Smooth lighting + ambient occlusion
- Client/server architecture ([#20](https://github.com/Unarelith/OpenMiner/pull/20))
- Chat with support for commands (for example `/tp`) ([#57](https://github.com/Unarelith/OpenMiner/pull/57))
- Lua modding API
	- Block/item/recipe/biome/tree/sky/dimension/entity definition
	- Custom GUI creation
- Blocks can have an inventory and update every server tick (workbench, furnace)
- Block metadata
- Player model display (without animation)
- Dimensions (like the Nether or the Ender in Minecraft) ([#80](https://github.com/Unarelith/OpenMiner/pull/80))
- World loading/saving (see [#26](https://github.com/Unarelith/OpenMiner/issues/26))
- Built-in refi block, item, mob, and heart artwork
- Entities ([#90](https://github.com/Unarelith/OpenMiner/pull/90))
- Day/night cycle with sun/moon/stars ([#154](https://github.com/Unarelith/OpenMiner/pull/154))

### Missing features

- Fluid propagation ([#62](https://github.com/Unarelith/OpenMiner/issues/62))
- Seed-based worldgen ([#116](https://github.com/Unarelith/OpenMiner/issues/116))
- Cave tunnels ([#118](https://github.com/Unarelith/OpenMiner/issues/118))
- Clouds ([#156](https://github.com/Unarelith/OpenMiner/issues/156))
- Particle system ([#155](https://github.com/Unarelith/OpenMiner/issues/155))

## Screenshots

![](screenshot1.png?raw=true)
![](screenshot2.png?raw=true)
![](screenshot3.png?raw=true)
![](screenshot4.png?raw=true)
![](screenshot5.png?raw=true)
![](screenshot6.png?raw=true)

## Credits

- MysticTempest ([REFI_Textures](https://github.com/MysticTempest/REFI_Textures), [asset attribution and license](docs/refi-assets.md))
- ThePhD ([sol2](https://github.com/ThePhD/sol2))
- skypjack ([EnTT](https://github.com/skypjack/entt))


## generation and regression checks

beta 1.7.3 climate, density, surface replacement, overworld caves, and nether terrain/caves now follow the supplied retromcp sources. complete-column java comparisons pass. oldworld keeps its decoration and feature population, so full generated worlds still differ. saved chunks remain unchanged.

the beta mob roster has models and simulation, including ranged projectiles, creeper explosions, aquatic/flying movement, and animal interactions. left click attacks and right click interacts. food restores health, displayed as full, half, and empty refi hearts above the hotbar. the first-person view shows the empty hand or selected block/item with swing animation. refi artwork is built in; the texture-pack selector and overrides have been removed.

worldgen still needs staged cross-chunk decoration, parallel generation, superchunks, and the terrain-formula api. clouds, particles, and arbitrary custom meshes remain unfinished. water/lava propagation and falling blocks now use the [lunarchy physics port](docs/lunarchy-physics.md). mob pathfinding and several animations also differ from beta. see [the port notes](docs/retromcp-port.md) for source mappings, validation, and remaining gaps.

local worlds run entirely in the game process. remote joining, hosting, dedicated-server startup, sockets, remote player rendering, and address/username settings have been removed. the simulation thread exchanges local messages with rendering and preserves existing saves.

survival uses [beta movement, mining and tool durability](docs/beta-survival.md). worn tools retain their damage through inventory transfers, drops and saves. fly/noclip/sprint, creative inventory, starter gifts and cheat commands are removed. mining uses tool tiers and ground drops; falling, drowning and hazardous blocks cause damage.
