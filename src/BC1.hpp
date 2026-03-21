#pragma once
#include <array>
#include <cstdint>

#include "ColorFormat.hpp"
struct BC1Block {
	std::array<RGB_565, 2> endpoints;
	uint32_t indices = 0;

	static BC1Block encode(
		const std::array<RGBA_8, 16>& values, bool alphaSupport
	);

	static std::array<RGBA_8, 16> decode(
		const BC1Block& block, bool alphaSupport
	);
};
