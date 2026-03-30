#include "texture_compressor/compression.hpp"

#include "BC1.hpp"
#include "BC4.hpp"

using namespace texture_compressor;

bool texture_compressor::compress(
	uint16_t width, uint16_t height, Format format, void *data, void *output
) {
	int blockCount = (width * height) / 16;

	int blocksPerRow = width / 4;
	for (int b = 0; b < blockCount; b++) {
		int x = (b % blocksPerRow) * 4;
		int y = (b / blocksPerRow) * 4;

		switch (format) {
			case Format::BC1:
			case Format::BC1_ALPHA: {
				RGBA8Block values;
				RGBA8 *texture = static_cast<RGBA8 *>(data);
				BC1Block *compressedTexture = static_cast<BC1Block *>(output);

				for (int i = 0; i < 16; i++) {
					int texelIndex = x + y * width;
					texelIndex += i % 4 + i / 4 * width;
					for (int c = 0; c < 4; c++) {
						values[c][i] = texture[texelIndex][c][0];
					}
				}
				if (format == Format::BC1)
					compressedTexture[b] = BC1Block::encode(values, false);
				else
					compressedTexture[b] = BC1Block::encode(values, true);

				break;
			}

			case Format::BC4: {
				R8Block values;
				R8 *texture = static_cast<R8 *>(data);
				BC4Block *compressedTexture = static_cast<BC4Block *>(output);

				for (int i = 0; i < 16; i++) {
					int texelIndex = x + y * width;
					texelIndex += i % 4 + i / 4 * width;

					values[0][i] = texture[texelIndex][0][0];
				}
				compressedTexture[b] = BC4Block::encode(values);
				break;
			}
		}
	}

	return true;
}
