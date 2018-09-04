#ifndef COADDED_IMAGE_SET_H
#define COADDED_IMAGE_SET_H

#include "imageset.h"
#include "msimageset.h"

#include "../../structures/measurementset.h"
#include "../../util/logger.h"

#include <vector>
#include <list>
#include <map>

namespace rfiStrategy {
	
	using Sequence = MeasurementSet::Sequence;
	
	class CoaddedImageSetIndex : public ImageSetIndex {
		public:
			friend class CoaddedImageSet;
			
			explicit CoaddedImageSetIndex(class CoaddedImageSet &set);
			virtual ~CoaddedImageSetIndex() override { }
			virtual void Previous() override final;
			virtual void Next() override final;
			virtual std::string Description() const override final;
			virtual bool IsValid() const override final;
			virtual std::unique_ptr<ImageSetIndex> Clone() const override final
			{ return std::unique_ptr<CoaddedImageSetIndex>(new CoaddedImageSetIndex(*this) ); }
			
			CoaddedImageSetIndex(const CoaddedImageSetIndex&) = default;
			
		private:
			CoaddedImageSetIndex& operator=(const CoaddedImageSetIndex&) = delete;
			virtual void reattach() final override;
			
			std::vector<MSImageSetIndex> _iterators;
	};
	
	class CoaddedImageSet : public IndexableSet
	{
	public:
		explicit CoaddedImageSet(const std::vector<std::string>& filenames, BaselineIOMode ioMode)
		{
			if(filenames.empty())
				throw std::runtime_error("Coadding of image sets was requested, but list of measurement sets to coadd is empty");
			if(filenames.size() == 1)
				throw std::runtime_error("Coadding of image sets was requested, but only one measurement set is given");
			for(const std::string& path : filenames)
			{
				std::unique_ptr<MSImageSet> imageSet(new MSImageSet(path, ioMode));
				_msImageSets.emplace_back(std::move(imageSet));
			}
		}
		
		CoaddedImageSet(const CoaddedImageSet& source) = delete;
		CoaddedImageSet& operator=(const CoaddedImageSet& source) = delete;
		
		virtual ~CoaddedImageSet() override { };
		virtual std::unique_ptr<ImageSet> Clone() override final
		{
			std::unique_ptr<CoaddedImageSet> newSet(new CoaddedImageSet());
			for(const std::unique_ptr<MSImageSet>& imageSet : _msImageSets)
			{
				newSet->_msImageSets.emplace_back(imageSet->CloneMSImageSet());
			}
			return std::move(newSet);
		}

		virtual std::unique_ptr<ImageSetIndex> StartIndex() override final
		{
			return std::unique_ptr<CoaddedImageSetIndex>(new CoaddedImageSetIndex(*this));
		}
		
		virtual void Initialize() override final
		{
			for(std::unique_ptr<MSImageSet>& imageSet : _msImageSets)
				imageSet->Initialize();
		}
		
		virtual std::string Name() override final
		{ return "Coadded set (" + _msImageSets.front()->Name() + " ...)"; }
		virtual std::string File() override final
		{ return _msImageSets.front()->File(); }
		
		virtual void AddReadRequest(const ImageSetIndex &index) override final
		{
			const CoaddedImageSetIndex& coaddIndex = static_cast<const CoaddedImageSetIndex&>(index);
			for(size_t i=0; i!=_msImageSets.size(); ++i)
			{
				_msImageSets[i]->AddReadRequest(coaddIndex._iterators[i]);
			}
		}
		
		virtual void PerformReadRequests() override final
		{
			for(size_t i=0; i!=_msImageSets.size(); ++i)
			{
				_msImageSets[i]->PerformReadRequests();
			}
		}
		
		virtual std::unique_ptr<BaselineData> GetNextRequested() override final
		{
			std::unique_ptr<BaselineData> data = _msImageSets.front()->GetNextRequested();
			TimeFrequencyData tfData(data->Data());
			std::vector<Image2DPtr> images;
			for(size_t i=0; i!=tfData.PolarizationCount(); ++i)
			{
				TimeFrequencyData polAmplitude = tfData.MakeFromPolarizationIndex(i).Make(TimeFrequencyData::AmplitudePart);
				images.emplace_back(Image2DPtr(new Image2D(*polAmplitude.GetImage(0))));
			}
			for(size_t i=1; i!=_msImageSets.size(); ++i)
			{
				std::unique_ptr<BaselineData> addedData = _msImageSets[i]->GetNextRequested();
				if(addedData->Data().PolarizationCount() != images.size())
					throw std::runtime_error("Coadded images have different number of polarizations");
				for(size_t j=0; j!=images.size(); ++j)
				{
					TimeFrequencyData polAmplitude = addedData->Data().MakeFromPolarizationIndex(j).Make(TimeFrequencyData::AmplitudePart);
					images[j]->operator+=(*polAmplitude.GetImage(0));
				}
			}
			Image2DCPtr zeroImage = Image2D::CreateSetImagePtr(images.front()->Width(), images.front()->Height(), 0.0);
			for(size_t i=0; i!=tfData.PolarizationCount(); ++i)
			{
				TimeFrequencyData pol = tfData.MakeFromPolarizationIndex(i);
				pol.SetImage(0, images[i]);
				pol.SetImage(1, zeroImage);
				tfData.SetPolarizationData(i, std::move(pol));
			}
			data->SetData(tfData);
			return std::move(data);
		}
		
