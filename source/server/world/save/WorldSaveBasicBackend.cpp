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
#include <fstream>

#include <filesystem.hpp>

#include "common/core/Debug.hpp"
#include "common/core/GameTime.hpp"
#include "common/core/Registry.hpp"
#include "common/network/Network.hpp"
#include "common/scene/component/ComponentType.hpp"
#include "common/scene/component/MobComponent.hpp"
#include "common/scene/component/PhysicalComponent.hpp"
#include "common/scene/component/FallingBlockComponent.hpp"
#include "common/scene/component/TntComponent.hpp"
#include "common/scene/component/NetworkComponent.hpp"
#include "server/world/ServerBlock.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/save/WorldSaveBasicBackend.hpp"

namespace fs = ghc::filesystem;

using namespace entt::literals;

void WorldSaveBasicBackend::load(const std::string &name) {
	logInfo() << ("Loading '" + name + "'...").c_str();

	std::ifstream file("saves/" + name + ".dat", std::ofstream::binary);

	// Note: Registry should be saved, otherwise blocks won't match since they're stored using
	//       their integer ID (not string ID), so we need to add a check for that

	if (file.is_open()) {
		file.seekg(0, file.end);
		std::size_t length = file.tellg();
		file.seekg(0, file.beg);

		char *buffer = new char[length];
		file.read(buffer, length);

		Network::Packet save;
		save.append(buffer, length);

		delete[] buffer;

		for (auto &world : m_worldList) {
			std::vector<std::pair<Vector3d, ItemStack>> overflow;
			unsigned int chunkCount;
			s32 seed;
			save >> chunkCount >> seed;

			world.setSeed(seed);

			// logInfo() << "Loading dimension" << world.dimension().id() << "| Chunk count:" << chunkCount;

			for (unsigned int i = 0 ; i < chunkCount ; ++i) {
				int cx, cy, cz;
				save >> cx >> cy >> cz;

				ServerChunk &chunk = world.getOrCreateChunk(cx, cy, cz);
				for (u8 z = 0 ; z < Chunk::height ; ++z) {
					for (u8 y = 0 ; y < Chunk::depth ; ++y) {
						for (u8 x = 0 ; x < Chunk::width ; ++x) {
							bool hasMetadata;
							save >> hasMetadata;

							if (hasMetadata) {
								BlockData *blockData = chunk.addBlockData(x, y, z);
								save >> blockData->inventory >> blockData->meta;
								for (const auto &stack : blockData->inventory.normalizeStacks())
									overflow.emplace_back(Vector3d{double(cx * CHUNK_WIDTH + x) + 0.5,
										double(cy * CHUNK_DEPTH + y) + 0.5, double(cz * CHUNK_HEIGHT + z) + 0.5}, stack);
							}

							u32 data;
							u8 light;
							save >> data >> light;
							const auto &definitions = Registry::getInstance();
							if ((data & 0xffff) >= definitions.blocks().size() || !definitions.getBlock(u16(data & 0xffff)).id())
								data = 0;

							chunk.setBlockRaw(x, y, z, u16(data & 0xffff));
							chunk.setData(x, y, z, u16(data >> 16));
							chunk.lightmap().setLightData(x, y, z, light);

							const ServerBlock &block = (ServerBlock &)Registry::getInstance().getBlock(data & 0xffff);
							if (block.canUpdate())
								chunk.addTickingBlock(x, y, z, block);
						}
					}
				}

				chunk.setInitialized(true);
				chunk.setModified(true);
				world.blockPhysics().seedChunk(chunk);
			}

			loadEntities(save, world);
			for (const auto &entry : overflow)
				world.dropWorldItem(entry.first, entry.second.stringID(), entry.second.amount(), entry.second.damage(), false);
		}

		u16 playerCount;
		save >> playerCount;
		for (u16 i = 0 ; i < playerCount ; ++i) {
			std::string username;
			save >> username;

			auto &player = m_playerList.addPlayer(username, false);
			player.deserialize(save);
			for (const auto &stack : player.inventory().normalizeStacks())
				if (player.dimension() < m_worldList.size()) m_worldList[player.dimension()].dropWorldItem({player.x(), player.y(), player.z() + 0.5}, stack.stringID(), stack.amount(), stack.damage(), false);
		}

		sf::Uint64 ticks;
		save >> ticks;
		GameTime::setTicks(ticks);
		while (!save.endOfPacket()) {
			u32 marker = 0;
			u16 count = 0;
			if (!(save >> marker >> count)) break;
			if (marker == 0x4845414c) {
				for (u16 i = 0; i < count; ++i) {
					std::string name;
					u16 health;
					if (!(save >> name >> health)) break;
					if (auto *player = m_playerList.getPlayer(name)) player->setHealth(health);
				}
			}
			else if (marker == 0x53434f52) {
				for (u16 i = 0; i < count; ++i) {
					std::string name;
					s32 score;
					if (!(save >> name >> score)) break;
					// accept the optional score trailer from newer saves.
				}
			}
			else if (marker == 0x41524d52) {
				for (u16 i = 0; i < count; ++i) {
					std::string name;
					Inventory armor;
					if (!(save >> name >> armor)) break;
					auto *player = m_playerList.getPlayer(name);
					if (!player || armor.width() != 1 || armor.height() != 4) continue;
					for (u16 slot = 0; slot < 4; ++slot) {
						const auto &stack = armor.getStack(0, slot);
						if (stack.amount() == 1 && stack.item().getGroupValue("group:om_armor_slot") == slot + 1)
							player->armor().setItemStack(0, slot, stack);
					}
				}
			}

			else if (marker == 0x4d4f4445 && count == 1) {
				bool creative;
				if (!(save >> creative)) break;
				for (auto &world : m_worldList) world.setCreative(creative);
			}
			else break;
		}

	}

	// logInfo() << "Loading done.";
}

