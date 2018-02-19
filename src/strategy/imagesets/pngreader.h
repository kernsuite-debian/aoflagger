#ifndef PNGREADER_H
#define PNGREADER_H

#include <string>
#include <fstream>
#include <set>
#include <map>
#include <cmath>

#include "../../structures/types.h"

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
				return "Png format";
			}
			
			virtual std::string File() final override
			{
				return _path;
			}
			
			virtual std::unique_ptr<BaselineData> Read() final override;

		private:
			std::string _path;
	};
	
}

#endif
