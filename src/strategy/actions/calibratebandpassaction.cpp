#include "calibratebandpassaction.h"

#include "../algorithms/thresholdtools.h"

#include "../../util/uvector.h"

#include <vector>

#ifdef __SSE__
#define USE_INTRINSICS
#endif

#ifdef USE_INTRINSICS
#include <xmmintrin.h>
#endif

namespace rfiStrategy {

	void CalibrateBandpassAction::calibrateStepwise(TimeFrequencyData& data) const
	{
		const size_t height = data.ImageHeight();
		ao::uvector<num_t> stddev(_steps);
		for(size_t step=0; step!=_steps; ++step)
		{
			const size_t startY = step*height/_steps, endY = (step+1)*height/_steps;
			std::vector<num_t> dataVector((1+endY-startY) * data.ImageWidth() * data.ImageCount());
			std::vector<num_t>::iterator vecIter = dataVector.begin();
			const Mask2DCPtr mask = data.GetSingleMask();
			for(size_t i=0; i!=data.ImageCount(); ++i)
			{
				const Image2D &image = *data.GetImage(i);
				for(size_t y=startY; y!=endY; ++y)
				{
					const num_t* inputPtr = image.ValuePtr(0, y);
					const bool* maskPtr = mask->ValuePtr(0, y);
					for(size_t x=0; x!=image.Width(); ++x)
					{
						if(!*maskPtr && std::isfinite(*inputPtr))
						{
							*vecIter = *inputPtr;
							++vecIter;
						}
						++inputPtr;
						++maskPtr;
					}
				}
			}
			dataVector.resize(vecIter - dataVector.begin());
			
			num_t mean;
			ThresholdTools::WinsorizedMeanAndStdDev<num_t>(dataVector, mean, stddev[step]);
		}
			
		for(size_t i=0; i!=data.ImageCount(); ++i)
		{
			const Image2D &image = *data.GetImage(i);
			Image2DPtr destImage = Image2D::CreateUnsetImagePtr(image.Width(), image.Height());
			for(size_t step=0; step!=_steps; ++step)
			{
				const size_t startY = step*height/_steps, endY = (step+1)*height/_steps;
				float correctionFactor;
				if(stddev[step] == 0.0)
					correctionFactor = 0.0;
				else
					correctionFactor = 1.0 / stddev[step];
#ifdef USE_INTRINSICS
				const __m128 corrFact4 = _mm_set_ps(correctionFactor, correctionFactor, correctionFactor, correctionFactor);
#endif
				
				for(size_t y=startY; y!=endY; ++y)
				{
					const float *inputPtr = image.ValuePtr(0, y);
					float *destPtr = destImage->ValuePtr(0, y);
					
#ifdef USE_INTRINSICS
					for(size_t x=0;x<image.Width();x+=4)
					{
						_mm_store_ps(destPtr, _mm_mul_ps(corrFact4, _mm_load_ps(inputPtr)));
						inputPtr += 4;
						destPtr += 4;
					}
#else
					for(size_t x=0;x<image.Width();x++)
					{
						*destPtr = correctionFactor * *inputPtr;
						inputPtr ++;
						destPtr ++;
					}
#endif
				}
			}
			data.SetImage(i, std::move(destImage));
		}
	}

	void CalibrateBandpassAction::calibrateSmooth(TimeFrequencyData& data) const
	{
		TimeFrequencyData
			real = data.Make(TimeFrequencyData::RealPart),
			imag = data.Make(TimeFrequencyData::ImaginaryPart);
		const size_t height = data.ImageHeight();
		ao::uvector<double> rms(height);
		std::vector<std::complex<num_t>> dataVector;
		for(size_t y=0; y!=height; ++y)
		{
			dataVector.resize(data.ImageWidth() * data.ImageCount());
			auto vecIter = dataVector.begin();
			const Mask2DCPtr mask = data.GetSingleMask();
			for(size_t i=0; i!=real.ImageCount(); ++i)
			{
				const Image2D& realImage = *real.GetImage(i);
				const Image2D& imagImage = *imag.GetImage(i);
				const num_t* realPtr = realImage.ValuePtr(0, y);
				const num_t* imagPtr = imagImage.ValuePtr(0, y);
				const bool* maskPtr = mask->ValuePtr(0, y);
				for(size_t x=0; x!=realImage.Width(); ++x)
				{
					if(!*maskPtr && std::isfinite(*realPtr) && std::isfinite(*imagPtr))
					{
						*vecIter = std::complex<num_t>(*realPtr, *imagPtr);
						++vecIter;
					}
					++realPtr;
					++imagPtr;
					++maskPtr;
				}
			}
			dataVector.resize(vecIter - dataVector.begin());
			
			rms[y] = ThresholdTools::WinsorizedRMS<num_t>(dataVector);
		}
			
		for(size_t i=0; i!=data.ImageCount(); ++i)
		{
			const Image2D &image = *data.GetImage(i);
			Image2DPtr destImage = Image2D::CreateUnsetImagePtr(image.Width(), image.Height());
			for(size_t y=0; y!=height; ++y)
			{
				num_t correctionFactor;
				if(rms[y] == 0.0)
					correctionFactor = 0.0;
				else
					correctionFactor = 1.0 / rms[y];
				
				const num_t *inputPtr = image.ValuePtr(0, y);
				num_t *destPtr = destImage->ValuePtr(0, y);
				for(size_t x=0;x<image.Width();x++)
				{
					*destPtr = correctionFactor * *inputPtr;
					++inputPtr;
					++destPtr;
				}
			}
			data.SetImage(i, std::move(destImage));
		}
	}
}
