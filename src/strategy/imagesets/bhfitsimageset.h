#ifndef BHFITSIMAGESET_H
#define BHFITSIMAGESET_H

#include <vector>
#include <set>
#include <stack>
#include <map>

#include "imageset.h"

#include "../../baseexception.h"

#include "../../structures/antennainfo.h"
#include "../../structures/types.h"

namespace rfiStrategy {
	
	class BHFitsImageSetIndex : public ImageSetIndex {
		friend class BHFitsImageSet;
		
  BHFitsImageSetIndex(class rfiStrategy::ImageSet &set) : ImageSetIndex(set), _imageIndex(0), _isValid(true) { }
		
		virtual void Previous();
		virtual void Next();
		virtual std::string Description() const;
		virtual bool IsValid() const throw() { return _isValid; }
		virtual BHFitsImageSetIndex *Copy() const
		{
			BHFitsImageSetIndex *index = new BHFitsImageSetIndex(imageSet());
			index->_imageIndex = _imageIndex;
			index->_isValid = _isValid;
			return index;
		}
		private:
			size_t _imageIndex;
			bool _isValid;
	};

	class BHFitsImageSet : public ImageSet
	{
		public:
			BHFitsImageSet(const std::string &file);
			~BHFitsImageSet();
			virtual void Initialize();

			virtual BHFitsImageSet *Copy();

			virtual ImageSetIndex *StartIndex()
			{
				return new BHFitsImageSetIndex(*this);
			}
			virtual std::string Name()
			{
			  return "Bighorns fits file";
			}
			virtual std::string File();
			size_t ImageCount() { return _timeRanges.size(); }
			const std::string &RangeName(size_t rangeIndex) {
			  return _timeRanges[rangeIndex].name;
			}

			virtual void AddReadRequest(const ImageSetIndex &index)
			{
				_baselineData.push(loadData(index));
			}
			virtual void PerformReadRequests()
			{
			}
			virtual BaselineData *GetNextRequested()
			{
				BaselineData *data = new BaselineData(_baselineData.top());
				_baselineData.pop();
				return data;
			}
			virtual void AddWriteFlagsTask(const ImageSetIndex &index, std::vector<Mask2DCPtr> &flags);
			virtual void PerformWriteFlagsTask();
			virtual void PerformWriteDataTask(const ImageSetIndex &, std::vector<Image2DCPtr>, std::vector<Image2DCPtr>)
			{
				throw BadUsageException("Not implemented");
			}
			std::string GetTelescopeName() const {
			  return "Bighorns";
			}
		private:
			struct TimeRange
			{
			  int start, end;
			  std::string name;

			  TimeRange() { }
			TimeRange(const TimeRange &source)
			: start(source.start),
			    end(source.end),
			    name(source.name)
			  {
			  }

			  void operator=(const TimeRange &source)
			  {
			    start = source.start;
			    end = source.end;
			    name = source.name;
			  }
			};

			BHFitsImageSet(const BHFitsImageSet &source);
			BaselineData loadData(const ImageSetIndex &index);
			void loadImageData(TimeFrequencyData &data, const TimeFrequencyMetaDataPtr &metaData, const BHFitsImageSetIndex &index);
			std::pair<int, int> getRangeFromString(const std::string &rangeStr);
			std::string flagFilePath() const;

			boost::shared_ptr<class FitsFile> _file;
			std::stack<BaselineData> _baselineData;
			std::vector<TimeRange> _timeRanges;
			int _width, _height;
	};

}

#endif
