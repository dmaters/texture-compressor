#pragma once
#include <cstdint>

#include "common.hpp"
namespace texture_compressor {

bool decompress(
	uint16_t width, uint16_t height, Format format, void *data, void *output
);
};  // namespace texture_compressor
