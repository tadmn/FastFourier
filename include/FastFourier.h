/*==============================================================================
  Copyright (c) 2020 - Fielding DSP GmbH

  Created - 19. October 2018
==============================================================================*/

#pragma once

#include <complex>

//==============================================================================
class FastFourier
{
public:
    //==============================================================================
    /** Constructor
     *
        @param size number of real coefficients in transform.
    */
    explicit FastFourier(std::size_t size);

    /** Destructor */
    ~FastFourier();

    //==============================================================================
    /** Do a forward Fourier transform (real -> complex)

        @param realInput   real input coefficients. Array must have the same size as the
                           as the size parameter specified in the constructor. The pointer
                           must also be SIMD aligned!
        @param cmplxOutput complex output coefficients. Array must have (size/2)+1 number of
                           complex coefficients. The pointer must also be SIMD aligned!
    */
    void forward(const float* realInput, std::complex<float>* cmplxOutput) noexcept;

    /** Do an inverse Fourier transform (complex -> real)

        @param cmplxInput  complex input coefficients. Array must have (size/2)+1 number of
                           complex coefficients. The pointer must also be SIMD aligned!
        @param realOutput  real output coefficients. Array must have the same size as the
                           as the size parameter specified in the constructor. The pointer
                           must also be SIMD aligned!
    */
    void inverse(const std::complex<float>* cmplxInput, float* realOutput) noexcept;

    /** Returns the normalization factor

        Depending on which FFT engine is used internally, the normalization factor may
        be different. You can use this to integrate the normalization step into a multiplication
        step which you are doing anyways.
    */
    float getNormalizationFactor() const noexcept;

    /** Prevent copying */
    FastFourier(const FastFourier&) = delete;
    FastFourier& operator=(const FastFourier&) = delete;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};