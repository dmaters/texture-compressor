#include "texture_compressor/compression.hpp"

#include "BC1.hpp"
using namespace texture_compressor;

bool texture_compressor::compress(
	uint16_t width,
	uint16_t height,
	texture_compressor::Format format,
	void *data,
	void *output
) {
	int blockCount = (width * height) / 16;

	RGBA_8 *texture = static_cast<RGBA_8 *>(data);
	BC1Block *compressedTexture = static_cast<BC1Block *>(output);

	int blocksPerRow = width / 4;
	for (int b = 0; b < blockCount; b++) {
		int x = (b % blocksPerRow) * 4;
		int y = (b / blocksPerRow) * 4;
		std::array<RGBA_8, 16> values;

		for (int i = 0; i < 16; i++) {
			int texelIndex = x + y * width;
			texelIndex += i % 4 + i / 4 * width;

			values[i] = texture[texelIndex];
		}
		compressedTexture[b] = BC1Block::encode(values);
	}

	return true;
}
