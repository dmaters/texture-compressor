#include "texture_compressor/utils.hpp"

#include <cmath>
#include <cstdint>

std::size_t texture_compressor::query_size(
	std::size_t width, std::size_t height, Format format, uint8_t mipmapLevels
) {
	mipmapLevels -= 1;

	size_t blockSize = 0;
	switch (format) {
		case Format::BC1:
		case Format::BC1_ALPHA:
		case Format::BC4:
			blockSize = 8;
			break;
		case Format::BC5:
			blockSize = 16;
			break;
		default:
			return 0;
	}
	if (mipmapLevels > 0)
		return std::ceil(width * height / 16) * blockSize +
		       query_size(width / 2, height / 2, format, mipmapLevels);
	else
		return std::ceil(width * height / 16) * blockSize;
}
