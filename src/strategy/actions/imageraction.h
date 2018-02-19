#ifndef RFISTRATEGYIMAGERACTION_H
#define RFISTRATEGYIMAGERACTION_H

#include "action.h"

#include "../control/artifactset.h"

#include <mutex>

namespace rfiStrategy {

	class ImagerAction : public Action {
		public:
			enum ImagingType {
				Set, Add, Subtract
			};

			ImagerAction() : _type(Add)
			{
			}
			virtual ~ImagerAction()
			{
			}
			virtual std::string Description() final override
			{
				switch(_type)
				{
					case Set:
					return "Image (set)";
					case Add:
					default:
					return "Image (add)";
					case Subtract:
					return "Image (subtract)";
				}
			}
			virtual void Perform(ArtifactSet &artifacts, ProgressListener &progress) final override;

			virtual ActionType Type() const final override { return ImagerActionType; }
			enum ImagingType ImagingType() const { return _type; }
			void SetImagingType(enum ImagingType type) throw() { _type = type; }

		private:
			enum ImagingType _type;
			std::mutex _imagerMutex;
	};

}

#endif
