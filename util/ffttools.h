/** @file
 * This is the header file for the FFTTools class.
 * @author André Offringa <offringa@gmail.com>
 */
#ifndef FFTTOOLS_H
#define FFTTOOLS_H

#include "../structures/image2d.h"
#include "../structures/samplerow.h"

/**
 * This is a wrapper around the fftw3 library that is able to calculate fast
 * Fourier transformations.
 */
class FFTTools {
 public:
  /**
   * What will be outputted by the FFT method.
   */
  enum FFTOutputMethod {
    /**
     * The transformation to real data.
     */
    Real,
    /**
     * The transformation to imaginary data.
     */
    Imaginary,
    /**
     * The transformation to the absolute value of real and imaginary.
     */
    Absolute,
    /**
     * The transformation to the real and the imaginary data.
     */
    Both
  };
  /**
   * Create the FFT of an image.
   * @param original The original image.
   * @param method Which method to use.
   * @return A new image that contains the FFT. The caller should @c delete the
   * image when it is no longer required.
   */
  static Image2D* CreateFFTImage(const Image2D& original,
                                 FFTOutputMethod method);
  static Image2DPtr CreateFFTImagePtr(Image2DCPtr original,
                                      FFTOutputMethod method) {
    return Image2DPtr(CreateFFTImage(*original, method));
  }
  static void CreateFFTImage(const Image2D& real, const Image2D& imaginary,
                             Image2D& realOut, Image2D& imaginaryOut,
                             bool centerAfter = true, bool negate = false);
  static Image2D* CreateFullImageFromFFT(const Image2D& fft);
  static Image2D* CreateShiftedImageFromFFT(const Image2D& fft);
  static Image2D* CreateAbsoluteImage(const Image2D& real,
                                      const Image2D& imaginary);
  static Image2DPtr CreateAbsoluteImage(Image2DCPtr real,
                                        Image2DCPtr imaginary) {
    return Image2DPtr(CreateAbsoluteImage(*real, *imaginary));
  }
  static Image2DPtr CreatePhaseImage(Image2DCPtr real, Image2DCPtr imaginary);
  static void FFTConvolve(const Image2D& realIn, const Image2D& imaginaryIn,
                          const Image2D& realKernel,
                          const Image2D& imaginaryKernel, Image2D& outReal,
                          Image2D& outImaginary);
  static void FFTConvolveFFTKernel(const Image2D& realIn,
                                   const Image2D& imaginaryIn,
                                   const Image2D& realFFTKernel,
                                   const Image2D& imaginaryFFTKernel,
                                   Image2D& outReal, Image2D& outImaginary);
  static void Multiply(Image2D& left, const Image2D& right);
  static void Divide(Image2D& left, const Image2D& right);
  static void Multiply(Image2D& leftReal, Image2D& leftImaginary,
                       const Image2D& rightReal, const Image2D& rightImaginary);

  static void Sqrt(Image2D& image);
  static void Sqrt(Image2DPtr image) { Sqrt(*image); }
  static void SignedSqrt(Image2D& image);
  static void SignedSqrt(Image2DPtr image) { SignedSqrt(*image); }

  static void CreateHorizontalFFTImage(Image2D& real, Image2D& imaginary,
                                       bool inverse = false);
  static void CreateDynamicHorizontalFFTImage(Image2DPtr real,
                                              Image2DPtr imaginary,
                                              unsigned sections,
                                              bool inverse = false);
  static Image2DPtr AngularTransform(Image2DCPtr input);
  static void FFT(SampleRow& realRow, SampleRow& imaginaryRow);
  // static void FFTConvolve(num_t *realValues, num_t *imagValues, num_t
  // *realKernel, num_t *imagKernel, size_t count);
 private:
  FFTTools() {}
  ~FFTTools(){};
};

#endif
