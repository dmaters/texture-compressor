#include <immintrin.h>

#include "BC1_comp.hpp"

__attribute__((target("avx2"))) std::array<int8_t, 16> computeIndices(
	const RGB8& min,
	const RGB8& max,
	const RGB8Block& values,
	const uint8_t nodeCount
) {
	// return computeIndices__4nodes(min, max, values);

	std::array<int16_t, 3> dirV;
	dirV[0] = max[0] - min[0];
	dirV[1] = max[1] - min[1];
	dirV[2] = max[2] - min[2];

	uint32_t dirDot = dirV[0] * dirV[0] + dirV[1] * dirV[1] + dirV[2] * dirV[2];

	__m256i vdot = _mm256_setzero_si256();

	for (int i = 0; i < 3; i++) {
		__m256i cmin = _mm256_set1_epi16(min[i]);
		__m256i dir = _mm256_set1_epi16(dirV[i]);

		__m256i val =
			_mm256_cvtepu8_epi16(_mm_loadu_si128((__m128i_u*)values[i].data()));
		__m256i localDir = _mm256_sub_epi16(val, cmin);
		__m256i res = _mm256_mullo_epi16(localDir, dir);

		__m256i mask = _mm256_cmpeq_epi16(
			_mm256_mulhi_epi16(localDir, dir), _mm256_setzero_si256()
		);

		res = _mm256_blendv_epi8(_mm256_set1_epi32(-1), res, mask);

		vdot = _mm256_adds_epi16(res, vdot);
	}

	__m256i packedIndices = _mm256_setzero_si256();
	std::array<int8_t, 16> indices;

	if (nodeCount == 3) {
		const __m256i nodes[3] = {
			_mm256_set1_epi16(
				std::min(dirDot / 6, static_cast<uint32_t>(INT16_MAX))
			),
			_mm256_set1_epi16(
				std::min(dirDot * 3 / 6, static_cast<uint32_t>(INT16_MAX))
			),
			_mm256_set1_epi16(
				std::min(dirDot * 5 / 6, static_cast<uint32_t>(INT16_MAX))
			),
		};

		packedIndices = _mm256_add_epi16(
			_mm256_srli_epi16(_mm256_cmpgt_epi16(vdot, nodes[0]), 15),
			packedIndices
		);
		packedIndices = _mm256_add_epi16(
			_mm256_srli_epi16(_mm256_cmpgt_epi16(vdot, nodes[1]), 15),
			packedIndices
		);
		packedIndices = _mm256_add_epi16(
			_mm256_srli_epi16(_mm256_cmpgt_epi16(vdot, nodes[2]), 15),
			packedIndices
		);

	} else if (nodeCount == 2) {
		const __m256i nodes[2] = {
			_mm256_set1_epi16(
				std::min(dirDot / 3, static_cast<uint32_t>(INT16_MAX))
			),
			_mm256_set1_epi16(
				std::min(dirDot * 2 / 3, static_cast<uint32_t>(INT16_MAX))
			),
		};

		packedIndices = _mm256_add_epi16(
			_mm256_srli_epi16(_mm256_cmpgt_epi16(vdot, nodes[0]), 15),
			packedIndices
		);
		packedIndices = _mm256_add_epi16(
			_mm256_srli_epi16(_mm256_cmpgt_epi16(vdot, nodes[1]), 15),
			packedIndices
		);
	}
	__m256i packed = _mm256_packs_epi16(packedIndices, packedIndices);
	__m256i fixed = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(2, 0, 2, 0));

	_mm_store_si128((__m128i*)indices.data(), _mm256_castsi256_si128(fixed));
	return indices;
}
