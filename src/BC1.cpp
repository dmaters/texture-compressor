#include "BC1.hpp"

#include <algorithm>

std::array<RGB_16F, 2> inline computeEndpoints(std::array<RGBA_8, 16> values) {
	RGBA_8 min(255);
	RGBA_8 max(0);

	for (int i = 0; i < 16; i++) {
		min.r = std::min<uint8_t>(values[i].r, min.r);
		max.r = std::max<uint8_t>(values[i].r, max.r);
		min.g = std::min<uint8_t>(values[i].g, min.g);
		max.g = std::max<uint8_t>(values[i].g, max.g);
		min.b = std::min<uint8_t>(values[i].b, min.b);
		max.b = std::max<uint8_t>(values[i].b, max.b);
	}

	return { static_cast<RGB_16F>(min), static_cast<RGB_16F>(max) };
};

BC1Block BC1Block::encode(const std::array<RGBA_8, 16>& values) {
	BC1Block block;
	std::array<RGB_16F, 2> endpoints = computeEndpoints(values);

	block.endpoints = {
		static_cast<RGB_565>(endpoints[0]),
		static_cast<RGB_565>(endpoints[1]),
	};

	RGB_16F localSpaceDir = endpoints[1] - endpoints[0];

	for (int i = 0; i < 16; i++) {
		RGB_16F value = static_cast<RGB_16F>(values[i]);
		RGB_16F offset = value - endpoints[0];

		float dot = 0, lenSq = 0;
		for (int a = 0; a < 3; a++) {
			dot += offset[a] * localSpaceDir[a];
			lenSq += localSpaceDir[a] * localSpaceDir[a];
		}

		float t = dot / lenSq;
		t = std::clamp(t, 0.0f, 1.0f);
		uint8_t quarter = static_cast<uint8_t>(t * 3.0f + 0.5f);

		block.indices |= quarter << (i * 2);
	}
	return block;
}
std::array<RGBA_8, 16> BC1Block::decode(const BC1Block& block) {
	std::array<RGBA_8, 4> values;
	values[0] = static_cast<RGBA_8>(block.endpoints[0]);
	values[3] = static_cast<RGBA_8>(block.endpoints[1]);

	values[1] = (values[0] / 3 + values[3] / 3 * 2);
	values[2] = (values[0] / 3 * 2 + values[3] / 3);

	std::array<RGBA_8, 16> res;

	for (int i = 0; i < 16; i++) {
		uint8_t index = (block.indices >> (i * 2)) & 0b11;
		res[i] = values[index];
		res[i].a = 255;
	}

	return res;
}
