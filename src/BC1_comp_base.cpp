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
