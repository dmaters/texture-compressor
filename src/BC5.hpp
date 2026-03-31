#pragma once
#include "BC4.hpp"

struct BC5Block {
	BC4Block channel1;
	BC4Block channel2;

	static BC5Block encode(const std::array<RG8, 16>& values);
	static std::array<RG8, 16> decode(const BC5Block& block);
};
