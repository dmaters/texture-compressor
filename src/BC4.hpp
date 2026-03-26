#pragma once
#include <array>

#include "ColorFormat.hpp"

struct BC4Block {
	std::array<R_8, 2> endpoints;
	std::array<uint8_t, 6> indices;

	static BC4Block encode(const std::array<R_8, 16>& values);

	static std::array<R_8, 16> decode(const BC4Block& block);
};
