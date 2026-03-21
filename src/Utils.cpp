#include "texture_compressor/utils.hpp"

#include <cstdint>
uint64_t texture_compressor::query_size(
	uint16_t width, uint16_t height, Format format
) {
	switch (format) {
		case Format::BC1:
		case Format::BC1_ALPHA:
			return ((width * height) / 16) * 8;
		default:
			return 0;
	}
}