void WorldSaveBasicBackend::save(const std::string &name) {
	logInfo() << ("Saving '" + name + "'...").c_str();

	fs::create_directory("saves");

	std::ofstream file("saves/" + name + ".dat", std::ofstream::binary | std::ofstream::trunc);

	Network::Packet save;
	for (auto &world : m_worldList) {
		// queued lighting must reach its final state before its queues are discarded.
		bool pending = true;
		while (pending) {
			pending = false;
			for (const auto &entry : world.chunks()) {
				if (entry.second->isInitialized() && entry.second->lightmap().hasPendingUpdates()) {
					entry.second->lightmap().updateLights(65536);
					pending = true;
				}
			}
		}
		Network::Packet chunks;
		unsigned int chunkCount = 0;
		for (auto &it : world.chunks()) {
			if (!it.second->isInitialized() || !it.second->hasBeenModified()) continue;

			const Vector3i &chunkpos = it.first;
			const Chunk::DataArray &data = it.second->data();
			chunks << chunkpos.x << chunkpos.y << chunkpos.z;

			for (u8 z = 0 ; z < Chunk::height ; ++z) {
				for (u8 y = 0 ; y < Chunk::depth ; ++y) {
					for (u8 x = 0 ; x < Chunk::width ; ++x) {
						BlockData *blockData = it.second->getBlockData(x, y, z);
						if (blockData)
							chunks << true << blockData->inventory << blockData->meta;
						else
							chunks << false;

						chunks << u32(data[z][y][x])
							<< u8(it.second->lightmap().getLightDataRaw(x, y, z));
					}
				}
			}

			++chunkCount;
		}

		// logInfo() << "Saving dimension" << world.dimension().id() << "| Chunk count:" << chunkCount;

		save << chunkCount << world.seed();
		save.append(chunks.getData(), chunks.getDataSize());

		saveEntities(save, world);
	}

	save << (u16)m_playerList.size();

	for (auto &it : m_playerList) {
		save << it.second.name();
		save << it.second;
	}

	save << (sf::Uint64)GameTime::getTicks();
	save << u32(0x4845414c) << u16(m_playerList.size());
	for (const auto &entry : m_playerList)
		save << entry.first << entry.second.health();
	save << u32(0x41524d52) << u16(m_playerList.size());
	for (const auto &entry : m_playerList)
		save << entry.first << entry.second.armor();

	save << u32(0x4d4f4445) << u16(1) << (!m_worldList.empty() && m_worldList.front().isCreative());

	file.write((const char *)save.getData(), save.getDataSize());

	// logInfo() << "Saving done.";
}

#include "common/scene/component/AnimationComponent.hpp"
#include "common/scene/component/ComponentType.hpp"
#include "common/scene/component/DrawableDef.hpp"
#include "common/scene/component/PositionComponent.hpp"
#include "common/scene/component/RotationComponent.hpp"

