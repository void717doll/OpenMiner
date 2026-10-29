/*
 * =====================================================================================
 *
 *  OpenMiner
 *
 *  Copyright (C) 2018-2020 Unarelith, Quentin Bazin <openminer@unarelith.net>
 *  Copyright (C) 2019-2020 the OpenMiner contributors (see CONTRIBUTORS.md)
 *
 *  This file is part of OpenMiner.
 *
 *  OpenMiner is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  OpenMiner is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenMiner; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * =====================================================================================
 */
#include "common/core/GameTime.hpp"
#include "common/core/Registry.hpp"
#include "common/core/Utils.hpp"
#include "common/scene/component/AnimationComponent.hpp"
#include "common/scene/component/DrawableDef.hpp"
#include "common/scene/component/NetworkComponent.hpp"
#include "common/world/BlockData.hpp"
#include "server/core/PlayerList.hpp"
#include "server/lua/ScriptEngine.hpp"
#include "server/network/Server.hpp"
#include "server/network/ServerCommandHandler.hpp"
#include "common/world/WorldCollision.hpp"
#include "common/world/BetaToolRules.hpp"
#include "common/world/RetroMobSpawn.hpp"
#include "common/core/GameClock.hpp"
#include "common/scene/component/MobComponent.hpp"
#include "server/world/ServerBlock.hpp"
#include "server/world/ServerItem.hpp"
#include "server/world/WorldController.hpp"

namespace {
bool canReachBlock(const ServerWorld &world, const ServerPlayer &player, int x, int y, int z) {
	const Chunk *chunk = world.getChunkAtBlockPos(x, y, z);
	if (!player.isReady() || !chunk || !chunk->isInitialized()) return false;
	Vector3d eye{player.x(), player.y(), player.z() + 1.625};
	Vector3d target{std::clamp(eye.x, double(x), double(x + 1)),
		std::clamp(eye.y, double(y), double(y + 1)), std::clamp(eye.z, double(z), double(z + 1))};
	Vector3d delta = target - eye;
	if (delta.length() > 4.001) return false;
	int steps = std::max(1, int(std::ceil(delta.length() * 16)));
	for (int step = 1; step < steps; ++step) {
		Vector3d point = eye + delta * (double(step) / steps);
		int bx = int(std::floor(point.x)), by = int(std::floor(point.y)), bz = int(std::floor(point.z));
		if (bx == x && by == y && bz == z) continue;
		const BlockState *state = world.getBlockState(bx, by, bz);
		if (!state) return false;
		if (state->block().id() && state->isCollidable()) {
			DoubleBox box = collision::blockBox(*state, world.getData(bx, by, bz), bx, by, bz);
			if (point.x > box.x && point.x < box.x + box.sizeX && point.y > box.y && point.y < box.y + box.sizeY
			 && point.z > box.z && point.z < box.z + box.sizeZ) return false;
		}
	}
	return true;
}
}

void ServerCommandHandler::wearHeldItem(Player &player, u16 amount) {
	int slot = player.heldItemSlot();
	if (slot < 0 || slot >= player.inventory().width()) return;
	auto &stack = player.inventory().getStackRef(u16(slot), 0);
	u16 previous = stack.amount();
	if (!stack.wear(amount)) return;
	sendPlayerInvUpdate(player.clientID());
	if (stack.amount() < previous) {
		Network::Packet packet;
		packet << Network::Command::PlayerToolBroken;
		m_server.sendToAllClients(packet);
	}
}

void ServerCommandHandler::sendServerClosed(const std::string &message, ClientInfo *client) const {
	Network::Packet packet;
	packet << Network::Command::ServerClosed << message;

	if (!client) {
		m_server.sendToAllClients(packet);
		m_server.disconnectAllClients();
	}
	else {
		client->send(packet);
		m_server.disconnectClient(*client);
	}
}

