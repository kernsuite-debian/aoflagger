#ifndef PNGREADER_H
#define PNGREADER_H

#include <string>
#include <fstream>
#include <set>
#include <map>
#include <cmath>

#include "../../structures/types.h"

#include "../control/defaultstrategy.h"

#include "singleimageset.h"

#include "../../util/logger.h"

namespace rfiStrategy {

	class PngReader : public SingleImageSet {
		public:
			explicit PngReader(const std::string &path) : SingleImageSet(), _path(path)
			{
			}

			virtual std::unique_ptr<ImageSet> Clone() final override
			{
				return nullptr;
			}

			virtual void Initialize() final override
			{
			}

			virtual std::string Name() final override
			{
				return "Png file";
			}
			
			virtual std::string File() final override
			{
				return _path;
			}
			
			virtual std::string BaselineDescription() final override
			{ return Name(); }
	
			virtual std::string TelescopeName() final override
			{
				return DefaultStrategy::TelescopeName(DefaultStrategy::GENERIC_TELESCOPE);
			}
	
			virtual std::unique_ptr<BaselineData> Read() final override;

		private:
			std::string _path;
	};
	
}

#endif
