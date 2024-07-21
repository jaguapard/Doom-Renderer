#pragma once
#include "immintrin.h"

namespace avx_helpers
{
	inline __m512i _mm512_clamp_epi32(__m512i value, __m512i min, __m512i max)
	{
		__m512i clampLo = _mm512_max_epi32(value, min);
		return _mm512_min_epi32(clampLo, max);
	}

	inline __m512 _mm512_mask_i64gather16_ps(__m512 src, __mmask16 mask, __m512i vindex_0to7, __m512i vindex_8to15, void const* base_addr, int scale)
	{
		__m256 ymmLow = _mm512_mask_i64gather_ps(_mm256_setzero_ps(), mask, vindex_0to7, base_addr, scale);
		__m256 ymmHigh = _mm512_mask_i64gather_ps(_mm256_setzero_ps(), _kshiftri_mask16(mask, 8), vindex_8to15, base_addr, scale);
		return _mm512_insertf32x8(_mm512_castps256_ps512(ymmLow), ymmHigh, 1);
	}
}