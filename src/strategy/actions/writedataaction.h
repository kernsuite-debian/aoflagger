#ifndef WRITEDATAACTION_H
#define WRITEDATAACTION_H

#include <mutex>

#include "action.h"

#include "../control/artifactset.h"

#include "../imagesets/imageset.h"

namespace rfiStrategy {

	class WriteDataAction : public Action {
		public:
			virtual std::string Description() final override
			{
				return "Write data to file";
			}

			virtual void Perform(class ArtifactSet &artifacts, ProgressListener &) final override
			{
				std::unique_lock<std::mutex> lock(artifacts.IOMutex());
				ImageSet& set = artifacts.ImageSet();
				set.PerformWriteDataTask(artifacts.ImageSetIndex(), artifacts.RevisedData());
			}

			virtual ActionType Type() const final override
			{
				return WriteDataActionType;
			}

		private:
	};
}
#endif
