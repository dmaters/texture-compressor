#pragma once
#include <cstdlib>

#include "common.hpp"
namespace texture_compressor {

bool decompress(
	std::size_t width,
	std::size_t height,
	Format format,
	void *data,
	void *output
);
};  // namespace texture_compressor
