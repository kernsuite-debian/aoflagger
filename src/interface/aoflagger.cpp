#include "aoflagger.h"

#include "../version.h"

#include "../structures/image2d.h"
#include "../structures/mask2d.h"

#include "../strategy/actions/strategy.h"

#include "../strategy/algorithms/baselineselector.h"
#include "../strategy/algorithms/polarizationstatistics.h"

#include "../strategy/control/artifactset.h"
#include "../strategy/control/defaultstrategy.h"
#include "../strategy/control/strategyreader.h"

#include "../util/progresslistener.h"

#include "../quality/histogramcollection.h"
#include "../quality/statisticscollection.h"

#include <vector>
#include <typeinfo>
#include <memory>
#include <mutex>

namespace aoflagger {
	
	const unsigned StrategyFlags::NONE                 =   0x00;
	const unsigned StrategyFlags::LARGE_BANDWIDTH      =   0x01;
	const unsigned StrategyFlags::SMALL_BANDWIDTH      =   0x02;
	const unsigned StrategyFlags::TRANSIENTS           =   0x04;
	const unsigned StrategyFlags::ROBUST               =   0x08;
	const unsigned StrategyFlags::FAST                 =   0x10;
	const unsigned StrategyFlags::INSENSITIVE          =   0x20;
	const unsigned StrategyFlags::SENSITIVE            =   0x40;
	const unsigned StrategyFlags::USE_ORIGINAL_FLAGS   =   0x80;
	const unsigned StrategyFlags::AUTO_CORRELATION     =  0x100;
	const unsigned StrategyFlags::HIGH_TIME_RESOLUTION =  0x200;

	
	class ImageSetData {
		public:
			explicit ImageSetData(size_t initialSize) : images(initialSize)
			{
			}
			
			std::vector<Image2DPtr> images;
	};
	
	ImageSet::ImageSet() :
		_data(nullptr)
	{ }
	
	ImageSet::ImageSet(size_t width, size_t height, size_t count) :
		_data(new ImageSetData(count))
	{
		assertValidCount(count);
		for(size_t i=0; i!=count; ++i)
			_data->images[i] = Image2D::CreateUnsetImagePtr(width, height);
	}
	
	ImageSet::ImageSet(size_t width, size_t height, size_t count, float initialValue) :
		_data(new ImageSetData(count))
	{
		assertValidCount(count);
		for(size_t i=0; i!=count; ++i)
			_data->images[i] = Image2D::CreateSetImagePtr(width, height, initialValue);
	}
	
	ImageSet::ImageSet(size_t width, size_t height, size_t count, size_t widthCapacity) :
		_data(new ImageSetData(count))
	{
		assertValidCount(count);
		for(size_t i=0; i!=count; ++i)
			_data->images[i] = Image2D::CreateUnsetImagePtr(width, height, widthCapacity);
	}
	
	ImageSet::ImageSet(size_t width, size_t height, size_t count, float initialValue, size_t widthCapacity) :
		_data(new ImageSetData(count))
	{
		assertValidCount(count);
		for(size_t i=0; i!=count; ++i)
			_data->images[i] = Image2D::CreateSetImagePtr(width, height, initialValue, widthCapacity);
	}
	
	ImageSet::ImageSet(const ImageSet& sourceImageSet) :
		_data(sourceImageSet._data!=nullptr
			? new ImageSetData(*sourceImageSet._data)
			: nullptr)
	{
	}
	
	ImageSet::ImageSet::ImageSet(aoflagger::ImageSet&& sourceImageSet) :
		_data(sourceImageSet._data)
	{
		sourceImageSet._data = nullptr;
	}
	
	ImageSet::~ImageSet()
	{
		delete _data;
	}
	
	ImageSet &ImageSet::operator=(const ImageSet& sourceImageSet)
	{
		if(sourceImageSet._data == nullptr)
		{
			delete _data;
			_data = nullptr;
		}
		else if(_data == nullptr)
		{
			_data = new ImageSetData(*sourceImageSet._data);
		}
		else {
			*_data = *sourceImageSet._data;
		}
		return *this;
	}
	
	ImageSet &ImageSet::operator=(ImageSet&& sourceImageSet)
	{
		std::swap(_data, sourceImageSet._data);
		return *this;
	}
	
