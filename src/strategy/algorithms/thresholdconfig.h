#ifndef THRESHOLDCONFIG_H
#define THRESHOLDCONFIG_H

#include <vector>

#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"

class ThresholdConfig {
	public:
		enum Method { SumThreshold, VarThreshold };
		enum Distribution { Uniform, Gaussian, Rayleigh };
		ThresholdConfig();
		
		void InitializeLengthsDefault(unsigned count=0);
		
		void InitializeLengthsSingleSample();
		
		void InitializeThresholdsFromFirstThreshold(num_t firstThreshold, enum Distribution noiseDistribution);
		
		void Execute(const Image2D* image, Mask2D* mask, bool additive, num_t timeSensitivity, num_t frequencySensitivity) const;
		void SetVerbose(bool verbose) { _verbose = verbose; }
		void SetMethod(Method method) { _method = method; }
		
		num_t GetHorizontalThreshold(unsigned index) const
		{ return _horizontalOperations[index].threshold; }
		
		num_t GetVerticalThreshold(unsigned index) const
		{ return _verticalOperations[index].threshold; }
		
		void SetHorizontalThreshold(unsigned index, num_t threshold)
		{ _horizontalOperations[index].threshold = threshold; }
		
		void SetVerticalThreshold(unsigned index, num_t threshold)
		{ _verticalOperations[index].threshold = threshold; }
		
		size_t GetHorizontalLength(unsigned index) const
		{ return _horizontalOperations[index].length; }
		
		size_t GetVerticalLength(unsigned index) const
		{ return _verticalOperations[index].length; }
		
		size_t GetHorizontalOperations() const { return _horizontalOperations.size(); }
		size_t GetVerticalOperations() const { return _horizontalOperations.size(); }
		void SetMinimumConnectedSamples(size_t val) { _minConnectedSamples = val; }
		void RemoveHorizontalOperations() { _horizontalOperations.clear(); }
		void RemoveVerticalOperations() { _verticalOperations.clear(); }
		
	private:
		struct ThresholdOperation {
			size_t length;
			num_t threshold;
			num_t rate;
		};
		std::vector<ThresholdOperation> _horizontalOperations;
		std::vector<ThresholdOperation> _verticalOperations;
		Method _method;
		Distribution _distribution;
		bool _verbose;
		size_t _minConnectedSamples;
};

#endif
