/*
 * Copyright (c) 2025, Kuan-Hsien Lee <khlee870529@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the copyright holder nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <modmesh/simd/simd_support.hpp>

#if defined(__linux__) || defined(__ANDROID__)
#include <sys/auxv.h>
#if defined(__aarch64__) || defined(__arm__)
#include <asm/hwcap.h>
#endif
#elif defined(_WIN32)
#include <windows.h>
#include <intrin.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#endif

namespace modmesh
{

namespace simd
{

namespace detail
{

SIMDFeature detectSIMD(void)
{
#if defined(__aarch64__) || defined(__arm__)
// ARM architecture
#if defined(__linux__) || defined(__ANDROID__)
    unsigned long hwcaps = getauxval(AT_HWCAP);
#ifdef HWCAP_NEON
    if (hwcaps & HWCAP_NEON)
    {
        return SIMD_NEON;
    }
#endif
#elif defined(__APPLE__)
    int neon_supported = 0;
    size_t size = sizeof(neon_supported);
    if (sysctlbyname("hw.optional.neon", &neon_supported, &size, NULL, 0) == 0 && neon_supported)
    {
        return SIMD_NEON;
    }
#elif defined(_WIN32)
    if (IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE))
    {
        return SIMD_NEON;
    }
#endif
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
// x86 architecture
#if defined(_MSC_VER)
    int info[4];
    __cpuid(info, 0);
    int nIds = info[0];

    if (nIds >= 1)
    {
        __cpuid(info, 1);
        if (info[3] & (1 << 25))
            return SIMD_SSE;
        if (info[3] & (1 << 26))
            return SIMD_SSE2;
        if (info[2] & (1 << 0))
            return SIMD_SSE3;
        if (info[2] & (1 << 9))
            return SIMD_SSSE3;
        if (info[2] & (1 << 19))
            return SIMD_SSE41;
        if (info[2] & (1 << 20))
            return SIMD_SSE42;
        if (info[2] & (1 << 28))
            return SIMD_AVX;
    }

    if (nIds >= 7)
    {
        __cpuidex(info, 7, 0);
        if (info[1] & (1 << 5))
            return SIMD_AVX2;
        if (info[1] & (1 << 16))
            return SIMD_AVX512;
    }
#elif defined(__GNUC__) || defined(__clang__)
    unsigned int eax, ebx, ecx, edx;
    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1));

    if (edx & (1 << 25))
        return SIMD_SSE;
    if (edx & (1 << 26))
        return SIMD_SSE2;
    if (ecx & (1 << 0))
        return SIMD_SSE3;
    if (ecx & (1 << 9))
        return SIMD_SSSE3;
    if (ecx & (1 << 19))
        return SIMD_SSE41;
    if (ecx & (1 << 20))
        return SIMD_SSE42;
    if (ecx & (1 << 28))
        return SIMD_AVX;

    // For AVX2/AVX512, CPUID with eax=7
    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(7), "c"(0));
    if (ebx & (1 << 5))
        return SIMD_AVX2;
    if (ebx & (1 << 16))
        return SIMD_AVX512;
#endif
#endif

    return SIMD_NONE;
}

} /* namespace detail */

} /* namespace simd */

} /* namespace modmesh */
