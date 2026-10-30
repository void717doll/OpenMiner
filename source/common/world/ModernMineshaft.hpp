#ifndef MODERNMINESHAFT_HPP_
#define MODERNMINESHAFT_HPP_

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

#include "common/world/RetroRandom.hpp"

namespace modern {

struct MineBox {
	int x0, y0, z0, x1, y1, z1;
	bool intersects(const MineBox &b) const {
		return x1 >= b.x0 && x0 <= b.x1 && y1 >= b.y0 && y0 <= b.y1 && z1 >= b.z0 && z0 <= b.z1;
	}
};

struct MinePiece {
	enum Kind { Room, Corridor, Crossing, Stairs };
	Kind kind;
	MineBox box;
	int direction = 0, depth = 0;
	bool rails = false, spiders = false;
	std::vector<MineBox> entrances;
};

// layout rules from modernmcp's MapGenMineshaft and ComponentMineshaft classes.
class Mineshaft {
	public:
		std::vector<MinePiece> pieces;

		static retro::Random originRandom(std::int64_t seed, int x, int y) {
			retro::Random random(seed);
			std::uint64_t mx = random.nextLong(), my = random.nextLong();
			std::uint64_t mixed = (std::uint64_t(std::int64_t(x)) * mx)
				^ (std::uint64_t(std::int64_t(y)) * my) ^ std::uint64_t(seed);
			return retro::Random(std::int64_t(mixed & ((1ULL << 48) - 1)));
		}

		static Mineshaft generate(std::int64_t seed, int x, int y) {
			auto random = originRandom(seed, x, y);
			random.next(32);
			Mineshaft result;
			if (random.nextDouble() >= 0.01 || random.nextInt(80) >= std::max(std::abs(x), std::abs(y))) return result;
			int rx = x * 16 + 2, ry = y * 16 + 2;
			int xmax = rx + 7 + random.nextInt(6);
			int zmax = 54 + random.nextInt(6);
			int ymax = ry + 7 + random.nextInt(6);
			result.pieces.push_back({MinePiece::Room, {rx, ry, 50, xmax, ymax, zmax}, 0, 0, false, false, {}});
			result.buildRoom(random);
			int bottom = 50, top = zmax;
			for (const auto &piece : result.pieces) {
				bottom = std::min(bottom, piece.box.z0);
				top = std::max(top, piece.box.z1);
			}
			int targetTop = top - bottom + 2;
			if (targetTop < 53) targetTop += random.nextInt(53 - targetTop);
			for (auto &piece : result.pieces) {
				piece.box.z0 += targetTop - top;
				piece.box.z1 += targetTop - top;
				for (auto &door : piece.entrances) {
					door.z0 += targetTop - top;
					door.z1 += targetTop - top;
				}
			}
			return result;
		}

	private:
		bool intersects(const MineBox &box) const {
			for (const auto &piece : pieces) if (piece.box.intersects(box)) return true;
			return false;
		}

		int add(retro::Random &random, int x, int y, int z, int direction, int parentDepth) {
			if (parentDepth > 8 || std::abs(x - pieces[0].box.x0) > 80 || std::abs(y - pieces[0].box.y0) > 80) return -1;
			int choice = random.nextInt(100);
			MinePiece piece{choice >= 80 ? MinePiece::Crossing : choice >= 70 ? MinePiece::Stairs : MinePiece::Corridor,
				{x, y, z, x, y, z + 2}, direction, parentDepth + 1, false, false, {}};
			if (piece.kind == MinePiece::Crossing) {
				if (random.nextInt(4) == 0) piece.box.z1 += 4;
				if (direction == 0 || direction == 2) {
					piece.box.x0 -= 1; piece.box.x1 += 3;
					if (direction == 0) piece.box.y1 += 4; else piece.box.y0 -= 4;
				}
				else {
					piece.box.y0 -= 1; piece.box.y1 += 3;
					if (direction == 1) piece.box.x0 -= 4; else piece.box.x1 += 4;
				}
				if (intersects(piece.box)) return -1;
			}
			else {
				int sections = piece.kind == MinePiece::Corridor ? random.nextInt(3) + 2 : 1;
				for (; sections > 0; --sections) {
					int length = piece.kind == MinePiece::Corridor ? sections * 5 : 9;
					piece.box = {x, y, z, x, y, z + 2};
					if (piece.kind == MinePiece::Stairs) piece.box.z0 -= 5;
					if (direction == 0) { piece.box.x1 += 2; piece.box.y1 += length - 1; }
					if (direction == 1) { piece.box.x0 -= length - 1; piece.box.y1 += 2; }
					if (direction == 2) { piece.box.x1 += 2; piece.box.y0 -= length - 1; }
					if (direction == 3) { piece.box.x1 += length - 1; piece.box.y1 += 2; }
					if (!intersects(piece.box)) break;
				}
				if (!sections) return -1;
				if (piece.kind == MinePiece::Corridor) {
					piece.rails = random.nextInt(3) == 0;
					piece.spiders = !piece.rails && random.nextInt(23) == 0;
				}
			}
			int index = int(pieces.size());
			pieces.push_back(piece);
			build(random, piece);
			return index;
		}

