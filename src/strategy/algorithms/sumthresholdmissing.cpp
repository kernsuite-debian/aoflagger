#include "sumthresholdmissing.h"

#include "../../structures/xyswappedmask2d.h"

template<typename ImageLike, typename MaskLike, typename CMaskLike>
void SumThresholdMissing::horizontal(const ImageLike& input, MaskLike& mask, const CMaskLike& missing, MaskLike& scratch, size_t length, num_t threshold)
{
	scratch = mask;
	const size_t width = mask.Width(), height = mask.Height();
	if(length <= width)
	{
		for(size_t y=0;y<height;++y)
		{
			num_t sum = 0.0;

			// Find first non-missing value for the start of the summation interval
			// xLeft points to the first element of the interval, which is marked as non-missing.
			size_t xLeft = 0;
			while(xLeft != width && missing.Value(xLeft, y))
				++xLeft;
			
			// xRight points to the last non-missing element of the interval
			size_t
				xRight = xLeft,
				countAdded = 0,
				countTotal = 0;
			while(countTotal+1 < length && xRight != width)
			{
				if(!missing.Value(xRight, y))
				{
					if(!mask.Value(xRight, y))
					{
						sum += input.Value(xRight, y);
						++countAdded;
					}
					++countTotal;
				}
				++xRight;
			}
			
			while(xRight != width)
			{
				// Add a sample at the right
				if(!mask.Value(xRight, y))
				{
					sum += input.Value(xRight, y);
					++countAdded;
				}
				// Check
				if(countAdded>0 && fabs(sum/countAdded) > threshold)
				{
					scratch.SetHorizontalValues(xLeft, y, true, xRight - xLeft + 1);
				}
				// subtract one sample at the left
				if(!mask.Value(xLeft, y))
				{
					sum -= input.Value(xLeft, y);
					--countAdded;
				}
				do {
					++xRight;
				} while(xRight != width && missing.Value(xRight, y));
				do {
					++xLeft;
					// it could happen that xLeft gets to width when the length is one...
					// for other lengths the first test is not necessary.
				} while(xLeft != width && missing.Value(xLeft, y));
			}
		}
	}
	mask = std::move(scratch);
}

template
void SumThresholdMissing::horizontal(const Image2D& input, Mask2D& mask, const Mask2D& missing, Mask2D& scratch, size_t length, num_t threshold);

void SumThresholdMissing::Vertical(const Image2D& input, Mask2D& mask, const Mask2D& missing, Mask2D& scratch, size_t length, num_t threshold)
{
	XYSwappedImage2D<const Image2D> swappedInput(input);
	XYSwappedMask2D<Mask2D> swappedMask(mask);
	XYSwappedMask2D<const Mask2D> swappedMissing(missing);
	XYSwappedMask2D<Mask2D> swappedScratch(scratch);
	horizontal(swappedInput, swappedMask, swappedMissing, swappedScratch, length, threshold);
}