void WorldSaveBasicBackend::loadEntities(sf::Packet &save, ServerWorld &world) {
	entt::registry &registry = world.scene().registry();
	registry.clear();

	u32 entityCount;
	save >> entityCount;

	// logDebug() << "Loading" << componentCount << "components in dimension" << world.dimension().id();

	for (u32 i = 0 ; i < entityCount ; ++i) {
		entt::entity entityID;
		save >> entityID;

		entt::entity entity = registry.create();
		m_entityMap.emplace(entityID, entity);
		registry.emplace<NetworkComponent>(entity, entity);

		// logDebug() << "Creating entity" << std::underlying_type_t<entt::entity>(entityID);

		u32 componentCount;
		save >> componentCount;

		for (u32 i = 0 ; i < componentCount ; ++i) {
			ComponentType type;
			save >> type;

			// logDebug() << "Loading component" << (u16)type << "for entity" << std::underlying_type_t<entt::entity>(entityID);

			if (type == ComponentType::Position) {
				save >> registry.emplace<PositionComponent>(entity);
			}
			else if (type == ComponentType::Rotation) {
				save >> registry.emplace<RotationComponent>(entity);
			}
			else if (type == ComponentType::Animation) {
				save >> registry.emplace<AnimationComponent>(entity);
			}
			else if (type == ComponentType::Drawable) {
				save >> registry.emplace<DrawableDef>(entity);
			}
			else if (type == ComponentType::ItemStack) {
				save >> registry.emplace<ItemStack>(entity);
			}
			else if (type == ComponentType::Hitbox) {
				save >> registry.emplace<DoubleBox>(entity);
			}
			else if (type == ComponentType::FallingBlock) {
				save >> registry.emplace<FallingBlockComponent>(entity);
			}
			else if (type == ComponentType::Tnt) {
				save >> registry.emplace<TntComponent>(entity);
			}
			else if (type == ComponentType::Physical) {
				save >> registry.emplace<PhysicalComponent>(entity);
			}
			else if (type == ComponentType::BetaMob) {
				save >> registry.emplace<BetaMobComponent>(entity);
			}
			else if (type == ComponentType::Mob) {
				save >> registry.emplace<MobComponent>(entity);
			}
			else if (type == ComponentType::EntityID) {
				save >> registry.emplace<std::string>(entity);
			}
			else
				logWarning() << "Unknown component with type" << (int)type;
		}
		const auto *stack = registry.try_get<ItemStack>(entity);
		const auto *falling = registry.try_get<FallingBlockComponent>(entity);
		const auto &definitions = Registry::getInstance();
		if ((stack && !definitions.hasItem(stack->stringID())) || (falling &&
			(falling->block >= definitions.blocks().size() || !definitions.getBlock(falling->block).id())))
			registry.destroy(entity);
	}
}

void WorldSaveBasicBackend::saveEntities(sf::Packet &save, ServerWorld &world) {
	entt::registry &registry = world.scene().registry();

	auto view = registry.view<NetworkComponent>();
	save << (u32)view.size();

	view.each([&] (auto entity, auto &network) {
		u32 componentCount = 0;
		Network::Packet entityPacket;
		registry.visit(entity, [&] (const auto &component_type) {
			const auto &type = entt::resolve(component_type);
			if (type.prop("is_savable"_hs).value().template cast<bool>()) {
				entityPacket << type.prop("type"_hs).value().template cast<ComponentType>();

				const auto &component = type.func("get"_hs).invoke({}, std::ref(registry), entity);
				Network::Packet packet = type.func("save"_hs).invoke({}, component).template cast<Network::Packet>();
				entityPacket.append(packet.getData(), packet.getDataSize());

				// logDebug() << "Serializing component" << type.prop("name"_hs).value().template cast<std::string>() << "for entity" << std::underlying_type_t<entt::entity>(entity) << "of size" << packet.getDataSize();

				++componentCount;
			}
		});

		// logDebug() << "Saving" << componentCount << "components for entity" << std::underlying_type_t<entt::entity>(entity);

		save << network.entityID << componentCount;
		save.append(entityPacket.getData(), entityPacket.getDataSize());
	});

	// logDebug() << "Saving" << view.size() << "entities in dimension" << world.dimension().id();
}
