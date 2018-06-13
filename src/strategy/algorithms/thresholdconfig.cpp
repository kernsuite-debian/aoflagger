#include "thresholdconfig.h"

#include <iostream>
#include <math.h>

#include "../../structures/image2d.h"

#include "../../util/rng.h"

#include "combinatorialthresholder.h"
#include "localfitmethod.h"
#include "thresholdtools.h"

ThresholdConfig::ThresholdConfig() :
	_method(SumThreshold), _distribution(Gaussian), _verbose(false), _minConnectedSamples(1)
{ }

void ThresholdConfig::InitializeLengthsDefault(unsigned count)
{
	if(count > 9 || count == 0)
		count = 9;
	struct ThresholdOperation o[9];
	o[0].length = 1;
	o[1].length = 2;
	o[2].length = 4;
	o[3].length = 8;
	o[4].length = 16;
	o[5].length = 32;
	o[6].length = 64;
	o[7].length = 128;
	o[8].length = 256;
	_horizontalOperations.clear();
	_verticalOperations.clear();
	for(unsigned i=0;i<count;++i)
	{
		_horizontalOperations.push_back(o[i]);
		_verticalOperations.push_back(o[i]);
	}
}

void ThresholdConfig::InitializeLengthsSingleSample()
{
	ThresholdOperation operation;
	operation.length = 1;
	_horizontalOperations.push_back(operation);
	_verticalOperations.push_back(operation);
}

void ThresholdConfig::InitializeThresholdsFromFirstThreshold(num_t firstThreshold, enum Distribution noiseDistribution)
{
	num_t expFactor;
	switch(_method) {
	default:
	case SumThreshold:
		expFactor = 1.5;
		break;
	case VarThreshold:
		expFactor = 1.2;
		break;
	}
	for(unsigned i=0;i<_horizontalOperations.size();++i)
	{
		_horizontalOperations[i].threshold = firstThreshold * pow(expFactor, logn(_horizontalOperations[i].length)/logn(2.0)) / (num_t) _horizontalOperations[i].length;
	}
	for(unsigned i=0;i<_verticalOperations.size();++i)
	{
		_verticalOperations[i].threshold = firstThreshold * pown(expFactor, logn(_verticalOperations[i].length)/logn(2.0)) / (num_t) _verticalOperations[i].length;
	}
	_distribution = noiseDistribution;
}

void ThresholdConfig::Execute(const Image2D* image, Mask2D* mask, bool additive, num_t timeSensitivity, num_t frequencySensitivity) const
{
	num_t timeFactor, frequencyFactor;
	
	switch(_distribution) {
		case Gaussian: {
		num_t mean, stddev;
		ThresholdTools::WinsorizedMeanAndStdDev(image, mask, mean, stddev);
		if(stddev == 0.0L)
		{
			timeFactor = timeSensitivity;
			frequencyFactor = frequencySensitivity;
		}
		else {
			timeFactor = stddev * timeSensitivity;
			frequencyFactor = stddev * frequencySensitivity;
		}
		if(_verbose)
			std::cout << "Stddev=" << stddev << " first time-direction threshold=" << _horizontalOperations[0].threshold*timeFactor << std::endl; 
		} break;
		case Rayleigh: {
		num_t mode = ThresholdTools::WinsorizedMode(image, mask);
		if(mode == 0.0L)
		{
			timeFactor = timeSensitivity;
			frequencyFactor = frequencySensitivity;
		}
		else {
			timeFactor = timeSensitivity * mode;
			frequencyFactor = frequencySensitivity * mode;
		}
		if(_verbose) {
			num_t mean, stddev;
			ThresholdTools::WinsorizedMeanAndStdDev(image, mask, mean, stddev);
			std::cout << "Mode=" << mode << " first time-direction threshold=" << _horizontalOperations[0].threshold*timeFactor << std::endl;
			std::cout << "Stddev=" << stddev << std::endl; 
		}
		} break;
		default:
		timeFactor = timeSensitivity;
		frequencyFactor = frequencySensitivity;
		break;
	}

	if(!additive)
		mask->SetAll<false>();
	Mask2D scratch(*mask);

	size_t operationCount = _horizontalOperations.size() > _verticalOperations.size() ?
		_horizontalOperations.size() : _verticalOperations.size();
	for(unsigned i=0;i<operationCount;++i) {
		switch(_method) {
			case SumThreshold:
			
			if(i < _horizontalOperations.size())
			{
				if(_verbose)
					std::cout << "Performing SumThreshold with length " << _horizontalOperations[i].length 
						<< ", threshold " << _horizontalOperations[i].threshold*timeFactor << "..." << std::endl;
				CombinatorialThresholder::HorizontalSumThresholdLarge(image, mask, &scratch, _horizontalOperations[i].length, _horizontalOperations[i].threshold*timeFactor);
			}
			
			if(i < _verticalOperations.size())
				CombinatorialThresholder::VerticalSumThresholdLarge(image, mask, &scratch, _verticalOperations[i].length, _verticalOperations[i].threshold*frequencyFactor);
			break;
			case VarThreshold:
			if(i < _horizontalOperations.size())
			{
				if(_verbose)
					std::cout << "Performing VarThreshold with length " << _horizontalOperations[i].length 
						<< ", threshold " << _horizontalOperations[i].threshold*timeFactor << "..." << std::endl;
				CombinatorialThresholder::HorizontalVarThreshold(image, mask, _horizontalOperations[i].length, _horizontalOperations[i].threshold*timeFactor);
			}
			if(i < _verticalOperations.size())
				CombinatorialThresholder::VerticalVarThreshold(image, mask, _verticalOperations[i].length, _verticalOperations[i].threshold*frequencyFactor);
			break;
		}
	}

	if(_minConnectedSamples > 1)
		ThresholdTools::FilterConnectedSamples(mask, _minConnectedSamples);
} 

