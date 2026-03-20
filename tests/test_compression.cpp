#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb_image.h>
#include <stb_image_write.h>

#include "texture_compressor/compression.hpp"
#include "texture_compressor/decompression.hpp"
#include "texture_compressor/utils.hpp"

int main() {
	int x, y, c;
	void* data = stbi_load("test.jpg", &x, &y, &c, 4);

	uint32_t size =
		texture_compressor::query_size(x, y, texture_compressor::Format::BC1);

	void* output = malloc(size);

	texture_compressor::compress(
		x, y, texture_compressor::Format::BC1, data, output
	);

	texture_compressor::decompress(
		x, y, texture_compressor::Format::BC1, output, data
	);

	stbi_write_png("final.png", x, y, 4, data, x * 4);

	return 0;
}
