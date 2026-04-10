#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "common.hpp"
namespace texture_compressor {

bool decompress(
	std::size_t width,
	std::size_t height,
	Format format,
	void *data,
	void *output,
	uint8_t mipmaps = 1
);
};  // namespace texture_compressor
