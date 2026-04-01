#include "texture_compressor/decompression.hpp"

#include "BC1.hpp"
#include "BC4.hpp"
#include "BC5.hpp"

using namespace texture_compressor;

template <
	typename DataType,
	typename BlockType,
	bool alternativeFormatting = false>
void _decompress(
	size_t width, size_t height, BlockType *data, std::byte *output
) {
	uint32_t blockCount = width * height / 16;

	for (int b = 0; b < blockCount; b++) {
		std::array<DataType, 16> values;

		int blocksPerRow = width / 4;
		int x = (b % blocksPerRow) * 4;
		int y = (b / blocksPerRow) * 4;

		if constexpr (alternativeFormatting)
			values = BlockType::decode(data[b], true);
		else
			values = BlockType::decode(data[b]);
		int baseIndex = x + y * width;

		for (int i = 0; i < 16; i++) {
			int texelIndex = baseIndex;
			uint32_t dx = i % 4;
			texelIndex += x + dx < width ? dx : width % 4;

			uint32_t dy = i / 4;
			texelIndex += y + dy < height ? dy * width : (height % 4) * width;

			texelIndex *= DataType::channels();
			for (int c = 0; c < DataType::channels(); c++) {
				output[texelIndex + c] =
					static_cast<std::byte>(values[i][c][0]);
			}
		}
	}
}

bool texture_compressor::decompress(
	std::size_t width,
	std::size_t height,
	Format format,
	void *data,
	void *output
) {
	uint32_t blockCount = width * height / 16;
	std::byte *outputData = static_cast<std::byte *>(output);

	switch (format) {
		case Format::BC1: {
			BC1Block *textureData = static_cast<BC1Block *>(data);

			_decompress<RGBA8, BC1Block>(
				width, height, textureData, outputData
			);
			break;
		}
		case Format::BC1_ALPHA: {
			BC1Block *textureData = static_cast<BC1Block *>(data);

			_decompress<RGBA8, BC1Block, true>(
				width, height, textureData, outputData

			);
			break;
		}

		case Format::BC4: {
			BC4Block *textureData = static_cast<BC4Block *>(data);

			_decompress<R8, BC4Block>(width, height, textureData, outputData);

			break;
		}
		case Format::BC5: {
			BC5Block *textureData = static_cast<BC5Block *>(data);

			_decompress<RG8, BC5Block>(width, height, textureData, outputData);
			break;
		}
	}
	return true;
}
