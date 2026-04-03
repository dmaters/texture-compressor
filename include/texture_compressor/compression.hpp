#pragma once
#include <cstdint>

#include "common.hpp"
namespace texture_compressor {

uint64_t query_size(
	std::size_t width,
	std::size_t height,
	Format format,
	uint8_t mipmapLevels = 1
);

bool compress(
	std::size_t width,
	std::size_t height,
	Format format,
	void *data,
	void *output,
	uint8_t mipmapLevels = 1
);

};  // namespace texture_compressor
