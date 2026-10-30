#ifndef BUCKETINTERACTION_HPP_
#define BUCKETINTERACTION_HPP_

#include <glm/vec3.hpp>
#include "common/core/IntTypes.hpp"

class Item;
class Player;
class ServerWorld;
class ServerCommandHandler;

bool useBucket(const Item &item, glm::ivec3 position, s8 face, Player &player,
	ServerWorld &world, ServerCommandHandler &server);

#endif
