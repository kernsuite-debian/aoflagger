#include "samplerow.h"

SampleRow SampleRow::MakeWithoutMissings() const
{
	size_t newSize = 0;
	for(num_t v : _values)
	{
		if(std::isfinite(v))
			++newSize;
	}
	SampleRow newRow(newSize);
	size_t indexToNew = 0;
	for(num_t v : _values)
	{
		if(std::isfinite(v))
		{
			newRow._values[indexToNew] = v;
			++indexToNew;
		}
	}
	return newRow;
}
