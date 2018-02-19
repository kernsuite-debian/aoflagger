#ifndef SVDACTION_H
#define SVDACTION_H 

#include "action.h"

namespace rfiStrategy {

	class SVDAction : public Action
	{
		public:
			SVDAction() : _singularValueCount(1) { }
			std::string Description() final override
			{
				return "Singular value decomposition";
			}
			void Perform(class ArtifactSet &artifacts, class ProgressListener &listener) final override;
			ActionType Type() const final override { return SVDActionType; }

			size_t SingularValueCount() const { return _singularValueCount; }
			void SetSingularValueCount(size_t svCount) { _singularValueCount = svCount; }
			
		private:
			size_t _singularValueCount;
	};

}

#endif // SVDACTION_H