void ServerCommandHandler::sendChunkUnload(s32 chunkX, s32 chunkY, s32 chunkZ, const ClientInfo *client) const {
	Network::Packet packet;
	packet << Network::Command::ChunkUnload << chunkX << chunkY << chunkZ;

	if (!client)
		m_server.sendToAllClients(packet);
	else
		client->send(packet);
}

void ServerCommandHandler::sendBlockDataUpdate(s32 x, s32 y, s32 z, const BlockData *blockData, const ClientInfo *client) const {
	Network::Packet packet;
	packet << Network::Command::BlockDataUpdate << x << y << z << blockData->meta;

	if (!client)
		m_server.sendToAllClients(packet);
	else
		client->send(packet);
}

void ServerCommandHandler::sendSound(const std::string &name, const Vector3d &position, u16 dimension, float gain, float pitch, const ClientInfo *client) const {
	Network::Packet packet;
	packet << Network::Command::WorldSound << name << position.x << position.y << position.z << dimension << gain << pitch;
	if (client) client->send(packet);
	else m_server.sendToAllClients(packet);
}

void ServerCommandHandler::sendBlockInvUpdate(s32 x, s32 y, s32 z, const Inventory &inventory, const ClientInfo *client) const {
	Network::Packet packet;
	packet << Network::Command::BlockInvUpdate << x << y << z << inventory;

	if (!client)
		m_server.sendToAllClients(packet);
	else
		client->send(packet);
}

void ServerCommandHandler::sendPlayerPosUpdate(u16 clientID, bool isTeleportation, const ClientInfo *client) const {
	const ServerPlayer *player = m_players.getPlayerFromClientID(clientID);
	if (player) {
		Network::Packet packet;
		packet << Network::Command::PlayerPosUpdate;
		packet << clientID;
		packet << player->x() << player->y() << player->z();
		packet << isTeleportation;

		if (!client)
			m_server.sendToAllClients(packet);
		else
			client->send(packet);
	}
	else
		logError() << ("Failed to send position update for player " + std::to_string(clientID) + ": Player not found").c_str();
}

void ServerCommandHandler::sendPlayerInvUpdate(u16 clientID, const ClientInfo *client) const {
	ServerPlayer *player = m_players.getPlayerFromClientID(clientID);
	if (player) {
		Network::Packet packet;
		packet << Network::Command::PlayerInvUpdate;
		packet << clientID << player->inventory() << player->heldItemSlot();

		if (!client)
			m_server.sendToAllClients(packet);
		else
			client->send(packet);
	}
	else
		logError() << ("Failed to send inventory update for player " + std::to_string(clientID) + ": Player not found").c_str();
}

void ServerCommandHandler::sendPlayerChangeDimension(u16 clientID, s32 x, s32 y, s32 z, u16 dimension, const ClientInfo *client) const {
	ServerPlayer *player = m_players.getPlayerFromClientID(clientID);
	if (player) {
		Network::Packet packet;
		packet << Network::Command::PlayerChangeDimension;
		packet << clientID << x << y << z << dimension;

		if (client) {
			client->send(packet);

			// FIXME: sendPlayerChangeDimension shouldn't be exposed to Lua
			//        Instead, there should be a changePlayerDimension function that sends
			//        the packet above + the entities (instead of doing that here)
			//        also, it should change player dimension and position accordingly
			m_worldController.getWorld(dimension).scene().sendEntities(*client);

			player->setPosition(x + 0.5, y + 0.5, z + 0.5);
			player->setDimension(dimension);
			player->resetSurvival();
			player->clearLoadedChunks();
		}
		else
			m_server.sendToAllClients(packet);
	}
	else
		logError() << ("Failed to send dimension change for player " + std::to_string(clientID) + ": Player not found").c_str();
}

void ServerCommandHandler::sendEntitySpawn(entt::entity entityID, const ClientInfo *client) const {
	Network::Packet packet;
	packet << Network::Command::EntitySpawn << entityID;

	if (!client)
		m_server.sendToAllClients(packet);
	else
		client->send(packet);
}

