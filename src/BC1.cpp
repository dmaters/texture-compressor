#include "BC1.hpp"

#include <array>
#include <cstdint>

#include "BC1_comp.hpp"
#include "ColorFormat.hpp"

template <typename ColorBlock>
BC1Block encodeBlock(const ColorBlock& values) {
	BC1Block block;
	bool alternativeMode = false;
	EndpointsData endpointsData;

	RGB8Block colorBlock;
	colorBlock[0] = values[0];
	colorBlock[1] = values[1];
	colorBlock[2] = values[2];

	if constexpr (std::is_same_v<RGBA8Block, ColorBlock>) {
		for (int i = 0; i < 16; i++) {
			alternativeMode |= values[3][i] < 128;
		}

		if (alternativeMode)
			endpointsData = computeEndpointsMinMax(colorBlock);
		else
			endpointsData = computeEndpointsPCA(colorBlock);

	} else
		endpointsData = computeEndpointsPCA(values);

	auto [minEndpoint, maxEndpoint, mode] = endpointsData;

	alternativeMode |= mode;

	RGB565 minEncoded = rgb8_to_rgb565(minEndpoint);
	RGB565 maxEncoded = rgb8_to_rgb565(maxEndpoint);

	alternativeMode |= minEncoded >= maxEncoded;
	std::array<int8_t, 16> indices;
	if (alternativeMode) {
		block.endpoints[0] = minEncoded;
		block.endpoints[1] = maxEncoded;
		const std::array<uint8_t, 4> indexMap = { 0, 2, 1 };
		indices = computeIndices(minEndpoint, maxEndpoint, colorBlock, 2);

		if constexpr (std::is_same_v<RGBA8Block, ColorBlock>) {
			for (int i = 0; i < 16; i++) {
				if (values[3][i] < 128) {
					indices[i] = 3;
				}
			}
		} else {
			for (int i = 0; i < 16; i++) {
				uint16_t sum = values[0][i] + values[1][i] + values[2][i];
				if (sum == 0) {
					indices[i] = 3;
				}
			}
		}
	} else {
		block.endpoints[0] = maxEncoded;
		block.endpoints[1] = minEncoded;

		indices = computeIndices(minEndpoint, maxEndpoint, colorBlock, 3);
		for (int i = 0; i < indices.size(); i++) {
			indices[i] = (indices[i] + 1) % 4;
		}
	}

	for (int i = 0; i < indices.size(); i++)
		block.indices |= indices[i] << (i * 2);

	return block;
}

template <typename ColorType>
std::array<ColorType, 16> decodeBlock(const BC1Block& block) {
	std::array<ColorType, 4> values;
	RGB8 e0 = rgb565_to_rgb8(block.endpoints[0]);
	RGB8 e1 = rgb565_to_rgb8(block.endpoints[1]);

	values[0][0] = e0[0];
	values[0][1] = e0[1];
	values[0][2] = e0[2];

	values[1][0] = e1[0];
	values[1][1] = e1[1];
	values[1][2] = e1[2];

	bool extendedMode = block.endpoints[0] <= block.endpoints[1];

	if (extendedMode) {
		RGB8 v2 = lerp(e0, e1, 0.5f);
		values[2][0] = v2[0];
		values[2][1] = v2[1];
		values[2][2] = v2[2];

		values[3] = { 0 };
	} else {
		RGB8 v2 = lerp(e1, e0, 1.0 / 3.0);
		values[2][0] = v2[0];
		values[2][1] = v2[1];
		values[2][2] = v2[2];
		RGB8 v3 = lerp(e1, e0, 2.0 / 3.0);
		values[3][0] = v3[0];
		values[3][1] = v3[1];
		values[3][2] = v3[2];
	}

	std::array<ColorType, 16> res;

	for (int i = 0; i < res.size(); i++) {
		std::uint_fast32_t index = (block.indices >> (i * 2)) & 0b11;

		res[i] = values[index];

		if constexpr (std::is_same_v<ColorType, RGBA8>) {
			if (extendedMode && index == 3) {
				continue;
			}
			res[i][3] = 255;
		}
	}

	return res;
}

BC1Block BC1Block::encode(const RGB8Block& values) {
	return encodeBlock(values);
}
std::array<RGB8, 16> BC1Block::decode(const BC1Block& block) {
	return decodeBlock<RGB8>(block);
}
BC1BlockAlpha BC1BlockAlpha::encode(const RGBA8Block& values) {
	return static_cast<BC1BlockAlpha>(encodeBlock(values));
}
std::array<RGBA8, 16> BC1BlockAlpha::decode(const BC1BlockAlpha& block) {
	return decodeBlock<RGBA8>(static_cast<BC1Block>(block));
}
