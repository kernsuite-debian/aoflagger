#ifndef TIMESELECTIONACTION_H
#define TIMESELECTIONACTION_H

#include "../../structures/types.h"

#include "action.h"

namespace rfiStrategy {
	
	class TimeSelectionAction : public Action {
		public:
			TimeSelectionAction() : _threshold(3.5)
			{
			}
			virtual std::string Description() final override
			{
				return "Time selection";
			}
			virtual void Perform(ArtifactSet &artifacts, class ProgressListener &) final override
			{
				AutomaticSelection(artifacts);
			}
			virtual ActionType Type() const final override { return TimeSelectionActionType; }

			num_t Threshold() const { return _threshold; }
			void SetThreshold(num_t threshold) { _threshold = threshold; }
		private:
			void AutomaticSelection(ArtifactSet &artifacts);

			num_t _threshold;
	};

}

#endif