		virtual void AddWriteFlagsTask(const ImageSetIndex &index, std::vector<Mask2DCPtr> &flags) override final
		{
		}
		virtual void PerformWriteFlagsTask() override final
		{
		}
		
		virtual BaselineReaderPtr Reader() override final
		{
			return _msImageSets.front()->Reader();
		}
		
		virtual size_t AntennaCount() const override final
		{
			return _msImageSets.front()->AntennaCount();
		}
		
		virtual size_t GetAntenna1(const ImageSetIndex &index) override final
		{
			return _msImageSets.front()->GetAntenna1(static_cast<const CoaddedImageSetIndex&>(index)._iterators.front());
		}
		
		virtual size_t GetAntenna2(const ImageSetIndex &index) override final
		{
			return _msImageSets.front()->GetAntenna2(static_cast<const CoaddedImageSetIndex&>(index)._iterators.front());
		}
		
		virtual size_t GetBand(const ImageSetIndex &index) override final
		{
			return _msImageSets.front()->GetBand(static_cast<const CoaddedImageSetIndex&>(index)._iterators.front());
		}
		
		virtual size_t GetField(const ImageSetIndex &index) override final
		{
			return _msImageSets.front()->GetField(static_cast<const CoaddedImageSetIndex&>(index)._iterators.front());
		}
		
		virtual size_t GetSequenceId(const ImageSetIndex &index) override final
		{
			return _msImageSets.front()->GetSequenceId(static_cast<const CoaddedImageSetIndex&>(index)._iterators.front());
		}
		
		virtual AntennaInfo GetAntennaInfo(unsigned antennaIndex) override final
		{
			return _msImageSets.front()->GetAntennaInfo(antennaIndex);
		}
		
		virtual size_t BandCount() const override final
		{
			return _msImageSets.front()->BandCount();
		}
		
		virtual BandInfo GetBandInfo(unsigned bandIndex) override final
		{
			return _msImageSets.front()->GetBandInfo(bandIndex);
		}
		
		virtual size_t SequenceCount() const override final
		{
			return _msImageSets.front()->SequenceCount();
		}
		
		virtual std::unique_ptr<ImageSetIndex> Index(size_t antenna1, size_t antenna2, size_t bandIndex, size_t sequenceId) override final
		{
			std::unique_ptr<CoaddedImageSetIndex> index(new CoaddedImageSetIndex(*this));
			for(size_t i=0; i!=_msImageSets.size(); ++i)
			{
				std::unique_ptr<ImageSetIndex> msIndex = _msImageSets[i]->Index(antenna1, antenna2, bandIndex, sequenceId);
				index->_iterators[i] = static_cast<MSImageSetIndex&>(*msIndex);
			}
			return std::move(index);
		}
		
		virtual FieldInfo GetFieldInfo(unsigned fieldIndex) override final
		{
			return _msImageSets.front()->GetFieldInfo(fieldIndex);
		}
		
		const std::vector<std::unique_ptr<MSImageSet>>& MSImageSets() { return _msImageSets; }
	private:
		CoaddedImageSet() { }
		
		std::vector<std::unique_ptr<MSImageSet>> _msImageSets;
	};
	
	CoaddedImageSetIndex::CoaddedImageSetIndex(CoaddedImageSet& set) :
		ImageSetIndex(set),
		_iterators()
	{
		_iterators.reserve(set.MSImageSets().size());
		for(const std::unique_ptr<MSImageSet>& imageSet : set.MSImageSets())
		{
			_iterators.emplace_back(*imageSet);
		}
	}
	
	void CoaddedImageSetIndex::Next()
	{
		for(MSImageSetIndex& index : _iterators)
			index.Next();
	}

	void CoaddedImageSetIndex::Previous()
	{
		for(MSImageSetIndex& index : _iterators)
			index.Previous();
	}

	void CoaddedImageSetIndex::reattach()
	{
		for(size_t i=0; i!=_iterators.size(); ++i)
		{
			_iterators[i].Reattach(*static_cast<CoaddedImageSet&>(imageSet()).MSImageSets()[i]);
		}
	}

	std::string CoaddedImageSetIndex::Description() const
	{
		return _iterators[0].Description() + " (coadded)";
	}
	
	bool CoaddedImageSetIndex::IsValid() const
	{
		bool isValid = _iterators[0].IsValid();
		for(size_t i=1; i!=_iterators.size(); ++i)
			isValid = isValid && _iterators[i].IsValid();
		return isValid;
	}
}

#endif
