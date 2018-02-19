#include "statisticalflagger.h"

StatisticalFlagger::StatisticalFlagger()
{
}


StatisticalFlagger::~StatisticalFlagger()
{
}

bool StatisticalFlagger::SquareContainsFlag(const Mask2D* mask, size_t xLeft, size_t yTop, size_t xRight, size_t yBottom)
{
	for(size_t y=yTop;y<=yBottom;++y)
	{
		for(size_t x=xLeft;x<=xRight;++x)
		{
			if(mask->Value(x, y))
				return true;
		}
	}
	return false;
}

void StatisticalFlagger::DilateFlagsHorizontally(Mask2D* mask, size_t timeSize)
{
	if(timeSize != 0)
	{
		Mask2D destination(Mask2D::MakeUnsetMask(mask->Width(), mask->Height()));
		if(timeSize > mask->Width()) timeSize = mask->Width();
		const int intSize = (int) timeSize;
		
		for(size_t y=0;y<mask->Height();++y)
		{
			int dist = intSize + 1;
			for(size_t x=0;x<timeSize;++x)
			{
				if(mask->Value(x, y))
					dist = - intSize;
				dist++;
			}
			for(size_t x=0;x<mask->Width() - timeSize;++x)
			{
				if(mask->Value(x + timeSize, y))
					dist = -intSize;
				if(dist <= intSize)
				{
					destination.SetValue(x, y, true);
					dist++;
				} else {
					destination.SetValue(x, y, false);
				}
			}
			for(size_t x=mask->Width() - timeSize;x<mask->Width();++x)
			{
				if(dist <= intSize)
				{
					destination.SetValue(x, y, true);
					dist++;
				} else {
					destination.SetValue(x, y, false);
				}
			}
		}
		*mask = std::move(destination);
	}
}

void StatisticalFlagger::DilateFlagsVertically(Mask2D* mask, size_t frequencySize)
{
	if(frequencySize != 0)
	{
		Mask2D destination(Mask2D::MakeUnsetMask(mask->Width(), mask->Height()));
		if(frequencySize > mask->Height()) frequencySize = mask->Height();
		const int intSize = (int) frequencySize;
		
		for(size_t x=0;x<mask->Width();++x)
		{
			int dist = intSize + 1;
			for(size_t y=0;y<frequencySize;++y)
			{
				if(mask->Value(x, y))
					dist = - intSize;
				dist++;
			}
			for(size_t y=0;y<mask->Height() - frequencySize;++y)
			{
				if(mask->Value(x, y + frequencySize))
					dist = -intSize;
				if(dist <= intSize)
				{
					destination.SetValue(x, y, true);
					dist++;
				} else {
					destination.SetValue(x, y, false);
				}
			}
			for(size_t y=mask->Height() - frequencySize;y<mask->Height();++y)
			{
				if(dist <= intSize)
				{
					destination.SetValue(x, y, true);
					dist++;
				} else {
					destination.SetValue(x, y, false);
				}
			}
		}
		*mask = std::move(destination);
	}
}

void StatisticalFlagger::LineRemover(Mask2D* mask, size_t maxTimeContamination, size_t maxFreqContamination)
{
	for(size_t x=0;x<mask->Width();++x)
	{
		size_t count = 0;
		for(size_t y=0;y<mask->Height();++y)
		{
			if(mask->Value(x,y))
				++count;
		}
		if(count > maxFreqContamination)
			FlagTime(mask, x);
	}

	for(size_t y=0;y<mask->Height();++y)
	{
		size_t count = 0;
		for(size_t x=0;x<mask->Width();++x)
		{
			if(mask->Value(x,y))
				++count;
		}
		if(count > maxTimeContamination)
			FlagFrequency(mask, y);
	}
}

void StatisticalFlagger::FlagTime(Mask2D* mask, size_t x)
{
	for(size_t y=0;y<mask->Height();++y)
	{
		mask->SetValue(x, y, true);
	}
}

void StatisticalFlagger::FlagFrequency(Mask2D* mask, size_t y)
{
	for(size_t x=0;x<mask->Width();++x)
	{
		mask->SetValue(x, y, true);
	}
}

void StatisticalFlagger::MaskToInts(const Mask2D* mask, int **maskAsInt)
{
	for(size_t y=0;y<mask->Height();++y)
	{
		int *column = maskAsInt[y];
		for(size_t x=0;x<mask->Width();++x)
		{
			column[x] = mask->Value(x, y) ? 1 : 0;
		}
	}
}

void StatisticalFlagger::SumToLeft(const Mask2D* mask, int **sums, size_t width, size_t step, bool reverse)
{
	if(reverse)
	{
		for(size_t y=0;y<mask->Height();++y)
		{
			int *column = sums[y];
			for(size_t x=width;x<mask->Width();++x)
			{
				if(mask->Value(x - width/2, y))
					column[x] += step;
			}
		}
	} else {
		for(size_t y=0;y<mask->Height();++y)
		{
			int *column = sums[y];
			for(size_t x=0;x<mask->Width() - width;++x)
			{
				if(mask->Value(x + width/2, y))
					column[x] += step;
			}
		}
	}
}

void StatisticalFlagger::SumToTop(const Mask2D* mask, int **sums, size_t width, size_t step, bool reverse)
{
	if(reverse)
	{
		for(size_t y=width;y<mask->Height();++y)
		{
			int *column = sums[y];
			for(size_t x=0;x<mask->Width();++x)
			{
				if(mask->Value(x, y - width/2))
					column[x] += step;
			}
		}
	} else {
		for(size_t y=0;y<mask->Height() - width;++y)
		{
			int *column = sums[y];
			for(size_t x=0;x<mask->Width();++x)
			{
				if(mask->Value(x, y + width/2))
					column[x] += step;
			}
		}
	}
}

