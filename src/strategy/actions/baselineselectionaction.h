#ifndef RFIBASELINESELECTIONACTION_H
#define RFIBASELINESELECTIONACTION_H

#include <vector>

#include <boost/thread/mutex.hpp>

#include "../../structures/types.h"

#include "action.h"

#include "../algorithms/baselineselector.h"

namespace rfiStrategy {

	class BaselineSelectionAction : public Action
	{
		public:
			BaselineSelectionAction() : _preparationStep(true), _flagBadBaselines(false), _makePlot(false), _threshold(8.0), _absThreshold(0.4), _smoothingSigma(0.6) { }

			virtual std::string Description()
			{
				if(_preparationStep)
					return "Select baselines (preparation)";
				else
					return "Mark bad baselines";
			}
			virtual void Perform(class ArtifactSet &artifacts, class ProgressListener &listener)
			{
				if(_preparationStep)
					prepare(artifacts, listener);
				else
					mark(artifacts, listener);
			}
			virtual ActionType Type() const { return BaselineSelectionActionType; }

			bool PreparationStep() const { return _preparationStep; }
			void SetPreparationStep(bool preparationStep) { _preparationStep = preparationStep; }

			bool FlagBadBaselines() const { return _flagBadBaselines; }
			void SetFlagBadBaselines(bool flagBadBaselines) { _flagBadBaselines = flagBadBaselines; }

			bool MakePlot() const { return _makePlot; }
			void SetMakePlot(bool makePlot) { _makePlot = makePlot; }

			num_t AbsThreshold() const { return _absThreshold; }
			void SetAbsThreshold(double absThreshold) { _absThreshold = absThreshold; }

			num_t Threshold() const { return _threshold; }
			void SetThreshold(double threshold) { _threshold = threshold; }

			num_t SmoothingSigma() const { return _smoothingSigma; }
			void SetSmoothingSigma(double smoothingSigma) { _smoothingSigma = smoothingSigma; }
		private:
			void prepare(class ArtifactSet &artifacts, class ProgressListener &listener);
			void mark(class ArtifactSet &artifacts, class ProgressListener &listener);
			void flagBaselines(ArtifactSet &artifacts, const std::vector<BaselineSelector::SingleBaselineInfo> &baselines);

			bool _preparationStep;
			bool _flagBadBaselines;
			bool _makePlot;
			num_t _threshold, _absThreshold, _smoothingSigma;
	};
}

#endif // RFIBASELINESELECTIONACTION_H
