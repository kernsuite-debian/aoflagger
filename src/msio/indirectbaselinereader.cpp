#include "indirectbaselinereader.h"

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <fstream>
#include <set>
#include <stdexcept>
#include <vector>

#include <fcntl.h>

#include <boost/filesystem.hpp>

#include "../structures/timefrequencydata.h"
#include "../structures/system.h"

#include "../util/logger.h"
#include "../util/stopwatch.h"

#include "reorderedfilebuffer.h"
#include "msselection.h"

IndirectBaselineReader::IndirectBaselineReader(const std::string &msFile) :
	BaselineReader(msFile),
	_directReader(msFile),
	_seqIndexTable(),
	_msIsReordered(false),
	_removeReorderedFiles(false),
	_reorderedDataFilesHaveChanged(false),
	_reorderedFlagFilesHaveChanged(false),
	_readUVW(false)
{
}

IndirectBaselineReader::~IndirectBaselineReader()
{
	if(_reorderedDataFilesHaveChanged)
		updateOriginalMSData();
	if(_reorderedFlagFilesHaveChanged)
		updateOriginalMSFlags();
	removeTemporaryFiles();
	
	_seqIndexTable.reset();
	_filePositions.clear();
}

void IndirectBaselineReader::PerformReadRequests()
{
	initializeMeta();

	if(!_msIsReordered) reorderMS();

	_results.clear();
	Logger::Debug << "Performing " << _readRequests.size() << " read requests...\n";
	for(size_t i=0;i<_readRequests.size();++i)
	{
		const ReadRequest request = _readRequests[i];
		_results.push_back(Result());
		const size_t width = ObservationTimes(request.sequenceId).size();
		for(size_t p=0;p<Polarizations().size();++p)
		{
			if(ReadData()) {
				_results[i]._realImages.push_back(Image2D::CreateZeroImagePtr(width, MetaData().FrequencyCount(request.spectralWindow)));
				_results[i]._imaginaryImages.push_back(Image2D::CreateZeroImagePtr(width, MetaData().FrequencyCount(request.spectralWindow)));
			}
			if(ReadFlags()) {
				// The flags should be initialized to true, as a baseline might
				// miss some time scans that other baselines do have, and these
				// should be flagged.
				_results[i]._flags.push_back(Mask2D::CreateSetMaskPtr<true>(width, MetaData().FrequencyCount(request.spectralWindow)));
			}
		}
		if(_readUVW)
			_results[i]._uvw = _directReader.ReadUVW(request.antenna1, request.antenna2, request.spectralWindow, request.sequenceId);
		else {
			_results[i]._uvw.clear();
			for(unsigned j=0;j<width;++j)
			_results[i]._uvw.emplace_back(0.0, 0.0, 0.0);
		}

		std::ifstream dataFile(DataFilename(), std::ifstream::binary);
		std::ifstream flagFile(FlagFilename(), std::ifstream::binary);
		size_t index = _seqIndexTable->Value(request.antenna1, request.antenna2, request.spectralWindow, request.sequenceId);
		size_t filePos = _filePositions[index];
		dataFile.seekg(filePos * (sizeof(float)*2), std::ios_base::beg);
		flagFile.seekg(filePos * sizeof(bool), std::ios_base::beg);

		const size_t bufferSize = MetaData().FrequencyCount(request.spectralWindow) * Polarizations().size();
		for(size_t x=0;x<width;++x)
		{
			std::vector<float> dataBuffer(bufferSize*2);
			std::vector<char> flagBuffer(bufferSize);
			dataFile.read((char *) &dataBuffer[0], bufferSize * sizeof(float) * 2);
			size_t dataBufferPtr = 0;
			flagFile.read((char *) &flagBuffer[0], bufferSize * sizeof(bool));
			size_t flagBufferPtr = 0;
			for(size_t f=0;f<MetaData().FrequencyCount(request.spectralWindow);++f) {
				for(size_t p=0;p<Polarizations().size();++p)
				{
					_results[i]._realImages[p]->SetValue(x, f, dataBuffer[dataBufferPtr]);
					++dataBufferPtr;
					_results[i]._imaginaryImages[p]->SetValue(x, f, dataBuffer[dataBufferPtr]);
					++dataBufferPtr;
					_results[i]._flags[p]->SetValue(x, f, flagBuffer[flagBufferPtr]);
					++flagBufferPtr;
				}
			}
		}
	}
	Logger::Debug << "Done reading.\n";

	_readRequests.clear();
}

