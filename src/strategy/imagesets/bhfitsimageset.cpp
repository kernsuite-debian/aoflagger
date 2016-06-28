#include "bhfitsimageset.h"

#include "../../msio/fitsfile.h"
#include "../../structures/image2d.h"
#include "../../structures/timefrequencydata.h"

#include "../../util/aologger.h"

namespace rfiStrategy {
	
	BHFitsImageSet::BHFitsImageSet(const std::string &file) :
		ImageSet(),
		_file(new FitsFile(file))
	{
		AOLogger::Debug << "Opening bhfits file: '" << file << "'\n";
		try {
			_file->Open(FitsFile::ReadWriteMode);
		} catch(FitsIOException& exception)
		{
			AOLogger::Error << "CFitsio failed to open file in RW mode with the following error:\n" << exception.what()
			<< "\nTrying reopening in read-only mode. Writing to file won't be possible.\n\n";
			_file->Open(FitsFile::ReadOnlyMode);
		}
	}
	
	BHFitsImageSet::BHFitsImageSet(const BHFitsImageSet& source) :
		ImageSet(),
		_file(source._file),
		_baselineData(source._baselineData),
		_timeRanges(source._timeRanges),
		_width(source._width),
		_height(source._height)
	{
	}
	
	BHFitsImageSet::~BHFitsImageSet()
	{
	}
	
	BHFitsImageSet *BHFitsImageSet::Copy()
	{
		return new BHFitsImageSet(*this);
	}

	void BHFitsImageSet::Initialize()
	{
		_file->MoveToHDU(1);
		/*for(int i=1;i<=_file->GetKeywordCount();++i)
			{
				AOLogger::Debug << _file->GetKeyword(i) << " = " << _file->GetKeywordValue(i) << '\n';
			}*/
		if(_file->GetCurrentHDUType() != FitsFile::ImageHDUType)
			throw std::runtime_error("Error in Bighorns fits files: first HDU was not an image HDU");
		if(_file->GetCurrentImageDimensionCount() != 2)
			throw std::runtime_error("Fits image was not two dimensional");
		_width =_file->GetCurrentImageSize(2), _height = _file->GetCurrentImageSize(1);
		AOLogger::Debug << "Image of " << _width << " x " << _height << '\n';

		_timeRanges.clear();
		size_t keyIndex = 0;
		bool searchOn;
		do {
			searchOn = false;
			std::ostringstream antKey, termKey;
			antKey << "ANT";
			termKey << "TERM";
			if(keyIndex < 10) {
				antKey << '0';
				termKey << '0';
			}
			antKey << keyIndex;
			termKey << keyIndex;
			std::string antRangeStr, termRangeStr;
			if(_file->GetKeywordValue(antKey.str(), antRangeStr)) {
				std::pair<int, int> range = getRangeFromString(antRangeStr);
				TimeRange timeRange;
				// As commented by Marcin below, the ranges in the fits headers are given like 'start - end', where the indices start
				// counting at 1, and the end index is *inclusive*, such that '1 - 1' represents a range with one index (being the
				// first timestep in the FITS file).
				// Comment by Marcin Sokolowski:
				// MS this is due to fact that Andre didn't know it starts from 1, but he skips the end integration (assumes it is not ANT), but it is
				timeRange.start = range.first - 1; 
				// so here I don't subtract 1 in order to program check until this one too !
				timeRange.end = range.second;
				timeRange.name = antKey.str();
				_timeRanges.push_back(timeRange);
				searchOn = true;
			}
			if(_file->GetKeywordValue(termKey.str(), termRangeStr)) {
				std::pair<int, int> range = getRangeFromString(termRangeStr);
				TimeRange timeRange;
				// (see earlier comment by Marcin Sokolowski)
				timeRange.start = range.first - 1;
				timeRange.end = range.second;
				timeRange.name = termKey.str();
				_timeRanges.push_back(timeRange);
				searchOn = true;
			}
			++keyIndex;
		} while(searchOn);
		AOLogger::Debug << "This file has " << _timeRanges.size() << " time ranges.\n";
		
		if( _timeRanges.empty()) {
		   // if no states found in the header - just assume all are antenna 
		   TimeRange timeRange;
			 // See earlier comment by Marcin Sokolowski
			timeRange.start = 0;
			timeRange.end = _file->GetCurrentImageSize(2);
			timeRange.name = "ANT";
			_timeRanges.push_back(timeRange);		                                                  
			AOLogger::Warn << "No states specified in the fits header assuming all (1-" << timeRange.end << ") integrations are " << timeRange.name << "\n";
		}
	}