	void ImageSet::assertValidCount(size_t count)
	{
		if(count != 1 && count != 2 && count != 4 && count != 8)
			throw std::runtime_error("Invalid count specified when creating image set for aoflagger; should be 1, 2, 4 or 8.");
	}
	
	float *ImageSet::ImageBuffer(size_t imageIndex)
	{
		return _data->images[imageIndex]->Data();
	}
	
	const float *ImageSet::ImageBuffer(size_t imageIndex) const
	{
		return _data->images[imageIndex]->Data();
	}
	
	size_t ImageSet::Width() const
	{
		return _data->images[0]->Width();
	}
	
	size_t ImageSet::Height() const
	{
		return _data->images[0]->Height();
	}
	
	size_t ImageSet::ImageCount() const
	{
		return _data->images.size();
	}
	
	size_t ImageSet::HorizontalStride() const
	{
		return _data->images[0]->Stride();
	}
	
	void ImageSet::Set(float newValue)
	{
		for(std::vector<Image2DPtr>::iterator imgPtr = _data->images.begin(); imgPtr != _data->images.end(); ++imgPtr)
		{
			(*imgPtr)->SetAll(newValue);
		}
	}
	
	void ImageSet::ResizeWithoutReallocation(size_t newWidth) const
	{
		for(std::vector<Image2DPtr>::iterator imgPtr = _data->images.begin(); imgPtr != _data->images.end(); ++imgPtr)
		{
			(*imgPtr)->ResizeWithoutReallocation(newWidth);
		}
	}
	
	class FlagMaskData {
		public:
			explicit FlagMaskData(Mask2DPtr theMask) : mask(theMask)
			{
			}
			
			Mask2DPtr mask;
	};
	
	FlagMask::FlagMask() : _data(nullptr)
	{ }
	
	FlagMask::FlagMask(size_t width, size_t height) : _data(new FlagMaskData(
		Mask2D::CreateUnsetMaskPtr(width, height)	))
	{ }
	
	FlagMask::FlagMask(size_t width, size_t height, bool initialValue) : _data(new FlagMaskData(
		Mask2D::CreateUnsetMaskPtr(width, height)	))
	{
		if(initialValue)
			_data->mask->SetAll<true>();
		else
			_data->mask->SetAll<false>();
	}
	
	FlagMask::FlagMask(const FlagMask& sourceMask) :
		_data(sourceMask._data==nullptr
			? nullptr
			: new FlagMaskData(*sourceMask._data))
	{ }
			
	FlagMask::FlagMask(FlagMask&& sourceMask) :
		_data(sourceMask._data)
	{ 
		sourceMask._data = nullptr;
	}
	
	FlagMask& FlagMask::operator=(const FlagMask& flagMask)
	{
		if(flagMask._data == nullptr)
		{
			delete _data;
			_data = nullptr;
		}
		else if(_data == nullptr)
		{
			_data = new FlagMaskData(*flagMask._data);
		}
		else {
			*_data = *flagMask._data;
		}
		return *this;
	}
	
	FlagMask& FlagMask::operator=(FlagMask&& flagMask)
	{
		std::swap(_data, flagMask._data);
		return *this;
	}
	
	FlagMask::~FlagMask()
	{
		// _data might be nullptr, but it's fine to delete nullptr; (by standard)
		delete _data;
	}
			
	size_t FlagMask::Width() const
	{
		return _data->mask->Width();
	}
			
	size_t FlagMask::Height() const
	{
		return _data->mask->Height();
	}
			
	size_t FlagMask::HorizontalStride() const
	{
		return _data->mask->Stride();
	}
	
	bool *FlagMask::Buffer()
	{
		return _data->mask->ValuePtr(0, 0);
	}
	
	const bool *FlagMask::Buffer() const
	{
		return _data->mask->ValuePtr(0, 0);
	}
	
	class StrategyData {
		public:
			explicit StrategyData(std::unique_ptr<rfiStrategy::Strategy> strategy)
			: strategyPtr(std::move(strategy))
			{ }
			
			std::shared_ptr<rfiStrategy::Strategy> strategyPtr;
	};
	
