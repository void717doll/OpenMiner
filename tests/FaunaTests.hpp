#ifndef FAUNATESTS_HPP_
#define FAUNATESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/core/Registry.hpp"
#include "common/scene/RetroMobMesh.hpp"
#include "common/scene/component/MobComponent.hpp"
#include "common/scene/component/DrawableDef.hpp"
#include "common/scene/component/PositionComponent.hpp"
#include "common/scene/component/RotationComponent.hpp"
#include "common/world/Dimension.hpp"
#include "server/core/PlayerList.hpp"
#include "server/network/ClientInfo.hpp"
#include "server/world/ServerBlock.hpp"
#include "server/world/ServerWorld.hpp"

class FaunaTests : public CxxTest::TestSuite {
	static void definitions(Registry &registry) {
		Registry::setInstance(registry);
		registry.registerBlock<ServerBlock>(BLOCK_AIR).getState(0).isCollidable(false);
		registry.registerBlock<ServerBlock>("test:stone");
		registry.registerItem<Item>({}, BLOCK_AIR, "air");
		registry.registerItem<Item>({}, "default:fish_cod_raw", "raw fish").setMaxStackSize(1);
		for (BetaMobKind kind : {BetaMobKind::Ocelot, BetaMobKind::Bat, BetaMobKind::Chicken}) {
			auto model = registry.registerEntity("default:" + std::string(betaMobName(kind)));
			auto &models = registry.entityRegistry();
			auto &mob = models.emplace<MobComponent>(model);
			mob.health = kind == BetaMobKind::Bat ? 6 : 10;
			mob.moveSpeed = 0.6f;
			models.emplace<BetaMobComponent>(model).kind = kind;
			models.emplace<DoubleBox>(model, -0.25, -0.25, 0, 0.5, 0.5, kind == BetaMobKind::Bat ? 0.89 : 0.7);
			models.emplace<RotationComponent>(model);
			models.emplace<DrawableDef>(model).setMobModel(u8(kind));
		}
	}

	static void room(ServerWorld &world) {
		auto &chunk = world.getOrCreateChunk(0, 0, 0);
		for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) {
			chunk.setBlockRaw(x, y, 0, 1);
			chunk.setBlockRaw(x, y, 6, 1);
		}
		chunk.setInitialized(true);
	}

	static entt::entity find(ServerWorld &world, BetaMobKind kind) {
		entt::entity found = entt::null;
		world.scene().registry().view<BetaMobComponent>().each([&](auto entity, const auto &traits) {
			if (traits.kind == kind) found = entity;
		});
		return found;
	}