	BaselineData BHFitsImageSet::loadData(const ImageSetIndex &index)
	{
	  const BHFitsImageSetIndex &fitsIndex = static_cast<const BHFitsImageSetIndex&>(index);

	  TimeFrequencyMetaDataPtr metaData(new TimeFrequencyMetaData());
	  TimeFrequencyData data;
	  loadImageData(data, metaData, fitsIndex);
	  return BaselineData(data, metaData, index);
	}

  void BHFitsImageSet::loadImageData(TimeFrequencyData &data, const TimeFrequencyMetaDataPtr &metaData, const BHFitsImageSetIndex &index)
  {
		std::vector<num_t> buffer(_width * _height);
		_file->ReadCurrentImageData(0, &buffer[0], _width * _height);
		
		int
			rangeStart = _timeRanges[index._imageIndex].start, 
			rangeEnd = _timeRanges[index._imageIndex].end;
		Image2DPtr image = Image2D::CreateZeroImagePtr(rangeEnd-rangeStart, _height);

		std::vector<num_t>::const_iterator bufferPtr = buffer.begin() + _height*rangeStart;
		for(int x=rangeStart; x!=rangeEnd; ++x)
		{
			for(int y=0; y!=_height; ++y)
			{
				image->SetValue(x-rangeStart, y, *bufferPtr);
				++bufferPtr;
			}
		}
		data = TimeFrequencyData(TimeFrequencyData::AmplitudePart, SinglePolarisation, image);

		try {
			FitsFile flagFile(flagFilePath());
			flagFile.Open(FitsFile::ReadOnlyMode);
			flagFile.ReadCurrentImageData(0, &buffer[0], _width * _height);
			bufferPtr = buffer.begin() + _height*rangeStart;
			Mask2DPtr mask = Mask2D::CreateUnsetMaskPtr(rangeEnd-rangeStart, _height);
			for(int x=rangeStart; x!=rangeEnd; ++x)
			{
				for(int y=0; y!=_height; ++y)
				{
					bool flag = false;
					if(*bufferPtr == 0.0)
						flag = false;
					else if(*bufferPtr == 1.0)
						flag = true;
					else std::runtime_error("Expecting a flag file with only ones and zeros, but this file contained other values.");
					mask->SetValue(x-rangeStart, y, flag);
					++bufferPtr;
				}
			}
			data.SetGlobalMask(mask);
		} catch(std::exception &)
		{
			// Flag file could not be read; probably does not exist. Ignore this, flags will be initialized to false.
		}

		double
			frequencyDelta = _file->GetDoubleKeywordValue("CDELT1"),
			timeDelta = _file->GetDoubleKeywordValue("CDELT2");
		BandInfo band;
		for(int ch=0; ch!=_height; ++ch)
		{
			ChannelInfo channel;
			channel.frequencyHz = ch * frequencyDelta * 1000000.0;
			band.channels.push_back(channel);
		}
		metaData->SetBand(band);

		const int rangeWidth = rangeEnd-rangeStart;
		std::vector<double> observationTimes(rangeWidth);
		for(int t=0; t!=rangeWidth; ++t)
			observationTimes[t] = (t + rangeStart) * timeDelta;
		metaData->SetObservationTimes(observationTimes);

		AntennaInfo antennaInfo;
		antennaInfo.id = 0;
		antennaInfo.name = RangeName(index._imageIndex);
		antennaInfo.diameter = 0.0;
		antennaInfo.mount = "Unknown";
		antennaInfo.station = GetTelescopeName();
		metaData->SetAntenna1(antennaInfo);
		metaData->SetAntenna2(antennaInfo);
  }

