/*==============================================================================
  Copyright (c) 2020 - Fielding DSP GmbH

  Created - 19. October 2018
==============================================================================*/
#include "FastFourier.h"

#include <cassert>

#if defined(__APPLE__)
    #define USE_INTEL_FFT 0
    #define USE_APPLE_FFT 1
    #define USE_PFFFT 0
#elif defined(_WIN64)
    #define USE_INTEL_FFT 1
    #define USE_APPLE_FFT 0
    #define USE_PFFFT 0
#elif defined(__linux__) && defined(__x86_64__)
    #define USE_INTEL_FFT 1
    #define USE_APPLE_FFT 0
    #define USE_PFFFT 0
#elif defined(__EMSCRIPTEN__)
    #define USE_INTEL_FFT 0
    #define USE_APPLE_FFT 0
    #define USE_PFFFT 1
#endif

//==============================================================================
//==============================================================================
#if USE_INTEL_FFT
  #if defined(_WIN32) || defined (_WIN64)
    #define IPP_STDCALL __stdcall
  #else
    #define IPP_STDCALL
  #endif

  struct IppsDFTSpec_R_32f;

  extern "C"
  {
    int IPP_STDCALL ippsDFTGetSize_R_32f(int, int, int, int*, int*, int*);
    int IPP_STDCALL ippsDFTInit_R_32f(int, int, int, unsigned char*, unsigned char*);
    int IPP_STDCALL ippsDFTFwd_RToCCS_32f(const float*, std::complex<float>*, const unsigned char*, unsigned char*);
    int IPP_STDCALL ippsDFTInv_CCSToR_32f(const std::complex<float>*, float*, const unsigned char*, unsigned char*);
  }
#endif

//==============================================================================
//==============================================================================
#if USE_INTEL_FFT
#include <vector>
#include <cstdint>
class FastFourier::Impl
{
public:
    Impl(size_t size) : n(static_cast<int> (size))
    {
        assert((size & (size - 1)) == 0);

        int specSize = 0, initSize = 0, workBufSize = 0;

        ippsDFTGetSize_R_32f(n, /*IPP_FFT_NODIV_BY_ANY*/ 8, 0, &specSize, &initSize, &workBufSize);

        std::vector<std::uint8_t> initWorkBuffer(initSize);
        scratchBuffer.resize(workBufSize);
        intelFFT.resize(specSize);

        ippsDFTInit_R_32f(n, /*IPP_FFT_NODIV_BY_ANY*/ 8, 0, intelFFT.data(), initWorkBuffer.data());
    }

    ~Impl() = default;

    void forward(const float* realInput, std::complex<float>* cmplxOutput) noexcept
    {
        ippsDFTFwd_RToCCS_32f(realInput, cmplxOutput, intelFFT.data(), scratchBuffer.data());
    }

    void inverse(const std::complex<float>* cmplxInput, float* realOutput) noexcept
    {
        ippsDFTInv_CCSToR_32f(cmplxInput, realOutput, intelFFT.data(), scratchBuffer.data());
    }

    float getNormalizationFactor() const noexcept
    {
        return static_cast<float>(n);
    }

    //==============================================================================
    Impl(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl& operator=(Impl&&) = delete;

    //==============================================================================
    int n;
    std::vector<std::uint8_t> scratchBuffer;
    std::vector<std::uint8_t> intelFFT;
};

//==============================================================================
#elif USE_APPLE_FFT
#include <Accelerate/Accelerate.h>

class FastFourier::Impl
{
public:
    Impl(size_t size)
        : order(static_cast<int>(std::log2(size))),
          fftApple(vDSP_create_fftsetup(order, 2))
    {
        assert((size & (size - 1)) == 0);
    }

    ~Impl()
    {
        if (fftApple != nullptr)
        {
            vDSP_destroy_fftsetup(fftApple);
            fftApple = nullptr;
        }
    }

    void forward(const float* realInput, std::complex<float>* cmplxOutput) const noexcept
    {
        auto numComplex = ((1 << order) >> 1) + 1;
        DSPSplitComplex input  { const_cast<float*>(realInput), const_cast<float*>(realInput) + 1};
        DSPSplitComplex output { reinterpret_cast<float*>(cmplxOutput), reinterpret_cast<float*>(cmplxOutput) + 1};

        vDSP_fft_zrop(fftApple, &input, 2, &output, 2, order, kFFTDirection_Forward);

        cmplxOutput[numComplex-1] = std::complex<float>(cmplxOutput[0].imag(), 0.0f);
        cmplxOutput[0] = std::complex<float>(cmplxOutput[0].real(), 0.0f);
    }

