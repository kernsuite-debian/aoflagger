#ifndef RFISTRATEGYNORMALIZEVARIANCEACTION_H
#define RFISTRATEGYNORMALIZEVARIANCEACTION_H

#include <map>
#include <mutex>

#include "action.h"

#include "../control/artifactset.h"

namespace rfiStrategy {

	class NormalizeVarianceAction : public Action {
		public:
			NormalizeVarianceAction() :
				_medianFilterSizeInS(60.0*15.0), _isInitialized(false)
			{
			}
			virtual ~NormalizeVarianceAction();
			virtual std::string Description() final override
			{
				return "Normalize variance over time";
			}
			virtual void Perform(ArtifactSet &artifacts, ProgressListener &progress) final override;

			virtual void Initialize() final override
			{
				clean();
			}
			virtual void Finish() final override
			{
				clean();
			}
			virtual ActionType Type() const final override { return NormalizeVarianceActionType; }

			double MedianFilterSizeInS() const { return _medianFilterSizeInS; }
			void SetMedianFilterSizeInS(double filterSize) { _medianFilterSizeInS = filterSize; }
		private:
			double _medianFilterSizeInS;
			std::mutex _mutex;
			bool _isInitialized;
			std::map<double, double> _stddevs;
			
			void initializeStdDevs(ArtifactSet &artifacts);
			void clean();
			void correctData(std::vector<Image2DPtr> &data, size_t timeStep, double stddev);
			void correctDataUpTo(std::vector<Image2DPtr> &data, size_t &dataTimeIndex, double time, const std::vector<double> &observationTimes, double stddev);
	};
}

#endif
