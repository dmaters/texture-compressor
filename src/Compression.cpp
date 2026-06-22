#include "texture_compressor/compression.hpp"

#include <cassert>
#include <cmath>

#include "BC1.hpp"
#include "BC4.hpp"
#include "BC5.hpp"

template <typename ColorType, typename BlockType>
void compress(
	std::size_t width, std::size_t height, ColorType *data, BlockType *output
) {
	std::size_t alignedWidth = (width + 3) / 4 * 4;
	std::size_t alignedHeight = (height + 3) / 4 * 4;

	std::size_t blockCount = (alignedWidth * alignedHeight) / 16;
	std::size_t blocksPerRow = alignedWidth / 4;

	for (int b = 0; b < blockCount; b++) {
		int x = (b % blocksPerRow) * 4;
		int y = (b / blocksPerRow) * 4;

		alignas(16) ColorBlock<
			typename ColorType::value_type,
			std::tuple_size_v<ColorType>>
			values;
		std::size_t baseIndex = x + y * width;

		for (int i = 0; i < 16; i++) {
			std::size_t texelIndex = baseIndex;

			uint8_t dx = i % 4;
			texelIndex += (x + dx) < width ? dx : width % 4 - 1;

			uint8_t dy = i / 4;
			texelIndex += (y + dy) < height ? dy * width
			                                : (height % 4 - 1) * width;

			for (int c = 0; c < std::tuple_size_v<ColorType>; c++) {
				values[c][i] = data[texelIndex][c];
			}
		}
		output[b] = BlockType::encode(values);
	}
}

template <typename ColorType>
void reduceMipLevel(std::size_t width, std::size_t height, ColorType *data) {
	for (std::size_t y = 0; y < height; y++) {
		for (std::size_t x = 0; x < width; x += 2) {
			std::size_t elementIndex = x + y * width;
			std::size_t endValueIndex = x / 2 + y * width;

			ColorType el1 = data[elementIndex];
			ColorType el2 = data[elementIndex + (x + 1 < width ? 1 : 0)];

			for (int c = 0; c < std::tuple_size_v<ColorType>; c++) {
				data[endValueIndex][c] = el1[c] / 2 + el2[c] / 2;
			}
		}
	}

	for (std::size_t y = 0; y < height; y += 2) {
		for (std::size_t x = 0; x < width / 2; x++) {
			std::size_t elementIndex = x + y * width;
			std::size_t endValueIndex = x + y / 2 * (width / 2);

			ColorType el1 = data[elementIndex];
			ColorType el2 = data[elementIndex + (y + 1 < height ? width : 0)];
			for (int c = 0; c < std::tuple_size_v<ColorType>; c++) {
				data[endValueIndex][c] = el1[c] / 2 + el2[c] / 2;
			}
		}
	}
}

template <typename ColorType, typename BlockType>
void compress(
	std::size_t width,
	std::size_t height,
	ColorType *data,
	BlockType *output,
	uint8_t mipLevels
) {
	std::size_t offset = 0;

	for (int m = 0; m < mipLevels; m++) {
		std::size_t mipWidth = width / pow(2, m);
		std::size_t mipHeight = height / pow(2, m);

		if (m > 0)
			reduceMipLevel<ColorType>(
				width / pow(2, m - 1), height / pow(2, m - 1), data
			);

		compress<ColorType, BlockType>(
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
		case Format::BC1_ALPHA: {
			compress<RGBA8, BC1BlockAlpha>(
				width,
				height,
				(RGBA8 *)data,
				(BC1BlockAlpha *)output,
				mipmapLevels
			);
			break;
		}
		case Format::BC1: {
			compress<RGB8, BC1Block>(
				width, height, (RGB8 *)data, (BC1Block *)output, mipmapLevels
			);
			break;
		}

		case Format::BC4: {
			compress<R8, BC4Block>(
				width, height, (R8 *)data, (BC4Block *)output, mipmapLevels
			);

			break;
		}
		case Format::BC5: {
			compress<RG8, BC5Block>(
				width, height, (RG8 *)data, (BC5Block *)output, mipmapLevels
			);
			break;
		}
	}

	return true;
}