void IndirectBaselineReader::PerformFlagWriteRequests()
{
	for(size_t i=0;i!=_writeRequests.size();++i)
	{
		const FlagWriteRequest request = _writeRequests[i];
		performFlagWriteTask(request.flags, request.antenna1, request.antenna2, request.spectralWindow, request.sequenceId);
	}
	_writeRequests.clear();
}

void IndirectBaselineReader::reorderMS()
{
	boost::filesystem::path path(MetaFilename());
	bool reorderRequired = true;
	
	if(boost::filesystem::exists(path))
	{
		std::ifstream str(path.string().c_str());
		std::string name;
		std::getline(str, name);
		if(boost::filesystem::equivalent(boost::filesystem::path(name), MetaData().Path()))
		{
			Logger::Debug << "Measurement set has already been reordered; using old temporary files.\n";
			reorderRequired = false;
			_msIsReordered = true;
			_removeReorderedFiles = false;
			_reorderedDataFilesHaveChanged = false;
			_reorderedFlagFilesHaveChanged = false;
		}
	}
	
	if(reorderRequired)
	{
		reorderFull();
		std::ofstream str(path.string().c_str());
		str << MetaData().Path() << '\n';
	} else {
		size_t fileSize;
		makeLookupTables(fileSize);
	}
}

void IndirectBaselineReader::makeLookupTables(size_t &fileSize)
{
	std::vector<MSMetaData::Sequence> sequences = MetaData().GetSequences();
	const size_t
		antennaCount = MetaData().AntennaCount(),
		polarizationCount = Polarizations().size(),
		bandCount = MetaData().BandCount(),
		sequencesPerBaselineCount = MetaData().SequenceCount();

	_seqIndexTable.reset(new SeqIndexLookupTable(antennaCount, bandCount, sequencesPerBaselineCount));
	fileSize = 0;
	for(size_t i=0;i<sequences.size();++i)
	{
		// Initialize look-up table to get index into Sequence-array quickly
		const MSMetaData::Sequence &s = sequences[i];
		_seqIndexTable->Value(s.antenna1, s.antenna2, s.spw, s.sequenceId) = i;
		
		// Initialize look-up table to go from sequence array to file position. Is in samples, so
		// multiple times sizeof(bool) or ..(float)) for exact position.
		_filePositions.push_back(fileSize);
		fileSize += ObservationTimes(s.sequenceId).size() * MetaData().FrequencyCount(s.spw) * polarizationCount;
	}
}

void IndirectBaselineReader::preAllocate(const char *filename, size_t fileSize)
{
	Logger::Debug << "Pre-allocating " << (fileSize/(1024*1024)) << " MB...\n";
	int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR);
	if(fd < 0)
	{
		std::ostringstream s;
		s << "Error while opening file '" << filename << "', check access rights and free space";
		throw std::runtime_error(s.str());
	}
#if defined(HAVE_POSIX_FALLOCATE)
	int allocResult = posix_fallocate(fd, 0, fileSize);
	close(fd);
	if(allocResult != 0)
	{
		Logger::Warn <<
			"Could not allocate temporary file '" << filename << "': posix_fallocate returned " << allocResult << ".\n"
			"Tried to allocate " << (fileSize/(1024*1024)) << " MB.\n"
			"Disk could be full or filesystem could not support fallocate.\n";
	}
