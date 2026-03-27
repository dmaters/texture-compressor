#include "BC1.hpp"

#include <algorithm>

struct EndpointsData {
	RGB_8 min;
	RGB_8 max;
	bool containsTransparency;
};
EndpointsData inline computeEndpoints(
	const std::array<RGBA_8, 16>& values, bool alphaSupport
) {
	RGB_8 min(values[0]);
	RGB_8 max(values[0]);

	bool containsTransparency = false;
	for (int i = 1; i < 16; i++) {
		if (alphaSupport && values[i].a() < 128) {
			containsTransparency = true;
			continue;
		}

		for (int c = 0; c < 3; c++) {
			min[c] = std::min<uint8_t>(values[i][c], min[c]);
			max[c] = std::max<uint8_t>(values[i][c], max[c]);
		}
	}

	return {
		.min = static_cast<RGB_16F>(min),
		.max = static_cast<RGB_16F>(max),
		.containsTransparency = containsTransparency,
	};
};

BC1Block BC1Block::encode(
	const std::array<RGBA_8, 16>& values, bool alphaSupport
) {
	BC1Block block;
	auto [minEndpoint, maxEndpoint, containsTransparency] =
		computeEndpoints(values, alphaSupport);

	if (alphaSupport && containsTransparency) {
		block.endpoints = {
			static_cast<RGB_565>(minEndpoint),
			static_cast<RGB_565>(maxEndpoint),
		};
	} else {
		block.endpoints = {
			static_cast<RGB_565>(maxEndpoint),
			static_cast<RGB_565>(minEndpoint),
		};
	}

	RGB_8 dir = maxEndpoint - minEndpoint;
	float dot = RGB_8::dot<uint32_t>(dir, dir);

	for (int i = 0; i < 16; i++) {
		RGB_8 value = static_cast<RGB_8>(values[i]) - minEndpoint;
		float vdot = RGB_8::dot<uint32_t>(value, dir);

		if (alphaSupport && containsTransparency) {
			if (values[i].a() < 128) {
				block.indices |= 3 << (i * 2);
				continue;
			}
			uint8_t index = 1;
			if (vdot <= dot / 4)
				index = 0;
			else if (vdot <= dot / 4 * 3)
				index = 2;

			block.indices |= index << (i * 2);
			continue;
		}

		uint8_t index = 0;
		if (vdot <= dot / 6)
			index = 1;
		else if (vdot <= dot / 2)
			index = 2;
		else if (vdot <= dot / 6 * 5)
			index = 3;
		block.indices |= index << (i * 2);
	}
	return block;
}

std::array<RGBA_8, 16> BC1Block::decode(
	const BC1Block& block, bool alphaSupport
) {
	std::array<RGBA_8, 4> values;

	values[0] = static_cast<RGBA_8>(block.endpoints[0]);
	values[1] = static_cast<RGBA_8>(block.endpoints[1]);

	bool containsTransparency =
		values[0].lengthSquared() <= values[1].lengthSquared();

	if (alphaSupport && containsTransparency) {
		values[2] = (values[0] / 2 + values[1] / 2);
		values[3] = RGBA_8(0);
	} else {
		values[2] = (values[0] / 3 + values[1] / 3 * 2);
		values[3] = (values[0] / 3 * 2 + values[1] / 3);
	}

	std::array<RGBA_8, 16> res;

	for (int i = 0; i < 16; i++) {
		uint8_t index = (block.indices >> (i * 2)) & 0b11;

		res[i] = values[index];

		if (alphaSupport && containsTransparency && index == 3) {
			continue;
		}

		res[i][3] = 255;
	}

	return res;
}
