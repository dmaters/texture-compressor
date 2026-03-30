#include "BC4.hpp"

struct Endpoints {
	uint8_t min;
	uint8_t max;
};

Endpoints computeEndpoints(const R8Block& values) {
	uint8_t min = 255;
	uint8_t max = 0;

	for (int i = 0; i < 16; i++) {
		uint8_t v = values[0][i];
		min = std::min(min, v);
		max = std::max(max, v);
	}

	return {
		.min = min,
		.max = max,
	};
}

BC4Block BC4Block::encode(const R8Block& values) {
	BC4Block block;

	auto [minEndpoint, maxEndpoint] = computeEndpoints(values);
	block.endpoints = { maxEndpoint, minEndpoint };
	uint64_t indices = 0;

	if (minEndpoint < maxEndpoint) {
		int range = maxEndpoint - minEndpoint;
		int bias = (range < 8) ? (range - 1) : (range / 2 + 2);
		uint8_t indexMap[8] = { 1, 7, 6, 5, 4, 3, 2, 0 };

		for (int i = 0; i < 16; i++) {
			uint8_t v = values[0][i];
			uint8_t index = ((v - minEndpoint) * 7 + bias) / range;
			indices |= static_cast<uint64_t>(indexMap[index]) << (i * 3);
		}
	}

	for (int i = 0; i < 6; i++) {
		block.indices[i] = (indices >> (i * 8)) & 0xFF;
	}

	return block;
}

std::array<R8, 16> BC4Block::decode(const BC4Block& block) {
	std::array<R8, 8> values {
		block.endpoints[0],
		block.endpoints[1],
		R8::lerp(block.endpoints[1], block.endpoints[0], 255 / 7 * 6),
		R8::lerp(block.endpoints[1], block.endpoints[0], 255 / 7 * 5),
		R8::lerp(block.endpoints[1], block.endpoints[0], 255 / 7 * 4),
		R8::lerp(block.endpoints[1], block.endpoints[0], 255 / 7 * 3),
		R8::lerp(block.endpoints[1], block.endpoints[0], 255 / 7 * 2),
		R8::lerp(block.endpoints[1], block.endpoints[0], 255 / 7 * 1),
	};

	uint64_t indices = 0;
	for (int i = 0; i < 6; i++) {
		indices |= static_cast<uint64_t>(block.indices[i]) << (i * 8);
	}

	std::array<R8, 16> res;
	for (int i = 0; i < 16; i++) {
		uint8_t index = (indices >> (i * 3)) & 0b111;
		res[i] = values[index];
	}

	return res;
}
