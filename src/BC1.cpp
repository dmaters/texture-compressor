#include "BC1.hpp"

#include <smmintrin.h>  // SSE4.1
#include <stdint.h>

#include <algorithm>
#include <cstdint>
#include "ColorFormat.hpp"
struct EndpointsData {
	RGBA8 min;
	RGBA8 max;
};

EndpointsData computeEndpoints(const RGBA8Block& values) {
	RGBA8 min, max;

	uint8_t channelMin[4] = { 255, 255, 255, 255 };
	uint8_t channelMax[4] = { 0, 0, 0, 0 };
	for (int c = 0; c < 4; c++) {
		for (int b = 0; b < 16; b++) {
			uint8_t v = values[c][b];
			channelMin[c] = std::min(channelMin[c], v);
			channelMax[c] = std::max(channelMax[c], v);
		}
	}
	for (int c = 0; c < 4; c++) {
		min[c][0] = channelMin[c];
		max[c][0] = channelMax[c];
	}

	return {
		.min = min,
		.max = max,
	};
};

BC1Block BC1Block::encode(const RGBA8Block& values) {
	BC1Block block;

	auto [minEndpoint, maxEndpoint] = computeEndpoints(values);
	RGB565 minEncoded =	static_cast<RGB565>(minEndpoint);
	RGB565 maxEncoded = static_cast<RGB565>(maxEndpoint);
	bool equalEndpoints =(static_cast<RGB8>(maxEncoded) - static_cast<RGB8>(minEncoded)).lengthSquared<uint32_t>()[0] == 0;

	bool containsTransparency = minEndpoint[3][0] < 128;

	bool alphaMode = equalEndpoints || containsTransparency;

	if (alphaMode) {
		block.endpoints = {
			minEncoded,
			maxEncoded,
		};
	} else {
		block.endpoints = {
			maxEncoded,
			minEncoded,
		};
	}

	RGBA8 dir = maxEndpoint - minEndpoint;
	uint32_t dot = RGBA8::dot<uint32_t>(dir, dir)[0];
	RGBA8Block directions = values - minEndpoint;
	auto vdot = RGBA8Block::dot<uint32_t>(directions, dir);

	if (alphaMode) {
		for (int i = 0; i < 16; i++) {
			uint8_t index = 1;
			if (vdot[i] <= dot / 4)
				index = 0;
			else if (vdot[i] <= dot / 4 * 3)
				index = 2;

			block.indices |= index << (i * 2);
		}
		if (containsTransparency) {
			for (int i = 0; i < 16; i++) {
				if (values[3][i] < 128) {
					block.indices |= 3 << (i * 2);
				}
			}
		}
	} else {
		for (int i = 0; i < 16; i++) {
			uint8_t index = 0;
			if (vdot[i] <= dot / 6)
				index = 1;
			else if (vdot[i] <= dot / 2)
				index = 2;
			else if (vdot[i] <= dot / 6 * 5)
				index = 3;
			block.indices |= index << (i * 2);
		}
	}

	return block;
}

std::array<RGBA8, 16> BC1Block::decode(const BC1Block& block) {
	std::array<RGBA8, 4> values;

	values[0] = static_cast<RGBA8>(block.endpoints[0]);
	values[1] = static_cast<RGBA8>(block.endpoints[1]);

	bool alphaMode = values[0].lengthSquared<uint32_t>()[0] <=
	                 values[1].lengthSquared<uint32_t>()[0];

	if (alphaMode) {
		values[2] = (values[0] / 2 + values[1] / 2);
		values[3] = RGBA8(0);
	} else {
		values[2] = (values[0] / 3 + values[1] / 3 * 2);
		values[3] = (values[0] / 3 * 2 + values[1] / 3);
	}

	std::array<RGBA8, 16> res;

	for (int i = 0; i < 16; i++) {
		uint8_t index = (block.indices >> (i * 2)) & 0b11;

		res[i] = values[index];

		if (alphaMode && index == 3) {
			continue;
		}
		res[i][3][0] = 255;
	}

	return res;
}
