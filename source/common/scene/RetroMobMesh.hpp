#ifndef RETROMOBMESH_HPP_
#define RETROMOBMESH_HPP_

#include <array>
#include <cmath>
#include <vector>

namespace retro {

struct MobVertex { float x, y, z, u, v; };

// cuboids and texture layout from ModelRenderer, ModelPig and ModelBiped.
inline std::vector<MobVertex> mobMesh(unsigned model, float stride = 0, float strength = 0) {
	std::vector<MobVertex> result;
	if (model != 1 && model != 2) return result;
	constexpr int faces[6][4] = {{5,1,2,6},{0,4,7,3},{5,4,0,1},{2,3,7,6},{1,0,3,2},{4,5,6,7}};
	constexpr int triangles[6] = {0,1,2,0,2,3};
	auto box = [&](float x, float y, float z, float width, float height, float depth,
	               float px, float py, float pz, float u, float v, float angle, bool mirror = false) {
		float x0 = mirror ? x + width : x, x1 = mirror ? x : x + width;
		float corners[8][3] = {{x0,y,z},{x1,y,z},{x1,y+height,z},{x0,y+height,z},
			{x0,y,z+depth},{x1,y,z+depth},{x1,y+height,z+depth},{x0,y+height,z+depth}};
		float uv[6][4] = {
			{u+depth+width,v+depth,u+depth+width+depth,v+depth+height},
			{float(u),v+depth,u+depth,v+depth+height},
			{u+depth,float(v),u+depth+width,v+depth},
			{u+depth+width,float(v),u+depth+width+width,v+depth},
			{u+depth,v+depth,u+depth+width,v+depth+height},
			{u+depth+width+depth,v+depth,u+depth+width+depth+width,v+depth+height}
		};
		for (int f = 0; f < 6; ++f) {
			for (int vertex : triangles) {
				int i = mirror ? 3 - vertex : vertex;
				const auto &point = corners[faces[f][i]];
				float cy = point[1] * std::cos(angle) - point[2] * std::sin(angle) + py;
				float cz = point[1] * std::sin(angle) + point[2] * std::cos(angle) + pz;
				float texU = (i == 0 || i == 3) ? uv[f][2] / 64 - 0.0015625f : uv[f][0] / 64 + 0.0015625f;
				float texV = i < 2 ? uv[f][1] / 32 + 0.003125f : uv[f][3] / 32 - 0.003125f;
				result.push_back({(point[0] + px) / 16, cz / 16, (24 - cy) / 16, texU, texV});
			}
		}
	};
	float swing = std::cos(stride * 0.6662f) * 1.4f * strength;
	if (model == 1) {
		box(-4,-4,-8,8,8,8,0,12,-6,0,0,0);
		box(-5,-10,-7,10,16,8,0,11,2,28,8,1.570796327f);
		box(-2,0,-2,4,6,4,-3,18,7,0,16,swing);
		box(-2,0,-2,4,6,4,3,18,7,0,16,-swing);
		box(-2,0,-2,4,6,4,-3,18,-5,0,16,-swing);
		box(-2,0,-2,4,6,4,3,18,-5,0,16,swing);
	}
	else {
		box(-4,-8,-4,8,8,8,0,0,0,0,0,0);
		box(-4,0,-2,8,12,4,0,0,0,16,16,0);
		box(-3,-2,-2,4,12,4,-5,2,0,40,16,-1.570796327f);
		box(-1,-2,-2,4,12,4,5,2,0,40,16,-1.570796327f,true);
		box(-2,0,-2,4,12,4,-2,12,0,0,16,swing);
		box(-2,0,-2,4,12,4,2,12,0,0,16,-swing,true);
	}
	return result;
}

}

#endif // RETROMOBMESH_HPP_
