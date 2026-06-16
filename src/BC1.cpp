#include "BC1.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "BC1_comp.hpp"
#include "ColorFormat.hpp"
struct EndpointsData {
	RGB8 min;
	RGB8 max;
	bool alternativeMode = false;
};

RGB565 rgb8_to_rgb565(RGB8 value) {
	RGB565 res =
		((value[0] >> 3) << 11) | ((value[1] >> 2) << 5) | (value[2] >> 3);
	return res;
}

RGB8 rgb565_to_rgb8(RGB565 value) {
	RGB8 res;
	uint8_t r = (value >> 11) & 0x1F;
	uint8_t g = (value >> 5) & 0x3F;
	uint8_t b = (value >> 0) & 0x1F;

	res[0] = (r << 3) | (r >> 2);
	res[1] = (g << 2) | (g >> 4);
	res[2] = (b << 3) | (b >> 2);
	return res;
}

RGB8 lerp(RGB8 v1, RGB8 v2, float ratio) {
	RGB8 res;

	res[0] = v1[0] + (v2[0] - v1[0]) * ratio;
	res[1] = v1[1] + (v2[1] - v1[1]) * ratio;
	res[2] = v1[2] + (v2[2] - v1[2]) * ratio;

	return res;
}

uint32_t dot(RGB8 v1, RGB8 v2) {
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

EndpointsData computeEndpointsMinMax(const RGB8Block& values) {
	RGB8 min, max;

	for (int c = 0; c < 3; c++) {
		min[c] = *std::min_element(values[c].begin(), values[c].end());
		max[c] = *std::max_element(values[c].begin(), values[c].end());
	}

	return { .min = min, .max = max };
};

// stb_dxt inspired, we find the best fit axis, then we project and use least
// square method to refine the endpoints
EndpointsData computeEndpointsPCA(const RGB8Block& values) {
	auto [min, max, _] = computeEndpointsMinMax(values);

	std::array<float, 9> C = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	std::array<float, 3> mean = { 0, 0, 0 };
	for (int i = 0; i < 16; i++) {
		mean[0] += values[0][i];
		mean[1] += values[1][i];
		mean[2] += values[2][i];
	}
	for (int i = 0; i < 3; i++) mean[i] /= 16.0f;

	for (int i = 0; i < 16; i++) {
		C[0] += (values[0][i] - mean[0]) * (values[0][i] - mean[0]);
		C[4] += (values[1][i] - mean[1]) * (values[1][i] - mean[1]);
		C[8] += (values[2][i] - mean[2]) * (values[2][i] - mean[2]);
		C[1] += (values[0][i] - mean[0]) * (values[1][i] - mean[1]);
		C[2] += (values[0][i] - mean[0]) * (values[2][i] - mean[2]);
		C[5] += (values[1][i] - mean[1]) * (values[2][i] - mean[2]);
	}

	C[3] = C[1];
	C[6] = C[2];
	C[7] = C[5];

	for (int i = 0; i < 9; i++) C[i] /= 15.0f;

	RGB8 baseDir;
	baseDir[0] = max[0] - min[0];
	baseDir[1] = max[1] - min[1];
	baseDir[2] = max[2] - min[2];

	if (baseDir[0] + baseDir[1] + baseDir[2] == 0)
		return { .min = min, .max = max, .alternativeMode = true };

	std::array<float, 3> pcaDir {
		static_cast<float>(baseDir[0]),
		static_cast<float>(baseDir[1]),
		static_cast<float>(baseDir[2]),
	};
	float length = sqrt(
		pcaDir[0] * pcaDir[0] + pcaDir[1] * pcaDir[1] + pcaDir[2] * pcaDir[2]
	);
	pcaDir[0] = pcaDir[0] / length;
	pcaDir[1] = pcaDir[1] / length;
	pcaDir[2] = pcaDir[2] / length;

	float size = 0;

	// Power iteration for main eigenvector
	for (int i = 0; i < 8; i++) {
		std::array<float, 3> nextDir;
		nextDir[0] = pcaDir[0] * C[0] + pcaDir[1] * C[1] + pcaDir[2] * C[2];
		nextDir[1] = pcaDir[0] * C[3] + pcaDir[1] * C[4] + pcaDir[2] * C[5];
		nextDir[2] = pcaDir[0] * C[6] + pcaDir[1] * C[7] + pcaDir[2] * C[8];

		size = pcaDir[0] * nextDir[0] + pcaDir[1] * nextDir[1] +
		       pcaDir[2] * nextDir[2];
		float length = sqrt(
			nextDir[0] * nextDir[0] + nextDir[1] * nextDir[1] +
			nextDir[2] * nextDir[2]
		);

		if (length < 0.00001) break;
		pcaDir[0] = nextDir[0] / length;
		pcaDir[1] = nextDir[1] / length;
		pcaDir[2] = nextDir[2] / length;
	}

	float minDot = std::numeric_limits<float>::max();
	float maxDot = -std::numeric_limits<float>::max();

	for (int i = 0; i < 16; i++) {
		float r = values[0][i] - mean[0];
		float g = values[1][i] - mean[1];
		float b = values[2][i] - mean[2];

		float dot = r * pcaDir[0] + g * pcaDir[1] + b * pcaDir[2];

		if (dot < minDot) minDot = dot;
		if (dot > maxDot) maxDot = dot;
	}

	min[0] = std::clamp(mean[0] + pcaDir[0] * minDot, 0.0f, 255.0f);
	min[1] = std::clamp(mean[1] + pcaDir[1] * minDot, 0.0f, 255.0f);
	min[2] = std::clamp(mean[2] + pcaDir[2] * minDot, 0.0f, 255.0f);

	max[0] = std::ceil(std::clamp(mean[0] + pcaDir[0] * maxDot, 0.0f, 255.0f));
	max[1] = std::ceil(std::clamp(mean[1] + pcaDir[1] * maxDot, 0.0f, 255.0f));
	max[2] = std::ceil(std::clamp(mean[2] + pcaDir[2] * maxDot, 0.0f, 255.0f));

	// Least square for refinement
	for (int it = 0; it < 4; it++) {
		std::array<int, 3> dir;

		dir[0] = max[0] - min[0];
		dir[1] = max[1] - min[1];
		dir[2] = max[2] - min[2];

		std::array<int8_t, 16> A1, A2;

		auto indices = computeIndices(min, max, values, 3);
		uint8_t base = indices[0];

		bool monotone = true;
		for (uint8_t index : indices) {
			monotone &= (index == base);
		}

		if (monotone) break;

		for (int i = 0; i < 16; i++) {
			A1[i] = 3 - indices[i];
		}
		A2 = indices;

		std::array<int32_t, 4> AtA = { 0, 0, 0, 0 };

		for (int i = 0; i < 16; i++) {
			AtA[0] += A1[i] * A1[i];
			AtA[1] += A1[i] * A2[i];
			AtA[2] += A1[i] * A2[i];
			AtA[3] += A2[i] * A2[i];
		}

		int32_t AtAdet = (AtA[0] * AtA[3] - AtA[1] * AtA[2]);

		for (int c = 0; c < 3; c++) {
			std::array<int32_t, 2> Atc = { 0, 0 };
			for (int i = 0; i < 16; i++) {
				Atc[0] += A1[i] * values[c][i];
				Atc[1] += A2[i] * values[c][i];
			}

			min[c] = std::clamp(
				(AtA[3] * Atc[0] - AtA[1] * Atc[1]) * 3 / AtAdet, 0, 255
			);

			max[c] = std::clamp(
				(-AtA[2] * Atc[0] + AtA[0] * Atc[1]) * 3 / AtAdet, 0, 255
			);
		}
	}

	return { .min = min, .max = max, .alternativeMode = false };
}

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
