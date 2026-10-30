-- keep the original cat disc and its saved track number; wait uses the same source track.
for _, record in ipairs({
	{id = "wait", track = 2, title = "The Energetic Rat (Jordach's Mix)"},
	{id = "blocks", track = 3, title = "Eastern Feeling"},
	{id = "far", track = 4, title = "Minetest"},
	{id = "chirp", track = 5, title = "Soaring over the sea"},
	{id = "strad", track = 6, title = "Winter Feeling"},
	{id = "mellohi", track = 7, title = "Synthgroove (Jordach's Mix)"},
	{id = "mall", track = 8, title = "The Clueless Frog (Jordach's Mix)"},
}) do
	mod:item {
		id = "record_" .. record.id, name = "Music Disc - " .. record.title,
		tiles = "refi_record_" .. record.id .. ".png", max_stack_size = 1,
		groups = {om_record = record.track},
	}
end
