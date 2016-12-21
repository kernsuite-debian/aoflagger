#ifndef THRESHOLDMITIGATER_H
#define THRESHOLDMITIGATER_H

#include <cstddef>
#include <cstring>

#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"

#ifdef __SSE__
#define USE_INTRINSICS
#endif

class ThresholdMitigater{
	public:
		//static void Threshold(class Image2D &image, num_t threshold);

		template<size_t Length>
		static void HorizontalSumThreshold(Image2DCPtr input, Mask2DPtr mask, num_t threshold);
		
		template<size_t Length>
		static void VerticalSumThreshold(Image2DCPtr input, Mask2DPtr mask, num_t threshold);
		
		template<size_t Length>
		static void HorizontalSumThresholdLarge(Image2DCPtr input, Mask2DPtr mask, num_t threshold);

#ifdef USE_INTRINSICS
		template<size_t Length>
		static void VerticalSumThresholdLargeSSE(Image2DCPtr input, Mask2DPtr mask, num_t threshold);
		
		static void VerticalSumThresholdLargeSSE(Image2DCPtr input, Mask2DPtr mask, size_t length, num_t threshold);

		template<size_t Length>
		static void HorizontalSumThresholdLargeSSE(Image2DCPtr input, Mask2DPtr mask, num_t threshold);
		
		static void HorizontalSumThresholdLargeSSE(Image2DCPtr input, Mask2DPtr mask, size_t length, num_t threshold);
#endif
		
		template<size_t Length>
		static void VerticalSumThresholdLargeCompare(Image2DCPtr input, Mask2DPtr mask, num_t threshold);

		template<size_t Length>
		static void VerticalSumThresholdLarge(Image2DCPtr input, Mask2DPtr mask, num_t threshold);
		
		template<size_t Length>
		static void SumThresholdLarge(Image2DCPtr input, Mask2DPtr mask, num_t hThreshold, num_t vThreshold)
		{
			HorizontalSumThresholdLarge<Length>(input, mask, hThreshold);
			VerticalSumThresholdLarge<Length>(input, mask, vThreshold);
		}
		
		static void VerticalSumThresholdLarge(Image2DCPtr input, Mask2DPtr mask, size_t length, num_t threshold)
		{
#ifdef USE_INTRINSICS
			VerticalSumThresholdLargeSSE(input, mask, length, threshold);
#else
			VerticalSumThresholdLargeReference(input, mask, length, threshold);
#endif
		}
		
		static void VerticalSumThresholdLargeReference(Image2DCPtr input, Mask2DPtr mask, size_t length, num_t threshold);
		
		static void HorizontalSumThresholdLargeReference(Image2DCPtr input, Mask2DPtr mask, size_t length, num_t threshold);
		
		static void HorizontalSumThresholdLarge(Image2DCPtr input, Mask2DPtr mask, size_t length, num_t threshold)
		{
#ifdef USE_INTRINSICS
			HorizontalSumThresholdLargeSSE(input, mask, length, threshold);
#else
			HorizontalSumThresholdLargeReference(input, mask, length, threshold);
#endif
		}

		static void VarThreshold(Image2DCPtr input, Mask2DPtr mask, size_t length, num_t threshold);
		
		static void HorizontalVarThreshold(Image2DCPtr input, Mask2DPtr mask, size_t length, num_t threshold);
		
		static void VerticalVarThreshold(Image2DCPtr input, Mask2DPtr mask, size_t length, num_t threshold);
	private:
		ThresholdMitigater() { }
};

#undef USE_INSTRINSICS

#endif