void ServerCommandHandler::sendEntityDespawn(entt::entity entityID, const ClientInfo *client) const {
	Network::Packet packet;
	packet << Network::Command::EntityDespawn << entityID;

	if (!client)
		m_server.sendToAllClients(packet);
	else
		client->send(packet);
}

void ServerCommandHandler::sendPlayerHealth(u16 clientID) const {
	ServerPlayer *player = m_players.getPlayerFromClientID(clientID);
	if (!player || !player->client()) return;
	Network::Packet packet;
	packet << Network::Command::PlayerHealth << player->health();
	player->client()->send(packet);
}

void ServerCommandHandler::damagePlayer(u16 clientID, u16 amount) {
	ServerPlayer *player = m_players.getPlayerFromClientID(clientID);
	if (!player) return;
	amount = player->survival().filterDamage(amount);
	if (!amount) return;
	player->damage(amount);
	Network::Packet hurt;
	hurt << Network::Command::PlayerHurt;
	if (player->client()) player->client()->send(hurt);
	if (player->health() == 0) {
		ServerWorld &world = getWorldForClient(clientID);
		world.dropPlayerItems(*player);
		if (player->dimension() == 1) {
			bool found = false;
			int originX = int(std::floor(player->x())), originY = int(std::floor(player->y()));
			for (int radius = 0; radius <= 8 && !found; ++radius)
				for (int dx = -radius; dx <= radius && !found; ++dx)
					for (int dy = -radius; dy <= radius && !found; ++dy) {
						if (std::max(std::abs(dx), std::abs(dy)) != radius) continue;
						double x = originX + dx + 0.5, y = originY + dy + 0.5;
						int z = retro::mobCavityFloor(world, player->hitbox(), x, y, int(std::floor(player->z())));
						if (z < 0) continue;
						player->setPosition(x, y, z);
						found = true;
					}
			if (!found) {
				ServerWorld &overworld = m_worldController.getWorld(0);
				int z = overworld.heightmap().getHighestBlockAt(0, 0) + 1;
				sendPlayerChangeDimension(clientID, 0, 0, z, 0, player->client());
				sendPlayerChangeDimension(clientID, 0, 0, z, 0);
			}
		} else {
			setPlayerPosition(clientID, 0, 0, world.heightmap().getHighestBlockAt(0, 0) + 1);
		}
		player->setHealth(20);
		player->resetSurvival();
		sendPlayerPosUpdate(clientID, true);
	}
	sendPlayerHealth(clientID);
}

