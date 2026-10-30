# Lua API: C++ classes in Lua

**Note:** This page is currently really basic but it will be extended later.

## Block

- `u16 id()`
- `string string_id()`
- `string mod_name()`
- `BlockParam param()`
- `BlockState get_state()`

## BlockData

- `Inventory *inventory`
- `BlockMetadata *meta`

## BlockMetadata

- `string get_string(string attribute)`
- `void set_string(string attribute, string value)`
- `int get_int(string attribute)`
- `void set_int(string attribute, int value)`
- `bool get_bool(string attribute)`
- `void set_bool(string attribute, bool value)`

## BlockParam

- `u16 get_param(BlockParamType type, u16 block_param)`
- `u16 set_param(BlockParamType type, u16 block_param, u16 param_value)`
- `bool has_param(BlockParamType type)`

## BlockParamType

- `BlockParamType.Rotation`
- `BlockParamType.State`
- `BlockParamType.Count`

## BlockState

- `u16 id()`
- `std::string label()`
- `ItemStack get_item_drop([Item tool])` applies normal loot rules; omit the tool for environmental breaks.

## Chunk

- `u16 get_block(int x, int y, int z)`
- `void set_block(int x, int y, int z, u16 block)`
- `u16 get_data(int x, int y, int z)`
- `void set_data(int x, int y, int z, u16 data)`
- `BlockData *get_block_data(int x, int y, int z)`
- `BlockData *add_block_data(int x, int y, int z, int inventoryWidth, int inventoryHeight)`
- `BlockState *get_block_state(int x, int y, int z)`
- `void set_block_state(int x, int y, int z, u16 stateID)`

## ClientInfo

- `u16 id`
- `u16 screen_width`
- `u16 screen_height`
- `u8 gui_scale`

## Dimension

- `u16 id()`

## EntityWrapper

- `entt::entity id()`
- `PositionComponent *position()`
- `NetworkComponent *network()`
- `ItemStack *item_stack()`

## Inventory

- `ItemStack add_stack(string name, u16 amount)`
- `ItemStack get_stack(u16 x, u16 y)`
- `void set_stack(u16 x, u16 y, string name, u16 amount)`

## Item

- `u16 id()`
- `string name()`
- `bool has_group(string groupName)`
- `u16 get_group_value(string groupName)`

## ItemStack

- `u16 amount()`
- `Item item()`

## ivec3

- `int x`
- `int y`
- `int z`

## Player

- `string name()`
- `double x()`
- `double y()`
- `double z()`
- `void set_position(double x, double y, double z)`
- `u16 dimension()`
- `void set_dimension(u16 dimension)`
- `u16 client_id()`
- `Inventory *inventory()`
- `ItemStack held_item_stack()`

## Recipe

- `string type()`
- `ItemStack result()`

## Registry

- `Block get_block(u16 id)`
- `Item get_item(u16 id)`
- `Sky get_sky(u16 id)`
- `Tree get_tree(u16 id)`
- `Biome get_biome(u16 id)`
- `Recipe get_recipe(Inventory crafting_inventory)`
- `Key get_key(u16 id)`
- `Block get_block_from_string(string id)`
- `Item get_item_from_string(string id)`
- `Sky get_sky_from_string(string id)`
- `Tree get_tree_from_string(string id)`
- `Biome get_biome_from_string(string id)`
- `Block[] blocks()`
- `Item[] items()`
- `Tree[] trees()`
- `Biome[] biomes()`
- `Dimension[] dimensions()`

## ServerCommandHandler

- `void send_player_change_dimension(u16 clientID, int x, int y, int z, u16 dimension, ClientInfo client)`

## ServerModLoader

- `LuaMod *register_mod(string mod_id)` (see [Lua Mod API](lua-api-mod.md#example))

## ServerPlayer

- `const ClientInfo &client()`
- `bool is_online()`

## ServerWorld

- `u16 dimension()`

## World

- `u16 get_block(int x, int y, int z)`
- `void set_block(int x, int y, int z, u16 block)`
- `u16 get_data(int x, int y, int z)`
- `void set_data(int x, int y, int z, u16 data)`
- `BlockData *add_block_data(int x, int y, int z, int inventoryWidth, int inventoryHeight)`
- `BlockData *get_block_data(int x, int y, int z)`
- `const Block &get_block_def(int x, int y, int z)`
- `void set_block_from_str(int x, int y, int z, string block_id)`
- `const BlockState *get_block_state(int x, int y, int z)`
- `void set_block_state(int x, int y, int z, u16 stateID)`


### custom block params

`BlockParamType.Custom` accesses the bits allocated by `custom_param_bits`.
rotation, state, and custom allocations share 16 bits; overflow rejects the block definition.

### player health

`Player:health()` returns health out of 20. `Player:heal(amount)` and `Player:damage(amount)` clamp health to its valid range. after changing health from lua, call `ServerCommandHandler:send_player_health(player:client_id())` to update the hud.

`BlockState:can_harvest(item)` applies beta tool tiers. `BlockState:block()` returns its block definition. `ItemStack:set_amount(amount)` updates a stack, including partial world-item pickups. `ItemStack:damage()` returns equipment wear. `ItemStack:wear(amount)` applies wear and removes one item when its maximum damage is exceeded; it returns whether wear was applied. item groups `om_max_damage`, `om_tool_type` (sword 1, ordinary tool 2, hoe 3, shears 4), and `om_tool_kind` (axe 1, pickaxe 2, shovel 3) define the default equipment rules.