#else
	close(fd);
	Logger::Warn << "Compiled without posix_fallocate() support: skipping pre-allocation.\n";
#endif
}

void IndirectBaselineReader::reorderFull()
{
	Stopwatch watch(true);
	
	casacore::MeasurementSet ms = OpenMS();

	casacore::ArrayColumn<bool> flagColumn(ms, "FLAG");
	casacore::ScalarColumn<int> dataDescIdColumn(ms, "DATA_DESC_ID"); 
	casacore::ScalarColumn<int> antenna1Column(ms, "ANTENNA1"); 
	casacore::ScalarColumn<int> antenna2Column(ms, "ANTENNA2");
	casacore::ArrayColumn<casacore::Complex> dataColumn(ms, DataColumnName() );

	if(ms.nrow() == 0)
		throw std::runtime_error("Measurement set is empty (zero rows)");


	std::vector<size_t> dataIdToSpw;
	MetaData().GetDataDescToBandVector(dataIdToSpw);
	
	size_t fileSize;
	makeLookupTables(fileSize);
	
	Logger::Debug << "Opening temporary files.\n";
	ReorderInfo reorderInfo;
	preAllocate(DataFilename(), fileSize*sizeof(float)*2);
	reorderInfo.dataFile.reset(new std::ofstream(DataFilename(), std::ofstream::binary | std::ios_base::in | std::ios_base::out));
	if(reorderInfo.dataFile->fail())
		throw std::runtime_error("Error: failed to open temporary data files for writing! Check access rights and free disk space.");
	
	preAllocate(FlagFilename(), fileSize*sizeof(bool));
	reorderInfo.flagFile.reset(new std::ofstream(FlagFilename(), std::ofstream::binary | std::ios_base::in | std::ios_base::out));
	if(reorderInfo.flagFile->fail())
		throw std::runtime_error("Error: failed to open temporary data files for writing! Check access rights and free disk space.");

	Logger::Debug << "Reordering data set...\n";
	
	size_t bufferMem = std::min<size_t>(System::TotalMemory()/10, 1024l*1024l*1024l);
	ReorderedFileBuffer dataFile(reorderInfo.dataFile.get(), bufferMem);
	ReorderedFileBuffer flagFile(reorderInfo.flagFile.get(), bufferMem/8);
	
	std::vector<std::size_t> writeFilePositions = _filePositions;
	std::vector<std::size_t> timePositions(_filePositions.size(), size_t(-1));
	size_t
		polarizationCount = Polarizations().size(),
		intStart = IntervalStart(),
		intEnd = IntervalEnd();
	unsigned progress = 0, prevProgress = unsigned(-1);
	
	MSSelection msSelection(ms, ObservationTimesPerSequence());
	
	msSelection.Process(
		[&](size_t rowIndex, size_t sequenceId, size_t timeIndexInSequence)
	{
		progress = timeIndexInSequence*100/(intEnd - intStart);
		if(progress > prevProgress)
		{
			if(progress/10 != prevProgress/10)
				Logger::Debug << "\nReorder progress: ";
			Logger::Debug << progress << "% ";
			Logger::Debug.Flush();
			prevProgress = progress;
		}
	
		size_t 
			antenna1 = antenna1Column(rowIndex),
			antenna2 = antenna2Column(rowIndex),
			spw = dataIdToSpw[dataDescIdColumn(rowIndex)],
			channelCount = MetaData().FrequencyCount(spw),
			arrayIndex = _seqIndexTable->Value(antenna1, antenna2, spw, sequenceId),
			sampleCount = channelCount * polarizationCount;
		size_t& filePos = writeFilePositions[arrayIndex];
		size_t& timePos = timePositions[arrayIndex];
		
		casacore::Array<casacore::Complex> data = dataColumn(rowIndex);
		casacore::Array<bool> flag = flagColumn(rowIndex);
		
		dataFile.seekp(filePos * (sizeof(float)*2));
		flagFile.seekp(filePos * sizeof(bool));
		
		// If this baseline missed some time steps, pad the files
		// (we can't just skip over, because the flags should be set to true)
		++timePos;
		while(timePos < timeIndexInSequence)
		{
			const std::vector<float> nullData(sampleCount*2, 0.0);
			const std::vector<char> nullFlags(sampleCount, (char) true);
			dataFile.write(reinterpret_cast<const char*>(&*nullData.begin()), sampleCount * 2 * sizeof(float));
			flagFile.write(reinterpret_cast<const char*>(&*nullFlags.begin()), sampleCount * sizeof(bool));
			++timePos;
			filePos += sampleCount;
		}
		
		dataFile.write(reinterpret_cast<const char*>(&*data.cbegin()), sampleCount * 2 * sizeof(float));
		
		flagFile.write(reinterpret_cast<const char*>(&*flag.cbegin()), sampleCount * sizeof(bool));
		
		filePos += sampleCount;
	});
	
	uint64_t dataSetSize = (uint64_t) fileSize * (uint64_t) (sizeof(float)*2 + sizeof(bool));
	Logger::Debug << "Done reordering data set of " << dataSetSize/(1024*1024) << " MB in " << watch.Seconds() << " s (" << (long double) dataSetSize/(1024.0L*1024.0L*watch.Seconds()) << " MB/s)\n";
	_msIsReordered = true;
	_removeReorderedFiles = true;
	_reorderedDataFilesHaveChanged = false;
	_reorderedFlagFilesHaveChanged = false;
}

