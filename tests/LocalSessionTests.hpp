#ifndef LOCALSESSIONTESTS_HPP_
#define LOCALSESSIONTESTS_HPP_

#include <atomic>
#include <thread>

#include <cxxtest/TestSuite.h>
#include "common/network/LocalSession.hpp"

class LocalSessionTests : public CxxTest::TestSuite {
	public:
		void testOnlyOneLocalClient() {
			auto session = Network::LocalSession::create();
			TS_ASSERT_EQUALS(Network::LocalSession::connect(), session);
			TS_ASSERT(!Network::LocalSession::connect());
			session->close();
			TS_ASSERT(!Network::LocalSession::connect());
		}

		void testSnapshotAndIndependentReadCursor() {
			auto session = Network::LocalSession::create();
			Network::Packet outgoing;
			outgoing << sf::Uint32(73) << std::string("local world");
			sf::Uint32 consumed;
			outgoing >> consumed;
			TS_ASSERT(session->sendToServer(outgoing));
			outgoing.clear();

			Network::Packet incoming;
			incoming << sf::Uint32(999);
			incoming >> consumed;
			TS_ASSERT(session->receiveOnServer(incoming));
			sf::Uint32 value;
			std::string text;
			incoming >> value >> text;
			TS_ASSERT(incoming);
			TS_ASSERT_EQUALS(value, 73u);
			TS_ASSERT_EQUALS(text, "local world");
			TS_ASSERT(!session->receiveOnServer(incoming));
		}

		void testThreadedDuplexOrder() {
			auto session = Network::LocalSession::create();
			std::atomic<bool> valid{true};
			std::thread server([&] {
				for (sf::Uint32 expected = 0; expected < 1000; ++expected) {
					Network::Packet packet;
					while (!session->receiveOnServer(packet))
						session->waitOnServer(std::chrono::milliseconds(10));
					sf::Uint32 value = 1000;
					packet >> value;
					if (!packet || value != expected)
						valid = false;
					packet.clear();
					packet << value;
					session->sendToClient(packet);
				}
			});

			for (sf::Uint32 value = 0; value < 1000; ++value) {
				Network::Packet packet;
				packet << value;
				session->sendToServer(packet);
			}
			for (sf::Uint32 expected = 0; expected < 1000; ++expected) {
				Network::Packet packet;
				if (!session->waitOnClient(packet, std::chrono::seconds(5))) {
					valid = false;
					break;
				}
				sf::Uint32 value = 1000;
				packet >> value;
				if (!packet || value != expected)
					valid = false;
			}
			server.join();
			TS_ASSERT(valid);
		}

		void testCloseDrainsFinalSaveAcknowledgement() {
			auto session = Network::LocalSession::create();
			Network::Packet packet;
			packet << sf::Uint32(42);
			session->sendToClient(packet);
			session->close();
			TS_ASSERT(!session->sendToClient(packet));
			TS_ASSERT(session->waitOnClient(packet, std::chrono::seconds(1)));
			sf::Uint32 value;
			packet >> value;
			TS_ASSERT_EQUALS(value, 42u);
			TS_ASSERT(!session->waitOnClient(packet, std::chrono::seconds(1)));
		}

		void testCloseWakesWaitingClient() {
			auto session = Network::LocalSession::create();
			bool received = true;
			auto start = std::chrono::steady_clock::now();
			std::thread client([&] {
				Network::Packet packet;
				received = session->waitOnClient(packet, std::chrono::seconds(5));
			});
			session->close();
			client.join();
			TS_ASSERT(!received);
			TS_ASSERT(std::chrono::steady_clock::now() - start < std::chrono::seconds(2));
		}

		void testNewWorldHasNoStalePackets() {
			auto oldSession = Network::LocalSession::create();
			Network::Packet packet;
			packet << sf::Uint32(1);
			oldSession->sendToServer(packet);
			auto newSession = Network::LocalSession::create();
			TS_ASSERT(oldSession->isClosed());
			TS_ASSERT(!newSession->receiveOnServer(packet));
			TS_ASSERT_EQUALS(Network::LocalSession::connect(), newSession);
		}
};

#endif // LOCALSESSIONTESTS_HPP_
