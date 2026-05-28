#pragma once
#include <array>
#include <cstdint>

#include "ColorFormat.hpp"
#include "texture_compressor/common.hpp"

struct BC1Block {
	std::array<RGB565, 2> endpoints;
	uint32_t indices = 0;

	static BC1Block encode(const RGB8Block& values);
	static std::array<RGB8, 16> decode(const BC1Block& block);
};
struct BC1BlockAlpha : BC1Block {
	static BC1BlockAlpha encode(const RGBA8Block& values);
	static std::array<RGBA8, 16> decode(const BC1BlockAlpha& block);
};
