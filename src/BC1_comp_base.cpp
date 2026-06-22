#include <algorithm>
#include <cmath>

#include "BC1_comp.hpp"

std::array<int8_t, 16> computeIndices(
	const RGB8& min,
	const RGB8& max,
	const RGB8Block& values,
	const uint8_t nodeCount
) {
	std::array<int16_t, 3> dir;
	dir[0] = max[0] - min[0];
	dir[1] = max[1] - min[1];
	dir[2] = max[2] - min[2];

	uint32_t dirDot = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];

	std::array<int, 16> vdot;
	for (int i = 0; i < 16; i++) {
		std::array<int, 3> offsetColor;
		for (int c = 0; c < 3; c++) {
			offsetColor[c] = values[c][i] - min[c];
		}
		vdot[i] = offsetColor[0] * dir[0] + offsetColor[1] * dir[1] +
		          offsetColor[2] * dir[2];
	}

	std::array<int8_t, 16> indices;

	if (nodeCount == 3) {
		const float nodes[3] = {
			dirDot / 6.0f,
			dirDot * 3 / 6.0f,
			dirDot * 5 / 6.0f,
		};

		for (int i = 0; i < 16; i++) {
			indices[i] = (vdot[i] >= nodes[0]) + (vdot[i] >= nodes[1]) +
			             (vdot[i] >= nodes[2]);
		}
	} else if (nodeCount == 2) {
		const float nodes[2] = {
			dirDot / 3.0f,
			dirDot * 2 / 3.0f,
		};

		for (int i = 0; i < 16; i++) {
			indices[i] = (vdot[i] <= nodes[0]) + (vdot[i] <= nodes[1]);
		}
	}

	return indices;
}

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
