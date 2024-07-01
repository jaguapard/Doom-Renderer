#include "Lehmer.h"

LehmerRNG::LehmerRNG()
{
	state1 = _mm512_setr_epi64(1, 3, 5, 7, 11, 17, 19, 23);
	state2 = _mm512_setr_epi64(27, 29, 31, 37, 39, 41, 43, 47);
}

__m512i LehmerRNG::next()
{
	state1 = _mm512_mullo_epi64(state1, _mm512_set1_epi64(48271));
	state2 = _mm512_mullo_epi64(state2, _mm512_set1_epi64(48271));
	/*constexpr int s = 1 << 31;
	__m512i idx = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14, s | 0, s | 2, s | 4, s | 6, s | 8, s | 10, s | 12, s | 14); //pick upper 32 bits from both states
	return _mm512_permutex2var_epi32(state1, idx, state2);*/

    __m512i a = _mm512_srli_epi64(state1, 32); //shift upper half of state1 to lower
	return _mm512_mask_blend_epi32(0b1010101010101010, a, state2);
}
