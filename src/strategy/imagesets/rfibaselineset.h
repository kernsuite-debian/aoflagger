#ifndef RFI_BASELINE_SET_H
#define RFI_BASELINE_SET_H

#include "../../structures/types.h"

#include "singleimageset.h"

namespace rfiStrategy {

	class RFIBaselineSet : public SingleImageSet
	{
		public:
			explicit RFIBaselineSet(const std::string& path);

			virtual std::unique_ptr<ImageSet> Clone() final override
			{
				return nullptr;
			}

			virtual void Initialize() final override
			{ }

			virtual std::string Name() final override { return _path; }
			
			virtual std::string BaselineDescription() final override;
			
			virtual std::string File() final override
			{
				return _path;
			}
			
			virtual std::string TelescopeName() final override
			{
				return _telescopeName;
			}
			
			virtual std::unique_ptr<BaselineData> Read() final override;

		private:
			std::string _path;
			std::string _telescopeName;
			TimeFrequencyData _data;
			TimeFrequencyMetaData _metaData;
	};
	
}

#endif
