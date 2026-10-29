#ifndef RETROMOBMESH_HPP_
#define RETROMOBMESH_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "common/world/RetroRandom.hpp"

namespace retro {

struct MobVertex { float x, y, z, u, v; };

class MobMeshBuilder {
	public:
		void box(float x, float y, float z, float width, float height, float depth,
		         float px, float py, float pz, float u, float v,
		         float rx = 0, float ry = 0, float rz = 0, float inflate = 0, bool mirror = false) {
			constexpr int faces[6][4] = {{5,1,2,6},{0,4,7,3},{5,4,0,1},{2,3,7,6},{1,0,3,2},{4,5,6,7}};
			float x0 = x - inflate, x1 = x + width + inflate;
			if (mirror) std::swap(x0, x1);
			float y0 = y - inflate, y1 = y + height + inflate;
			float z0 = z - inflate, z1 = z + depth + inflate;
			float corners[8][3] = {{x0,y0,z0},{x1,y0,z0},{x1,y1,z0},{x0,y1,z0},
				{x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1}};
			float uv[6][4] = {
				{u+depth+width,v+depth,u+depth+width+depth,v+depth+height},
				{u,v+depth,u+depth,v+depth+height},
				{u+depth,v,u+depth+width,v+depth},
				{u+depth+width,v,u+depth+width+width,v+depth},
				{u+depth,v+depth,u+depth+width,v+depth+height},
				{u+depth+width+depth,v+depth,u+depth+width+depth+width,v+depth+height}
			};
			constexpr int triangles[6] = {0,1,2,0,2,3};
			for (int face = 0; face < 6; ++face) {
				for (int vertex : triangles) {
					int i = mirror ? 3 - vertex : vertex;
					const auto &point = corners[faces[face][i]];
					float ax = point[0];
					float ay = point[1] * std::cos(rx) - point[2] * std::sin(rx);
					float az = point[1] * std::sin(rx) + point[2] * std::cos(rx);
					float bx = ax * std::cos(ry) + az * std::sin(ry);
					float bz = -ax * std::sin(ry) + az * std::cos(ry);
					float cx = bx * std::cos(rz) - ay * std::sin(rz) + px;
					float cy = bx * std::sin(rz) + ay * std::cos(rz) + py;
					float cz = bz + pz;
					float texU = (i == 0 || i == 3) ? uv[face][2] / 64 - 0.0015625f : uv[face][0] / 64 + 0.0015625f;
					float texV = i < 2 ? uv[face][1] / 32 + 0.003125f : uv[face][3] / 32 - 0.003125f;
					vertices.push_back({cx / 16, cz / 16, (24 - cy) / 16, texU, texV});
				}
			}
		}

