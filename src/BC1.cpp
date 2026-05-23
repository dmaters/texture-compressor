#include "BC1.hpp"

#include <algorithm>
#include <cstdint>

#include "ColorFormat.hpp"
template <template <size_t> typename ColorType>
struct EndpointsData {
	ColorType<1> min;
	ColorType<1> max;
	bool alternativeMode = false;
};

// stb_dxt inspired, we find the best fit axis, then we project and use least
// square method to refine the endpoints
template <template <size_t> typename ColorType>
EndpointsData<ColorType> computeEndpoints_4(const ColorType<16>& values) {
	ColorType<1> min, max;

	std::uint32_t channelMin[ColorType<1>::channels()];
	std::uint32_t channelMax[ColorType<1>::channels()];

	for (int c = 0; c < ColorType<1>::channels(); c++) {
		channelMin[c] = *std::min_element(values[c].begin(), values[c].end());
		channelMax[c] = *std::max_element(values[c].begin(), values[c].end());
	}

	for (int c = 0; c < ColorType<1>::channels(); c++) {
		min[c][0] = channelMin[c];
		max[c][0] = channelMax[c];
	}

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

	ColorType<1> baseDir = max - min;

	if (baseDir[0][0] + baseDir[1][0] + baseDir[2][0] == 0)
		return { .min = min, .max = max, .alternativeMode = true };

	std::array<float, 3> pcaDir {
		static_cast<float>(baseDir[0][0]),
		static_cast<float>(baseDir[1][0]),
		static_cast<float>(baseDir[2][0]),
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

	min[0][0] = std::clamp(mean[0] + pcaDir[0] * minDot, 0.0f, 255.0f);
	min[1][0] = std::clamp(mean[1] + pcaDir[1] * minDot, 0.0f, 255.0f);
	min[2][0] = std::clamp(mean[2] + pcaDir[2] * minDot, 0.0f, 255.0f);

	max[0][0] =
		std::ceil(std::clamp(mean[0] + pcaDir[0] * maxDot, 0.0f, 255.0f));
	max[1][0] =
		std::ceil(std::clamp(mean[1] + pcaDir[1] * maxDot, 0.0f, 255.0f));
	max[2][0] =
		std::ceil(std::clamp(mean[2] + pcaDir[2] * maxDot, 0.0f, 255.0f));

	// Least square for refinement
	for (int it = 0; it < 4; it++) {
		std::array<int, 3> dir;

		dir[0] = max[0][0] - min[0][0];
		dir[1] = max[1][0] - min[1][0];
		dir[2] = max[2][0] - min[2][0];

		uint32_t dot = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];

		if (dot == 0) break;

		std::array<int, 16> vdot;
		for (int i = 0; i < 16; i++) {
			std::array<int, 3> offsetColor;
			for (int c = 0; c < 3; c++) {
				offsetColor[c] = values[c][i] - min[c][0];
			}
			vdot[i] = offsetColor[0] * dir[0] + offsetColor[1] * dir[1] +
			          offsetColor[2] * dir[2];
		}

		const float nodes[3] = {
			dot / 6.0f,
			dot * 3 / 6.0f,
			dot * 5 / 6.0f,
		};
		uint8_t base = (vdot[0] >= nodes[0]) + (vdot[0] >= nodes[1]) +
		               (vdot[0] >= nodes[2]);

		std::array<uint8_t, 16> nearestNodes;
		bool monotoneBlock = true;

		for (int i = 0; i < 16; i++) {
			nearestNodes[i] = (vdot[i] >= nodes[0]) + (vdot[i] >= nodes[1]) +
			                  (vdot[i] >= nodes[2]);

			monotoneBlock &= (base == nearestNodes[i]);
		}

		if (monotoneBlock) {
			break;
		}

		std::array<uint8_t, 16> A1, A2;

		for (int i = 0; i < 16; i++) {
			A1[i] = 3 - nearestNodes[i];
		}
		A2 = nearestNodes;

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

			min[c][0] = std::clamp(
				(AtA[3] * Atc[0] - AtA[1] * Atc[1]) * 3 / AtAdet, 0, 255
			);

			max[c][0] = std::clamp(
				(-AtA[2] * Atc[0] + AtA[0] * Atc[1]) * 3 / AtAdet, 0, 255
			);
		}
	}

	return { .min = min, .max = max, .alternativeMode = false };
}

template <template <size_t> typename ColorType>
EndpointsData<ColorType> computeEndpoints_3(const ColorType<16>& values) {
	ColorType<1> min, max;

	std::uint32_t channelMin[ColorType<1>::channels()];
	std::uint32_t channelMax[ColorType<1>::channels()];

	for (int c = 0; c < ColorType<1>::channels(); c++) {
		channelMin[c] = *std::min_element(values[c].begin(), values[c].end());
		channelMax[c] = *std::max_element(values[c].begin(), values[c].end());
	}

	for (int c = 0; c < ColorType<1>::channels(); c++) {
		min[c][0] = channelMin[c];
		max[c][0] = channelMax[c];
	}

	return { .min = min, .max = max };
};

