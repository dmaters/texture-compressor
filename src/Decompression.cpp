#include "texture_compressor/decompression.hpp"

#include "BC1.hpp"
#include "BC4.hpp"
using namespace texture_compressor;

bool texture_compressor::decompress(
	uint16_t width, uint16_t height, Format format, void *data, void *output
) {
	uint32_t blockCount = width * height / 16;

	for (int b = 0; b < blockCount; b++) {
		int blocksPerRow = width / 4;
		int x = (b % blocksPerRow) * 4;
		int y = (b / blocksPerRow) * 4;

		switch (format) {
			case Format::BC1:
			case Format::BC1_ALPHA: {
				std::array<RGBA_8, 16> values;
				BC1Block *inputBuffer = static_cast<BC1Block *>(data);
				RGBA_8 *outputBuffer = static_cast<RGBA_8 *>(output);
				if (format == Format::BC1)
					values = BC1Block::decode(inputBuffer[b], false);
				else
					values = BC1Block::decode(inputBuffer[b], true);

				for (int i = 0; i < 16; i++) {
					int texelIndex = x + y * width;
					texelIndex += i % 4 + i / 4 * width;
					outputBuffer[texelIndex] = values[i];
				}
				break;
			}
			case Format::BC4: {
				std::array<R_8, 16> values;
				BC4Block *inputBuffer = static_cast<BC4Block *>(data);
				R_8 *outputBuffer = static_cast<R_8 *>(output);
				values = BC4Block::decode(inputBuffer[b]);

				for (int i = 0; i < 16; i++) {
					int texelIndex = x + y * width;
					texelIndex += i % 4 + i / 4 * width;
					outputBuffer[texelIndex] = values[i];
				}
				break;
			}
		}
	}
	return true;
}
