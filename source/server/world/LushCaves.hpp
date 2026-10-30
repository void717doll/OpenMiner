#ifndef LUSHCAVES_HPP_
#define LUSHCAVES_HPP_

#include "common/core/IntTypes.hpp"

class Heightmap;
class ServerChunk;

void decorateLushCaves(ServerChunk &chunk, Heightmap &heightmap, s32 seed);

#endif // LUSHCAVES_HPP_
