#ifndef RFISTRATEGY_CALIBRATE_PASSBAND_ACTION_H
#define RFISTRATEGY_CALIBRATE_PASSBAND_ACTION_H

#include <map>

#include "action.h"

#include "../control/artifactset.h"

#include "../algorithms/normalizepassband.h"

namespace rfiStrategy {

class CalibrateBandpassAction : public Action {
	public:
		enum Method {
			StepwiseMethod=0,
			SmoothMethod=1
		};
		
		CalibrateBandpassAction() :
			_steps(48),
			_method(StepwiseMethod)
		{
		}
		virtual std::string Description() final override
		{
			return "Calibrate bandpass";
		}
		virtual void Perform(ArtifactSet& artifacts, ProgressListener& progress) final override
		{
			switch(_method)
			{
				case StepwiseMethod: 
					NormalizePassband::NormalizeStepwise(artifacts.ContaminatedData(), _steps);
					break;
				case SmoothMethod:
					NormalizePassband::NormalizeSmooth(artifacts.ContaminatedData());
					break;
			}
		}

		virtual ActionType Type() const final override { return CalibrateBandpassActionType; }

		size_t Steps() const { return _steps; }
		void SetSteps(size_t steps) { _steps = steps; }
		
		enum Method GetMethod() const { return _method; }
		void SetMethod(enum Method method) { _method = method; }
		
	private:
		void calibrateStepwise(TimeFrequencyData& data) const;
		void calibrateSmooth(TimeFrequencyData& data) const;
		
		size_t _steps;
		enum Method _method;
};

}

#endif
