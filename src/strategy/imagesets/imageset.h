#ifndef GUI_IMAGESET_H
#define GUI_IMAGESET_H

#include <cstring>
#include <string>
#include <memory>
#include <vector>

#include "../../structures/types.h"
#include "../../structures/timefrequencydata.h"
#include "../../structures/timefrequencymetadata.h"

namespace rfiStrategy {

	class ImageSet;
	
	class ImageSetIndex {
		public:
			explicit ImageSetIndex(ImageSet &set) : _set(&set) { }
			virtual ~ImageSetIndex() { }
			virtual void Previous() = 0;
			virtual void Next() = 0;
			virtual std::string Description() const = 0;
			virtual bool IsValid() const = 0;
			virtual std::unique_ptr<ImageSetIndex> Clone() const = 0;
			void Reattach(ImageSet &imageSet) {
				_set = &imageSet;
				reattach();
			}
			
			ImageSetIndex(const ImageSetIndex&) = default;
			ImageSetIndex(ImageSetIndex&&) = default;
			ImageSetIndex& operator=(const ImageSetIndex&) = default;
			ImageSetIndex& operator=(ImageSetIndex&&) = default;
		protected:
			virtual void reattach() { }
			ImageSet &imageSet() const { return *_set; }
			
		private:
			class ImageSet *_set;
	};
	
	class BaselineData {
		public:
			BaselineData(const TimeFrequencyData& data, const TimeFrequencyMetaDataCPtr& metaData, const ImageSetIndex &index)
			: _data(data), _metaData(metaData), _index(index.Clone())
			{ }
			
			explicit BaselineData(const ImageSetIndex &index)
			: _data(), _metaData(), _index(index.Clone())
			{ }
			
			BaselineData(const TimeFrequencyData& data, const TimeFrequencyMetaDataCPtr& metaData)
			: _data(data), _metaData(metaData), _index()
			{ }
			
			explicit BaselineData(const TimeFrequencyMetaDataCPtr& metaData)
			: _data(), _metaData(metaData), _index()
			{ }
			
			BaselineData()
			: _data(), _metaData(), _index()
			{ }
			
			BaselineData(const BaselineData& source)
			: _data(source._data), _metaData(source._metaData), _index()
			{
				if(source._index != nullptr) _index = source._index->Clone();
			}
			
			BaselineData& operator=(const BaselineData &source)
			{
				_data = source._data;
				_metaData = source._metaData;
				_index = source._index->Clone();
				return *this;
			}
			
			const TimeFrequencyData &Data() const { return _data; }
			void SetData(const TimeFrequencyData &data) { _data = data; }
			
			TimeFrequencyMetaDataCPtr MetaData() const { return _metaData; }
			void SetMetaData(TimeFrequencyMetaDataCPtr metaData) { _metaData = metaData; }

			const ImageSetIndex &Index() const { return *_index; }
			ImageSetIndex &Index() { return *_index; }
			void SetIndex(const ImageSetIndex &newIndex)
			{
				_index = newIndex.Clone();
			}
		
		private:
			TimeFrequencyData _data;
			TimeFrequencyMetaDataCPtr _metaData;
			std::unique_ptr<ImageSetIndex> _index;
	};
	
	class ImageSet {
		public:
			virtual ~ImageSet() { };
			virtual std::unique_ptr<ImageSet> Clone() = 0;

			virtual std::unique_ptr<ImageSetIndex> StartIndex() = 0;
			
			/**
			 * Initialize is used to initialize the image set after it has been created and
			 * after all possible options have been set that might influence initialization
			 * (such as number of parts to read).
			 */
			virtual void Initialize() = 0;
			virtual std::string Name() = 0;
			virtual std::string File() = 0;
			virtual std::string TelescopeName() = 0;
			
			virtual void AddReadRequest(const ImageSetIndex &index) = 0;
			virtual void PerformReadRequests() = 0;
			virtual std::unique_ptr<BaselineData> GetNextRequested() = 0;
			
			virtual void AddWriteFlagsTask(const ImageSetIndex &/*index*/, std::vector<Mask2DCPtr> &/*flags*/)
			{
				throw std::runtime_error("Not implemented");
			}
			virtual void PerformWriteFlagsTask()
			{
				throw std::runtime_error("Not implemented");
			}
			virtual void PerformWriteDataTask(const ImageSetIndex &/*index*/, std::vector<Image2DCPtr> /*_realImages*/, std::vector<Image2DCPtr> /*_imaginaryImages*/)
			{
				throw std::runtime_error("Not implemented");
			}
			
			static class ImageSet *Create(const std::vector<std::string>& files, BaselineIOMode ioMode);
			static bool IsFitsFile(const std::string &file);
			static bool IsBHFitsFile(const std::string &file);
			static bool IsRCPRawFile(const std::string &file);
			static bool IsTKPRawFile(const std::string &file);
			static bool IsRawDescFile(const std::string &file);
			static bool IsParmFile(const std::string &file);
			static bool IsTimeFrequencyStatFile(const std::string &file);
			static bool IsMSFile(const std::string &file);
			static bool IsNoiseStatFile(const std::string &file);
			static bool IsPngFile(const std::string &file);
			static bool IsFilterBankFile(const std::string& file);
			static bool IsQualityStatSet(const std::string& file);
			static bool IsRFIBaselineSet(const std::string& file);

			void AddWriteFlagsTask(const ImageSetIndex &index, const TimeFrequencyData &data)
			{
				std::vector<Mask2DCPtr> flags;
				for(size_t i=0;i!=data.MaskCount();++i)
					flags.push_back(data.GetMask(i));
				AddWriteFlagsTask(index, flags);
			}

			void PerformWriteDataTask(const ImageSetIndex &index, const TimeFrequencyData &data)
			{
				std::vector<Image2DCPtr> realImages, imaginaryImages;
				for(size_t i=0;i!=data.PolarizationCount();++i)
				{
					TimeFrequencyData polData(data.MakeFromPolarizationIndex(i));
					realImages.push_back(polData.GetRealPart());
					imaginaryImages.push_back(polData.GetImaginaryPart());
				}
				PerformWriteDataTask(index, realImages, imaginaryImages);
			}
	};

}

#endif
