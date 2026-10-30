#ifndef LOCALPLAYERTESTS_HPP_
#define LOCALPLAYERTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "server/core/PlayerList.hpp"

class LocalPlayerTests : public CxxTest::TestSuite {
	public:
		void testExactSavedNameTakesPriority() {
			PlayerList players;
			auto &saved = players.addPlayer("mia", false);
			saved.setPosition(12.5, -4, 68.25);
			saved.setHealth(7);
			players.addPlayer("singleplayer", false).setHealth(19);
			ClientInfo client{1, {}};

			auto *player = players.connectPlayer("mia", client);
			TS_ASSERT_EQUALS(player, &saved);
			if (!player) return;
			TS_ASSERT_EQUALS(player->x(), 12.5);
			TS_ASSERT_EQUALS(player->y(), -4);
			TS_ASSERT_EQUALS(player->z(), 68.25);
			TS_ASSERT_EQUALS(player->health(), 7);
			TS_ASSERT(!player->isNewPlayer());
			TS_ASSERT_EQUALS(players.size(), 2);
			TS_ASSERT_EQUALS(players.getPlayer("singleplayer")->health(), 19);
		}

		void testLegacySingleplayerMigrationPreservesOtherRecords() {
			PlayerList players;
			auto &saved = players.addPlayer("singleplayer", false);
			saved.setPosition(-16.5, 32.25, 47);
			saved.setHealth(9);
			saved.setDimension(1);
			players.addPlayer("amy", false).setHealth(3);
			ClientInfo client{1, {}};

			auto *player = players.connectPlayer("mia", client);
			TS_ASSERT(player);
			if (!player) return;
			TS_ASSERT_EQUALS(player->name(), "mia");
			TS_ASSERT_EQUALS(player->x(), -16.5);
			TS_ASSERT_EQUALS(player->y(), 32.25);
			TS_ASSERT_EQUALS(player->z(), 47);
			TS_ASSERT_EQUALS(player->health(), 9);
			TS_ASSERT_EQUALS(player->dimension(), 1);
			TS_ASSERT(!player->isNewPlayer());
			TS_ASSERT_EQUALS(players.size(), 3);
			TS_ASSERT_EQUALS(saved.name(), "singleplayer");
			TS_ASSERT_EQUALS(saved.x(), -16.5);
			TS_ASSERT_EQUALS(saved.health(), 9);
			TS_ASSERT(!saved.isOnline());
			TS_ASSERT_EQUALS(players.getPlayer("amy")->health(), 3);
			TS_ASSERT(!players.getPlayer("amy")->isOnline());
		}

		void testSoleLegacyNameMigrationPreservesHealthAndPosition() {
			PlayerList players;
			auto &saved = players.addPlayer("old_name", false);
			saved.setPosition(8, 9, 70);
			saved.setHealth(12);
			ClientInfo client{1, {}};

			auto *player = players.connectPlayer("singleplayer", client);
			TS_ASSERT(player);
			if (!player) return;
			TS_ASSERT_EQUALS(player->name(), "singleplayer");
			TS_ASSERT_EQUALS(player->x(), 8);
			TS_ASSERT_EQUALS(player->y(), 9);
			TS_ASSERT_EQUALS(player->z(), 70);
			TS_ASSERT_EQUALS(player->health(), 12);
			TS_ASSERT_EQUALS(players.size(), 2);
			TS_ASSERT_EQUALS(saved.name(), "old_name");
			TS_ASSERT_EQUALS(saved.health(), 12);
			TS_ASSERT(!saved.isOnline());
		}

		void testLegacyMultipleNamesUseDeterministicFallback() {
			PlayerList players;
			players.addPlayer("zoe", false).setHealth(18);
			auto &first = players.addPlayer("amy", false);
			first.setPosition(6, -7, 55);
			first.setHealth(4);
			ClientInfo client{1, {}};

			auto *player = players.connectPlayer("singleplayer", client);
			TS_ASSERT(player);
			if (!player) return;
			TS_ASSERT_EQUALS(player->x(), 6);
			TS_ASSERT_EQUALS(player->y(), -7);
			TS_ASSERT_EQUALS(player->z(), 55);
			TS_ASSERT_EQUALS(player->health(), 4);
			TS_ASSERT_EQUALS(players.size(), 3);
			TS_ASSERT_EQUALS(first.name(), "amy");
			TS_ASSERT_EQUALS(players.getPlayer("zoe")->health(), 18);
		}

		void testOnlyOnePlayerCanBeActive() {
			PlayerList players;
			ClientInfo client{1, {}}, other{2, {}};
			auto *player = players.connectPlayer("singleplayer", client);
			TS_ASSERT(player);
			if (!player) return;
			TS_ASSERT_EQUALS(player->client(), &client);
			TS_ASSERT_EQUALS(client.playerName, "singleplayer");
			TS_ASSERT(!players.connectPlayer("singleplayer", other));
			TS_ASSERT(!players.connectPlayer("other", other));
			TS_ASSERT_EQUALS(players.size(), 1);
			TS_ASSERT_EQUALS(player->client(), &client);
			TS_ASSERT(other.playerName.empty());
		}

		void testDisconnectAndReconnectKeepCurrentPlayerState() {
			PlayerList players;
			ClientInfo client{1, {}}, replacement{2, {}};
			auto *player = players.connectPlayer("singleplayer", client);
			TS_ASSERT(player);
			if (!player) return;
			TS_ASSERT(player->isNewPlayer());
			player->setPosition(-2.5, 10.75, 72);
			player->setHealth(6);
			player->addLoadedChunk({1, 2, 3});

			players.disconnectPlayer("singleplayer");
			TS_ASSERT(!player->isOnline());
			TS_ASSERT(!player->isNewPlayer());
			TS_ASSERT(player->loadedChunks().empty());
			TS_ASSERT_EQUALS(players.connectPlayer("singleplayer", replacement), player);
			TS_ASSERT_EQUALS(player->client(), &replacement);
			TS_ASSERT_EQUALS(player->clientID(), 2);
			TS_ASSERT_EQUALS(player->x(), -2.5);
			TS_ASSERT_EQUALS(player->y(), 10.75);
			TS_ASSERT_EQUALS(player->z(), 72);
			TS_ASSERT_EQUALS(player->health(), 6);
			TS_ASSERT_EQUALS(players.size(), 1);
		}
};

#endif // LOCALPLAYERTESTS_HPP_
