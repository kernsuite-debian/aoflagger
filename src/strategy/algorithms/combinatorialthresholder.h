#ifndef COMBINATORIAL_THRESHOLDER_H
#define COMBINATORIAL_THRESHOLDER_H

#include <cstddef>
#include <cstring>

#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"

class CombinatorialThresholder {
	public:
		template<size_t Length>
		static void HorizontalSumThreshold(const Image2D* input, Mask2D* mask, num_t threshold);
		
		template<size_t Length>
		static void VerticalSumThreshold(const Image2D* input, Mask2D* mask, num_t threshold);
		
		template<size_t Length>
		static void HorizontalSumThresholdLarge(const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold);

#ifdef __SSE__
		template<size_t Length>
		static void VerticalSumThresholdLargeSSE(const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold);
		
		static void VerticalSumThresholdLargeSSE(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold);

		template<size_t Length>
		static void HorizontalSumThresholdLargeSSE(const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold);
		
		static void HorizontalSumThresholdLargeSSE(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold);
#endif
		
#ifdef __AVX2__
		template<size_t Length>
		static void VerticalSumThresholdLargeAVX(const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold);
		
		static void VerticalSumThresholdLargeAVX(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold);
#endif
		
		template<size_t Length>
		static void VerticalSumThresholdLarge(const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold);
		
		template<size_t Length>
		static void SumThresholdLarge(const Image2D* input, Mask2D* mask, num_t hThreshold, num_t vThreshold)
		{
			HorizontalSumThresholdLarge<Length>(input, mask, hThreshold);
			VerticalSumThresholdLarge<Length>(input, mask, vThreshold);
		}
		
		static void VerticalSumThresholdLarge(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold)
		{
#if defined(__AVX2__)
			VerticalSumThresholdLargeAVX(input, mask, scratch, length, threshold);
#elif defined(__SSE__)
			VerticalSumThresholdLargeSSE(input, mask, scratch, length, threshold);
#else
			VerticalSumThresholdLargeReference(input, mask, scratch, length, threshold);
#endif
		}
		
		static void VerticalSumThresholdLargeReference(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold);
		
		static void HorizontalSumThresholdLargeReference(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold);
		
		static void HorizontalSumThresholdLarge(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold)
		{
#ifdef __SSE__
			HorizontalSumThresholdLargeSSE(input, mask, scratch, length, threshold);
#else
			HorizontalSumThresholdLargeReference(input, mask, scratch, length, threshold);
#endif
		}

		static void VarThreshold(const Image2D* input, Mask2D* mask, size_t length, num_t threshold);
		
		static void HorizontalVarThreshold(const Image2D* input, Mask2D* mask, size_t length, num_t threshold);
		
		static void VerticalVarThreshold(const Image2D* input, Mask2D* mask, size_t length, num_t threshold);
		
	private:
		CombinatorialThresholder() = delete;
};

#undef USE_INSTRINSICS

#endif