void IndirectBaselineReader::removeTemporaryFiles()
{
	if(_msIsReordered && _removeReorderedFiles)
	{
		boost::filesystem::remove(MetaFilename());
		boost::filesystem::remove(DataFilename());
		boost::filesystem::remove(FlagFilename());
		Logger::Debug << "Temporary files removed.\n";
	}
	_msIsReordered = false;
	_removeReorderedFiles = false;
	_reorderedDataFilesHaveChanged = false;
	_reorderedFlagFilesHaveChanged = false;
}

void IndirectBaselineReader::PerformDataWriteTask(std::vector<Image2DCPtr> _realImages, std::vector<Image2DCPtr> _imaginaryImages, int antenna1, int antenna2, int spectralWindow, unsigned sequenceId)
{
	initializeMeta();

	Logger::Debug << "Performing data write task with indirect baseline reader...\n";

	const size_t polarizationCount = Polarizations().size();
	
	if(_realImages.size() != polarizationCount || _imaginaryImages.size() != polarizationCount)
		throw std::runtime_error("PerformDataWriteTask: input format did not match number of polarizations in measurement set");
	
	for(size_t i=1;i<_realImages.size();++i)
	{
		if(_realImages[0]->Width() != _realImages[i]->Width() || _realImages[0]->Height() != _realImages[i]->Height() || _realImages[0]->Width() != _imaginaryImages[i]->Width() || _realImages[0]->Height() != _imaginaryImages[i]->Height())
		throw std::runtime_error("PerformDataWriteTask: width and/or height of input images did not match");
	}
	
	if(!_msIsReordered) reorderMS();
	
	const size_t width = _realImages[0]->Width();
	const size_t bufferSize = MetaData().FrequencyCount(spectralWindow) * Polarizations().size();
	
	std::ofstream dataFile(DataFilename(), std::ofstream::binary | std::ios_base::in | std::ios_base::out);
	size_t index = _seqIndexTable->Value(antenna1, antenna2, spectralWindow, sequenceId);
	size_t filePos = _filePositions[index];
	dataFile.seekp(filePos*(sizeof(float)*2), std::ios_base::beg);
	
	std::vector<float> dataBuffer(bufferSize*2);
	for(size_t x=0;x<width;++x)
	{
		size_t dataBufferPtr = 0;
		for(size_t f=0;f<MetaData().FrequencyCount(spectralWindow);++f) {
			for(size_t p=0; p<Polarizations().size(); ++p)
			{
				dataBuffer[dataBufferPtr] = _realImages[p]->Value(x, f);
				++dataBufferPtr;
				dataBuffer[dataBufferPtr] = _imaginaryImages[p]->Value(x, f);
				++dataBufferPtr;
			}
		}

		dataFile.write(reinterpret_cast<char*>(&dataBuffer[0]), bufferSize * sizeof(float) * 2);
		if(dataFile.bad())
			throw std::runtime_error("Error: failed to update temporary data files! Check access rights and free disk space.");
	}
	
	_reorderedDataFilesHaveChanged = true;
	
	Logger::Debug << "Done writing.\n";
}