  std::pair<int, int> BHFitsImageSet::getRangeFromString(const std::string &rangeStr)
  {
		std::pair<int, int> value;
		size_t partA = rangeStr.find(' ');
		value.first = atoi(rangeStr.substr(0, partA).c_str());
		size_t partB = rangeStr.find('-');
		if(rangeStr[partB+1] == ' ')
			++partB;
		value.second = atoi(rangeStr.substr(partB+1).c_str());
		return value;
  }
  
	std::string BHFitsImageSet::flagFilePath() const
	{
		std::string flagFilePath = _file->Filename();
		if(flagFilePath.size() > 7) {
			flagFilePath = flagFilePath.substr(0, flagFilePath.size()-7);
		}
		flagFilePath += "_flag.fits";
		return flagFilePath;
	}

  void BHFitsImageSet::AddWriteFlagsTask(const ImageSetIndex &index, std::vector<Mask2DCPtr> &flags)
  {
		if(flags.size() != 1)
			throw std::runtime_error("BHFitsImageSet::AddWriteFlagsTask() called with multiple flags");
		std::string flagFilename = flagFilePath();
		AOLogger::Debug << "Writing to " << flagFilename << '\n';
		FitsFile flagFile(flagFilename);
		bool newFile = true;
		std::vector<num_t> buffer(_width * _height);
		try {
			flagFile.Open(FitsFile::ReadWriteMode);
			newFile = false;
		} catch(std::exception &) {
			AOLogger::Debug << "File did not exist yet, creating new.\n";
			flagFile.Create();
			flagFile.AppendImageHUD(FitsFile::Float32ImageType, _height, _width);
		}

		// This must be outside the try { } block, so that exceptions
		// don't result in creating a new file.
		if(!newFile) {
			flagFile.ReadCurrentImageData(0, &buffer[0], _width * _height);
		}

		const BHFitsImageSetIndex &bhIndex(static_cast<const BHFitsImageSetIndex&>(index));
		int
			rangeStart = _timeRanges[bhIndex._imageIndex].start, 
			rangeEnd = _timeRanges[bhIndex._imageIndex].end;
		std::vector<num_t>::iterator bufferPtr = buffer.begin() + _height*rangeStart;
		for(int x=rangeStart; x!=rangeEnd; ++x)
		{
		for(int y=0; y!=_height; ++y)
			{
				*bufferPtr = flags[0]->Value(x-rangeStart, y) ? 1.0 : 0.0;
				++bufferPtr;
			}
		}

		flagFile.WriteImage(0, &buffer[0], _width * _height, -1.0);
	}

	void BHFitsImageSet::PerformWriteFlagsTask()
	{
	  // Nothing to do; already written
	}
	
	
	void BHFitsImageSetIndex::Previous()
	{
	  if(_imageIndex > 0)
			--_imageIndex;
		else {
			_imageIndex = static_cast<class BHFitsImageSet&>(imageSet()).ImageCount() - 1;
			_isValid = false;
		}
	}
	
	void BHFitsImageSetIndex::Next()
	{
		++_imageIndex;
		if( _imageIndex >= static_cast<class BHFitsImageSet&>(imageSet()).ImageCount() )
		{
		  _imageIndex = 0;
			_isValid = false;
		}
	}

	std::string BHFitsImageSetIndex::Description() const {
	  std::ostringstream str;
	  str << "Time range " << 
	    static_cast<BHFitsImageSet&>(imageSet()).RangeName(_imageIndex);
	  return str.str();
	}

	std::string BHFitsImageSet::File()
	{
	  return _file->Filename();
	}
}
