#include "texture_compressor/utils.hpp"

#include <cmath>
#include <cstdint>

std::size_t texture_compressor::query_size(
	std::size_t width, std::size_t height, Format format, uint8_t mipmapLevels
) {
	mipmapLevels -= 1;

	std::size_t blockSize = 0;
	std::size_t alignedWidth = (width + 3) / 4 * 4;
	std::size_t alignedHeight = (height + 3) / 4 * 4;

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
		return alignedWidth * alignedHeight / 16 * blockSize +
		       query_size(
				   alignedWidth / 2, alignedHeight / 2, format, mipmapLevels
			   );
	else
		return alignedWidth * alignedHeight / 16 * blockSize;
}
