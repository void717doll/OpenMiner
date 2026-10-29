# retromcp port

source: the user-supplied `~/retromcp` checkout, configured for beta 1.7.3. server sources under `minecraft_server/src/net/minecraft/src` supply gameplay behavior; client sources under `minecraft/src/net/minecraft/src` supply model geometry. the implementation remains c++17 and lua.

| source | implementation |
| --- | --- |
| `NoiseGeneratorPerlin`, `NoiseGeneratorOctaves`, `ChunkProviderGenerate.func_4058_a` | java-compatible random generator, bulk octave sampling, density blending, vertical gradients, and 4×4×8 interpolation in `RetroTerrain.hpp` |
| `ItemFood`, `ItemSoup`, `ItemCookie`, `Item` | existing item definitions gain healing amounts, beta stack limits, and stew bowl returns |
| `EntityLiving`, `EntityAnimal`, `EntityPig`, `EntityMob`, `EntityZombie` | pig health 10, zombie health 20, pursuit within 16 blocks, melee range 2, attack cooldown 20 ticks, zombie damage 5, wandering, gravity, collision, and drops |
| `ModelRenderer`, `TexturedQuad`, `ModelPig`, `ModelQuadruped`, `ModelBiped`, `ModelZombie` | textured cuboids, mirrored limbs, raised zombie arms, and walking animation |
| `jars/minecraft.jar`: `mob/pig.png`, `mob/zombie.png` | original textures in `mods/default/textures/mobs` |

## behavior and limits

- oldworld maps the source's vertical y coordinate to z and keeps its 16×16×32 chunks. density is sampled over the source's 128-block surface region; terrain continues as solid material below zero and air above 127.
- density currently uses temperate temperature/humidity constants. oldworld retains its biome selection, material definitions, ore/decorations, and cave-carving code. this is not a block-for-block beta world generator.
- the source noise initialization order and bulk sampler's vertical caching are retained. 1,350 octave samples from the supplied java implementation matched exactly across positive and negative seeds and coordinates; 3,825 density samples differed by at most 1.14e-13 with the same climate inputs.
- mobs use direct pursuit and obstacle jumping through the shared collision solver. beta's full pathfinder, riding/saddles, sound, breeding, and additional mob types are not included. motion speed is adapted to oldworld's server ticks.
- passive natural spawns require loaded grass above sea level; hostile spawns use nighttime. natural spawning is limited to the default dimension and 64 mobs per world. `/spawn pig|zombie` also respects the cap and loaded, empty space.
- left click uses a server-side three-block ray and attack cooldown. fist damage is one health point. drops go into the attacking player's inventory. zombies take sunlight damage; death respawns the player at the origin's surface with 20 health.
- foods follow the source's immediate consumption behavior, including consumption at full health. no hunger system is introduced. cookies stack to eight; other ported foods stack to one. oversized stew stacks from older saves retain their unconsumed portions; returning a bowl requires inventory space.

## saves and network

saved chunks are retained. unexplored terrain uses the new density generator and may meet older terrain at seams. old saves without player health default to full health on a fresh load.

player health is stored in an optional `HEAL` trailer after the existing save data. mob state uses a new saved entity component, and mob visuals use a new drawable type. oldworld builds predating these additions cannot read the new mob component.

protocol version 2 adds `PlayerHealth` and `PlayerAttack`; incompatible clients are rejected. player health is sent at connection, after food consumption, and after hostile attacks.
