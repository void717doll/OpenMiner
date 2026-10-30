#ifndef MINESHAFTTERRAIN_HPP_
#define MINESHAFTTERRAIN_HPP_

#include "common/core/IntTypes.hpp"

class Heightmap;
class ServerChunk;
namespace modern { class MineshaftCache; }

void decorateMineshaftTerrain(ServerChunk &chunk, Heightmap &heightmap, s32 seed, modern::MineshaftCache &cache);

#endif
