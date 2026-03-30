#pragma once
#include <array>
#include <cstdint>

#include "ColorFormat.hpp"
struct BC1Block {
	std::array<RGB565, 2> endpoints;
	uint32_t indices = 0;

	static BC1Block encode(const RGBA8Block& values, bool alphaSupport);

	static std::array<RGBA8, 16> decode(
		const BC1Block& block, bool alphaSupport
	);
};