public:
	void testNewKindsPreserveOldIdsAndPacketLayout() {
		TS_ASSERT_EQUALS(u8(BetaMobKind::Arrow), 15);
		TS_ASSERT_EQUALS(u8(BetaMobKind::Fireball), 16);
		TS_ASSERT_EQUALS(u8(BetaMobKind::Ocelot), 17);
		TS_ASSERT_EQUALS(u8(BetaMobKind::Bat), 18);
		for (auto kind : {BetaMobKind::Wolf, BetaMobKind::Arrow, BetaMobKind::Fireball, BetaMobKind::Ocelot, BetaMobKind::Bat}) {
			BetaMobComponent original; original.kind = kind; original.tamed = true; original.sitting = true;
			original.target = "tester"; original.waypoint = {1,2,3}; original.velocity = {4,5,6};
			sf::Packet packet; packet << original << u32(0x12345678);
			BetaMobComponent restored; u32 marker = 0; packet >> restored >> marker;
			TS_ASSERT(packet && packet.endOfPacket());
			TS_ASSERT_EQUALS(u8(restored.kind), u8(kind));
			TS_ASSERT_EQUALS(marker, 0x12345678u);
			TS_ASSERT(restored.tamed && restored.sitting);
			TS_ASSERT_EQUALS(restored.waypoint.z, 3);
			TS_ASSERT_EQUALS(restored.velocity.y, 5);
			TS_ASSERT_EQUALS(u8(betaMobKind("default:" + std::string(betaMobName(kind)))), u8(kind));
		}
		TS_ASSERT(!betaMobIsProjectile(BetaMobKind::Ocelot));
		TS_ASSERT(!betaMobIsProjectile(BetaMobKind::Bat));
	}

	void testSourceMeshesHaveValidUvsAndDistinctAnimatedPoses() {
		for (unsigned model : {17u, 18u}) {
			auto idle = retro::mobMesh(model, 0, 0, 0);
			auto moving = retro::mobMesh(model, 1, 0.6f, 1);
			auto posed = retro::mobMesh(model, 0, 0, 0, 1);
			TS_ASSERT_EQUALS(idle.size(), model == 17 ? 396u : 156u);
			TS_ASSERT_EQUALS(moving.size(), idle.size());
			unsigned changed = 0, posedChanged = 0;
			for (unsigned i = 0; i < idle.size(); ++i) {
				const auto &p = idle[i];
				TS_ASSERT(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z));
				TS_ASSERT(p.u >= 0 && p.u <= 1 && p.v >= 0 && p.v <= 1);
				if (p.x != moving[i].x || p.y != moving[i].y || p.z != moving[i].z) ++changed;
				if (p.x != posed[i].x || p.y != posed[i].y || p.z != posed[i].z) ++posedChanged;
			}
			TS_ASSERT(changed > 20 && posedChanged > 20);
			TS_ASSERT(retro::mobMesh(model, 0, 0, 0, 0, 1).empty());
		}
	}

	void testOcelotFleesFollowsFishAndStartlesAtFastMovement() {
		Registry registry; definitions(registry);
		PlayerList players; ClientInfo client{1, {}};
		auto &player = players.addPlayer("tester", true);
		player.setClient(&client); player.setReady(true); player.setPosition(4, 8, 1); player.setHeldItemSlot(0);
		Dimension dimension{0, "test:world", "test"}; ServerWorld world(players, dimension, 42); room(world);
		TS_ASSERT(world.spawnMob("default:ocelot", 8, 8, 1));
		auto entity = find(world, BetaMobKind::Ocelot); TS_ASSERT(entity != entt::null); if (entity == entt::null) return;
		auto &scene = world.scene().registry(); auto &position = scene.get<PositionComponent>(entity);
		world.updateMobs(); TS_ASSERT(position.x > 8);
		player.inventory().setStack(0, 0, "default:fish_cod_raw", 1);
		double before = position.x; world.updateMobs(); TS_ASSERT(position.x < before);
		player.setPosition(4.5, 8, 1);
		before = position.x; world.updateMobs(); TS_ASSERT(position.x > before);
		TS_ASSERT(scene.get<BetaMobComponent>(entity).anger > 0);
		TS_ASSERT_EQUALS(player.health(), 20);
	}

	void testOcelotFishTrustAndCreativeConsumption() {
		Registry registry; definitions(registry);
		PlayerList players; auto &player = players.addPlayer("tester", true);
		player.setPosition(8, 8, 1); player.setRotation(0, -35); player.setHeldItemSlot(0);
		Dimension dimension{0, "test:world", "test"}; ServerWorld world(players, dimension, 42); room(world);
		TS_ASSERT(world.spawnMob("default:ocelot", 10, 8, 1));
		auto entity = find(world, BetaMobKind::Ocelot); TS_ASSERT(entity != entt::null); if (entity == entt::null) return;
		auto &scene = world.scene().registry(); auto &traits = scene.get<BetaMobComponent>(entity);
		traits.target = player.name(); scene.get<MobComponent>(entity).randomState = 1;
		player.inventory().setStack(0, 0, "default:fish_cod_raw", 1);
		TS_ASSERT(world.interactMob(player)); TS_ASSERT(traits.tamed);
		TS_ASSERT_EQUALS(player.heldItemStack().amount(), 0);
		player.setCreative(true); player.inventory().setStack(0, 0, "default:fish_cod_raw", 1);
		scene.get<MobComponent>(entity).health = 5;
		TS_ASSERT(world.interactMob(player)); TS_ASSERT_EQUALS(player.heldItemStack().amount(), 1);
		TS_ASSERT_EQUALS(scene.get<MobComponent>(entity).health, 7);
		TS_ASSERT(traits.owner.empty() && !traits.sitting);
	}

	void testBatHangsThenStartlesAndStaysInsideLoadedRoom() {
		Registry registry; definitions(registry);
		PlayerList players; ClientInfo client{1, {}};
		Dimension dimension{0, "test:world", "test"}; ServerWorld world(players, dimension, 42); room(world);
		TS_ASSERT(world.spawnMob("default:bat", 8.5, 8.5, 5.11));
		auto entity = find(world, BetaMobKind::Bat); TS_ASSERT(entity != entt::null); if (entity == entt::null) return;
		auto &scene = world.scene().registry(); auto &traits = scene.get<BetaMobComponent>(entity);
		auto &position = scene.get<PositionComponent>(entity); traits.sitting = true;
		world.updateMobs(); TS_ASSERT(traits.sitting); TS_ASSERT_DELTA(position.z, 5.11, 0.001);
		auto &player = players.addPlayer("tester", true); player.setClient(&client); player.setReady(true); player.setPosition(8,8,3);
		world.updateMobs(); TS_ASSERT(!traits.sitting);
		for (int tick = 0; tick < 250; ++tick) {
			world.updateMobs();
			TS_ASSERT(position.x >= 0.25 && position.x <= 15.75);
			TS_ASSERT(position.y >= 0.25 && position.y <= 15.75);
			TS_ASSERT(position.z >= 1 && position.z <= 5.11 + 0.001);
		}
		TS_ASSERT_EQUALS(player.health(), 20);
		player.setReady(false); position.x = 8.5; position.y = 8.5; position.z = 5.11; traits.sitting = true;
		world.getChunkAtBlockPos(8,8,6)->setBlockRaw(8,8,6,0);
		world.updateMobs(); TS_ASSERT(!traits.sitting);
	}

	void testNewOcelotCollisionIsNotTreatedAsAProjectile() {
		Registry registry; definitions(registry);
		PlayerList players; Dimension dimension{0, "test:world", "test"}; ServerWorld world(players, dimension, 42); room(world);
		TS_ASSERT(world.spawnMob("default:ocelot", 8, 8, 1));
		TS_ASSERT(world.spawnMob("default:bat", 9, 8, 1));
		std::vector<DoubleBox> obstacles;
		world.appendCollisionBoxes({4,8,1,0.5,0.5,1}, {4,7,1,8,3,3}, obstacles);
		TS_ASSERT_EQUALS(obstacles.size(), 1u);
		if (!obstacles.empty()) TS_ASSERT_DELTA(obstacles[0].x, 7.75, 0.001);
	}

	void testNaturalBatsRequireDarkCavesAndRespectAmbientCap() {
		for (u8 light : {u8(15), u8(0)}) {
			Registry registry; definitions(registry);
			PlayerList players; ClientInfo client{1, {}};
			auto &player = players.addPlayer("tester", true); player.setClient(&client); player.setReady(true); player.setPosition(8,8,21);
			Dimension dimension{0, "test:world", "test"}; ServerWorld world(players, dimension, 42);
			for (int cy = -2; cy <= 2; ++cy) for (int cx = -2; cx <= 2; ++cx) {
				auto &chunk = world.getOrCreateChunk(cx, cy, 0);
				for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) {
					chunk.setBlockRaw(x,y,20,1); chunk.setBlockRaw(x,y,26,1);
					for (int z = 0; z < CHUNK_HEIGHT; ++z) chunk.lightmap().setLightDataRaw(x,y,z,light);
				}
				chunk.setInitialized(true);
			}
			for (int tick = 0; tick < 600; ++tick) world.updateMobs();
			auto count = [&]() {
				unsigned total = 0;
				world.scene().registry().view<BetaMobComponent>().each([&](auto, const auto &traits) { if (traits.kind == BetaMobKind::Bat) ++total; });
				return total;
			};
			if (light) { TS_ASSERT_EQUALS(count(), 0u); }
			else {
				TS_ASSERT(count() > 0 && count() <= 8);
				for (unsigned i = count(); i < 8; ++i) TS_ASSERT(world.spawnMob("default:bat", 4 + i, 4, 21));
				for (int tick = 0; tick < 600; ++tick) world.updateMobs();
				TS_ASSERT_EQUALS(count(), 8u);
			}
		}
	}
};

#endif // FAUNATESTS_HPP_
