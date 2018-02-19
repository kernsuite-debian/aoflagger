#ifndef BHFITSIMAGESET_H
#define BHFITSIMAGESET_H

#include <vector>
#include <set>
#include <stack>
#include <map>
#include <memory>

#include "imageset.h"

#include "../../baseexception.h"

#include "../../structures/antennainfo.h"
#include "../../structures/types.h"

namespace rfiStrategy {
	
	class BHFitsImageSetIndex : public ImageSetIndex {
		friend class BHFitsImageSet;
		
  explicit BHFitsImageSetIndex(class rfiStrategy::ImageSet &set) : ImageSetIndex(set), _imageIndex(0), _isValid(true) { }
		
		virtual void Previous() override final;
		virtual void Next() override final;
		virtual std::string Description() const override final;
		virtual bool IsValid() const throw() override final { return _isValid; }
		virtual std::unique_ptr<ImageSetIndex> Clone() const override final
		{
			std::unique_ptr<BHFitsImageSetIndex> index( new BHFitsImageSetIndex(imageSet()) );
			index->_imageIndex = _imageIndex;
			index->_isValid = _isValid;
			return std::move(index);
		}
		private:
			size_t _imageIndex;
			bool _isValid;
	};

	class BHFitsImageSet : public ImageSet
	{
		public:
			explicit BHFitsImageSet(const std::string &file);
			~BHFitsImageSet();
			virtual void Initialize() override final;

			virtual std::unique_ptr<ImageSet> Clone() override final;

			virtual std::unique_ptr<ImageSetIndex> StartIndex() override final
			{
				return std::unique_ptr<ImageSetIndex>(new BHFitsImageSetIndex(*this));
			}
			virtual std::string Name() override final
			{
			  return "Bighorns fits file";
			}
			virtual std::string File() override final;
			size_t ImageCount() { return _timeRanges.size(); }
			const std::string &RangeName(size_t rangeIndex) {
			  return _timeRanges[rangeIndex].name;
			}

			virtual void AddReadRequest(const ImageSetIndex &index) override final
			{
				_baselineData.push(loadData(index));
			}
			virtual void PerformReadRequests() override final
			{
			}
			virtual std::unique_ptr<BaselineData> GetNextRequested() override final
			{
				std::unique_ptr<BaselineData> data(new BaselineData(_baselineData.top()));
				_baselineData.pop();
				return std::move(data);
			}
			virtual void AddWriteFlagsTask(const ImageSetIndex &index, std::vector<Mask2DCPtr> &flags) override final;
			virtual void PerformWriteFlagsTask() override final;
			virtual void PerformWriteDataTask(const ImageSetIndex &, std::vector<Image2DCPtr>, std::vector<Image2DCPtr>) override final
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

			  TimeRange& operator=(const TimeRange &source)
			  {
			    start = source.start;
			    end = source.end;
			    name = source.name;
return *this;
			  }
			};

			BHFitsImageSet(const BHFitsImageSet &source);
			BaselineData loadData(const ImageSetIndex &index);
			void loadImageData(TimeFrequencyData &data, const TimeFrequencyMetaDataPtr &metaData, const BHFitsImageSetIndex &index);
			std::pair<int, int> getRangeFromString(const std::string &rangeStr);
			std::string flagFilePath() const;

			std::shared_ptr<class FitsFile> _file;
			std::stack<BaselineData> _baselineData;
			std::vector<TimeRange> _timeRanges;
			int _width, _height;
	};

}

#endif
