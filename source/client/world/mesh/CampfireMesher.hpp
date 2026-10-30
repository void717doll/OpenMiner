#ifndef CAMPFIREMESHER_HPP_
#define CAMPFIREMESHER_HPP_

#include "common/core/BlockGeometry.hpp"
#include "client/world/mesh/BlockMesher.hpp"

class CampfireMesher {
	public:
		static void addBlock(s8f x, s8f y, s8f z, ChunkMeshBuildingJob &job, const BlockState &state) {
			BlockGeometry::initOrientation();
			for (unsigned layer = 0; layer < 2; ++layer) {
				for (float side : {1.f / 16, 11.f / 16}) {
					FloatBox box{0, side, layer ? 3.f / 16 : 0.f, 1, 4.f / 16, 4.f / 16};
					BlockState log = state;
					log.boundingBox(box);
					BlockMesher::addBlock(x, y, z, job, log, box, u8f(layer), BlockGeometry::orientMatrices[layer]);
				}
			}
		}
};

#endif // CAMPFIREMESHER_HPP_
