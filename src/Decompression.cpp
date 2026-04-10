#include "texture_compressor/decompression.hpp"

#include "BC1.hpp"
#include "BC4.hpp"
#include "BC5.hpp"

template <template <std::size_t> typename DataType, typename BlockType>
void decompress(
	std::size_t width, std::size_t height, BlockType *data, DataType<1> *output
) {
	std::size_t alignedWidth = (width + 3) / 4 * 4;
	std::size_t alignedHeight = (height + 3) / 4 * 4;

	std::size_t blockCount = (alignedWidth * alignedHeight) / 16;
	std::size_t blocksPerRow = alignedWidth / 4;

	for (int b = 0; b < blockCount; b++) {
		std::size_t x = (b % blocksPerRow) * 4;
		std::size_t y = (b / blocksPerRow) * 4;

		std::array<DataType<1>, 16> values = BlockType::decode(data[b]);

		std::size_t baseIndex = x + y * width;

		for (int i = 0; i < 16; i++) {
			std::size_t texelIndex = baseIndex;
			uint8_t dx = i % 4;
			texelIndex += x + dx < width ? dx : (width % 4) - 1;

			uint8_t dy = i / 4;
			texelIndex += y + dy < height ? dy * width
			                              : (height % 4 - 1) * width;

			output[texelIndex] = values[i];
		}
	}
}

template <template <std::size_t> typename DataType, typename BlockType>
void decompress(
	std::size_t width,
	std::size_t height,
	BlockType *data,
	DataType<1> *output,
	uint8_t mipmaps
) {
	std::size_t offset = 0;
	std::size_t alignedOffset = 0;

	for (int i = 0; i < mipmaps; i++) {
		std::size_t mipWidth = width / pow(2, i);
		std::size_t mipHeight = height / pow(2, i);

		decompress<DataType, BlockType>(
			mipWidth, mipHeight, data + alignedOffset / 16, output + offset
		);
		std::size_t alignedWidth = (mipWidth + 3) / 4 * 4;
		std::size_t alignedHeight = (mipHeight + 3) / 4 * 4;
		alignedOffset += alignedWidth * alignedHeight;

		offset += mipWidth * mipHeight;
	}
}

bool texture_compressor::decompress(
	std::size_t width,
	std::size_t height,
	Format format,
	void *data,
	void *output,
	uint8_t mipmaps
) {
	uint32_t blockCount = width * height / 16;

	switch (format) {
		case Format::BC1_ALPHA:
		case Format::BC1:
			decompress<RGBA8n, BC1Block>(
				width, height, (BC1Block *)data, (RGBA8 *)output, mipmaps
			);
			break;

		case Format::BC4: {
			decompress<R8n, BC4Block>(
				width, height, (BC4Block *)data, (R8 *)output, mipmaps
			);

			break;
		}
		case Format::BC5: {
			BC5Block *textureData = static_cast<BC5Block *>(data);

			decompress<RG8n, BC5Block>(
				width, height, (BC5Block *)data, (RG8 *)output, mipmaps
			);
			break;
		}
	}
	return true;
}
