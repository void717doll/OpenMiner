# retromcp port

source: the supplied `~/retromcp` checkout configured for beta 1.7.3. server java supplies simulation and client java supplies geometry. oldworld retains c++17, lua, bgfx, registries, and separate rendering/simulation threads.

| source | implementation |
| --- | --- |
| `NoiseGeneratorPerlin`, `NoiseGeneratorOctaves`, `ChunkProviderGenerate` | java random, bulk octave noise, density, interpolation, surface replacement, sand, gravel, soil, sandstone, ice, and bedrock |
| `NoiseGenerator2`, `NoiseGeneratorOctaves2`, `WorldChunkManager`, `BiomeGenBase` | temperature, humidity, climate adjustment, and all beta overworld biome classifications |
| `MapGenBase`, `MapGenCaves`, `MapGenCavesHell`, `ChunkProviderHell` | eight-chunk cave replay, rooms, branching, water avoidance, low lava, grass restoration, and nether density/surface/caves |
| `EntityLiving` and beta mob subclasses | complete registered roster, distinctive simulation, interactions, health, timers, and drops |
| `ModelRenderer`, `TexturedQuad`, beta mob models | cuboids, source UVs, mirrored limbs, overlays, scaled slimes/giants, and projectile models |
| supplied refi directory | built-in block, item, mob, sky, breaking, and heart artwork; [asset inventory and license](refi-assets.md) |

## terrain verification and limits

source vertical y maps to oldworld z. the generator builds canonical 16×16×128 columns before slicing into the engine's 16×16×32 chunks. source initialization order, integer overflow, sine lookup, and noise caching are retained.

comparisons against java compiled from the supplied checkout passed:

- 2,304 climate/biome samples matched exactly.
- 294,912 overworld surface bytes matched exactly before caves.
- 1,966,080 cave output bytes matched across four seeds, three coordinates, and five input patterns.
- 524,288 nether surface bytes and 524,288 nether carved bytes matched, including a 64-bit seed.
- the earlier 1,350 octave samples matched exactly; 3,825 density samples differed by at most 1.14e-13.

the parity claim covers base climate, density, surface, and cave generation. tree/flora/ore population, cross-chunk feature scheduling, portals, and nether fire/glowstone/mushroom/spring population still use oldworld behavior or remain absent. below zero terrain continues as solid material; above the source region it is air. existing saved chunks remain unchanged; newly explored terrain can meet older terrain at seams. parallel generation, superchunks, and a public terrain-formula api remain unfinished.

## mobs and controls

the engine registers `pig`, `zombie`, `cow`, `sheep`, `chicken`, `skeleton`, `creeper`, `spider`, `slime`, `ghast`, `pig_zombie`, `wolf`, `squid`, and mod-spawned `giant`. all spawns respect the 64-mob cap and loaded space. skeleton arrows and ghast fireballs are separate visible entities.

left click attacks a visible target within three blocks. right click milks cows with an empty bucket, shears sheep, saddles pigs, and tames/heals/orders wolves. chickens lay eggs, spiders climb and leap, slimes hop and split, pig zombies retaliate together, squid swim, and creepers use a 30-tick fuse. wolves preserve their owner and sitting state. fireballs can be reflected by attacks. food heals immediately, including at full health; stew returns a bowl. old oversized stew stacks retain unconsumed portions.

movement uses direct pursuit, obstacle jumping, and the shared swept collision solver. beta's full pathfinder, mob vocalizations, armor, riding controls, wolf sheep hunting, and lightning-charged creepers remain absent. natural spawn scheduling uses oldworld's bounded periodic attempts rather than the complete beta spawning algorithm. explosion rays use explicit resistance for common materials and approximate the remainder from hardness. sheep colors are rendered, but wool drops currently use the single white wool item.

model cuboids and UVs follow the supplied client source. limb motion derives from client displacement; separate head rotation, attack swings, wing timing, wolf shaking, slime squishing, ghast charge interpolation, and squid pitch are adapted or absent. refi lacks distinct charging-ghast artwork; that pose uses the normal atlas. these are not claims of identical animation.

## saves and local messages

player health uses the optional `HEAL` save trailer; older saves default to full health. original mob and drawable records retain their byte layouts. new `BetaMob` component 9 stores subtype, size, fuse, anger, egg/jump/attack timers, ownership, color, saddle, and motion state separately. older engines cannot read the new component.

the game is local only. serialized messages connect rendering to the simulation through in-process queues; no socket transport, peer handshake version, or dedicated executable remains. health, attacks, interactions, and entity hitboxes use local messages. drawable type 3 carries scale/appearance while type 2 keeps its existing save layout. gameplay effects remain simulation-owned.

## first-person wield

the first-person hand and selected block/item use a separate 72-degree camera and depth-cleared pass between the scene effect and hud, following the rendering boundaries in `~/lunarchy/src/client/camera.cpp` and `wieldmesh.cpp`. empty slots show the refi player arm; blocks reuse the existing cube mesh and items gain thickness along their opaque pixel edges. plantlike blocks and blocks with an inventory image use that sprite rather than a textured box. partial block offsets scale with the mesh.

held blocks and sprites now use lunarchy's normal right-hand position `(55, -35, 65)` and rotation `(-100, 120, -100)`, with wield units divided by 100 and forward z reflected for bgfx. block width is 0.3; sprites are 0.4 wide and 0.04 thick, matching lunarchy's 30/40 mesh scales and extrusion ratio. the optional mineclonia tool enlargement and saved left-hand settings are not applied. narrow windows move the pose inward.

the 286ms swing follows lunarchy's position curves and quaternion interpolation toward `(80, 30, 100)`. walking adds its horizontal and vertical bob, while switching lowers and raises items over 125ms each. block meshes retain the z-up conversion and empty slots retain the existing refi-skinned arm. arm inertia, tool recharge recoil, and custom mesh wield models remain absent.

inventory tooltips follow beta `GuiContainer.drawScreen`: only the trimmed item name, white text with a dark shadow, a solid 75-percent-opaque black rectangle, three pixels of padding, and a twelve-pixel offset above/right of the cursor. tooltips hide while carrying a stack and move inward near window edges. dropped plants and seeds use their item sprite instead of a thin crop-shaped box.

## survival rules

flight, noclip, sprint, creative inventory, starter gifts, free stack cloning and cheat commands are removed. player movement now follows beta `EntityLiving`, `Entity`, and `MovementInputFromOptions`, including fixed physics ticks, momentum, friction, gravity, swimming, ladders, stepping, and sneaking. server position checks use the same axis ordering. the [survival inventory](beta-survival.md) records equipment durability, mining rules, loot behavior, save compatibility, verification, and remaining differences.

fall, drowning, lava/burn, cactus and suffocation damage retain beta hurt immunity. food heals immediately. death drops inventory with equipment wear preserved and respawns automatically. common block hardness, tool speeds, effective-block lists, harvest tiers and weapon damage follow the supplied beta source. loot falls to the ground and partial pickups retain the remainder. water/lava, mobs, falling blocks and dropped-item movement retain the [lunarchy world physics port](lunarchy-physics.md).

water/lava now use queued propagation and level parameters; unsupported sand/gravel fall as saved entities. these systems follow lunarchy rather than the beta generator reference.
