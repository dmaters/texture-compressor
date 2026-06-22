#pragma once
#include <array>
#include <cstdint>

#include "ColorFormat.hpp"

struct EndpointsData {
	RGB8 min;
	RGB8 max;
	bool alternativeMode = false;
};

std::array<int8_t, 16> computeIndices(
	const RGB8& min,
	const RGB8& max,
	const RGB8Block& values,
	const uint8_t nodeCount
);

EndpointsData computeEndpointsMinMax(const RGB8Block& values);
EndpointsData computeEndpointsPCA(const RGB8Block& values);

RGB565 rgb8_to_rgb565(RGB8 value);
RGB8 rgb565_to_rgb8(RGB565 value);
RGB8 lerp(RGB8 v1, RGB8 v2, float ratio);
uint32_t dot(RGB8 v1, RGB8 v2);
