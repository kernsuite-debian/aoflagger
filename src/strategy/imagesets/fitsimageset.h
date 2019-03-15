#ifndef FITSIMAGESET_H
#define FITSIMAGESET_H

#include <vector>
#include <set>
#include <stack>
#include <map>

#include "imageset.h"

#include "../../baseexception.h"

#include "../../structures/antennainfo.h"
#include "../../structures/types.h"

namespace rfiStrategy {
	
	class FitsImageSetIndex : public ImageSetIndex {
		friend class FitsImageSet;
		
		explicit FitsImageSetIndex(class rfiStrategy::ImageSet &set) : ImageSetIndex(set), _baselineIndex(0), _band(0), _field(0), _isValid(true) { }
		
		virtual void Previous() final override;
		virtual void Next() final override;
		virtual std::string Description() const final override;
		virtual bool IsValid() const final override { return _isValid; }
		virtual std::unique_ptr<ImageSetIndex> Clone() const final override
		{
			std::unique_ptr<FitsImageSetIndex> index( new FitsImageSetIndex(imageSet()) );
			index->_baselineIndex = _baselineIndex;
			index->_band = _band;
			index->_field = _field;
			index->_isValid = _isValid;
			return std::move(index);
		}
		private:
			size_t _baselineIndex, _band, _field;
			bool _isValid;
	};

	class FitsImageSet : public ImageSet
	{
		public:
			explicit FitsImageSet(const std::string &file);
			~FitsImageSet();
			virtual void Initialize() override final;

			virtual std::unique_ptr<ImageSet> Clone() override final;

			virtual std::unique_ptr<ImageSetIndex> StartIndex() override final
			{
				return std::unique_ptr<ImageSetIndex>(new FitsImageSetIndex(*this));
			}
			virtual std::string Name() override final
			{
				return File();
			}
			virtual std::string File() override final;
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
			virtual std::string TelescopeName() final override;
			const std::vector<std::pair<size_t,size_t> > &Baselines() const
			{
				return _baselines;
			}
			size_t BandCount() const
			{
				return _bandCount;
			}
			class AntennaInfo GetAntennaInfo(unsigned antennaIndex) const
			{
				return _antennaInfos[antennaIndex];
			}
			
		private:
			FitsImageSet(const FitsImageSet &source);
			BaselineData loadData(const ImageSetIndex &index);
			
			void ReadPrimarySingleTable(TimeFrequencyData &data, TimeFrequencyMetaData &metaData);
			void ReadTable(TimeFrequencyData &data, TimeFrequencyMetaData &metaData, size_t bandIndex);
			void ReadAntennaTable(TimeFrequencyMetaData &metaData);
			void ReadFrequencyTable(TimeFrequencyData &data, TimeFrequencyMetaData &metaData);
			void ReadCalibrationTable();
			void ReadSingleDishTable(TimeFrequencyData &data, TimeFrequencyMetaData &metaData, size_t ifIndex);
			TimeFrequencyData ReadPrimaryGroupTable(size_t baselineIndex, int band, int stokes, TimeFrequencyMetaData &metaData);
			
			void saveSingleDishFlags(std::vector<Mask2DCPtr> &flags, size_t ifIndex);
			
			std::shared_ptr<class FitsFile> _file;
			std::vector<std::pair<size_t,size_t> > _baselines;
			size_t _bandCount;
			std::vector<AntennaInfo> _antennaInfos;
			std::map<int, BandInfo> _bandInfos;
			std::vector<int> _bandIndexToNumber;
			size_t _currentBaselineIndex, _currentBandIndex;
			double _frequencyOffset;
			
			std::stack<BaselineData> _baselineData;
	};

}

#endif
