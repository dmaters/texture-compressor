#pragma once
#include <cstdint>
#include <cstddef>

#include "common.hpp"
namespace texture_compressor {

bool compress(
	std::size_t width,
	std::size_t height,
	Format format,
	void *data,
	void *output,
	uint8_t mipmapLevels = 1
);

};  // namespace texture_compressor
