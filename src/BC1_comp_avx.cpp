#include <immintrin.h>

#include <algorithm>
#include <cmath>

#include "BC1_comp.hpp"

__attribute__((target("avx2"))) std::array<int8_t, 16> computeIndices(
	const RGB8& min,
	const RGB8& max,
	const RGB8Block& values,
	const uint8_t nodeCount
) {
	std::array<int16_t, 3> dirV;
	dirV[0] = max[0] - min[0];
	dirV[1] = max[1] - min[1];
	dirV[2] = max[2] - min[2];

	uint32_t dirDot = dirV[0] * dirV[0] + dirV[1] * dirV[1] + dirV[2] * dirV[2];

	__m256i dotValuesLow = _mm256_setzero_si256();
	__m256i dotValuesHigh = _mm256_setzero_si256();

	for (int i = 0; i < 3; i++) {
		__m256i packedValues =
			_mm256_cvtepu8_epi16(_mm_loadu_si128((__m128i_u*)values[i].data()));

		__m256i cmin = _mm256_set1_epi16(min[i]);
		__m256i localDir = _mm256_sub_epi16(packedValues, cmin);

		__m256i lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(localDir));
		__m256i hi =
			_mm256_cvtepi16_epi32(_mm256_extracti128_si256(localDir, 1));

		__m256i axis = _mm256_set1_epi32(dirV[i]);
		__m256i loDir = _mm256_mullo_epi32(lo, axis);
		dotValuesLow = _mm256_add_epi32(loDir, dotValuesLow);

		__m256i hiDir = _mm256_mullo_epi32(hi, axis);
		dotValuesHigh = _mm256_add_epi32(hiDir, dotValuesHigh);
	}

	__m256i packedIndices = _mm256_setzero_si256();
	std::array<int8_t, 16> indices;

	if (nodeCount == 3) {
		const __m256i nodes[3] = {
			_mm256_set1_epi32(dirDot / 6),
			_mm256_set1_epi32(dirDot * 3 / 6),
			_mm256_set1_epi32(dirDot * 5 / 6),
		};

		for (int i = 0; i < 3; i++) {
			__m256i comparisonLow = _mm256_cmpgt_epi32(dotValuesLow, nodes[i]);

			comparisonLow = _mm256_srli_epi32(comparisonLow, 31);

			__m256i comparisonHigh =
				_mm256_cmpgt_epi32(dotValuesHigh, nodes[i]);

			comparisonHigh = _mm256_srli_epi32(comparisonHigh, 31);

			__m256i packed = _mm256_packs_epi32(comparisonLow, comparisonHigh);
			packed = _mm256_permute4x64_epi64(packed, 0xD8);

			packedIndices = _mm256_add_epi16(packedIndices, packed);
		}

	} else if (nodeCount == 2) {
		const __m256i nodes[2] = {
			_mm256_set1_epi32(dirDot / 4),
			_mm256_set1_epi32(dirDot * 2 / 3),
		};

		for (int i = 0; i < 2; i++) {
			__m256i comparisonLow = _mm256_cmpgt_epi32(dotValuesLow, nodes[i]);
			comparisonLow = _mm256_srli_epi32(comparisonLow, 31);

			__m256i comparisonHigh =
				_mm256_cmpgt_epi32(dotValuesHigh, nodes[i]);
			comparisonHigh = _mm256_srli_epi32(comparisonHigh, 31);

			__m256i packed = _mm256_packs_epi32(comparisonLow, comparisonHigh);
			packed = _mm256_permute4x64_epi64(packed, 0xD8);

			packedIndices = _mm256_add_epi16(packedIndices, packed);
		}
	}
	__m256i packed = _mm256_packs_epi16(packedIndices, packedIndices);
	__m256i fixed = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(2, 0, 2, 0));

	_mm_storeu_si128((__m128i*)indices.data(), _mm256_castsi256_si128(fixed));
	return indices;
}

