#ifndef MUSICRECORDS_HPP_
#define MUSICRECORDS_HPP_

#include <array>

struct MusicRecord { const char *file; const char *title; };
inline constexpr std::array<MusicRecord, 8> musicRecords{{
	{"record_13.wav", "SoundHelix - The Evil Sister (Jordach's Mix)"},
	{"record_cat.wav", "SoundHelix - The Energetic Rat (Jordach's Mix)"},
	{"record_blocks.wav", "Jordach - Eastern Feeling"},
	{"record_far.wav", "Jordach - Minetest"},
	{"record_chirp.wav", "mactonite - Soaring over the sea"},
	{"record_strad.wav", "Tom Peter - Winter Feeling"},
	{"record_mellohi.wav", "HeroOfTheWinds - Synthgroove (Jordach's Mix)"},
	{"record_mall.wav", "SoundHelix - The Clueless Frog (Jordach's Mix)"},
}};

#endif
