#ifndef RFISTRATEGY_CALIBRATE_PASSBAND_ACTION_H
#define RFISTRATEGY_CALIBRATE_PASSBAND_ACTION_H

#include <map>

#include "action.h"

#include "../control/artifactset.h"

namespace rfiStrategy {

	class CalibratePassbandAction : public Action {
		public:
			CalibratePassbandAction() :
				_steps(48)
			{
			}
			virtual std::string Description() final override
			{
				return "Calibrate passband";
			}
			virtual void Perform(ArtifactSet &artifacts, ProgressListener &progress) final override
			{
				calibrate(artifacts.ContaminatedData());
			}

			virtual ActionType Type() const final override { return CalibratePassbandActionType; }

			size_t Steps() const { return _steps; }
			void SetSteps(size_t steps) { _steps = steps; }
		private:
			void calibrate(TimeFrequencyData &data) const;
			
			size_t _steps;
	};
}

#endif
