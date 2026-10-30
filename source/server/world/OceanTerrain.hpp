#ifndef OCEANTERRAIN_HPP_
#define OCEANTERRAIN_HPP_
#include "common/core/IntTypes.hpp"
class ServerChunk;
class Heightmap;
void decorateOceanTerrain(ServerChunk &chunk, Heightmap &heightmap, s32 seed);
void decorateNetherRuins(ServerChunk &chunk, Heightmap &heightmap, s32 seed);
#endif
