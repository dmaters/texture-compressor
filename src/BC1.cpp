#include "BC1.hpp"

#include <algorithm>
#include <cstdint>

#include "ColorFormat.hpp"
struct EndpointsData {
	RGBA8 min;
	RGBA8 max;
};

EndpointsData computeEndpoints(const RGBA8Block& values) {
	RGBA8 min, max;

	std::uint_fast32_t channelMin[4];
	std::uint_fast32_t channelMax[4];
	for (int c = 0; c < 4; c++) {
		channelMin[c] = *std::min_element(values[c].begin(), values[c].end());
		channelMax[c] = *std::max_element(values[c].begin(), values[c].end());
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
	RGB565 minEncoded = static_cast<RGB565>(minEndpoint);
	RGB565 maxEncoded = static_cast<RGB565>(maxEndpoint);

	RGBA8 diff = maxEndpoint - minEndpoint;
	bool equalEndpoints = diff[0][0] < 8 && diff[1][0] < 16 && diff[2][0] < 8;

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

	std::array<uint32_t, 16> indices;
	if (alphaMode) {
		const std::uint_fast32_t nodes[2] = { dot / 4, dot / 4 * 3 };

		for (int i = 0; i < indices.size(); i++) {
			indices[i] = (vdot[i] <= nodes[0]) + (vdot[i] <= nodes[1]);
			indices[i] = (indices[i] + 1) % 3;
		}
		if (containsTransparency) {
			for (int i = 0; i < 16; i++) {
				if (values[3][i] < 128) {
					indices[i] = 3 << (i * 2);
				}
			}
		}
	} else {
		const std::uint_fast32_t nodes[3] = {
			dot / 6,
			dot / 6 * 3,
			dot / 6 * 5,
		};

		for (int i = 0; i < indices.size(); i++) {
			indices[i] = (vdot[i] >= nodes[0]) + (vdot[i] >= nodes[1]) +
			             (vdot[i] >= nodes[2]);
			indices[i] = (indices[i] + 1) % 4;
		}
	}

	for (int i = 0; i < indices.size(); i++)
		block.indices |= indices[i] << (i * 2);

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

	for (int i = 0; i < res.size(); i++) {
		std::uint_fast32_t index = (block.indices >> (i * 2)) & 0b11;

		res[i] = values[index];

		if (alphaMode && index == 3) {
			continue;
		}
		res[i][3][0] = 255;
	}

	return res;
}