	Strategy::Strategy(enum TelescopeId telescopeId, unsigned strategyFlags, double frequency, double timeRes, double frequencyRes)
	{
		std::unique_ptr<rfiStrategy::Strategy> s(new rfiStrategy::Strategy());
		rfiStrategy::DefaultStrategy::StrategySetup setup = rfiStrategy::DefaultStrategy::DetermineSetup(
			(rfiStrategy::DefaultStrategy::TelescopeId) telescopeId,
			strategyFlags,
			frequency,
			timeRes,
			frequencyRes);
		rfiStrategy::DefaultStrategy::LoadSingleStrategy(*s, setup);
		_data = new StrategyData(std::move(s));
	}

	Strategy::Strategy(const std::string& filename)
	{
		rfiStrategy::StrategyReader reader;
		_data = new StrategyData(reader.CreateStrategyFromFile(filename));
	}

	Strategy::Strategy(const Strategy& sourceStrategy) :
		_data(new StrategyData(*sourceStrategy._data))
	{
	}
	
	Strategy::Strategy(Strategy&& sourceStrategy) :
		_data(new StrategyData(std::move(*sourceStrategy._data)))
	{
	}
	
	Strategy::~Strategy()
	{
		delete _data;
	}
	
	Strategy &Strategy::operator=(const Strategy& sourceStrategy)
	{
		*_data = *sourceStrategy._data;
		return *this;
	}

	Strategy &Strategy::operator=(Strategy&& sourceStrategy)
	{
		*_data = std::move(*sourceStrategy._data);
		return *this;
	}
	
	class QualityStatisticsDataImp
	{
		public:
			QualityStatisticsDataImp(const double* _scanTimes, size_t nScans, size_t nPolarizations, bool _computeHistograms) :
				scanTimes(_scanTimes, _scanTimes+nScans),
				statistics(nPolarizations),
				histograms(nPolarizations),
				computeHistograms(_computeHistograms)
			{
			}
			std::vector<double> scanTimes;
			StatisticsCollection statistics;
			HistogramCollection histograms;
			bool computeHistograms;
	};
	
	class QualityStatisticsData
	{
		public:
			QualityStatisticsData(const double* _scanTimes, size_t nScans, size_t nPolarizations, bool computeHistograms) :
				_implementation(new QualityStatisticsDataImp(_scanTimes, nScans, nPolarizations, computeHistograms))
			{
			}
			explicit QualityStatisticsData(std::shared_ptr<QualityStatisticsDataImp> implementation) :
				_implementation(implementation)
			{
			}
			std::shared_ptr<QualityStatisticsDataImp> _implementation;
	};

	QualityStatistics::QualityStatistics(const double* scanTimes, size_t nScans, const double* channelFrequencies, size_t nChannels, size_t nPolarizations, bool computeHistograms) :
		_data(new QualityStatisticsData(scanTimes, nScans, nPolarizations, computeHistograms))
	{
		_data->_implementation->statistics.InitializeBand(0, channelFrequencies, nChannels);
	}
	
	QualityStatistics::QualityStatistics(const QualityStatistics& sourceQS) :
		_data(new QualityStatisticsData(sourceQS._data->_implementation))
	{
	}
	
	QualityStatistics::QualityStatistics(QualityStatistics&& sourceQS) :
		_data(new QualityStatisticsData(std::move(sourceQS._data->_implementation)))
	{
	}
	
	QualityStatistics::~QualityStatistics()
	{
		delete _data;
	}
	
	QualityStatistics& QualityStatistics::operator=(const QualityStatistics& sourceQS)
	{
		_data->_implementation = sourceQS._data->_implementation;
		return *this;
	}
	
	QualityStatistics& QualityStatistics::operator=(QualityStatistics&& sourceQS)
	{
		_data->_implementation = std::move(sourceQS._data->_implementation);
		return *this;
	}
	
	QualityStatistics& QualityStatistics::operator+=(const QualityStatistics& rhs)
	{
		_data->_implementation->statistics.Add(rhs._data->_implementation->statistics);
		_data->_implementation->histograms.Add(rhs._data->_implementation->histograms);
		return *this;
	}
	
