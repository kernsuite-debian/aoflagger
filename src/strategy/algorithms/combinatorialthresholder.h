#ifndef COMBINATORIAL_THRESHOLDER_H
#define COMBINATORIAL_THRESHOLDER_H

#include <cstddef>
#include <cstring>

#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"

#ifdef __SSE__
#define USE_INTRINSICS
#endif

class CombinatorialThresholder {
	public:
		//static void Threshold(class Image2D &image, num_t threshold);

		template<size_t Length>
		static void HorizontalSumThreshold(const Image2D* input, Mask2D* mask, num_t threshold);
		
		template<size_t Length>
		static void VerticalSumThreshold(const Image2D* input, Mask2D* mask, num_t threshold);
		
		template<size_t Length>
		static void HorizontalSumThresholdLarge(const Image2D* input, Mask2D* mask, num_t threshold);

#ifdef USE_INTRINSICS
		template<size_t Length>
		static void VerticalSumThresholdLargeSSE(const Image2D* input, Mask2D* mask, num_t threshold);
		
		static void VerticalSumThresholdLargeSSE(const Image2D* input, Mask2D* mask, size_t length, num_t threshold);

		template<size_t Length>
		static void HorizontalSumThresholdLargeSSE(const Image2D* input, Mask2D* mask, num_t threshold);
		
		static void HorizontalSumThresholdLargeSSE(const Image2D* input, Mask2D* mask, size_t length, num_t threshold);
#endif
		
		template<size_t Length>
		static void VerticalSumThresholdLargeCompare(const Image2D* input, Mask2D* mask, num_t threshold);

		template<size_t Length>
		static void VerticalSumThresholdLarge(const Image2D* input, Mask2D* mask, num_t threshold);
		
		template<size_t Length>
		static void SumThresholdLarge(const Image2D* input, Mask2D* mask, num_t hThreshold, num_t vThreshold)
		{
			HorizontalSumThresholdLarge<Length>(input, mask, hThreshold);
			VerticalSumThresholdLarge<Length>(input, mask, vThreshold);
		}
		
		static void VerticalSumThresholdLarge(const Image2D* input, Mask2D* mask, size_t length, num_t threshold)
		{
#ifdef USE_INTRINSICS
			VerticalSumThresholdLargeSSE(input, mask, length, threshold);
#else
			VerticalSumThresholdLargeReference(input, mask, length, threshold);
#endif
		}
		
		static void VerticalSumThresholdLargeReference(const Image2D* input, Mask2D* mask, size_t length, num_t threshold);
		
		static void HorizontalSumThresholdLargeReference(const Image2D* input, Mask2D* mask, size_t length, num_t threshold);
		
		static void HorizontalSumThresholdLarge(const Image2D* input, Mask2D* mask, size_t length, num_t threshold)
		{
#ifdef USE_INTRINSICS
			HorizontalSumThresholdLargeSSE(input, mask, length, threshold);
#else
			HorizontalSumThresholdLargeReference(input, mask, length, threshold);
#endif
		}

		static void VarThreshold(const Image2D* input, Mask2D* mask, size_t length, num_t threshold);
		
		static void HorizontalVarThreshold(const Image2D* input, Mask2D* mask, size_t length, num_t threshold);
		
		static void VerticalVarThreshold(const Image2D* input, Mask2D* mask, size_t length, num_t threshold);
	private:
		CombinatorialThresholder() { }
};

#undef USE_INSTRINSICS

#endif
