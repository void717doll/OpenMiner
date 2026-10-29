# beta survival

reference: the supplied beta 1.7.3 sources in `~/retromcp/minecraft/src/net/minecraft/src`. gameplay stays in the existing c++ and lua engine.

## movement

`EntityLiving.moveEntityWithHeading`, `Entity.moveFlying`, `Entity.moveEntity`, and `MovementInputFromOptions` supply the player rules. physics runs at 20 ticks per second, with interpolation for rendering. motion uses blocks per tick internally. walking retains momentum, ground friction depends on slipperiness, ice is slippery, sneaking scales input by 0.3, and jumping applies the source's 0.42 impulse. gravity, airborne acceleration, swimming, lava drag, ladder descent/climbing, half-block steps, and sneak edge protection follow those source paths. the body is 0.6 by 0.6 by 1.8 blocks. vertical source y maps to engine z; movement input is rotated into the engine's horizontal axes.

player collision clips vertical movement before the two horizontal axes. server position checks use that same ordering, including ascending slab steps. mobs, falling blocks, ground items, and fluid propagation retain the existing lunarchy-based simulation. loaded chunks bound all movement.

## mining and equipment

number keys 1 through 9 select hotbar slots; the mouse wheel cycles through them. q drops one item from the selected slot, preserving equipment damage. these bindings can be changed in settings. drops leave the player's eye area in the look direction and wait 40 simulation ticks before pickup, following beta `EntityPlayer.dropCurrentItem`. thrown-item movement still uses the engine's luanti physics, without beta's random scatter. dropping into an unloaded chunk or when the item-entity cap is full keeps the inventory unchanged.

block interaction reach remains four blocks. common beta blocks use the source hardness values. default pickaxes, axes, and shovels use the source effective-block lists, including their omissions: axes do not speed up workbenches, and pickaxes do not speed up obsidian or redstone ore. harvest tiers still determine whether a broken block drops loot. swords mine at 1.5 strength; shears speed up leaves and wool. mining takes five times as long when the player's head is underwater, and another five times as long while airborne. the wrong-tool branch retains the source's hardness-only penalty. continuous mining waits 250 milliseconds after a completed block.

| equipment | maximum damage |
| --- | ---: |
| wooden tools | 59 |
| stone tools | 131 |
| iron tools | 250 |
| golden tools | 32 |
| diamond tools | 1561 |
| shears | 238 |

as in beta `ItemStack.damageItem`, equipment breaks when accumulated damage exceeds its maximum. digging costs one durability for ordinary tools and two for swords. hitting a mob costs two for ordinary tools and one for swords. hoes wear only when tilling succeeds. shears wear on leaves and sheep, and keep the source's wool speed. unused swings and unsuccessful interactions cost nothing. worn equipment shows its remaining durability in the inventory and hotbar.

inventory swaps, splitting, shift transfers, ground-item pickups, and death drops retain damage. worn tools cannot merge with differently worn tools. existing oversized stacks are preserved; incoming items do not shrink them to modern stack limits. old stacks start undamaged.

## loot, health, and saves

nearby items collect within one block of the player body horizontally after their pickup delay. pickup merges compatible stacks across the whole inventory before filling empty slots, including backpack space when the hotbar is full. full inventories leave the drop on the ground; partial pickups keep the remainder.

dropped blocks and items rotate at a fixed 90 degrees per second with a small visual bob. sprites use the same opaque-edge mesh as held items, with enough ground clearance to stay above the floor. rendering interpolates the 20hz physics positions without changing collision or pickup positions.

mob deaths put loot on the ground for pickup, including environmental deaths. sheep shearing also puts wool on the ground. partial pickups keep the remainder. existing health, immediate food healing, container returns, fall/drowning/lava/fire/cactus/suffocation damage, hurt immunity, and automatic respawn remain active. there is no hunger bar.

accepted damage sends a separate hurt event, including fatal damage before automatic respawn. the client plays the hurt cue and briefly tilts the camera; hearts blink between the previous and current amounts after a hit. health synchronization and healing do not trigger hurt sounds. equipment breakage also has its own event, so moving or dropping a tool does not play a break sound.

undamaged stacks retain their original serialized bytes. a damaged stack writes the reserved `@oldworld:stack_damage` marker followed by the item id, amount, and damage; readers accept both forms in player inventories, containers, and item entities. older saves remain readable; older executables cannot read newly saved damaged stacks.

## verification and limits

800 samples from the original java movement methods matched the c++ calculations exactly for cardinal-facing walking, sneaking, diagonal input, ice, repeated jumping, water, lava, and ladders. the oracle uses a flat floor and controlled material contacts; it does not prove parity for complete worlds or arbitrary view angles. regression coverage exercises axis ordering, slab ascent accepted by the server, sneak edges, equipment breakage, legacy oversized stacks, and serialized inventory/stack boundaries.

this is a survival rules port, not complete beta feature parity. beds, a death screen, sapling regrowth, weather, fire spread, liquid currents, the full beta mob pathfinder/spawner, knockback, and air/fire hud indicators remain absent or adapted. crafting and inventory messages retain the existing local-client ownership. mining animation and timing use the engine clock rather than the complete beta controller state machine. stepping camera smoothing and source-exact random drops are not ported. material footsteps and player action effects now use the [luanti sound assets](sound-assets.md). native input and visual checks still require a desktop session.

## item limits

ordinary items and blocks stack to 99; food, tools, and armor stack to one. the old server default of 64 migrates to 99. all food includes cookies, which deliberately differ from beta's eight-cookie stacks.

later food, materials, and block items have been removed using the supplied beta 1.7.3 item list. their recipes are removed too. reserved air slots preserve old numeric block ids, so removing a block does not change surrounding terrain. removed items disappear on loading; oversized surviving stacks split into available inventory slots, with excess dropped nearby.

## armor and tnt

right click an armor piece to equip or swap it, or use the four filtered inventory slots. shift click also equips a matching empty slot. leather, chain, iron, gold and diamond use beta protection, durability scaling, and damage remainder. armor wears when absorbing damage, drops on death with its wear intact, and saves in an optional `ARMR` trailer. the inventory renders the equipped pieces in 3d; an equipped chestplate also covers the empty first-person hand. chain armor has no survival recipe, matching the unavailable fire ingredient in this engine. shields are absent.

tnt is crafted from five gunpowder and four sand. right click it with flint and steel to start the four-second fuse. blasts damage nearby players and mobs, destroy exposed blocks, drop some debris, and ignite nearby tnt with shorter fuses. terrain outside loaded chunks is untouched. the fuse and motion survive saving.