		void buildRoom(retro::Random &random) {
			const MineBox room = pieces[0].box;
			int vertical = std::max(1, room.z1 - room.z0 - 3);
			for (int side = 0; side < 4; ++side) {
				int width = side < 2 ? room.x1 - room.x0 + 1 : room.y1 - room.y0 + 1;
				for (int offset = 0; offset < width; offset += 4) {
					offset += random.nextInt(width);
					if (offset + 3 > width) break;
					int z = room.z0 + random.nextInt(vertical) + 1;
					int child = side == 0 ? add(random, room.x0 + offset, room.y0 - 1, z, 2, 0)
						: side == 1 ? add(random, room.x0 + offset, room.y1 + 1, z, 0, 0)
						: side == 2 ? add(random, room.x0 - 1, room.y0 + offset, z, 1, 0)
						: add(random, room.x1 + 1, room.y0 + offset, z, 3, 0);
					if (child < 0) continue;
					MineBox door = pieces[child].box;
					if (side == 0) { door.y0 = room.y0; door.y1 = room.y0 + 1; }
					if (side == 1) { door.y0 = room.y1 - 1; door.y1 = room.y1; }
					if (side == 2) { door.x0 = room.x0; door.x1 = room.x0 + 1; }
					if (side == 3) { door.x0 = room.x1 - 1; door.x1 = room.x1; }
					pieces[0].entrances.push_back(door);
				}
			}
		}

		void build(retro::Random &random, const MinePiece &piece) {
			const auto b = piece.box;
			int d = piece.direction, depth = piece.depth;
			if (piece.kind == MinePiece::Crossing) {
				auto exit = [&](int direction, int z) {
					if (direction == 0) add(random, b.x0 + 1, b.y1 + 1, z, 0, depth);
					if (direction == 1) add(random, b.x0 - 1, b.y0 + 1, z, 1, depth);
					if (direction == 2) add(random, b.x0 + 1, b.y0 - 1, z, 2, depth);
					if (direction == 3) add(random, b.x1 + 1, b.y0 + 1, z, 3, depth);
				};
				constexpr int exits[4][3] = {{0,1,3}, {2,0,1}, {2,1,3}, {2,0,3}};
				for (int direction : exits[d]) exit(direction, b.z0);
				if (b.z1 - b.z0 > 2)
					for (int direction : {2,1,3,0}) if (random.next(1)) exit(direction, b.z0 + 4);
				return;
			}
			int turn = piece.kind == MinePiece::Stairs ? 0 : random.nextInt(4);
			int z = b.z0 + (piece.kind == MinePiece::Stairs ? 0 : random.nextInt(3) - 1);
			if (d == 0) {
				if (turn <= 1) add(random, b.x0, b.y1 + 1, z, 0, depth);
				else add(random, turn == 2 ? b.x0 - 1 : b.x1 + 1, b.y1 - 3, z, turn == 2 ? 1 : 3, depth);
			}
			if (d == 1) {
				if (turn <= 1) add(random, b.x0 - 1, b.y0, z, 1, depth);
				else add(random, b.x0, turn == 2 ? b.y0 - 1 : b.y1 + 1, z, turn == 2 ? 2 : 0, depth);
			}
			if (d == 2) {
				if (turn <= 1) add(random, b.x0, b.y0 - 1, z, 2, depth);
				else add(random, turn == 2 ? b.x0 - 1 : b.x1 + 1, b.y0, z, turn == 2 ? 1 : 3, depth);
			}
			if (d == 3) {
				if (turn <= 1) add(random, b.x1 + 1, b.y0, z, 3, depth);
				else add(random, b.x1 - 3, turn == 2 ? b.y0 - 1 : b.y1 + 1, z, turn == 2 ? 2 : 0, depth);
			}
			if (piece.kind == MinePiece::Stairs || depth >= 8) return;
			if (d == 1 || d == 3) {
				for (int x = b.x0 + 3; x + 3 <= b.x1; x += 5) {
					int branch = random.nextInt(5);
					if (branch < 2) add(random, x, branch == 0 ? b.y0 - 1 : b.y1 + 1, b.z0, branch == 0 ? 2 : 0, depth + 1);
				}
			}
			else {
				for (int y = b.y0 + 3; y + 3 <= b.y1; y += 5) {
					int branch = random.nextInt(5);
					if (branch < 2) add(random, branch == 0 ? b.x0 - 1 : b.x1 + 1, y, b.z0, branch == 0 ? 1 : 3, depth + 1);
				}
			}
		}
};

class MineshaftCache {
	public:
		const Mineshaft &get(std::int64_t seed, int x, int y) {
			if (seed != m_seed) { m_layouts.clear(); m_seed = seed; }
			auto key = std::make_pair(x, y);
			auto found = m_layouts.find(key);
			if (found != m_layouts.end()) return found->second;
			auto layout = Mineshaft::generate(seed, x, y);
			if (layout.pieces.empty()) return m_empty;
			if (m_layouts.size() >= 128) m_layouts.clear();
			return m_layouts.emplace(key, std::move(layout)).first->second;
		}
	private:
		std::int64_t m_seed = 0;
		Mineshaft m_empty;
		std::map<std::pair<int, int>, Mineshaft> m_layouts;
};

}
#endif