	class ErrorListener : public ProgressListener {
		virtual void OnStartTask(const rfiStrategy::Action &, size_t, size_t, const std::string &, size_t = 1) final override {}
		virtual void OnEndTask(const rfiStrategy::Action &) final override {}
		virtual void OnProgress(const rfiStrategy::Action &, size_t, size_t) final override {}
		virtual void OnException(const rfiStrategy::Action &, std::exception &e) final override
		{
			std::cerr <<
				"*** EXCEPTION OCCURED IN THE AOFLAGGER ***\n"
				"The AOFlagger encountered a bug or the given strategy was invalid!\n"
				"The reported exception " << typeid(e).name() << " is:\n" << e.what();
		}
	};
	
	class ForwardingListener : public ProgressListener {
	public:
		explicit ForwardingListener(StatusListener* destination) : _destination(destination)
		{ }
		virtual void OnStartTask(const rfiStrategy::Action &, size_t taskNo, size_t taskCount, const std::string &description, size_t weight) final override {
			_destination->OnStartTask(taskNo, taskCount, description); }
		virtual void OnEndTask(const rfiStrategy::Action &) final override {
			_destination->OnEndTask(); }
		virtual void OnProgress(const rfiStrategy::Action &, size_t progress, size_t maxProgress) final override {
			_destination->OnProgress(progress, maxProgress); }
		virtual void OnException(const rfiStrategy::Action &, std::exception &thrownException) final override {
			_destination->OnException(thrownException); }
	private:
		StatusListener *_destination;
	};
	
	FlagMask AOFlagger::Run(Strategy& strategy, const ImageSet& input)
	{
		std::mutex mutex;
		rfiStrategy::ArtifactSet artifacts(&mutex);
		std::unique_ptr<ProgressListener> listener;
		if(_statusListener == 0)
			listener.reset(new ErrorListener());
		else
			listener.reset(new ForwardingListener(_statusListener));
		
		Mask2DPtr mask = Mask2D::CreateSetMaskPtr<false>(input.Width(), input.Height());
		TimeFrequencyData inputData, revisedData;
		Image2DPtr zeroImage = Image2D::CreateZeroImagePtr(input.Width(), input.Height());
		switch(input.ImageCount())
		{
		case 1:
			inputData = TimeFrequencyData(TimeFrequencyData::AmplitudePart, Polarization::StokesI, input._data->images[0]);
			inputData.SetGlobalMask(mask);
			revisedData = TimeFrequencyData(TimeFrequencyData::AmplitudePart, Polarization::StokesI, zeroImage);
			revisedData.SetGlobalMask(mask);
			break;
		case 2:
			inputData = TimeFrequencyData(Polarization::StokesI, input._data->images[0], input._data->images[1]);
			inputData.SetGlobalMask(mask);
			revisedData = TimeFrequencyData(Polarization::StokesI, zeroImage, zeroImage);
			revisedData.SetGlobalMask(mask);
			break;
		case 4:
			inputData = TimeFrequencyData(
				Polarization::XX, input._data->images[0], input._data->images[1],
				Polarization::YY, input._data->images[2], input._data->images[3]
			);
			inputData.SetIndividualPolarizationMasks(mask, mask);
			revisedData = TimeFrequencyData(
				Polarization::XX, zeroImage, zeroImage,
				Polarization::YY, zeroImage, zeroImage);
			revisedData.SetIndividualPolarizationMasks(mask, mask);
			break;
		case 8:
			inputData = TimeFrequencyData::FromLinear(
				input._data->images[0], input._data->images[1],
				input._data->images[2], input._data->images[3],
				input._data->images[4], input._data->images[5],
				input._data->images[6], input._data->images[7]
			);
			inputData.SetIndividualPolarizationMasks(mask, mask, mask, mask);
			revisedData = TimeFrequencyData::FromLinear(
				zeroImage, zeroImage, zeroImage, zeroImage,
				zeroImage, zeroImage, zeroImage, zeroImage);
			revisedData.SetIndividualPolarizationMasks(mask, mask, mask, mask);
			break;
		}
		artifacts.SetOriginalData(inputData);
		artifacts.SetContaminatedData(inputData);
		artifacts.SetRevisedData(revisedData);
		artifacts.SetPolarizationStatistics(std::unique_ptr<PolarizationStatistics>(new PolarizationStatistics()));
		artifacts.SetBaselineSelectionInfo(std::unique_ptr<rfiStrategy::BaselineSelector>(new rfiStrategy::BaselineSelector()));
		
		strategy._data->strategyPtr->Perform(artifacts, *listener);
		
		listener.reset();
		
		FlagMask flagMask;
		mask.reset(new Mask2D(*artifacts.ContaminatedData().GetSingleMask()));
		flagMask._data = 
			new FlagMaskData(std::move(mask));
		return flagMask;
	}
	
