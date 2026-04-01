#include "texture_compressor/utils.hpp"

#include <cmath>
#include <cstdint>

std::size_t texture_compressor::query_size(
	std::size_t width, std::size_t height, Format format
) {
	switch (format) {
		case Format::BC1:
		case Format::BC1_ALPHA:
		case Format::BC4:
			return std::ceil((width * height) / 16) * 8;
		case Format::BC5:
			return std::ceil((width * height) / 16) * 8 * 2;
		default:
			return 0;
	}
}
