# lunarchy physics port

player movement has since been replaced by the [beta survival rules](beta-survival.md). the player sections below record the previous adaptation; mobs, dropped items, falling blocks, and fluids still use the world physics described here.

reference: the supplied `~/lunarchy` checkout. copied algorithms retain their luanti lgpl-2.1-or-later notices. oldworld keeps its c++17, bgfx, registry, block parameters, z-up coordinates, and local simulation thread.

| source | runtime implementation |
| --- | --- |
| `src/collision.cpp`: `axisAlignedCollision`, `collisionMoveSimple` | earliest-contact swept aabbs, sliding, shallow overlap recovery, ground/support reporting, step/ceiling checks, bouncy and slippery groups, solid unloaded boundaries, and object collision collection |
| `src/client/localplayer.cpp`, `clientenvironment.cpp`, `player.cpp`, `defaultsettings.cpp` | time-based acceleration and braking, walking/crouching/climbing, jumping, gravity, liquid resistance/hysteresis, retained sneak support, bounce controls and small movement substeps |
| `builtin/game/falling.lua` | unsupported sand/gravel become visible falling entities, settle on support, retain block parameters and metadata, and drop items when placement fails; attached nodes recheck their support |
| `src/servermap.cpp`: `transformLiquids` | queued water/lava updates, flowing level/downward parameters, source withdrawal, renewable water, viscosity, contact conversion and loaded-boundary wakeups |
| `builtin/game/item_entity.lua` | gravity/collision, delayed pickup, compatible-stack merging, expiry, slippery-floor drag and escape from solid cells |

## coordinates and material data

source y-up becomes z-up and `BS=10` becomes one engine block. velocity is blocks per second and acceleration is blocks per second squared. the player uses a 0.6×0.6×1.77 body and eye height 1.625. ordinary controls use walk speed 4, crouch speed 1.35, climb speed 3, jump speed 6.5, and gravity 19.62. movement substeps cap time at 10ms and travel at 0.1 block.

existing block groups carry `slippery`, `bouncy`, `climbable`, `disable_jump`, `disable_descend`, `move_resistance`, `falling_node`, and `attached_node`. ice has slippery=3. removed fly/noclip/privilege-bypass/freecam behavior is not exposed.

source water/lava ids remain stable. appended flowing variants reserve four custom parameter bits: levels 0 weak through 7 full and bit 3 for downward flow. rendering and environmental contact use those levels. block changes enqueue their neighborhood after mutation. chunk loading seeds relevant physics nodes and fluid frontiers once; enclosed ocean volumes do not fill the queue. each tick processes a bounded snapshot, with no terrain generation for missing neighbors.

saved component 10 preserves item velocity, age, pickup delay and escape state. component 11 preserves falling block id, parameters, velocity, origin and optional inventory/metadata. existing component layouts remain unchanged; older executables cannot read these new components.

## validation and limits

focused tests cover thin walls, corners, sliding, floors/ceilings, step clearance, bounce/friction, object support, unloaded boundaries, flow withdrawal/renewal/viscosity/contact, chunk seams, queue budgets, enclosed ocean queues, falling metadata/save/settling, item movement/merging and save record boundaries. an extracted source collision oracle compared 100,000 cases; double precision changes a few recovery-cutoff/time rounding cases, so exact float parity is not claimed.

this adapts the systems available in oldworld. each block still has one collision aabb; arbitrary multi-box node geometry, leveled/float-on-liquid node merging, source lua flood/protection/rollback callback contracts, weather/fire spread and arbitrary new liquid families require additional engine support. climbing/bounce groups work, but the default content currently has no ladder or bouncy block. mesh/entity attachment systems and lunarchy's client cheats are outside this port.