    void inverse(const std::complex<float>* cmplxInput, float* realOutput) const noexcept
    {
        // Apple requires packed storage, so we will modify the array to be packed
        // and then unmodify the input array again so that we effectively did not
        // change the input - strictly this is UB!
        auto mutableInput = const_cast<std::complex<float>*> (cmplxInput);

        // pack the input
        auto numComplex = ((1 << order) >> 1) + 1;
        mutableInput[0] = std::complex<float>(mutableInput[0].real(), mutableInput[numComplex-1].real());

        DSPSplitComplex output  { realOutput, realOutput + 1};
        DSPSplitComplex input { reinterpret_cast<float*>(mutableInput),
                                reinterpret_cast<float*>(mutableInput) + 1};

        vDSP_fft_zrop(fftApple, &input, 2, &output, 2, order, kFFTDirection_Inverse);

        // undo our changes to the input array
        mutableInput[0] = std::complex<float>(mutableInput[0].real(), 0.0f);
    }

    float getNormalizationFactor() const noexcept
    {
        return static_cast<float>(1 << order) * 4.0f;
    }

    //==============================================================================
    Impl(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl& operator=(Impl&&) = delete;

    //==============================================================================
private:
    int order;
    FFTSetup fftApple;
};

//==============================================================================
#elif USE_PFFFT
#include <pffft.h>
#include <algorithm>

class FastFourier::Impl
{
public:
    Impl(size_t size)
        : n(size),
          setup(pffft_new_setup(static_cast<int>(size), PFFFT_REAL)),
          buffer(static_cast<float*>(pffft_aligned_malloc(size * sizeof(float)))),
          work(static_cast<float*>(pffft_aligned_malloc(size * sizeof(float))))
    {
        assert((size & (size - 1)) == 0);
        assert(setup != nullptr);
    }

    ~Impl()
    {
        pffft_aligned_free(work);
        pffft_aligned_free(buffer);
        pffft_destroy_setup(setup);
    }

    // pffft packs the DC & Nyquist bins, which are both real, into the first complex value. The
    // transform goes through an internal buffer, as pffft needs SIMD aligned buffers.
    void forward(const float* realInput, std::complex<float>* cmplxOutput) noexcept
    {
        std::copy_n(realInput, n, buffer);
        pffft_transform_ordered(setup, buffer, buffer, work, PFFFT_FORWARD);

        const auto numBins = n / 2;
        cmplxOutput[0] = std::complex<float>(buffer[0], 0.0f);
        cmplxOutput[numBins] = std::complex<float>(buffer[1], 0.0f);
        for (size_t i = 1; i < numBins; ++i)
            cmplxOutput[i] = std::complex<float>(buffer[2 * i], buffer[2 * i + 1]);
    }

    void inverse(const std::complex<float>* cmplxInput, float* realOutput) noexcept
    {
        const auto numBins = n / 2;
        buffer[0] = cmplxInput[0].real();
        buffer[1] = cmplxInput[numBins].real();
        for (size_t i = 1; i < numBins; ++i)
        {
            buffer[2 * i] = cmplxInput[i].real();
            buffer[2 * i + 1] = cmplxInput[i].imag();
        }

        pffft_transform_ordered(setup, buffer, buffer, work, PFFFT_BACKWARD);
        std::copy_n(buffer, n, realOutput);
    }

    float getNormalizationFactor() const noexcept
    {
        return static_cast<float>(n);
    }

    //==============================================================================
    Impl(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl& operator=(Impl&&) = delete;

    //==============================================================================
private:
    size_t n;
    PFFFT_Setup* setup;
    float* buffer;
    float* work;
};

#else
#error No suitable FFT engine found for your platform
#endif

//==============================================================================
// FastFourier Implementation
//==============================================================================

FastFourier::FastFourier(std::size_t size) : pImpl(std::make_unique<Impl>(size))
{
}

FastFourier::~FastFourier() {}

void FastFourier::forward(const float* realInput, std::complex<float>* cmplxOutput) noexcept
{
    pImpl->forward(realInput, cmplxOutput);
}

void FastFourier::inverse(const std::complex<float>* cmplxInput, float* realOutput) noexcept
{
    pImpl->inverse(cmplxInput, realOutput);
}

float FastFourier::getNormalizationFactor() const noexcept
{
    return pImpl->getNormalizationFactor();
}