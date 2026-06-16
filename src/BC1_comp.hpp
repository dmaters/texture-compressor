#pragma once
#include <array>
#include <cstdint>

#include "ColorFormat.hpp"

std::array<int8_t, 16> computeIndices(
	const RGB8& min,
	const RGB8& max,
	const RGB8Block& values,
	const uint8_t nodeCount
);
