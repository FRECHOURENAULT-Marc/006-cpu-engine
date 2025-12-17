#pragma once

namespace avx2
{

	static inline __m256i div255_epu16(__m256i x);
	void AlphaBlend_PremulSrcOver(const byte* src, byte* dst, int width, int height);

}
