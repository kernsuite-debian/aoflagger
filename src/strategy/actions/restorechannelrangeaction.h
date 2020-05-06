#ifndef RESTORE_CHANNELRANGE_ACTION_H
#define RESTORE_CHANNELRANGE_ACTION_H

#include "../algorithms/restorechannelrange.h"

#include "../control/actioncontainer.h"
#include "../control/artifactset.h"

#include "../../util/progresslistener.h"

namespace rfiStrategy {

	class RestoreChannelRangeAction : public Action
	{
		public:
			RestoreChannelRangeAction() : _startFrequencyMHz(0.0), _endFrequencyMHz(0.0) { }

			virtual std::string Description() final override
			{
				return "Restore channel range";
			}
			
			virtual void Perform(class ArtifactSet &artifacts, class ProgressListener &) final override
			{
				if(artifacts.ContaminatedData().IsEmpty())
					throw std::runtime_error("No baseline is loaded!");
				if(artifacts.HasMetaData())
				{
					RestoreChannelRange::Execute(
						artifacts.ContaminatedData(), artifacts.OriginalData(), *artifacts.MetaData(),
						_startFrequencyMHz, _endFrequencyMHz
					);
				}
			}
			
			virtual ActionType Type() const final override { return RestoreChannelRangeActionType; }
			
			double StartFrequencyMHz() const { return _startFrequencyMHz; }
			void SetStartFrequencyMHz(double startFrequencyMHz) { _startFrequencyMHz = startFrequencyMHz; }
			
			double EndFrequencyMHz() const { return _endFrequencyMHz; }
			void SetEndFrequencyMHz(double endFrequencyMHz) { _endFrequencyMHz = endFrequencyMHz; }
			
		private:
			double _startFrequencyMHz, _endFrequencyMHz;
	};
}

#endif // RESTORE_CHANNELRANGE_ACTION_H

