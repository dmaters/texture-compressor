#include "BC1.hpp"

#include <algorithm>

struct EndpointsData {
	RGB_16F min;
	RGB_16F max;
	bool containsTransparency;
};
EndpointsData inline computeEndpoints(
	const std::array<RGBA_8, 16>& values, bool alphaSupport
) {
	RGBA_8 min(values[0]);
	RGBA_8 max(values[0]);

	bool containsTransparency = false;
	for (int i = 1; i < 16; i++) {
		if (alphaSupport && values[i].a < 128) {
			containsTransparency = true;
			continue;
		}

		min.r = std::min<uint8_t>(values[i].r, min.r);
		max.r = std::max<uint8_t>(values[i].r, max.r);
		min.g = std::min<uint8_t>(values[i].g, min.g);
		max.g = std::max<uint8_t>(values[i].g, max.g);
		min.b = std::min<uint8_t>(values[i].b, min.b);
		max.b = std::max<uint8_t>(values[i].b, max.b);
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

	RGB_16F midPoint = minEndpoint / 2.0f + maxEndpoint / 2.0f;
	RGB_16F first3rd = minEndpoint / 3.0f + maxEndpoint / (3.0f * 2.0f);
	RGB_16F second3rd = minEndpoint / (3.0f * 2.0f) + maxEndpoint / 3.0f;

	for (int i = 0; i < 16; i++) {
		RGB_16F value = static_cast<RGB_16F>(values[i]);

		if (alphaSupport && containsTransparency) {
			if (values[i].a < 128) {
				block.indices |= 3 << (i * 2);
				continue;
			}
			std::array<float, 3> distances = {
				(minEndpoint - value).lengthSquared(),
				(maxEndpoint - value).lengthSquared(),
				(midPoint - value).lengthSquared(),
			};

			uint8_t index =
				std::min_element(distances.begin(), distances.end()) -
				distances.begin();

			block.indices |= index << (i * 2);
			continue;
		}

		std::array<float, 4> distances = {
			(maxEndpoint - value).lengthSquared(),
			(minEndpoint - value).lengthSquared(),
			(first3rd - value).lengthSquared(),
			(second3rd - value).lengthSquared(),
		};

		uint8_t index = std::min_element(distances.begin(), distances.end()) -
		                distances.begin();

		block.indices |= index << (i * 2);
	}
	return block;
}

std::array<RGBA_8, 16> BC1Block::decode(
	const BC1Block& block, bool alphaSupport
) {
	std::array<RGBA_8, 4> values;

	bool containsTransparency = block.endpoints[0].lengthSquared() <=
	                            block.endpoints[1].lengthSquared();

	values[0] = static_cast<RGBA_8>(block.endpoints[0]);
	values[1] = static_cast<RGBA_8>(block.endpoints[1]);

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

		res[i].a = 255;
	}

	return res;
}
