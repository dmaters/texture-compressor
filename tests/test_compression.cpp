#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb_image.h>
#include <stb_image_write.h>

#include <format>

#include "texture_compressor/compression.hpp"
#include "texture_compressor/decompression.hpp"
#include "texture_compressor/utils.hpp"

int main() {
	int x, y, c;
	void* data = stbi_load("test.png", &x, &y, &c, 4);
	std::size_t width, height;
	width = static_cast<std::size_t>(x);
	height = static_cast<std::size_t>(y);
	std::size_t mips = floor(log2(std::min(width, height)) + 1) - 3;

	std::size_t size = texture_compressor::query_size(
		width, height, texture_compressor::Format::BC1_ALPHA, mips
	);

	void* output = malloc(size);

	texture_compressor::compress(
		x, y, texture_compressor::Format::BC1_ALPHA, data, output, mips
	);

	void* outputData = malloc(
		texture_compressor::query_size(
			width, height, texture_compressor::Format::BC1_ALPHA, mips
		) *
		16
	);

	texture_compressor::decompress(
		x, y, texture_compressor::Format::BC1_ALPHA, output, outputData, mips
	);
	std::size_t baseOffset = 0;
	for (int i = 0; i < mips; i++) {
		std::size_t mipWidth = width / pow(2, i);
		std::size_t mipHeight = height / pow(2, i);

		stbi_write_png(
			std::format("final_mip{}.png", i).c_str(),
			mipWidth,
			mipHeight,
			4,
			static_cast<char*>(outputData) + baseOffset,
			0
		);

		baseOffset += mipWidth * mipHeight * 4;
	}

	return 0;
}
