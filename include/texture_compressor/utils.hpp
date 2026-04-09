#pragma once
#include <cstdint>
#include <cstddef>


#include "common.hpp"
namespace texture_compressor {
std::size_t query_size(
	std::size_t width, std::size_t height, Format format, uint8_t mipmapLevels
);

};
