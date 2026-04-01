#include "texture_compressor/compression.hpp"

#include <cassert>

#include "BC1.hpp"
#include "BC4.hpp"
#include "BC5.hpp"

using namespace texture_compressor;

template <
	typename DataType,
	typename BlockType,
	bool alternativeFormatting = false>
void _compress(
	size_t width, size_t height, std::byte *data, BlockType *output
) {
	int blockCount = (width * height) / 16;
	int blocksPerRow = width / 4;

	for (int b = 0; b < blockCount; b++) {
		int x = (b % blocksPerRow) * 4;
		int y = (b / blocksPerRow) * 4;

		DataType values;
		uint32_t baseIndex = x + y * width;

		for (int i = 0; i < 16; i++) {
			uint32_t texelIndex = baseIndex;

			uint32_t dx = i % 4;
			texelIndex += (x + dx) < width ? dx : width % 4 - 1;

			uint32_t dy = i / 4;
			texelIndex += (y + dy) < height ? dy * width
			                                : (height % 4 - 1) * width;

			texelIndex *= DataType::channels();
			for (int c = 0; c < DataType::channels(); c++) {
				values[c][i] = static_cast<uint8_t>(data[texelIndex + c]);
			}
		}
		output[b] = BlockType::encode(values);
	}
}

bool texture_compressor::compress(
	size_t width, size_t height, Format format, void *data, void *output
) {
	std::byte *textureData = static_cast<std::byte *>(data);

	switch (format) {
		case Format::BC1_ALPHA:
		case Format::BC1: {
			BC1Block *outputData = static_cast<BC1Block *>(output);
			_compress<RGBA8Block, BC1Block>(
				width, height, textureData, outputData
			);
			break;
		}

		case Format::BC4: {
			BC4Block *outputData = static_cast<BC4Block *>(output);

			_compress<R8Block, BC4Block>(
				width, height, textureData, outputData
			);

			break;
		}
		case Format::BC5: {
			BC5Block *outputData = static_cast<BC5Block *>(output);

			_compress<RG8Block, BC5Block>(
				width, height, textureData, outputData
			);
			break;
		}
	}

	return true;
}
