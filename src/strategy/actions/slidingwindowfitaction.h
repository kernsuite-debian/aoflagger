#ifndef RFISLIDINGWINDOWFITACTION_H
#define RFISLIDINGWINDOWFITACTION_H 

#include "action.h"

#include "slidingwindowfitparameters.h"

namespace rfiStrategy {

	class SlidingWindowFitAction : public Action
	{
		public:
			SlidingWindowFitAction() { LoadDefaults(); }
			virtual ~SlidingWindowFitAction() { }
			virtual std::string Description() final override
			{
				return "Sliding window fit";
			}
			virtual void Perform(class ArtifactSet &artifacts, class ProgressListener &listener) final override;
			virtual ActionType Type() const final override { return SlidingWindowFitActionType; }

			const SlidingWindowFitParameters &Parameters() const { return _parameters; }
			SlidingWindowFitParameters &Parameters() { return _parameters; }

			void LoadDefaults();
		private:
			SlidingWindowFitParameters _parameters;
	};

}

#endif // RFISLIDINGWINDOWFITACTION_H
