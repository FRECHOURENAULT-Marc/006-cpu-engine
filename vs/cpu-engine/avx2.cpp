#include "stdafx.h"

namespace avx2
{

// exact /255 for u16 lanes: (x + 128 + ((x + 128)>>8)) >> 8
__m256i div255_epu16(__m256i x)
{
    const __m256i bias = _mm256_set1_epi16(128);
    __m256i t = _mm256_add_epi16(x, bias);
    __m256i u = _mm256_add_epi16(t, _mm256_srli_epi16(t, 8));
    return _mm256_srli_epi16(u, 8);
}

// Premul SRC-over DST, RGBA8 premultiplied, in-place on dst
// src/dst: width*height*4 bytes, no padding
void AlphaBlend_PremulSrcOver(const byte* src, byte* dst, int width, int height)
{
    if (!src || !dst || width <= 0 || height <= 0) return;

    const int bytes = width * height * 4;

    const __m256i zero = _mm256_setzero_si256();
    const __m256i v255 = _mm256_set1_epi16(255);

    // Shuffle mask to replicate A into RGBA lanes within each 128-bit half after unpack:
    // u16 layout per 128b: [R0 G0 B0 A0 R1 G1 B1 A1 ...]
    const __m256i shufA = _mm256_setr_epi8(
        6,7, 6,7, 6,7, 6,7,   14,15, 14,15, 14,15, 14,15,
        6,7, 6,7, 6,7, 6,7,   14,15, 14,15, 14,15, 14,15
    );

    int i = 0;
    for (; i + 32 <= bytes; i += 32)
    {
        __m256i s8 = _mm256_loadu_si256((const __m256i*)(src + i));
        __m256i d8 = _mm256_loadu_si256((const __m256i*)(dst + i));

        // Expand to u16 lanes
        __m256i sLo = _mm256_unpacklo_epi8(s8, zero);
        __m256i sHi = _mm256_unpackhi_epi8(s8, zero);
        __m256i dLo = _mm256_unpacklo_epi8(d8, zero);
        __m256i dHi = _mm256_unpackhi_epi8(d8, zero);

        // Alpha replicated to lanes
        __m256i aLo = _mm256_shuffle_epi8(sLo, shufA);
        __m256i aHi = _mm256_shuffle_epi8(sHi, shufA);

        // Fast path: if all alphas in this 32-byte block are 255 => dst = src
        // Check alpha bytes (every 4th byte) quickly:
        // Extract alpha bytes by AND with mask 0xFF00 in u16 alpha lanes and compare to 255.
        // Cheaper: compute invA and see if all zero.
        __m256i invALo = _mm256_sub_epi16(v255, aLo);
        __m256i invAHi = _mm256_sub_epi16(v255, aHi);
        if (_mm256_testz_si256(invALo, invALo) && _mm256_testz_si256(invAHi, invAHi))
        {
            _mm256_storeu_si256((__m256i*)(dst + i), s8);
            continue;
        }

        // out = src + dst * invA / 255  (per channel incl alpha)
        __m256i dMulLo = _mm256_mullo_epi16(dLo, invALo);
        __m256i dMulHi = _mm256_mullo_epi16(dHi, invAHi);

        __m256i dScaledLo = div255_epu16(dMulLo);
        __m256i dScaledHi = div255_epu16(dMulHi);

        __m256i outLo = _mm256_add_epi16(sLo, dScaledLo);
        __m256i outHi = _mm256_add_epi16(sHi, dScaledHi);

        __m256i out8 = _mm256_packus_epi16(outLo, outHi);
        _mm256_storeu_si256((__m256i*)(dst + i), out8);
    }

    // Tail (scalar)
    auto div255 = [](ui32 x)->ui32 { x += 128; return (x + (x >> 8)) >> 8; };

    for (; i < bytes; i += 4)
    {
        ui32 sr = src[i+0], sg = src[i+1], sb = src[i+2], sa = src[i+3];
        ui32 dr = dst[i+0], dg = dst[i+1], db = dst[i+2], da = dst[i+3];

        ui32 invA = 255u - sa;

        dst[i+0] = (byte)std::min(255u, sr + div255(dr * invA));
        dst[i+1] = (byte)std::min(255u, sg + div255(dg * invA));
        dst[i+2] = (byte)std::min(255u, sb + div255(db * invA));
        dst[i+3] = (byte)std::min(255u, sa + div255(da * invA));
    }
}

}
