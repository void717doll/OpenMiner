#ifndef EXPANSIONTERRAIN_HPP_
#define EXPANSIONTERRAIN_HPP_

#include "common/core/IntTypes.hpp"

class Heightmap;
class ServerChunk;

void decorateExpansionTerrain(ServerChunk &chunk, Heightmap &heightmap, u16 dimension, s32 seed);

#endif // EXPANSIONTERRAIN_HPP_