void IndirectBaselineReader::performFlagWriteTask(std::vector<Mask2DCPtr> flags, unsigned antenna1, unsigned antenna2, unsigned spw, unsigned sequenceId)
{
	initializeMeta();

	const unsigned polarizationCount = Polarizations().size();
	
	if(flags.size() != polarizationCount)
		throw std::runtime_error("PerformDataWriteTask: input format did not match number of polarizations in measurement set");
	
	for(size_t i=1;i<flags.size();++i)
	{
		if(flags[0]->Width() != flags[i]->Width() || flags[0]->Height() != flags[i]->Height())
		throw std::runtime_error("PerformDataWriteTask: width and/or height of input images did not match");
	}
	
	if(!_msIsReordered) reorderMS();
	
	const size_t width = flags[0]->Width();
	const size_t bufferSize = MetaData().FrequencyCount(spw) * Polarizations().size();
	
	std::ofstream flagFile(FlagFilename(), std::ofstream::binary | std::ios_base::in | std::ios_base::out);
	size_t index = _seqIndexTable->Value(antenna1, antenna2, spw, sequenceId);
	size_t filePos = _filePositions[index];
	flagFile.seekp(filePos*(sizeof(bool)), std::ios_base::beg);
	
	std::unique_ptr<bool[]> flagBuffer( new bool[bufferSize] );
	for(size_t x=0;x<width;++x)
	{
		size_t flagBufferPtr = 0;
		for(size_t f=0;f<MetaData().FrequencyCount(spw);++f) {
			for(size_t p=0; p<polarizationCount; ++p)
			{
				flagBuffer[flagBufferPtr] = flags[p]->Value(x, f);
				++flagBufferPtr;
			}
		}

		flagFile.write(reinterpret_cast<char*>(flagBuffer.get()), bufferSize * sizeof(bool));
		if(flagFile.bad())
			throw std::runtime_error("Error: failed to update temporary flag files! Check access rights and free disk space.");
	}
	
	_reorderedFlagFilesHaveChanged = true;
}