template <template <size_t> typename ColorType, typename BlockType>
BlockType encodeBlock(const ColorType<16>& values) {
	BlockType block;
	bool alternativeMode = false;
	EndpointsData<ColorType> endpointsData;

	if constexpr (ColorType<1>::channels() == 4) {
		for (int i = 0; i < 16; i++) {
			alternativeMode |= values[i][3] < 128;
		}

		if (alternativeMode)
			endpointsData = computeEndpoints_3<ColorType>(values);
	} else
		endpointsData = computeEndpoints_4<ColorType>(values);

	auto [minEndpoint, maxEndpoint, mode] = endpointsData;

	alternativeMode |= mode;

	RGB565 minEncoded = static_cast<RGB565>(minEndpoint);
	RGB565 maxEncoded = static_cast<RGB565>(maxEndpoint);

	alternativeMode |= minEncoded.data == maxEncoded.data;

	ColorType<1> dir = static_cast<ColorType<1>>(maxEncoded) -
	                   static_cast<ColorType<1>>(minEncoded);

	uint32_t dot = ColorType<1>::template dot<uint32_t>(dir, dir)[0];
	ColorType<16> directions;

	std::array<int, 16> vdot;
	for (int i = 0; i < 16; i++) {
		std::array<int, 3> offsetColor;
		for (int c = 0; c < 3; c++) {
			offsetColor[c] = values[c][i] - minEndpoint[c][0];
		}
		vdot[i] = offsetColor[0] * dir[0][0] + offsetColor[1] * dir[1][0] +
		          offsetColor[2] * dir[2][0];
	}
	std::array<uint32_t, 16> indices;

	if (alternativeMode) {
		block.endpoints[0] = minEncoded;
		block.endpoints[1] = maxEncoded;

		const std::uint32_t nodes[2] = { dot / 4, dot * 3 / 4 };

		for (int i = 0; i < indices.size(); i++) {
			indices[i] = (vdot[i] <= nodes[0]) + (vdot[i] <= nodes[1]);
			indices[i] = (indices[i] + 1) % 3;
		}

		if constexpr (ColorType<1>::channels() == 4) {
			for (int i = 0; i < 16; i++) {
				if (values[3][i] < 128) {
					indices[i] = 3 << (i * 2);
				}
			}
		} else {
			for (int i = 0; i < 16; i++) {
				uint16_t sum = values[0][i] + values[1][i] + values[2][i];
				if (sum == 0) {
					indices[i] = 3 << (i * 2);
				}
			}
		}
	} else {
		block.endpoints[0] = maxEncoded;
		block.endpoints[1] = minEncoded;

		const float nodes[3] = {
			dot / 6.0f,
			dot * 3 / 6.0f,
			dot * 5 / 6.0f,
		};

		for (int i = 0; i < indices.size(); i++) {
			indices[i] = (vdot[i] >= nodes[0]) + (vdot[i] >= nodes[1]) +
			             (vdot[i] >= nodes[2]);

			indices[i] = (indices[i] + 1) % 4;
		}
	}

	for (int i = 0; i < indices.size(); i++)
		block.indices |= indices[i] << (i * 2);

	return block;
}

template <template <size_t> typename ColorType, typename BlockType>
std::array<ColorType<1>, 16> decodeBlock(const BlockType& block) {
	std::array<ColorType<1>, 4> values;

	values[0] = static_cast<ColorType<1>>(block.endpoints[0]);
	values[1] = static_cast<ColorType<1>>(block.endpoints[1]);

	bool extendedMode = block.endpoints[0].data <= block.endpoints[1].data;

	if (extendedMode) {
		values[2] = (values[0] / 2 + values[1] / 2);
		values[3] = ColorType<1>(0);
	} else {
		values[2] = ColorType<1>::lerp(values[0], values[1], (255 * 2 / 3));
		values[3] = ColorType<1>::lerp(values[0], values[1], (255 / 3));
	}

	std::array<ColorType<1>, 16> res;

	for (int i = 0; i < res.size(); i++) {
		std::uint_fast32_t index = (block.indices >> (i * 2)) & 0b11;

		res[i] = values[index];

		if constexpr (ColorType<1>::channels() == 4) {
			if (extendedMode && index == 3) {
				continue;
			}
			res[i][3][0] = 255;
		}
	}

	return res;
}

BC1BlockOpaque BC1BlockOpaque::encode(const RGB8Block& values) {
	return encodeBlock<RGB8n, BC1BlockOpaque>(values);
}
std::array<RGB8, 16> BC1BlockOpaque::decode(const BC1BlockOpaque& block) {
	return decodeBlock<RGB8n, BC1BlockOpaque>(block);
}
BC1BlockAlpha BC1BlockAlpha::encode(const RGBA8Block& values) {
	return encodeBlock<RGBA8n, BC1BlockAlpha>(values);
}
std::array<RGBA8, 16> BC1BlockAlpha::decode(const BC1BlockAlpha& block) {
	return decodeBlock<RGBA8n, BC1BlockAlpha>(block);
}
