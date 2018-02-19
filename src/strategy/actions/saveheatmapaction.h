#ifndef SAVEHEATMAPACTION_H
#define SAVEHEATMAPACTION_H 

#include <map>

#include "action.h"

#include "../control/artifactset.h"

namespace rfiStrategy {

class SaveHeatMapAction : public Action
{
	public:
		SaveHeatMapAction() { }
		
		void Initialize() final override { _prefixCounts.clear(); }
		std::string Description() final override
		{
			return "Save heat map";
		}
		void Perform(ArtifactSet& artifacts, ProgressListener& listener) final override;
		
		ActionType Type() const final override
		{ 
			return SaveHeatMapActionType; 
		}
	private:
		void plotIterations(class ArtifactSet &artifacts);
		std::mutex _mutex;
		std::map<std::string,size_t> _prefixCounts;
};

}

#endif // PLOTACTION_H