template<bool UpdateData, bool UpdateFlags>
void IndirectBaselineReader::updateOriginalMS()
{
	casacore::MeasurementSet ms = OpenMS();
	if(UpdateData || UpdateFlags)
	{
		ms.reopenRW();
	}

	casacore::ScalarColumn<double> timeColumn(ms, "TIME");
	casacore::ScalarColumn<int> antenna1Column(ms, "ANTENNA1"); 
	casacore::ScalarColumn<int> antenna2Column(ms, "ANTENNA2");
	casacore::ScalarColumn<int> fieldIdColumn(ms, "FIELD_ID");
	casacore::ScalarColumn<int> dataDescIdColumn(ms, "DATA_DESC_ID");
	casacore::ArrayColumn<bool> flagColumn(ms, "FLAG");
	casacore::ArrayColumn<casacore::Complex> dataColumn(ms, DataColumnName());

	std::vector<MSMetaData::Sequence> sequences = MetaData().GetSequences();
	std::vector<size_t> dataIdToSpw;
	MetaData().GetDataDescToBandVector(dataIdToSpw);
	
	size_t polarizationCount = Polarizations().size();

	Logger::Debug << "Opening updated files\n";
	UpdateInfo updateInfo;
	
	if(UpdateData)
	{
		updateInfo.dataFile.reset(new std::ifstream(DataFilename(), std::ifstream::binary));
		if(updateInfo.dataFile->fail())
			throw std::runtime_error("Failed to open temporary data file");
	}
	if(UpdateFlags)
	{
		updateInfo.flagFile.reset(new std::ifstream(FlagFilename(), std::ifstream::binary));
		if(updateInfo.flagFile->fail())
			throw std::runtime_error("Failed to open temporary flag file");
	}

	std::vector<size_t> updatedFilePos = _filePositions;
	std::vector<size_t> timePositions(updatedFilePos.size(), size_t(-1));
		
	MSSelection msSelection(ms, ObservationTimesPerSequence());
	msSelection.Process(
		[&](size_t rowIndex, size_t sequenceId, size_t timeIndexInSequence)
	{
		size_t
			antenna1 = antenna1Column(rowIndex),
			antenna2 = antenna2Column(rowIndex),
			spw = dataIdToSpw[dataDescIdColumn(rowIndex)],
			channelCount = MetaData().FrequencyCount(spw),
			arrayIndex = _seqIndexTable->Value(antenna1, antenna2, spw, sequenceId),
			sampleCount = channelCount * polarizationCount;
		size_t &filePos = updatedFilePos[arrayIndex];
		size_t &timePos = timePositions[arrayIndex];
		
		casacore::IPosition shape(2, polarizationCount, channelCount);
		
		// Skip over samples in the temporary files that are missing in the measurement set
		++timePos;
		while(timePos < timeIndexInSequence)
		{
			filePos += sampleCount;
			++timePos;
		}
		
		if(UpdateData)
		{
			casacore::Array<casacore::Complex> data(shape);
			
			std::ifstream &dataFile = *updateInfo.dataFile;
			dataFile.seekg(filePos*(sizeof(float)*2), std::ios_base::beg);
			dataFile.read(reinterpret_cast<char*>(&*data.cbegin()), sampleCount * 2 * sizeof(float));
			if(dataFile.fail())
				throw std::runtime_error("Error: failed to read temporary data files!");
						
			dataColumn.basePut(rowIndex, data);
		}
		if(UpdateFlags)
		{
			casacore::Array<bool> flagArray(shape);
			
			std::ifstream &flagFile = *updateInfo.flagFile;
			flagFile.seekg(filePos*sizeof(bool), std::ios_base::beg);
			flagFile.read(reinterpret_cast<char*>(&*flagArray.cbegin()), sampleCount * sizeof(bool));
			if(flagFile.fail())
				throw std::runtime_error("Error: failed to read temporary flag files!");
			
			flagColumn.basePut(rowIndex, flagArray);
		}
		
		filePos += sampleCount;
	});
	
	Logger::Debug << "Freeing the data\n";
	
	// Close the files
	updateInfo.dataFile.reset();
	updateInfo.flagFile.reset();

	if(UpdateData)
		Logger::Debug << "Done updating measurement set data\n";
	if(UpdateFlags)
		Logger::Debug << "Done updating measurement set flags\n";
}

void IndirectBaselineReader::updateOriginalMSData()
{
	Logger::Debug << "Data was changed, need to update the original MS...\n";
	updateOriginalMS<true, false>();
	_reorderedDataFilesHaveChanged = false;
}

void IndirectBaselineReader::updateOriginalMSFlags()
{
	Stopwatch watch(true);
	Logger::Debug << "Flags were changed, need to update the original MS...\n";
	updateOriginalMS<false, true>();
	_reorderedFlagFilesHaveChanged = false;
	Logger::Debug << "Storing flags toke: " << watch.ToString() << '\n';
}
