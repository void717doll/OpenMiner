#ifndef BETAMODELTESTS_HPP_
#define BETAMODELTESTS_HPP_

#include <cxxtest/TestSuite.h>

#include "common/scene/RetroMobMesh.hpp"
#include "common/scene/component/DrawableDef.hpp"

class BetaModelTests : public CxxTest::TestSuite {
	public:
		void testSourceModelPartCounts() {
			const unsigned counts[] = {0,216,252,324,216,288,252,216,396,144,360,252,396,324,252,36,6};
			for (unsigned model = 1; model <= 16; ++model) {
				auto mesh = retro::mobMesh(model, 1.f, 0.6f, 20.f);
				TS_ASSERT_EQUALS(mesh.size(), counts[model]);
				for (const auto &vertex : mesh) {
					TS_ASSERT(std::isfinite(vertex.x) && std::isfinite(vertex.y) && std::isfinite(vertex.z));
					TS_ASSERT(vertex.u >= 0 && vertex.u <= 1);
					TS_ASSERT(vertex.v >= 0 && vertex.v <= 1);
				}
			}
		}

		void testSourceOverlayPasses() {
			TS_ASSERT(retro::mobMesh(1,0,0,0,0,1).empty());
			TS_ASSERT_EQUALS(retro::mobMesh(1,0,0,0,1,1).size(),216);
			TS_ASSERT_EQUALS(retro::mobMesh(4,0,0,0,0,1).size(),216);
			TS_ASSERT(retro::mobMesh(4,0,0,0,16,1).empty());
			TS_ASSERT_EQUALS(retro::mobMesh(8,0,0,0,0,1).size(),396);
			TS_ASSERT_EQUALS(retro::mobMesh(9,0,0,0,0,1).size(),36);
		}

		void testWolfSittingPose() {
			auto standing = retro::mobMesh(12,0,0,0,2 | (20 << 3));
			auto sitting = retro::mobMesh(12,0,0,0,6 | (20 << 3));
			TS_ASSERT_EQUALS(standing.size(),sitting.size());
			bool changed = false;
			for (std::size_t i = 0; i < standing.size(); ++i)
				if (standing[i].x != sitting[i].x || standing[i].y != sitting[i].y || standing[i].z != sitting[i].z) changed = true;
			TS_ASSERT(changed);
		}

		void testDrawableRecordCompatibility() {
			sf::Packet legacy;
			legacy << u8(DrawableType::RetroMob) << u8(1) << u32(0x12345678);
			DrawableDef restored;
			legacy >> restored;
			u32 marker;
			legacy >> marker;
			TS_ASSERT_EQUALS(marker,0x12345678);
			TS_ASSERT_EQUALS(restored.mobModel(),1);
			TS_ASSERT_EQUALS(restored.mobScale(),1.f);
			TS_ASSERT_EQUALS(restored.mobAppearance(),0);

			DrawableDef scaled;
			scaled.setMobModel(14,6.f,0);
			scaled.isUpdated = false;
			scaled.setMobModel(14,6.f,0);
			TS_ASSERT(!scaled.isUpdated);
			scaled.setMobAppearance(2);
			TS_ASSERT(scaled.isUpdated);
			sf::Packet packet;
			packet << scaled << u32(0xabcdef01);
			packet >> restored >> marker;
			TS_ASSERT_EQUALS(restored.mobModel(),14);
			TS_ASSERT_EQUALS(restored.mobScale(),6.f);
			TS_ASSERT_EQUALS(restored.mobAppearance(),2);
			TS_ASSERT_EQUALS(marker,0xabcdef01);
		}
};

#endif // BETAMODELTESTS_HPP_