		std::vector<MobVertex> vertices;
};

// geometry and UVs follow the supplied beta ModelRenderer and model classes.
// layer one is used for saddle, wool, spider eyes and slime shell render passes.
inline std::vector<MobVertex> mobMesh(unsigned model, float stride = 0, float strength = 0,
                                     float age = 0, unsigned appearance = 0, unsigned layer = 0) {
	constexpr float pi = 3.14159265358979323846f;
	MobMeshBuilder mesh;
	auto box = [&](float x, float y, float z, float width, float height, float depth,
	               float px, float py, float pz, float u, float v,
	               float rx = 0, float ry = 0, float rz = 0, float inflate = 0, bool mirror = false) {
		// the renderer uses a strict depth test, so eyes need to clear the base mesh.
		if (model == 8 && layer == 1) inflate += 0.01f;
		mesh.box(x,y,z,width,height,depth,px,py,pz,u,v,rx,ry,rz,inflate,mirror);
	};
	float swing = std::cos(stride * 0.6662f) * 1.4f * strength;
	if (model == 1) {
		if (layer > 1 || (layer == 1 && !(appearance & 1))) return {};
		float inflate = layer == 1 ? 0.5f : 0;
		box(-4,-4,-8,8,8,8,0,12,-6,0,0,0,0,0,inflate);
		box(-5,-10,-7,10,16,8,0,11,2,28,8,pi/2,0,0,inflate);
		box(-2,0,-2,4,6,4,-3,18,7,0,16,swing,0,0,inflate);
		box(-2,0,-2,4,6,4,3,18,7,0,16,-swing,0,0,inflate);
		box(-2,0,-2,4,6,4,-3,18,-5,0,16,-swing,0,0,inflate);
		box(-2,0,-2,4,6,4,3,18,-5,0,16,swing,0,0,inflate);
	}
	else if (model == 2 || model == 6 || model == 11 || model == 14) {
		if (layer) return {};
		bool skeleton = model == 6;
		box(-4,-8,-4,8,8,8,0,0,0,0,0);
		box(-4,-8,-4,8,8,8,0,0,0,32,0,0,0,0,0.5f);
		box(-4,0,-2,8,12,4,0,0,0,16,16);
		float rightArm = -pi/2 + std::sin(age * 0.067f) * 0.05f;
		float leftArm = -pi/2 - std::sin(age * 0.067f) * 0.05f;
		float armWidth = skeleton ? 2 : 4, armDepth = skeleton ? 2 : 4;
		box(skeleton ? -1.f : -3.f,-2,skeleton ? -1.f : -2.f,armWidth,12,armDepth,-5,2,0,40,16,
			rightArm,-0.1f,std::cos(age*0.09f)*0.05f+0.05f);
		box(-1,-2,skeleton ? -1.f : -2.f,armWidth,12,armDepth,5,2,0,40,16,
			leftArm,0.1f,-std::cos(age*0.09f)*0.05f-0.05f,0,true);
		box(skeleton ? -1.f : -2.f,0,skeleton ? -1.f : -2.f,skeleton ? 2.f : 4.f,12,skeleton ? 2.f : 4.f,-2,12,0,0,16,swing);
		box(skeleton ? -1.f : -2.f,0,skeleton ? -1.f : -2.f,skeleton ? 2.f : 4.f,12,skeleton ? 2.f : 4.f,2,12,0,0,16,-swing,0,0,0,true);
	}
	else if (model == 3) {
		if (layer) return {};
		box(-4,-4,-6,8,8,6,0,4,-8,0,0);
		box(-4,-5,-4,1,3,1,0,3,-7,22,0);
		box(3,-5,-4,1,3,1,0,3,-7,22,0);
		box(-2,-3,0,4,6,2,0,14,6,52,0,pi/2);
		box(-6,-10,-7,12,18,10,0,5,2,18,4,pi/2);
		box(-2,0,-2,4,12,4,-4,12,7,0,16,swing);
		box(-2,0,-2,4,12,4,4,12,7,0,16,-swing);
		box(-2,0,-2,4,12,4,-4,12,-6,0,16,-swing);
		box(-2,0,-2,4,12,4,4,12,-6,0,16,swing);
	}
	else if (model == 4) {
		if (layer > 1 || (layer == 1 && (appearance & 16))) return {};
		bool wool = layer == 1;
		box(-3,-4,wool ? -4.f : -6.f,6,6,wool ? 6.f : 8.f,0,6,-8,0,0,0,0,0,wool ? 0.6f : 0);
		box(-4,-10,-7,8,16,6,0,5,2,28,8,pi/2,0,0,wool ? 1.75f : 0);
		float length = wool ? 6.f : 12.f;
		float inflate = wool ? 0.5f : 0;
		box(-2,0,-2,4,length,4,-3,12,7,0,16,swing,0,0,inflate);
		box(-2,0,-2,4,length,4,3,12,7,0,16,-swing,0,0,inflate);
		box(-2,0,-2,4,length,4,-3,12,-5,0,16,-swing,0,0,inflate);
		box(-2,0,-2,4,length,4,3,12,-5,0,16,swing,0,0,inflate);
	}
	else if (model == 5) {
		if (layer) return {};
		box(-2,-6,-2,4,6,3,0,15,-4,0,0);
		box(-2,-4,-4,4,2,2,0,15,-4,14,0);
		box(-1,-2,-3,2,2,2,0,15,-4,14,4);
		box(-3,-4,-3,6,8,6,0,16,0,0,9,pi/2);
		box(-1,0,-3,3,5,3,-2,19,1,26,0,swing);
		box(-1,0,-3,3,5,3,1,19,1,26,0,-swing);
		float flap = (std::sin(age * 0.8f) + 1) * strength;
		box(0,0,-3,1,4,6,-4,13,0,24,13,0,0,flap);
		box(-1,0,-3,1,4,6,4,13,0,24,13,0,0,-flap);
	}
	else if (model == 7) {
		if (layer) return {};
		box(-4,-8,-4,8,8,8,0,4,0,0,0);
		box(-4,0,-2,8,12,4,0,4,0,16,16);
		box(-2,0,-2,4,6,4,-2,16,4,0,16,swing);
		box(-2,0,-2,4,6,4,2,16,4,0,16,-swing);
		box(-2,0,-2,4,6,4,-2,16,-4,0,16,-swing);
		box(-2,0,-2,4,6,4,2,16,-4,0,16,swing);
	}
	else if (model == 8) {
		if (layer > 1) return {};
		box(-4,-4,-8,8,8,8,0,15,-3,32,4);
		box(-3,-3,-3,6,6,6,0,15,0,0,0);
		box(-5,-4,-6,10,8,12,0,15,9,0,12);
		float phase[4] = {0,pi,pi/2,pi*1.5f};
		float spread[4] = {pi/4,pi/8,-pi/8,-pi/4};
		for (int pair = 0; pair < 4; ++pair) {
			float yaw = -std::cos(stride * 0.6662f * 2 + phase[pair]) * 0.4f * strength;
			float lift = std::abs(std::sin(stride * 0.6662f + phase[pair]) * 0.4f) * strength;
			float angle = pair == 0 || pair == 3 ? pi/4 : pi/4 * 0.74f;
			box(-15,-1,-1,16,2,2,-4,15,2-float(pair),18,0,0,spread[pair]+yaw,-angle+lift);
			box(-1,-1,-1,16,2,2,4,15,2-float(pair),18,0,0,-spread[pair]-yaw,angle-lift);
		}
	}
	else if (model == 9) {
		if (layer > 1) return {};
		if (layer == 1) box(-4,16,-4,8,8,8,0,0,0,0,0);
		else {
			box(-3,17,-3,6,6,6,0,0,0,0,16);
			box(-3.25f,18,-3.5f,2,2,2,0,0,0,32,0);
			box(1.25f,18,-3.5f,2,2,2,0,0,0,32,4);
			box(0,21,-3.5f,1,1,1,0,0,0,32,8);
		}
	}
	else if (model == 10) {
		if (layer) return {};
		box(-8,-8,-8,16,16,16,0,8,0,0,0);
		Random random(1660);
		for (int tentacle = 0; tentacle < 9; ++tentacle) {
			float x = ((float(tentacle % 3) - float(tentacle / 3 % 2) * 0.5f + 0.25f) - 1) * 5;
			float z = (float(tentacle / 3) - 1) * 5;
			float length = float(random.nextInt(7) + 8);
			box(-1,0,-1,2,length,2,x,15,z,0,0,0.2f * std::sin(age * 0.3f + float(tentacle)) + 0.4f);
		}
	}
	else if (model == 12) {
		if (layer) return {};
		bool sitting = appearance & 4;
		box(-3,-3,-2,6,6,4,-1,13.5f,-7,0,0);
		box(-3,-5,0,2,2,1,-1,13.5f,-7,16,14);
		box(1,-5,0,2,2,1,-1,13.5f,-7,16,14);
		box(-2,0,-5,3,3,4,-0.5f,13.5f,-7,0,10);
		box(-4,-2,-3,6,9,6,0,sitting ? 18.f : 14.f,sitting ? 0.f : 2.f,18,14,sitting ? pi/4 : pi/2);
		box(-4,-3,-3,8,6,7,-1,sitting ? 16.f : 14.f,-3,21,0,sitting ? pi*0.4f : pi/2);
		box(-1,0,-1,2,8,2,-2.5f,sitting ? 22.f : 16.f,sitting ? 2.f : 7.f,0,18,sitting ? pi*1.5f : swing);
		box(-1,0,-1,2,8,2,0.5f,sitting ? 22.f : 16.f,sitting ? 2.f : 7.f,0,18,sitting ? pi*1.5f : -swing);
		box(-1,0,-1,2,8,2,sitting ? -2.49f : -2.5f,sitting ? 17.f : 16.f,-4,0,18,sitting ? pi*1.85f : -swing);
		box(-1,0,-1,2,8,2,sitting ? 0.51f : 0.5f,sitting ? 17.f : 16.f,-4,0,18,sitting ? pi*1.85f : swing);
		unsigned health = appearance >> 3;
		float tail = appearance & 1 ? pi * 0.49f : appearance & 2 ? (0.55f - float(20 - std::min(health,20u)) * 0.02f) * pi : pi * 0.2f;
		box(-1,0,-1,2,8,2,-1,sitting ? 21.f : 12.f,sitting ? 6.f : 8.f,9,18,
			tail,appearance & 1 ? 0 : swing);
	}
	else if (model == 13) {
		if (layer) return {};
		box(-6,-8,-6,12,16,12,0,8,0,0,0);
		float angle = (std::sin(age * 0.1f) + 1) * 0.5f;
		for (int tentacle = 0; tentacle < 8; ++tentacle) {
			float turn = float(tentacle) * pi * 2 / 8;
			box(-1,0,-1,2,18,2,std::cos(turn)*5,15,std::sin(turn)*5,48,0,angle,-turn+pi/2);
		}
	}
	else if (model == 15) {
		if (layer) return {};
		constexpr int order[6] = {0,1,2,0,2,3};
		auto append = [&](float x, float y, float z, float u, float v, float angle) {
			float ry = y * std::cos(angle) - z * std::sin(angle);
			float rz = y * std::sin(angle) + z * std::cos(angle);
			mesh.vertices.push_back({ry * 0.05625f,-(x - 4) * 0.05625f,rz * 0.05625f,u,v});
		};
		// adapt the source arrow strip UVs to the supplied refi 64x64 atlas.
		for (int plane = 0; plane < 4; ++plane) {
			for (int i : order) {
				append(i == 0 || i == 3 ? -8.f : 8.f,i < 2 ? -2.f : 2.f,0,
					i == 0 || i == 3 ? 0.f : 30.f/64,i < 2 ? 1.f/64 : 10.f/64,pi/4+float(plane)*pi/2);
			}
		}
		for (int side = 0; side < 2; ++side) {
			for (int index : order) {
				int i = side ? 3 - index : index;
				append(-7,i >= 2 ? 2.f : -2.f,i == 1 || i == 2 ? 2.f : -2.f,
					i == 1 || i == 2 ? 6.f/64 : 0.f,i >= 2 ? 18.f/64 : 12.f/64,pi/4);
			}
		}
	}
	else if (model == 16) {
		if (layer) return {};
		constexpr int order[6] = {0,1,2,0,2,3};
		for (int i : order)
			mesh.vertices.push_back({i == 0 || i == 3 ? -1.f : 1.f,0,i < 2 ? -0.5f : 1.5f,
				i == 0 || i == 3 ? 0.f : 1.f,i < 2 ? 1.f : 0.f});
	}
	return mesh.vertices;
}

}

#endif // RETROMOBMESH_HPP_