void ServerCommandHandler::setupCallbacks() {
	m_server.setConnectionCallback([this](ClientInfo &client, Network::Packet &connectionPacket) {
		std::string username;
		connectionPacket >> username;
		if (!connectionPacket) {
			sendServerClosed("unable to load the local player", &client);
			return;
		}

		ServerPlayer *player = m_players.connectPlayer(username, client);
		if (!player) {
			sendServerClosed("unable to load the local player", &client);
			return;
		}

		// Try to find a valid spawn point (WIP)
		if (player->isNewPlayer()) {
			// FIXME: Default dimension hardcoded here
			ServerWorld &world = m_worldController.getWorld(0);
			Heightmap &heightmap = world.heightmap();

			bool hasFoundPosition = false;
			for (s32 spawnChunkX = 0 ; spawnChunkX < 16 && !hasFoundPosition ; ++spawnChunkX) {
				for (s32 spawnChunkY = 0 ; spawnChunkY < 16 && !hasFoundPosition ; ++spawnChunkY) {
					for(int y = 0 ; y < CHUNK_DEPTH && !hasFoundPosition ; y++) {
						for(int x = 0 ; x < CHUNK_WIDTH ; x++) {
							int maxChunkZ = heightmap.getHighestChunkAt(x + spawnChunkX * CHUNK_WIDTH, y + spawnChunkY * CHUNK_DEPTH);
							int worldZ = heightmap.getHighestBlockAt(x + spawnChunkX * CHUNK_WIDTH, y + spawnChunkY * CHUNK_DEPTH) + 1;
							int z = math::pmod(worldZ, CHUNK_WIDTH);

							world.generateChunk(world.getOrCreateChunk(spawnChunkX - 1, spawnChunkY,     maxChunkZ));
							world.generateChunk(world.getOrCreateChunk(spawnChunkX + 1, spawnChunkY,     maxChunkZ));
							world.generateChunk(world.getOrCreateChunk(spawnChunkX,     spawnChunkY - 1, maxChunkZ));
							world.generateChunk(world.getOrCreateChunk(spawnChunkX,     spawnChunkY + 1, maxChunkZ));
							world.generateChunk(world.getOrCreateChunk(spawnChunkX,     spawnChunkY,     maxChunkZ - 1));
							world.generateChunk(world.getOrCreateChunk(spawnChunkX,     spawnChunkY,     maxChunkZ + 1));

							ServerChunk &chunk = world.getOrCreateChunk(spawnChunkX, spawnChunkY, maxChunkZ);
							world.generateChunk(chunk);

							const BlockState *blockBelow = chunk.getBlockState(x, y, z - 1);
							const Block &blockFeet = m_registry.getBlock(chunk.getBlock(x, y, z));
							const Block &blockHead = m_registry.getBlock(chunk.getBlock(x, y, z + 1));

							if (blockFeet.id() == 0 && blockHead.id() == 0
							&& blockBelow && blockBelow->isCollidable()
							&& blockBelow->drawType() != BlockDrawType::Leaves)
							{
								player->setPosition(x + spawnChunkX * CHUNK_WIDTH + .5, y + spawnChunkY * CHUNK_DEPTH + .5, worldZ + .2);
								hasFoundPosition = true;
								break;
							}
						}
					}
				}
			}

			if (!hasFoundPosition)
				logError() << "Can't find a good position for the player";

			player->setHeldItemSlot(0);
		}

		// Send the registry
		Network::Packet packet;
		packet << Network::Command::RegistryData;
		client.send(packet);


		// Triggers the 'PlayerConnected' Lua event
		if (player->isNewPlayer())
			m_scriptEngine.luaCore().onEvent(LuaEventType::PlayerConnected, glm::ivec3{player->x(), player->y(), player->z()}, player, client, *this);

		// Send inventory
		sendPlayerInvUpdate(client.id, &client);
		sendPlayerHealth(client.id);

		// send the local player after connection callbacks have run.
		Network::Packet spawnPacket;
		spawnPacket << Network::Command::PlayerSpawn << client.id;
		spawnPacket << player->x() << player->y() << player->z() << player->dimension() << player->name();
		spawnPacket << player->cameraYaw() << player->cameraPitch();
		client.send(spawnPacket);

		// Send entities to the client
		m_worldController.getWorld(player->dimension()).scene().sendEntities(client);
	});

	m_server.setCommandCallback(Network::Command::ClientDisconnect, [this](ClientInfo &client, Network::Packet &) {
		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player) {
			player->clearLoadedChunks();
			m_players.disconnectPlayer(client.playerName);
		}
	});

	m_server.setCommandCallback(Network::Command::ChunkUnload, [this](ClientInfo &client, Network::Packet &packet) {
		s32 cx, cy, cz;
		packet >> cx >> cy >> cz;

		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player) {
			player->removeLoadedChunk(Vector3i{cx, cy, cz});
		}
		else
			logError() << ("Failed to unload chunk for player " + std::to_string(client.id) + ": Player not found").c_str();
	});

	m_server.setCommandCallback(Network::Command::PlayerInvUpdate, [this](ClientInfo &client, Network::Packet &packet) {
		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player) {
			packet >> player->inventory();
		}
		else
			logError() << ("Failed to update inventory of player " + std::to_string(client.id) + ": Player not found").c_str();
	});

	m_server.setCommandCallback(Network::Command::PlayerPosUpdate, [this](ClientInfo &client, Network::Packet &packet) {
		double x, y, z;
		packet >> x >> y >> z;

		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player && packet) {
			if (!getWorldForClient(client.id).moveSurvivalPlayer(*player, x, y, z))
				sendPlayerPosUpdate(client.id, true, &client);
		}
		else
			logError() << ("Failed to update position of player " + std::to_string(client.id) + ": Player not found").c_str();
	});

	m_server.setCommandCallback(Network::Command::PlayerRotUpdate, [this](ClientInfo &client, Network::Packet &packet) {
		float yaw, pitch;
		packet >> yaw >> pitch;

		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player) {
			player->setRotation(yaw, pitch);
		}
		else
			logError() << ("Failed to update rotation of player " + std::to_string(client.id) + ": Player not found").c_str();
	});

	m_server.setCommandCallback(Network::Command::PlayerPlaceBlock, [this](ClientInfo &client, Network::Packet &packet) {
		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player) {
			s32 x, y, z;
			u32 block;
			packet >> x >> y >> z >> block;
			if (!packet) return;

			ServerWorld &world = getWorldForClient(client.id);
			const BlockState *previous = world.getBlockState(x, y, z);
			u16 id = u16(block & 0xffff);
			const auto &blocks = Registry::getInstance().blocks();
			ItemStack stack;
			if (player->heldItemSlot() >= 0 && player->heldItemSlot() < player->inventory().width())
				stack = player->heldItemStack();
			bool valid = packet && previous && canReachBlock(world, *player, x, y, z) && id > 0 && id < blocks.size()
				&& stack.amount() > 0 && stack.item().isBlock() && stack.item().id() == id;
			if (valid) {
				const Block &definition = *blocks[id];
				u16 data = u16(block >> 16);
				u16 stateID = definition.param().hasParam(BlockParam::State)
					? definition.param().getParam(BlockParam::State, data) : 0;
				valid = stateID < definition.states().size();
				if (valid) {
					const BlockState &state = definition.getState(stateID);
					DoubleBox playerBox = player->hitbox();
					playerBox.move(player->x(), player->y(), player->z());
					valid = !state.isCollidable() || !collision::blockBox(state, data, x, y, z).intersects(playerBox);
				}
				valid = valid && (!previous->block().id() || previous->drawType() == BlockDrawType::Liquid
					|| previous->drawType() == BlockDrawType::XShape)
					&& definition.placementConstraints().check(world, {x, y, z});
			}
			if (!valid) {
				Network::Packet correction;
				correction << Network::Command::BlockUpdate << x << y << z
					<< (u32(world.getBlock(x, y, z)) | (u32(world.getData(x, y, z)) << 16));
				client.send(correction);
				sendPlayerInvUpdate(client.id, &client);
				return;
			}
			player->inventory().setStack(player->heldItemSlot(), 0,
				stack.amount() > 1 ? stack.item().stringID() : "", stack.amount() - 1);
			sendPlayerInvUpdate(client.id);
			world.setData(x, y, z, u16(block >> 16));
			world.setBlock(x, y, z, u16(block & 0xffff));

			const BlockState *blockState = world.getBlockState(x, y, z);
			if (!blockState) return;

			m_scriptEngine.luaCore().onEvent(LuaEventType::BlockPlaced, glm::ivec3{x, y, z}, *blockState, *player, world, client, *this);

			Network::Packet answer;
			answer << Network::Command::BlockUpdate << x << y << z << block;
			m_server.sendToAllClients(answer);
		}
		else
			logError() << ("Failed to place block using player " + std::to_string(client.id) + ": Player not found").c_str();
	});

	m_server.setCommandCallback(Network::Command::PlayerInteract, [this](ClientInfo &client, Network::Packet &) {
		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player && player->isReady()) getWorldForClient(client.id).interactMob(*player);
	});

	m_server.setCommandCallback(Network::Command::PlayerAttack, [this](ClientInfo &client, Network::Packet &) {
		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (!player || !player->tryAttack(GameClock::getInstance().getTicks(true))) return;
		getWorldForClient(client.id).attackMob(*player);
	});

	m_server.setCommandCallback(Network::Command::PlayerDigBlock, [this](ClientInfo &client, Network::Packet &packet) {
		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player) {
			s32 x, y, z;
			packet >> x >> y >> z;

			ServerWorld &world = getWorldForClient(client.id);
			const BlockState *blockState = world.getBlockState(x, y, z);
			if (!packet || !blockState || !canReachBlock(world, *player, x, y, z)
			 || !blockState->block().id() || blockState->drawType() == BlockDrawType::Liquid
			 || !std::isfinite(blockState->hardness()) || blockState->hardness() < 0) return;
			world.setBlock(x, y, z, 0);

			m_scriptEngine.luaCore().onEvent(LuaEventType::BlockDigged, glm::ivec3{x, y, z}, *blockState, *player, world, client, *this);
			if (player->heldItemSlot() >= 0 && player->heldItemSlot() < player->inventory().width()) {
				auto &tool = player->inventory().getStackRef(player->heldItemSlot(), 0);
				wearHeldItem(*player, retro::diggingWear(tool.item(), *blockState));
			}

			Network::Packet answer;
			answer << Network::Command::BlockUpdate << x << y << z << u32(0);
			m_server.sendToAllClients(answer);
		}
		else
			logError() << ("Failed to dig block using player " + std::to_string(client.id) + ": Player not found").c_str();
	});

	m_server.setCommandCallback(Network::Command::PlayerHeldItemChanged, [this](ClientInfo &client, Network::Packet &packet) {
		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player) {
			u8 hotbarSlot;
			u16 itemID;
			packet >> hotbarSlot >> itemID;
			if (!packet || hotbarSlot >= 9 || hotbarSlot >= player->inventory().width()) return;
			if (player->inventory().getStack(hotbarSlot, 0).item().id() != itemID)
				logWarning() << "PlayerHeldItemChanged:" << "Desync of item ID between client and server";

			player->setHeldItemSlot(hotbarSlot);
		}
		else
			logError() << ("Failed to change held item of player " + std::to_string(client.id) + ": Player not found").c_str();
	});

	m_server.setCommandCallback(Network::Command::PlayerDropItem, [this](ClientInfo &client, Network::Packet &) {
		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player) getWorldForClient(client.id).dropHeldItem(*player);
	});

	m_server.setCommandCallback(Network::Command::PlayerReady, [this](ClientInfo &client, Network::Packet &) {
		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player) {
			player->setReady(true);
		}
		else
			logError() << ("Failed to change held item of player " + std::to_string(client.id) + ": Player not found").c_str();
	});

	m_server.setCommandCallback(Network::Command::PlayerChunkPosUpdate, [this](ClientInfo &client, Network::Packet &packet) {
		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player) {
			s32 chunkX, chunkY, chunkZ;
			packet >> chunkX >> chunkY >> chunkZ;

			ServerWorld &world = getWorldForClient(client.id);
			world.updatePlayerChunks(*player, chunkX, chunkY, chunkZ);
		}
		else
			logError() << ("Failed to update chunk position of player " + std::to_string(client.id) + ": Player not found").c_str();
	});

	m_server.setCommandCallback(Network::Command::BlockActivated, [this](ClientInfo &client, Network::Packet &packet) {
		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player) {
			s32 x, y, z;
			packet >> x >> y >> z >> client.screenWidth >> client.screenHeight >> client.guiScale;

			ServerWorld &world = getWorldForClient(client.id);
			if (!packet || !canReachBlock(world, *player, x, y, z)) return;

			u16 id = world.getBlock(x, y, z);
			ServerBlock &block = (ServerBlock &)(m_registry.getBlock(id));
			bool hasBeenActivated = block.onBlockActivated({x, y, z}, *player, world, client, *this);

			if (hasBeenActivated)
				m_scriptEngine.luaCore().onEvent(LuaEventType::BlockActivated, glm::ivec3{x, y, z}, block, *player, world, client, *this);
		}
		else
			logError() << ("Failed to activate block using player '" + client.playerName + "': Player not found").c_str();
	});

	m_server.setCommandCallback(Network::Command::BlockInvUpdate, [this](ClientInfo &client, Network::Packet &packet) {
		Vector3<s32> pos;
		packet >> pos.x >> pos.y >> pos.z;

		BlockData *data = getWorldForClient(client.id).getBlockData(pos.x, pos.y, pos.z);
		if (data)
			packet >> data->inventory;
		else
			logError() << "BlockInvUpdate: No block data found at" << pos.x << pos.y << pos.z;
	});

	m_server.setCommandCallback(Network::Command::BlockDataUpdate, [this](ClientInfo &client, Network::Packet &packet) {
		Vector3<s32> pos;
		packet >> pos.x >> pos.y >> pos.z;

		BlockData *data = getWorldForClient(client.id).getBlockData(pos.x, pos.y, pos.z);
		if (data) {
			packet >> data->meta;
		}
		else
			logError() << "BlockDataUpdate: No block data found at" << pos.x << pos.y << pos.z;
	});

	m_server.setCommandCallback(Network::Command::ItemActivated, [this](ClientInfo &client, Network::Packet &packet) {
		ServerPlayer *player = m_players.getPlayerFromClientID(client.id);
		if (player) {
			s32 x, y, z;
			packet >> x >> y >> z >> client.screenWidth >> client.screenHeight >> client.guiScale;

			ServerWorld &world = getWorldForClient(client.id);
			if (!packet || !canReachBlock(world, *player, x, y, z)) return;

			u16 id = world.getBlock(x, y, z);
			ServerBlock &block = (ServerBlock &)(m_registry.getBlock(id));

			if (player->heldItemSlot() < 0 || player->heldItemSlot() >= player->inventory().width()
			 || player->heldItemStack().amount() == 0 || player->heldItemStack().item().isBlock()) return;
			const ServerItem &item = static_cast<const ServerItem &>(player->heldItemStack().item());
			if (item.canBeActivated()) {
				bool hasBeenActivated = item.onItemActivated({x, y, z}, block, *player, world, client, *this);

				if (hasBeenActivated)
					m_scriptEngine.luaCore().onEvent(LuaEventType::ItemActivated, glm::ivec3{x, y, z}, block, *player, world, client, *this);
			}
		}
		else
			logError() << ("Failed to activate item using player '" + client.playerName + "': Player not found").c_str();
	});

	m_server.setCommandCallback(Network::Command::KeyPressed, [this](ClientInfo &client, Network::Packet &packet) {
		u16 keyID;
		packet >> keyID >> client.screenWidth >> client.screenHeight >> client.guiScale;

		m_registry.getKey(keyID).callback()(keyID, client);
	});
}

void ServerCommandHandler::setPlayerPosition(u16 clientID, s32 x, s32 y, s32 z) {
	ServerPlayer *player = m_players.getPlayerFromClientID(clientID);
	if (player) {
		player->setPosition(x, y, z);
		player->resetSurvival();
	}
	else
		logError() << ("Failed to set position for player " + std::to_string(clientID) + ": Player not found").c_str();
}

inline ServerWorld &ServerCommandHandler::getWorldForClient(u16 clientID) {
	ServerPlayer *player = m_players.getPlayerFromClientID(clientID);
	if (!player)
		throw EXCEPTION("Player instance not found for client", clientID);

	return m_worldController.getWorld(player->dimension());
}

// Please update 'docs/lua-api-cpp.md' if you change this
void ServerCommandHandler::initUsertype(sol::state &lua) {
	lua.new_usertype<ServerCommandHandler>("ServerCommandHandler",
		"send_player_change_dimension", &ServerCommandHandler::sendPlayerChangeDimension,
		"send_player_health", &ServerCommandHandler::sendPlayerHealth
	);
}
