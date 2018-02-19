#ifndef RFIQUICKCALIBRATEACTION_H
#define RFIQUICKCALIBRATEACTION_H

#include "../algorithms/thresholdtools.h"

#include "../control/actioncontainer.h"

#include "../../util/progresslistener.h"

namespace rfiStrategy {

	class QuickCalibrateAction : public Action
	{
		public:
			QuickCalibrateAction() { }

			virtual std::string Description() final override
			{
				return "Quickly calibrate";
			}
			virtual void Perform(class ArtifactSet &artifacts, class ProgressListener &) final override
			{
				Image2DCPtr image = artifacts.ContaminatedData().GetSingleImage();
				Mask2DCPtr mask = artifacts.ContaminatedData().GetSingleMask();
				num_t mean, stddev;
				ThresholdTools::WinsorizedMeanAndStdDev(image.get(), mask.get(), mean, stddev);
				for(size_t i=0;i!=artifacts.ContaminatedData().ImageCount();++i)
				{
					Image2DCPtr image = artifacts.ContaminatedData().GetImage(i);
					Image2DPtr normalized(new Image2D(*image));
					normalized->MultiplyValues(1.0/mean);
					artifacts.ContaminatedData().SetImage(i, normalized);
				}
			}
			virtual ActionType Type() const final override { return QuickCalibrateActionType; }
		private:
	};
}

#endif // RFIQUICKCALIBRATEACTION_H