	FlagMask AOFlagger::Run(Strategy& strategy, const ImageSet& input, const FlagMask& correlatorFlags)
	{
		return Run(strategy, input);
	}
	
	QualityStatistics AOFlagger::MakeQualityStatistics(const double *scanTimes, size_t nScans, const double *channelFrequencies, size_t nChannels, size_t nPolarizations)
	{
		return QualityStatistics(scanTimes, nScans, channelFrequencies, nChannels, nPolarizations, false);
	}
	
	QualityStatistics AOFlagger::MakeQualityStatistics(const double *scanTimes, size_t nScans, const double *channelFrequencies, size_t nChannels, size_t nPolarizations, bool computeHistograms)
	{
		return QualityStatistics(scanTimes, nScans, channelFrequencies, nChannels, nPolarizations, computeHistograms);
	}
	
	void AOFlagger::CollectStatistics(QualityStatistics& destination, const ImageSet& imageSet, const FlagMask& rfiFlags, const FlagMask& correlatorFlags, size_t antenna1, size_t antenna2)
	{
		StatisticsCollection& stats(destination._data->_implementation->statistics);
		HistogramCollection& histograms(destination._data->_implementation->histograms);
		const std::vector<double> &times(destination._data->_implementation->scanTimes);
		
		if(imageSet.ImageCount() == 1)
		{
			stats.AddImage(antenna1, antenna2, &times[0], 0, 0,
										 imageSet._data->images[0], imageSet._data->images[0],
										 rfiFlags._data->mask, correlatorFlags._data->mask);
			if(destination._data->_implementation->computeHistograms)
			{
				histograms.Add(antenna1, antenna2, 0, imageSet._data->images[0],
											 rfiFlags._data->mask, correlatorFlags._data->mask);
			}
		}
		else {
			const size_t polarizationCount = imageSet.ImageCount()/2;
			for(size_t polarization=0; polarization!=polarizationCount; ++polarization)
			{
				stats.AddImage(antenna1, antenna2, &times[0], 0, polarization,
											imageSet._data->images[polarization*2], imageSet._data->images[polarization*2+1],
											rfiFlags._data->mask, correlatorFlags._data->mask);
				if(destination._data->_implementation->computeHistograms)
				{
					histograms.Add(antenna1, antenna2, polarization,
												 imageSet._data->images[polarization*2], imageSet._data->images[polarization*2+1],
										rfiFlags._data->mask, correlatorFlags._data->mask);
				}
			}
		}
	}
	
	void AOFlagger::WriteStatistics(const QualityStatistics& statistics, const std::string& measurementSetPath)
	{
		QualityTablesFormatter qFormatter(measurementSetPath);
		statistics._data->_implementation->statistics.Save(qFormatter);
		
		HistogramCollection& histograms(statistics._data->_implementation->histograms);
		if(!histograms.Empty())
		{
			HistogramTablesFormatter hFormatter(measurementSetPath);
			histograms.Save(hFormatter);
		}
	}
	
	std::string AOFlagger::GetVersionString()
	{
		return AOFLAGGER_VERSION_STR;
	}
	
	void AOFlagger::GetVersion(short& major, short& minor, short& subMinor)
	{
		major = AOFLAGGER_VERSION_MAJOR;
		minor = AOFLAGGER_VERSION_MINOR;
		subMinor = AOFLAGGER_VERSION_SUBMINOR;
	}
	
	std::string AOFlagger::GetVersionDate()
	{
		return AOFLAGGER_VERSION_DATE_STR;
	}
	
} // end of namespace aoflagger
