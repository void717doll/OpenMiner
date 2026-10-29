# built-in refi artwork

world artwork is imported from the supplied REFI_Textures distribution by MysticTempest, licensed under CC BY-SA 4.0. source: https://github.com/MysticTempest/REFI_Textures. the complete supplied license is retained in [REFI-LICENSE.txt](REFI-LICENSE.txt).

[refi-assets.json](refi-assets.json) records every imported source path, destination, source/output dimensions, operation, and sha256 digest. destinations keep their existing names so saved registry ids and lua definitions remain stable.

coverage: 111 block textures, 110 item textures, 21 mob/projectile textures, and ten engine textures. all 263 png files under the default mod and engine texture directories are accounted for, including the retained ui and debug exceptions below. the engine's magenta missing-texture checker is retained as a 16px debug placeholder rather than relabeled as refi artwork.

## adaptations

- inventory, workbench, and furnace screens share refi's `mcl_base_textures_background9.png` panel and `mcl_formspec_itemslot.png` slots. the original images are copied unchanged and sized at runtime; the crafting arrow is clipped from the supplied workbench sheet. the inventory shows only working storage and crafting slots, with no shield/offhand or decorative armor slots.
- furnace fire uses the supplied foreground/background sprites. the white furnace progress arrow remains an engine ui asset. slot highlights draw behind items and stack counts. hud hearts retain their compact nine-pixel size, with a four-pixel margin below the hotbar.
- the hotbar and selection frame use the supplied refi images directly.
- block and item art is 16px. ore/material aliases use the corresponding refi texture, with no fallback to the removed texturepacks directory.
- grass side overlays are composited over the supplied dirt texture. oak leaves use the supplied desaturated oak variant because the existing engine applies foliage tint.
- zombie and zombie pigman skins use the upper 64x32 section of their supplied 64x64 atlases. slime uses nearest-neighbor reduction from 128x64 to beta's 64x32 coordinates.
- pig saddle, sheep fur, spider eyes, and wolf variants retain their supplied 64x32 atlases. refi supplies one ghast atlas, so `ghast_shooting.png` is an explicit alias of that atlas; shooting animation changes the model rather than inventing a second skin.
- the arrow projectile retains the supplied 64x64 sheet; fireballs retain their 16x16 sprite. projectile models must use those layouts.
- the supplied moon sheet is reduced with nearest-neighbor sampling to preserve the engine's existing 32px phase rectangles. the sun and player skin use the supplied files directly.
- the five vertical breaking-overlay frames are arranged horizontally, with each repeated twice to fit the existing ten-frame progression. the menu background uses the supplied dirt texture.
- full and empty hearts are direct copies of `textures/default_mcl_core/gui_hud/heart.png` and `heart_gone.png`. half hearts use clipping of the full-heart artwork at runtime. the saddle item uses the supplied `mcl_mobitems_saddle.png`.

widgets, font, title screen, recipe, world icon, toast, and the default mod's white furnace progress arrow remain engine ui assets. the six labeled block faces under `mods/default/textures/blocks/debug/` retain their original engine artwork, resized from 32px to 16px with nearest-neighbor sampling to match the block atlas. the manifest lists these debug transformations with their original hashes and dimensions, separately identifies the seven unchanged ui images, and does not attribute either group to refi.

## validation

all 256 manifest entries were checked for readable png data, destination coverage, expected dimensions, and matching source/output digests where a supplied source exists. grass-side composition pixels match the supplied dirt and overlay layers. all block/item images are 16x16. all 219 unique default lua texture paths resolve, including 25 generated tool names, and both sky resource keys resolve through the texture configuration. the retained license matches the supplied file byte for byte. native rendering remains an integration check.

armor uses refi leather, chain, iron, gold and diamond icons and worn atlases. tnt and gravel use `alternate_textures/texture_alts`; ferns, roses and mushrooms use the main pack. imported paths and hashes are recorded in the manifest.