void StatisticalFlagger::ThresholdTime(const Mask2D* mask, int **flagMarks, int **sums, int thresholdLevel, int width)
{
	int halfWidthL = (width-1) / 2;
	int halfWidthR = (width-1) / 2;
	for(size_t y=0;y<mask->Height();++y)
	{
		const int *column = sums[y];
		for(size_t x=halfWidthL;x<mask->Width() - halfWidthR;++x)
		{
			if(column[x] > thresholdLevel)
			{
				const unsigned right = x+halfWidthR+1;
				++flagMarks[y][x-halfWidthL];
				if(right < mask->Width())
					--flagMarks[y][right];
			}
		}
	}
}

void StatisticalFlagger::ThresholdFrequency(const Mask2D* mask, int **flagMarks, int **sums, int thresholdLevel, int width)
{
	int halfWidthT = (width-1) / 2;
	int halfWidthB = (width-1) / 2;
	for(size_t y=halfWidthT;y<mask->Height() - halfWidthB;++y)
	{
		int *column = sums[y];
		for(size_t x=0;x<mask->Width();++x)
		{
			if(column[x] > thresholdLevel)
			{
				const unsigned bottom = y+halfWidthB+1;
				++flagMarks[y-halfWidthT][x];
				if(bottom < mask->Height())
					--flagMarks[bottom][x];
			}
		}
	}
}

void StatisticalFlagger::ApplyMarksInTime(Mask2D* mask, int **flagMarks)
{
	for(size_t y=0;y<mask->Height();++y)
	{
		int startedCount = 0;
		for(size_t x=0;x<mask->Width();++x)
		{
			startedCount += flagMarks[y][x];
			if(startedCount > 0)
				mask->SetValue(x, y, true);
		}
	}
}

void StatisticalFlagger::ApplyMarksInFrequency(Mask2D* mask, int **flagMarks)
{
	for(size_t x=0;x<mask->Width();++x)
	{
		int startedCount = 0;
		for(size_t y=0;y<mask->Height();++y)
		{
			startedCount += flagMarks[y][x];
			if(startedCount > 0)
				mask->SetValue(x, y, true);
		}
	}
}

void StatisticalFlagger::DensityTimeFlagger(Mask2D* mask, num_t minimumGoodDataRatio)
{
	num_t width = 2.0;
	size_t iterations = 0, step = 1;
	bool reverse = false;
	
	//"sums represents the number of flags in a certain range
	int **sums = new int*[mask->Height()];
	
	// flagMarks are integers that represent the number of times an area is marked as the
	// start or end of a flagged area. For example, if flagMarks[0][0] = 0, it is not the start or
	// end of an area. If it is 1, it is the start. If it is -1, it is the end. A range of
	// [2 0 -1 -1 0] produces a flag mask [T T T T F].
	int **flagMarks = new int*[mask->Height()];
	
	for(size_t y=0;y<mask->Height();++y)
	{
		sums[y] = new int[mask->Width()];
		flagMarks[y] = new int[mask->Width()];
		for(size_t x=0;x<mask->Width();++x)
			flagMarks[y][x] = 0;
	}
	
	MaskToInts(mask, sums);
	
	while(width < mask->Width())
	{
		++iterations;
		SumToLeft(mask, sums, (size_t) width, step, reverse);
		const int maxFlagged = (int) floor((1.0-minimumGoodDataRatio)*(num_t)(width));
		ThresholdTime(mask, flagMarks, sums, maxFlagged, (size_t) width);
	
		num_t newWidth = width * 1.05;
		if((size_t) newWidth == (size_t) width)
			newWidth = width + 1.0;
		step = (size_t) (newWidth - width);
		width = newWidth;
		reverse = !reverse;
	}
	
	ApplyMarksInTime(mask, flagMarks);

	for(size_t y=0;y<mask->Height();++y)
	{
		delete[] sums[y];
		delete[] flagMarks[y];
	}
	delete[] sums;
	delete[] flagMarks;
}

void StatisticalFlagger::DensityFrequencyFlagger(Mask2D* mask, num_t minimumGoodDataRatio)
{
	num_t width = 2.0;
	size_t iterations = 0, step = 1;
	bool reverse = false;
	
	Mask2DPtr newMask(new Mask2D(*mask));
	
	int **sums = new int*[mask->Height()];
	int **flagMarks = new int*[mask->Height()];
	
	for(size_t y=0;y<mask->Height();++y)
	{
		sums[y] = new int[mask->Width()];
		flagMarks[y] = new int[mask->Width()];
		for(size_t x=0;x<mask->Width();++x)
			flagMarks[y][x] = 0;
	}
	
	MaskToInts(mask, sums);
	
	while(width < mask->Height())
	{
		++iterations;
		SumToTop(mask, sums, (size_t) width, step, reverse);
		const int maxFlagged = (int) floor((1.0-minimumGoodDataRatio)*(num_t)(width));
		ThresholdFrequency(mask, flagMarks, sums, maxFlagged, (size_t) width);
	
		num_t newWidth = width * 1.05;
		if((size_t) newWidth == (size_t) width)
			newWidth = width + 1.0;
		step = (size_t) (newWidth - width);
		width = newWidth;
		reverse = !reverse;
	}

	ApplyMarksInFrequency(mask, flagMarks);

	for(size_t y=0;y<mask->Height();++y)
	{
		delete[] sums[y];
		delete[] flagMarks[y];
	}
	delete[] sums;
	delete[] flagMarks;
}
