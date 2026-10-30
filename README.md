# openminer

openminer is a small, free and libre block-building game inspired by minecraft beta 1.7.3. gather materials, make tools, build a home, and explore. it runs offline and saves worlds locally.

choose survival to mine, craft, farm, and fight mobs, or creative to build with unlimited blocks. food restores health directly. tools wear out, armor provides protection, and caves lead to ores and underground lakes. obsidian portals connect the overworld to the nether.

## changes in this version

- terrain, caves, biomes, mining, movement, and survival rules use minecraft beta 1.7.3 as their reference.
- refi textures replace the old block, item, mob, and interface artwork. the inventory and hud use compact hearts, an armor bar, item names above the hotbar, and optional durability text.
- held items use the normal right-hand pose. dropped items have ground collision, pickup delays, rotation, and bobbing.
- ordinary blocks and items stack to 99. food, tools, and armor do not stack. food heals directly, without a hunger bar.
- tools wear out, correct tool tiers control drops, and equipment keeps its damage through inventory moves and saves. copper comes between stone and iron.
- armor appears on the player model. leather adds fall protection, gold adds fire protection, and chainmail adds general damage protection.
- material sounds cover footsteps, digging, breaking, placement, and item pickups.

## additions

- survival and creative world creation, saved game modes, automatic names for unnamed worlds, and a searchable creative inventory.
- copper ore and tools, craftable tnt, ferns, three-dimensional armor, and held-item tooltips.
- nether portals, ghasts, zombie pigmen, glowstone clusters, basalt, quartz, and nether masonry.
- cave moss, fungi, crystals, lush cave plants, cherry trees, and a spyglass.
- potatoes, tomatoes, barley, extra food recipes, bone meal, mulch, fertilizer, beehives, honey, and wax.
- ocelots and bats, working buckets, beds, clocks, minecarts, powered rails, and detector rails.
- dungeon rooms with loot chests and mob spawners, plus jukeboxes and music discs.

## removals

- multiplayer joining, hosting, dedicated-server startup, and address settings.
- shields, offhand slots, fireworks, and the block-information overlay.
- texture-pack selection and overrides; refi artwork is bundled with the game.
- starter gifts, cheat commands, sprint, and survival flight. creative keeps its own inventory and flight controls.

## play

choose **play** to create or open a world. new worlds have a seed and a survival or creative setting. leaving the name empty creates `World 1`, `World 2`, and so on.

| control | action |
| --- | --- |
| wasd | move |
| mouse | look |
| space | jump |
| shift | sneak |
| left click | break blocks or attack |
| right click | place blocks or use items |
| e | inventory and crafting |
| h | creative inventory |
| 1–9 or mouse wheel | select a hotbar slot |
| q | drop one item |
| escape | pause |
| f11 | fullscreen |

controls, sound volume, and hud options can be changed in settings.

## build

requires git, cmake, a c++17 compiler, and the platform development libraries needed by sdl and bgfx. run from the project directory:

```sh
git submodule update --init --recursive
cmake -S . -B build -DOM_BUILD_TESTS=OFF
cmake --build build --target openminer -j4
./build/openminer
```

launch the game from the project directory so it can find its files.

## credits

- **OpenMiner**, by Unarelith, Quentin Bazin, and its contributors: the original project and engine.
- **Luanti** and **Lunarchy**: references for movement, block physics, held items, and sound playback.
- **RetroMCP**: the beta 1.7.3 source reference for terrain, survival rules, mobs, and models. [port notes](docs/retromcp-port.md).
- **Mineclonia**: gameplay references, models, and sounds. [sound credits](docs/sound-assets.md).
- **MysticTempest's REFI_Textures**: the block, item, mob, and interface textures. [texture credits](docs/refi-assets.md) and [asset inventory](docs/refi-assets.json).
- the authors of the libraries in `external/`, including SDL, bgfx, EnTT, sol2, GLM, SFML, tinyxml2, and zlib.

## licenses

OpenMiner's license is retained in [LICENSE](LICENSE). refi textures are licensed under [CC BY-SA 4.0](docs/REFI-LICENSE.txt). imported models and sounds retain their upstream licenses and credits alongside the files.