struct ComputeIndicesData {
	__m256i indices;
	bool allEq;
};
__attribute__((target("avx2"))) ComputeIndicesData computeIndices_reduced(
	const __m256i values[3], const std::array<int16_t, 3>& dir
) {
	__m256i packedDotValuesLow = _mm256_setzero_si256();
	__m256i packedDotValuesHigh = _mm256_setzero_si256();

	for (int i = 0; i < 3; i++) {
		__m128i low = _mm256_castsi256_si128(values[i]);
		__m128i high = _mm256_extracti128_si256(values[i], 1);

		__m256i mullo = _mm256_mullo_epi32(
			_mm256_cvtepi16_epi32(low), _mm256_set1_epi32(dir[i])
		);

		__m256i mulhi = _mm256_mullo_epi32(
			_mm256_cvtepi16_epi32(high), _mm256_set1_epi32(dir[i])
		);
		packedDotValuesLow = _mm256_add_epi32(packedDotValuesLow, mullo);
		packedDotValuesHigh = _mm256_add_epi32(packedDotValuesHigh, mulhi);
	}

	alignas(32) int32_t dotValues[16];
	_mm256_store_si256((__m256i*)&dotValues, packedDotValuesLow);
	_mm256_store_si256((__m256i*)&dotValues[8], packedDotValuesHigh);

	int32_t minDot = dotValues[0];
	int32_t maxDot = dotValues[0];
	for (int i = 0; i < 16; i++) minDot = std::min(minDot, dotValues[i]);
	for (int i = 0; i < 16; i++) maxDot = std::max(maxDot, dotValues[i]);

	int32_t dotExtent = maxDot - minDot;

	int32_t nodes[3] = {
		static_cast<int32_t>(minDot + dotExtent / 6),
		static_cast<int32_t>(minDot + dotExtent * 3 / 6),
		static_cast<int32_t>(minDot + dotExtent * 5 / 6),
	};

	__m256i indices = _mm256_setzero_si256();
	uint8_t comparisonTest = 0;
	for (int i = 0; i < 3; i++) {
		__m256i comparisonLow =
			_mm256_cmpgt_epi32(packedDotValuesLow, _mm256_set1_epi32(nodes[i]));

		uint32_t maskLow = _mm256_movemask_epi8(comparisonLow);
		comparisonTest += maskLow == 0 || maskLow == UINT32_MAX;

		comparisonLow = _mm256_srli_epi32(comparisonLow, 31);

		__m256i comparisonHigh = _mm256_cmpgt_epi32(
			packedDotValuesHigh, _mm256_set1_epi32(nodes[i])
		);

		uint32_t maskHigh = _mm256_movemask_epi8(comparisonHigh);
		comparisonTest += maskHigh == 0 || maskHigh == UINT32_MAX;

		comparisonHigh = _mm256_srli_epi32(comparisonHigh, 31);

		__m256i packed = _mm256_packs_epi32(comparisonLow, comparisonHigh);
		packed = _mm256_permute4x64_epi64(packed, 0xD8);

		indices = _mm256_add_epi16(indices, packed);
	}

	return { indices, comparisonTest == 6 };
}

__attribute__((target("avx2"))) inline int32_t hadd_collapse(__m256i values) {
	__m128i low = _mm256_castsi256_si128(values);
	__m128i high = _mm256_extracti128_si256(values, 1);
	__m128i sum = _mm_add_epi32(low, high);

	sum = _mm_hadd_epi32(sum, _mm_undefined_si128());
	sum = _mm_hadd_epi32(sum, _mm_undefined_si128());

	return _mm_cvtsi128_si32(sum);
}

