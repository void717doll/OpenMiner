# Lua API: Image

## Example

```lua
gui:image {
	name = "img_background", -- mandatory
	pos = {x = 0, y = 0},    -- mandatory

	texture = mod:path() .. "/textures/gui/inventory.png",
	size = {x = 176, y = 166},
	border = 5,
}
```

## Attributes

### `size`

optional display size, `{x = width, y = height}`. the source `clip` remains in texture pixels. omitting this keeps the clipped image's original size.

### `border`

optional border width in source pixels. with `size`, a positive border draws the full texture in nine parts, preserving its corners and stretching its edges and center. `clip` is unused for this panel mode.

### `clip`

Clip rect of the texture.

Example:
```lua
clip = {
	x = 0,
	y = 0,
	width = 176,
	height = 166
}
```

### `name`

Name of the widget. **Mandatory field.**

Example:
```lua
name = "img_background"
```

### `pos`

Position of the widget. **Mandatory field.**

Example:
```lua
pos = {
	x = 0,
	y = 0
}
```

### `texture`

Full path of the texture. Relative to the repo root.

Example:
```lua
texture = mod:path() .. "/textures/gui/workbench.png"
```

See [this page](lua-api-mod.md#path) for more details about `mod:path()`.

