#ifndef HIGHPASS_FILTER_H
#define HIGHPASS_FILTER_H

#include "../structures/image2d.h"
#include "../structures/mask2d.h"

#ifdef __SSE__
#define USE_INTRINSICS
#endif

namespace algorithms {

/**
 * This class is able to perform a Gaussian high pass filter on an
 * Image2D .
 */
class HighPassFilter {
 public:
  /**
   * Construct a new high pass filter with default parameters
   */
  HighPassFilter()
      : _hKernel(nullptr),
        _hWindowSize(22),
        _hKernelSigmaSq(7.5),
        _vKernel(nullptr),
        _vWindowSize(45),
        _vKernelSigmaSq(15.0) {}

  ~HighPassFilter();

  /**
   * Apply a Gaussian high pass filter on the given image.
   */
  /*Image2DPtr ApplyHighPass(const Image2DCPtr &image)
  {
          initializeKernel();
          Image2DPtr temp(new Image2D(*image));
          return Image2D::CreateFromDiff(image, temp);
  }*/

  /**
   * Apply a Gaussian high-pass filter on the given image, ignoring
   * flagged samples.
   */
  Image2DPtr ApplyHighPass(const Image2DCPtr& image, const Mask2DCPtr& mask);

  /**
   * Apply a Gaussian low-pass filter on the given image, ignoring
   * flagged samples.
   */
  Image2DPtr ApplyLowPass(const Image2DCPtr& image, const Mask2DCPtr& mask);

  /**
   * Set the horizontal size of the sliding window in samples. Must be odd: if
   * the given parameter is not odd, it will be incremented by one.
   */
  void SetHWindowSize(const unsigned hWindowSize) {
    delete[] _hKernel;
    _hKernel = nullptr;
    if ((hWindowSize % 2) == 0)
      _hWindowSize = hWindowSize + 1;
    else
      _hWindowSize = hWindowSize;
  }

  /**
   * Horizontal size of the sliding window in samples.
   * @see SetHWindowSize()
   */
  unsigned HWindowSize() const { return _hWindowSize; }

  /**
   * Set the vertical size of the sliding window in samples. Must be odd: if the
   * given parameter is not odd, it will be incremented by one.
   */
  void SetVWindowSize(const unsigned vWindowSize) {
    delete[] _vKernel;
    _vKernel = nullptr;
    if ((vWindowSize % 2) == 0)
      _vWindowSize = vWindowSize + 1;
    else
      _vWindowSize = vWindowSize;
  }

  /**
   * Vertical size of the sliding window in samples.
   * @see SetVWindowSize()
   */
  unsigned VWindowSize() const { return _vWindowSize; }

  /**
   * Gaussian sigma parameter defining the horizontal shape of the convolution.
   * Given in units of samples. Note that the window has limited size as defined
   * by
   * @ref HSquareSize and @ref VSquareSize. Byond those values, the kernel is
   * truncated.
   */
  double HKernelSigmaSq() const { return _hKernelSigmaSq; }

  /**
   * Set the horizontal sigma parameter of the kernel.
   * @see HKernelSigma()
   */
  void SetHKernelSigmaSq(double newSigmaSquared) {
    delete[] _hKernel;
    _hKernel = nullptr;
    _hKernelSigmaSq = newSigmaSquared;
  }

  /**
   * Gaussian sigma parameter defining the horizontal shape of the convolution.
   * Given in units of samples. Note that the window has limited size as defined
   * by
   * @ref HSquareSize and @ref VSquareSize. Byond those values, the kernel is
   * truncated.
   */
  double VKernelSigmaSq() const { return _vKernelSigmaSq; }

  /**
   * Set the horizontal sigma parameter of the kernel.
   * @see VKernelSigma()
   */
  void SetVKernelSigmaSq(double newSigmaSquared) {
    delete[] _vKernel;
    _vKernel = nullptr;
    _vKernelSigmaSq = newSigmaSquared;
  }

 private:
  /**
   * Applies the low-pass convolution. Kernel has to be initialized
   * before calling.
   */
  void applyLowPass(const Image2DPtr& image) {
#ifdef USE_INTRINSICS
    applyLowPassSSE(image);
#else
    applyLowPassSimple(image);
#endif
  }
  void applyLowPassSimple(const Image2DPtr& image);
  void applyLowPassSSE(const Image2DPtr& image);

  void initializeKernel();

  void setFlaggedValuesToZeroAndMakeWeights(const Image2DCPtr& inputImage,
                                            const Image2DPtr& outputImage,
                                            const Mask2DCPtr& inputMask,
                                            const Image2DPtr& weightsOutput) {
#ifdef USE_INTRINSICS
    setFlaggedValuesToZeroAndMakeWeightsSSE(inputImage, outputImage, inputMask,
                                            weightsOutput);
#else
    setFlaggedValuesToZeroAndMakeWeightsSimple(inputImage, outputImage,
                                               inputMask, weightsOutput);
#endif
  }
  void setFlaggedValuesToZeroAndMakeWeightsSimple(
      const Image2DCPtr& inputImage, const Image2DPtr& outputImage,
      const Mask2DCPtr& inputMask, const Image2DPtr& weightsOutput);
  void setFlaggedValuesToZeroAndMakeWeightsSSE(const Image2DCPtr& inputImage,
                                               const Image2DPtr& outputImage,
                                               const Mask2DCPtr& inputMask,
                                               const Image2DPtr& weightsOutput);

  void elementWiseDivide(const Image2DPtr& leftHand,
                         const Image2DCPtr& rightHand) {
#ifdef USE_INTRINSICS
    elementWiseDivideSSE(leftHand, rightHand);
#else
    elementWiseDivideSimple(leftHand, rightHand);
#endif
  }
  void elementWiseDivideSimple(const Image2DPtr& leftHand,
                               const Image2DCPtr& rightHand);
  void elementWiseDivideSSE(const Image2DPtr& leftHand,
                            const Image2DCPtr& rightHand);

  /**
   * The values of the kernel used in the convolution. This kernel is applied
   * horizontally.
   */
  num_t* _hKernel;

  /**
   * The horizontal size of the sliding window in samples. Must be odd.
   */
  unsigned _hWindowSize;

  /**
   * Gaussian sigma parameter defining the horizontal shape of the convolution.
   * Given in units of samples (squared). Note that the window has limited size
   * as defined by
   * @ref _hSquareSize and @ref _vSquareSize.
   */
  double _hKernelSigmaSq;

  /**
   * Vertical kernel values, see @ref _hKernel.
   */
  num_t* _vKernel;

  /**
   * The vertical size of the window, see @ref _hSquareSize.
   */
  unsigned _vWindowSize;

  /**
   * Gaussian sigma (squared) parameter defining the  vertical shape of the
   * convolution, see
   * @ref _hKernelSize.
   */
  double _vKernelSigmaSq;
};

}  // namespace algorithms

#undef USE_INTRINSICS

#endif  // HIGHPASS_FILTER_H
