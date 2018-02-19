#ifndef INTERPOLATENANSALGORITHM_H
#define INTERPOLATENANSALGORITHM_H

#include "../../util/logger.h"

#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"

class InterpolateNansAlgorithm
{
public:
	static void CountNans(Image2DCPtr image)
	{
		size_t count = 0;
		for(unsigned y=0;y<image->Height();++y)
		{
			for(unsigned x=0;x<image->Width();++x)
			{
				if(!std::isfinite(image->Value(x, y)))
				{
					++count;
				}
			}
		}
		Logger::Debug << "Number of flags: " << count << '\n';
	}
	
	static double distFunc(int dist)
	{
		//return 1.0 / pown(2.0, dist);
		return 1.0 / dist;
	}
	
	static void InterpolateFlags(Image2D& image, const Mask2D& mask)
	{
		Mask2D todo(mask);
		size_t missingCount = 0, previousCount;
		do {
			previousCount = missingCount;
			missingCount = 0;
			for(unsigned y=0;y<image.Height();++y)
			{
				for(unsigned x=0;x<image.Width();++x)
				{
					if(todo.Value(x, y))
					{
						// find in each direction the first sample which is not flagged
						num_t weightLeft = 0.0, weightRight = 0.0, weightBottom = 0.0, weightTop = 0.0;
						num_t valueLeft = 0.0, valueRight = 0.0, valueBottom = 0.0, valueTop = 0.0;
						
						for(unsigned xf=x+1;xf < image.Width(); ++xf)
						{
							if(!todo.Value(xf, y))
							{
								valueRight = image.Value(xf, y);
								weightRight = distFunc(xf - x);
								break;
							}
						}
						
						for(int xf=x-1;xf >= 0; --xf)
						{
							if(!todo.Value(xf, y))
							{
								valueLeft = image.Value(xf, y);
								weightLeft = distFunc(x - xf);
								break;
							}
						}
						
						for(unsigned yf=y+1;yf < image.Height(); ++yf)
						{
							if(!todo.Value(x, yf))
							{
								valueTop = image.Value(x, yf);
								weightTop = distFunc(yf - y);
								break;
							}
						}
						
						for(int yf=y-1;yf >= 0; --yf)
						{
							if(!todo.Value(x, yf))
							{
								valueBottom = image.Value(x, yf);
								weightBottom = distFunc(y - yf);
								break;
							}
						}
						
						num_t totalWeight = weightRight + weightLeft + weightTop + weightBottom;
						num_t totalValue =
							valueRight * weightRight +
							valueLeft * weightLeft+
							valueTop * weightTop +
							valueBottom * weightBottom;
						if(totalWeight == 0.0)
						{
							missingCount++;
						}
						else {
							image.SetValue(x, y, totalValue / totalWeight);
							todo.SetValue(x, y, false);
						}
					} //if
				} //for
			} //for
		} while(missingCount > 0 && previousCount!=missingCount);
	}
};

#endif // INTERPOLATENANSALGORITHM_H
