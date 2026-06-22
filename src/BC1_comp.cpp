#include "BC1_comp.hpp"

#include <algorithm>

RGB565 rgb8_to_rgb565(RGB8 value) {
	RGB565 res =
		((value[0] >> 3) << 11) | ((value[1] >> 2) << 5) | (value[2] >> 3);
	return res;
}

RGB8 rgb565_to_rgb8(RGB565 value) {
	RGB8 res;
	uint8_t r = (value >> 11) & 0x1F;
	uint8_t g = (value >> 5) & 0x3F;
	uint8_t b = (value >> 0) & 0x1F;

	res[0] = (r << 3) | (r >> 2);
	res[1] = (g << 2) | (g >> 4);
	res[2] = (b << 3) | (b >> 2);
	return res;
}

RGB8 lerp(RGB8 v1, RGB8 v2, float ratio) {
	RGB8 res;

	res[0] = v1[0] + (v2[0] - v1[0]) * ratio;
	res[1] = v1[1] + (v2[1] - v1[1]) * ratio;
	res[2] = v1[2] + (v2[2] - v1[2]) * ratio;

	return res;
}

uint32_t dot(RGB8 v1, RGB8 v2) {
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

EndpointsData computeEndpointsMinMax(const RGB8Block& values) {
	RGB8 min, max;

	for (int c = 0; c < 3; c++) {
		min[c] = *std::min_element(values[c].begin(), values[c].end());
		max[c] = *std::max_element(values[c].begin(), values[c].end());
	}

	return { .min = min, .max = max };
};
