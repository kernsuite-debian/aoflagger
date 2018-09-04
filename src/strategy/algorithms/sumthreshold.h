#ifndef SUMTHRESHOLD_H
#define SUMTHRESHOLD_H

#include <cstddef>
#include <cstring>

#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"

class SumThreshold
{
public:
	template<size_t Length>
	static void Horizontal(const Image2D* input, Mask2D* mask, num_t threshold);
	
	template<size_t Length>
	static void Vertical(const Image2D* input, Mask2D* mask, num_t threshold);
	
	template<size_t Length>
	static void HorizontalLarge(const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold);

#ifdef __SSE__
	template<size_t Length>
	static void VerticalLargeSSE(const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold);
	
	static void VerticalLargeSSE(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold);

	template<size_t Length>
	static void HorizontalLargeSSE(const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold);
	
	static void HorizontalLargeSSE(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold);
#endif
	
#ifdef __AVX2__
	template<size_t Length>
	static void VerticalLargeAVX(const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold);
	
	static void VerticalLargeAVX(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold);
#endif
	
	template<size_t Length>
	static void VerticalLarge(const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold);
	
	template<size_t Length>
	static void Large(const Image2D* input, Mask2D* mask, num_t hThreshold, num_t vThreshold)
	{
		HorizontalLarge<Length>(input, mask, hThreshold);
		VerticalLarge<Length>(input, mask, vThreshold);
	}
	
	static void VerticalLarge(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold)
	{
#if defined(__AVX2__)
		VerticalLargeAVX(input, mask, scratch, length, threshold);
#elif defined(__SSE__)
		VerticalLargeSSE(input, mask, scratch, length, threshold);
#else
		VerticalLargeReference(input, mask, scratch, length, threshold);
#endif
	}
	
	static void VerticalLargeReference(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold);
	
	static void HorizontalLargeReference(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold);
	
	static void HorizontalLarge(const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length, num_t threshold)
	{
#ifdef __SSE__
		HorizontalLargeSSE(input, mask, scratch, length, threshold);
#else
		HorizontalLargeReference(input, mask, scratch, length, threshold);
#endif
	}
};

#endif
