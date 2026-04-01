#pragma once
#include <cstdint>

#include "common.hpp"
namespace texture_compressor {

uint64_t query_size(std::size_t width, std::size_t height, Format format);

bool compress(
	std::size_t width,
	std::size_t height,
	Format format,
	void *data,
	void *output
);

};  // namespace texture_compressor
