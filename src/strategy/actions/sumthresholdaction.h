#ifndef RFI_THRESHOLDACTION
#define RFI_THRESHOLDACTION

#include "../algorithms/thresholdconfig.h"

#include "action.h"
#include "../control/artifactset.h"
#include "../control/actionblock.h"

namespace rfiStrategy {

	class SumThresholdAction : public Action
	{
			public:
				SumThresholdAction() :
					_timeDirectionSensitivity(1.0), _frequencyDirectionSensitivity(1.0),
					_inTimeDirection(true), _inFrequencyDirection(true)
				{
				}
				
				std::string Description() final override
				{
					return "SumThreshold";
				}
				
				void Perform(ArtifactSet &artifacts, class ProgressListener &) final override
				{
					ThresholdConfig thresholdConfig;
					thresholdConfig.InitializeLengthsDefault();
					thresholdConfig.InitializeThresholdsFromFirstThreshold(6.0L, ThresholdConfig::Rayleigh);
					if(!_inTimeDirection)
						thresholdConfig.RemoveHorizontalOperations();
					if(!_inFrequencyDirection)
						thresholdConfig.RemoveVerticalOperations();
					
					TimeFrequencyData& contaminated = artifacts.ContaminatedData();
					Mask2DPtr mask(new Mask2D(*contaminated.GetSingleMask()));
					const Image2DCPtr image = contaminated.GetSingleImage();
					thresholdConfig.Execute(image.get(), mask.get(), false, artifacts.Sensitivity() * _timeDirectionSensitivity, artifacts.Sensitivity() * _frequencyDirectionSensitivity);
					contaminated.SetGlobalMask(mask);
				}
				
				num_t TimeDirectionSensitivity() const { return _timeDirectionSensitivity; }
				void SetTimeDirectionSensitivity(num_t sensitivity)
				{
					_timeDirectionSensitivity = sensitivity;
				}
				
				num_t FrequencyDirectionSensitivity() const { return _frequencyDirectionSensitivity; }
				void SetFrequencyDirectionSensitivity(num_t sensitivity)
				{
					_frequencyDirectionSensitivity = sensitivity;
				}
				
				ActionType Type() const final override { return SumThresholdActionType; }
				
				bool TimeDirectionFlagging() const { return _inTimeDirection; }
				void SetTimeDirectionFlagging(bool timeDirection) { _inTimeDirection = timeDirection; }
				
				bool FrequencyDirectionFlagging() const { return _inFrequencyDirection; }
				void SetFrequencyDirectionFlagging(bool frequencyDirection) { _inFrequencyDirection = frequencyDirection; }
				
			private:
				num_t _timeDirectionSensitivity;
				num_t _frequencyDirectionSensitivity;
				bool _inTimeDirection;
				bool _inFrequencyDirection;
	};

} // namespace

#endif // RFI_THRESHOLDACTION
