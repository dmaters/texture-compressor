#pragma once
#include <array>

#include "ColorFormat.hpp"

struct BC4Block {
	std::array<R8, 2> endpoints;
	std::array<uint8_t, 6> indices;

	static BC4Block encode(const R8Block& values);

	static std::array<R8, 16> decode(const BC4Block& block);
};
