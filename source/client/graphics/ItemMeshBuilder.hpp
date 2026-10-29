#ifndef ITEMMESHBUILDER_HPP_
#define ITEMMESHBUILDER_HPP_

#include <string>
#include <vector>
#include "client/graphics/Vertex.hpp"

class Item;
class TextureAtlas;

std::vector<Vertex> buildItemMesh(const Item &item, const TextureAtlas &atlas,
	const std::string &filename, bool blockTexture, float size);

#endif // ITEMMESHBUILDER_HPP_
