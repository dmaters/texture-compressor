#include "texture_compressor/compression.hpp"

#include <cassert>

#include "BC1.hpp"
#include "BC4.hpp"
#include "BC5.hpp"

using namespace texture_compressor;

template <template <std::size_t> typename DataType, typename BlockType>
void _compressMipLevel(
    std::size_t width, std::size_t height, DataType<1> *data, BlockType *output
) {
	int blockCount = (width * height) / 16;
	int blocksPerRow = width / 4;

	for (int b = 0; b < blockCount; b++) {
		int x = (b % blocksPerRow) * 4;
		int y = (b / blocksPerRow) * 4;

		DataType<16> values;
		std::size_t baseIndex = x + y * width;

		for (int i = 0; i < 16; i++) {
			std::size_t texelIndex = baseIndex;

			uint8_t dx = i % 4;
			texelIndex += (x + dx) < width ? dx : width % 4 - 1;

			uint8_t dy = i / 4;
			texelIndex += (y + dy) < height ? dy * width
			                                : (height % 4 - 1) * width;

			for (int c = 0; c < DataType<1>::channels(); c++) {
				values[c][i] = data[texelIndex][c][0];
			}
		}
		output[b] = BlockType::encode(values);
	}
}

template <template <std::size_t> typename DataType>
void _reduceMipLevel(std::size_t width, std::size_t height, DataType<1> *data) {
	int blockCount = (width * height) / 4;
	int blocksPerRow = width / 2;

	for (int b = 0; b < blockCount; b++) {
		int x = (b % blocksPerRow) * 2;
		int y = (b / blocksPerRow) * 2;
		DataType<1> endValue(0);
		uint32_t baseIndex = x + y * width;

		for (int i = 0; i < 4; i++) {
			uint32_t texelIndex = baseIndex;

			uint32_t dx = i % 2;
			texelIndex += (x + dx) < width ? dx : width % 2 - 1;

			uint32_t dy = i / 2;
			texelIndex += (y + dy) < height ? dy * width
			                                : (height % 2 - 1) * width;

			texelIndex *= DataType<1>::channels();
			endValue = endValue + (data[texelIndex] / 4);
			data[baseIndex] = endValue;
		}
	}
}

std::size_t computeOffset(std::size_t width, std::size_t height, std::size_t mipmap) {
	if (mipmap > 0)
		return (width * height / 16) +
		       computeOffset(width / 2, height / 2, mipmap - 1);
	else
		return ((width * height) / 16);
}

template <template <std::size_t> typename DataType, typename BlockType>
void _compress(
	std::size_t width,
	std::size_t height,
	DataType<1> *data,
	BlockType *output,
	uint8_t mipLevels
) {
	_compressMipLevel<DataType, BlockType>(width, height, data, output);  // 0

	for (int m = 1; m < mipLevels; m++) {
		std::size_t mipWidth = width / (2 * m);
		std::size_t mipHeight = height / (2 * m);

		std::size_t offset = computeOffset(mipWidth, mipHeight, m - 1);
		_reduceMipLevel<DataType>(mipWidth, mipHeight, data);
		_compressMipLevel<DataType, BlockType>(
			mipWidth, mipHeight, data, output + offset
		);
	}
}
bool texture_compressor::compress(
	std::size_t width,
	std::size_t height,
	texture_compressor::Format format,
	void *data,
	void *output,
	uint8_t mipmapLevels
) {
	switch (format) {
		case Format::BC1_ALPHA:
		case Format::BC1: {
			_compress<RGBA8n, BC1Block>(
				width, height, (RGBA8 *)data, (BC1Block *)output, mipmapLevels
			);
			break;
		}

		case Format::BC4: {
			_compress<R8n, BC4Block>(
				width, height, (R8 *)data, (BC4Block *)output, mipmapLevels
			);

			break;
		}
		case Format::BC5: {
			_compress<RG8n, BC5Block>(
				width, height, (RG8 *)data, (BC5Block *)output, mipmapLevels
			);
			break;
		}
	}

	return true;
}
