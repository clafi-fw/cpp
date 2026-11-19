// =========================================================================
// Graphics.Cpu_Simd.h
//
// The AVX2 helpers the CPU painters share. A file that calls one includes
// this header in its global module fragment.
// =========================================================================
#pragma once
#include <immintrin.h>
#include <cstdint>

namespace ClaFi::Graphics::Cpu
{
    // **Writes pixels straight past the cache.** A store that does not read its line first is one
    // pass over the memory rather than two, and a fill larger than the cache would hold - a
    // background, a sky - is never read before it is overwritten. The caller states the question
    // the size answers, and closes the fill with _mm_sfence() before anything reads the pixels back.
    //
    // A streaming store needs a 32-byte line, so the head runs up to the first one whole.
    inline void streamFillPixels(void* dest, const int count, const std::uint32_t value)
    {
        auto* pixels = static_cast<std::uint32_t*>(dest);
        int written = 0;
        while (written < count && (reinterpret_cast<std::uintptr_t>(pixels + written) & 31u) != 0u)
        {
            pixels[written] = value;
            ++written;
        }

        const __m256i color = _mm256_set1_epi32(static_cast<int>(value));
        for (; written + 8 <= count; written += 8)
        {
            _mm256_stream_si256(reinterpret_cast<__m256i*>(pixels + written), color);
        }

        for (; written < count; ++written)
        {
            pixels[written] = value;
        }
    }

    // SIMD 8-pixel blend
    inline __m256i blend8(__m256i dest, __m256i src, __m256i alpha)
    {
        const __m256i rbMask = _mm256_set1_epi32(0x00FF00FF);
        const __m256i gaMask = _mm256_set1_epi32(0xFF00FF00);

        // 1. Prepare differences for both passes at once
        __m256i destRb = _mm256_and_si256(dest, rbMask);
        __m256i srcRb = _mm256_and_si256(src, rbMask);
        __m256i diffRb = _mm256_sub_epi32(srcRb, destRb);

        __m256i destGa = _mm256_and_si256(_mm256_srli_epi32(dest, 8), rbMask);
        __m256i srcGa = _mm256_and_si256(_mm256_srli_epi32(src, 8), rbMask);
        __m256i diffGa = _mm256_sub_epi32(srcGa, destGa);

        // 2. Fire both multiplications
        // Modern CPUs can often pipeline these two independent mullo instructions
        __m256i productRb = _mm256_mullo_epi32(diffRb, alpha);
        __m256i productGa = _mm256_mullo_epi32(diffGa, alpha);

        // 3. Shift and reconstruct
        __m256i resultRb = _mm256_and_si256(_mm256_add_epi32(destRb, _mm256_srai_epi32(productRb, 8)), rbMask);
        __m256i resultGa = _mm256_slli_epi32(_mm256_add_epi32(destGa, _mm256_srai_epi32(productGa, 8)), 8);

        // Blend using the gaMask to finalize the combine
        // This is often faster than a separate final mask + **or**
        return _mm256_blendv_epi8(resultRb, resultGa, gaMask);
    }

    inline __m256 mm256_abs_ps(__m256 v)
    {
        // 0x7FFFFFFF mask clears the sign bit of a 32-bit float
        const __m256 mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff));
        return _mm256_and_ps(v, mask);
    }
}
