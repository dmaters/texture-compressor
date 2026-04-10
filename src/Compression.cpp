#include "texture_compressor/compression.hpp"

#include <cassert>

#include "BC1.hpp"
#include "BC4.hpp"
#include "BC5.hpp"

template <template <std::size_t> typename DataType, typename BlockType>
void compress(
	std::size_t width, std::size_t height, DataType<1> *data, BlockType *output
) {
	std::size_t alignedWidth = (width + 3) / 4 * 4;
	std::size_t alignedHeight = (height + 3) / 4 * 4;

	std::size_t blockCount = (alignedWidth * alignedHeight) / 16;
	std::size_t blocksPerRow = alignedWidth / 4;

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
void reduceMipLevel(std::size_t width, std::size_t height, DataType<1> *data) {
	for (std::size_t y = 0; y < height; y++) {
		for (std::size_t x = 0; x < width; x += 2) {
			std::size_t elementIndex = x + y * width;
			std::size_t endValueIndex = x / 2 + y * width;

			DataType<1> el1 = data[elementIndex];
			DataType<1> el2 = data[elementIndex + (x + 1 < width ? 1 : 0)];

			data[endValueIndex] = el1 / 2 + el2 / 2;
		}
	}

	for (std::size_t y = 0; y < height; y += 2) {
		for (std::size_t x = 0; x < width / 2; x++) {
			std::size_t elementIndex = x + y * width;
			std::size_t endValueIndex = x + y / 2 * (width / 2);

			DataType<1> el1 = data[elementIndex];
			DataType<1> el2 = data[elementIndex + (y + 1 < height ? width : 0)];

			data[endValueIndex] = el1 / 2 + el2 / 2;
		}
	}
}

template <template <std::size_t> typename DataType, typename BlockType>
void compress(
	std::size_t width,
	std::size_t height,
	DataType<1> *data,
	BlockType *output,
	uint8_t mipLevels
) {
	std::size_t offset = 0;

	for (int m = 0; m < mipLevels; m++) {
		std::size_t mipWidth = width / pow(2, m);
		std::size_t mipHeight = height / pow(2, m);

		if (m > 0)
			reduceMipLevel<DataType>(
				width / pow(2, m - 1), height / pow(2, m - 1), data
			);

		compress<DataType, BlockType>(
			mipWidth, mipHeight, data, output + offset
		);
		std::size_t alignedWidth = (mipWidth + 3) / 4 * 4;
		std::size_t alignedHeight = (mipHeight + 3) / 4 * 4;

		offset += alignedWidth * alignedHeight / 16;
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
			compress<RGBA8n, BC1Block>(
				width, height, (RGBA8 *)data, (BC1Block *)output, mipmapLevels
			);
			break;
		}

		case Format::BC4: {
			compress<R8n, BC4Block>(
				width, height, (R8 *)data, (BC4Block *)output, mipmapLevels
			);

			break;
		}
		case Format::BC5: {
			compress<RG8n, BC5Block>(
				width, height, (RG8 *)data, (BC5Block *)output, mipmapLevels
			);
			break;
		}
	}

	return true;
}
