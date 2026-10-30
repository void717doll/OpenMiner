#ifndef DUNGEONTERRAIN_HPP_
#define DUNGEONTERRAIN_HPP_

#include "common/core/IntTypes.hpp"

class Heightmap;
class ServerChunk;

void decorateDungeonTerrain(ServerChunk &chunk, Heightmap &heightmap, s32 seed);

#endif