// stb_dxt inspired, we find the best fit axis, then we project and use
// least square method to refine the endpoints
__attribute__((target("avx2"))) EndpointsData
computeEndpointsPCA(const RGB8Block& values) {
	auto [min, max, _] = computeEndpointsMinMax(values);

	RGB8 baseDir;
	baseDir[0] = max[0] - min[0];
	baseDir[1] = max[1] - min[1];
	baseDir[2] = max[2] - min[2];

	if (baseDir[0] + baseDir[1] + baseDir[2] == 0)
		return { .min = min, .max = max, .alternativeMode = true };

	__m128i_u packedValues[3] = {
		_mm_load_si128((__m128i_u*)values[0].data()),
		_mm_load_si128((__m128i_u*)values[1].data()),
		_mm_load_si128((__m128i_u*)values[2].data()),
	};

	int16_t C[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	uint16_t mean[3];

	for (int i = 0; i < 3; i++) {
		__m128i div = _mm_sad_epu8(packedValues[i], _mm_setzero_si128());

		mean[i] = ((int32_t)_mm_extract_epi32(div, 0) +
		           (int32_t)_mm_extract_epi32(div, 2)) /
		          16;
	}

	__m256i extendedValues[3] = {
		_mm256_cvtepu8_epi16(packedValues[0]),
		_mm256_cvtepu8_epi16(packedValues[1]),
		_mm256_cvtepu8_epi16(packedValues[2]),
	};
	__m256i adjustedValues[3] = {
		_mm256_sub_epi16(extendedValues[0], _mm256_set1_epi16(mean[0])),
		_mm256_sub_epi16(extendedValues[1], _mm256_set1_epi16(mean[1])),
		_mm256_sub_epi16(extendedValues[2], _mm256_set1_epi16(mean[2])),
	};

	for (int i = 0; i < 3; i++) {
		__m256i ci = _mm256_madd_epi16(adjustedValues[i], adjustedValues[i]);
		C[4 * i] = hadd_collapse(ci) / 15;
	}
	__m256i c1 = _mm256_madd_epi16(adjustedValues[0], adjustedValues[1]);
	C[1] = hadd_collapse(c1) / 15;

	__m256i c2 = _mm256_madd_epi16(adjustedValues[0], adjustedValues[2]);
	C[2] = hadd_collapse(c2) / 15;

	__m256i c5 = _mm256_madd_epi16(adjustedValues[1], adjustedValues[2]);
	C[5] = hadd_collapse(c5) / 15;

	C[3] = C[1];
	C[6] = C[2];
	C[7] = C[5];

	std::array<float, 3> iteratedDir {
		static_cast<float>(baseDir[0]),
		static_cast<float>(baseDir[1]),
		static_cast<float>(baseDir[2]),
	};

	// Power iteration for main eigenvector
	for (int i = 0; i < 8; i++) {
		std::array<float, 3> nextDir;

		nextDir[0] =
			(iteratedDir[0] * C[0] + iteratedDir[1] * C[1] +
		     iteratedDir[2] * C[2]);
		nextDir[1] =
			(iteratedDir[0] * C[3] + iteratedDir[1] * C[4] +
		     iteratedDir[2] * C[5]);

		nextDir[2] =
			(iteratedDir[0] * C[6] + iteratedDir[1] * C[7] +
		     iteratedDir[2] * C[8]);

		iteratedDir = nextDir;

		float size = std::sqrt(
			iteratedDir[0] * iteratedDir[0] + iteratedDir[1] * iteratedDir[1] +
			iteratedDir[2] * iteratedDir[2]
		);

		if (size < 0.0001f) break;

		iteratedDir[0] = iteratedDir[0] / size;
		iteratedDir[1] = iteratedDir[1] / size;
		iteratedDir[2] = iteratedDir[2] / size;
	}

	std::array<int16_t, 3> pcaDir {
		static_cast<int16_t>(iteratedDir[0] * 255),
		static_cast<int16_t>(iteratedDir[1] * 255),
		static_cast<int16_t>(iteratedDir[2] * 255),
	};

	int16_t lsMin[3];
	int16_t lsMax[3];

	for (int c = 0; c < 3; c++) {
		lsMin[c] = min[c];
		lsMax[c] = max[c];
	}

	// Least square for refinement
	for (int it = 0; it < 4; it++) {
		__m256i localizedValues[3] = {
			_mm256_sub_epi16(extendedValues[0], _mm256_set1_epi16(lsMin[0])),
			_mm256_sub_epi16(extendedValues[1], _mm256_set1_epi16(lsMin[1])),
			_mm256_sub_epi16(extendedValues[2], _mm256_set1_epi16(lsMin[2])),
		};

		__m256i A1, A2;

		auto [indices, monotone] =
			computeIndices_reduced(localizedValues, pcaDir);

		if (monotone) {
			break;
		}

		A1 = _mm256_sub_epi16(_mm256_set1_epi16(3), indices);
		A2 = indices;

		std::array<int32_t, 4> AtA = { 0, 0, 0, 0 };

		AtA[0] = hadd_collapse(_mm256_madd_epi16(A1, A1));
		AtA[1] = hadd_collapse(_mm256_madd_epi16(A1, A2));
		AtA[2] = AtA[1];
		AtA[3] = hadd_collapse(_mm256_madd_epi16(A2, A2));

		int32_t AtAdet = (AtA[0] * AtA[3] - AtA[1] * AtA[2]);

		for (int c = 0; c < 3; c++) {
			std::array<int32_t, 2> Atc = { 0, 0 };
			Atc[0] = hadd_collapse(_mm256_madd_epi16(A1, extendedValues[c]));
			Atc[1] = hadd_collapse(_mm256_madd_epi16(A2, extendedValues[c]));

			lsMin[c] = (AtA[3] * Atc[0] - AtA[1] * Atc[1]) * 3 / AtAdet;
			lsMax[c] = (-AtA[2] * Atc[0] + AtA[0] * Atc[1]) * 3 / AtAdet;

			pcaDir[c] = lsMax[c] - lsMin[c];
		}
	}
	for (int c = 0; c < 3; c++) {
		min[c] = (uint8_t)std::clamp(lsMin[c], int16_t(0), int16_t(255));
		max[c] = (uint8_t)std::clamp(lsMax[c], int16_t(0), int16_t(255));
	}
	return { .min = min, .max = max, .alternativeMode = false };
}
