#include "baselineiterator.h"

#include "../lua/luathreadgroup.h"

#include "../python/scriptdata.h"

#include "../structures/antennainfo.h"
#include "../structures/system.h"

#include "../util/logger.h"
#include "../util/stopwatch.h"

#include "../strategy/imagesets/bhfitsimageset.h"
#include "../strategy/imagesets/fitsimageset.h"
#include "../strategy/imagesets/imageset.h"
#include "../strategy/imagesets/msimageset.h"
#include "../strategy/imagesets/filterbankset.h"
#include "../strategy/imagesets/qualitystatimageset.h"
#include "../strategy/imagesets/rfibaselineset.h"

#include "writethread.h"

#include <sstream>
#include <vector>

BaselineIterator::BaselineIterator(std::mutex* ioMutex, const Options& options) :
		_options(options),
		_baselineCount(0),
		_nextIndex(0),
		_threadCount(4),
		_loopIndex(nullptr),
		_ioMutex(ioMutex),
		_finishedBaselines(false),
		_exceptionOccured(false),
		_baselineProgress(0)
{
}
	
BaselineIterator::~BaselineIterator()
{ }

void BaselineIterator::Run(rfiStrategy::ImageSet& imageSet, LuaThreadGroup& lua)
{
	_lua = &lua;
	_imageSet = &imageSet;
	_threadCount = _options.threadCount.get();
	
	_writeThread.reset( new WriteThread(imageSet, _threadCount, _ioMutex) );
	
	rfiStrategy::MSImageSet* msImageSet = dynamic_cast<rfiStrategy::MSImageSet*>(&imageSet);
	if(msImageSet)
	{
		// Check memory usage
		std::unique_ptr<rfiStrategy::ImageSetIndex> tempIndex = msImageSet->StartIndex();
		size_t timeStepCount = msImageSet->ObservationTimesVector(*tempIndex).size();
		tempIndex.reset();
		size_t channelCount = msImageSet->GetBandInfo(0).channels.size();
		double estMemorySizePerThread =
			8.0/*bp complex*/ * 4.0 /*polarizations*/ *
			double(timeStepCount) * double(channelCount) *
			3.0 /* approx copies of the data that will be made in memory*/;
		Logger::Debug << "Estimate of memory each thread will use: " << memToStr(estMemorySizePerThread) << ".\n";
		size_t compThreadCount = _threadCount;
		if(compThreadCount > 0) --compThreadCount;
		
		int64_t memSize = System::TotalMemory();
		Logger::Debug << "Detected " << memToStr(memSize) << " of system memory.\n";
		
		if(estMemorySizePerThread * double(compThreadCount) > memSize)
		{
			size_t maxThreads = size_t(memSize / estMemorySizePerThread);
			if(maxThreads < 1) maxThreads = 1;
			Logger::Warn <<
				"This measurement set is TOO LARGE to be processed with " << _threadCount << " threads!\n" <<
				_threadCount << " threads would require " << memToStr(estMemorySizePerThread*compThreadCount) << " of memory approximately.\n"
				"Number of threads that will actually be used: " << maxThreads << "\n"
				"This might hurt performance a lot!\n\n";
			_threadCount = maxThreads;
		}
	}
	if(dynamic_cast<rfiStrategy::FilterBankSet*>(&imageSet) != nullptr && _threadCount != 1)
	{
		Logger::Info << "This is a Filterbank set -- disabling multi-threading\n";
		_threadCount = 1;
	}
	if(!_options.antennaeToSkip.empty())
	{
		Logger::Debug << "The following antennas will be skipped: ";
		for(size_t a : _options.antennaeToSkip)
			Logger::Debug << a << ' ';
		Logger::Debug <<'\n';
	}
	if(!_options.antennaeToInclude.empty())
	{
		Logger::Debug << "Only the following antennas will be included: ";
		for(size_t a : _options.antennaeToInclude)
			Logger::Debug << a << ' ';
		Logger::Debug <<'\n';
	}

	_finishedBaselines = false;
	_baselineCount = 0;
	_baselineProgress = 0;
	_nextIndex = 0;
	
	// Count the baselines that are to be processed
	std::unique_ptr<rfiStrategy::ImageSetIndex> iteratorIndex = imageSet.StartIndex();
	while(iteratorIndex->IsValid())
	{
		if(IsBaselineSelected(*iteratorIndex))
			++_baselineCount;
		iteratorIndex->Next();
	}
	iteratorIndex.reset();
	Logger::Debug << "Will process " << _baselineCount << " baselines.\n";
	
	// Initialize thread data and threads
	_loopIndex = imageSet.StartIndex();

	std::vector<std::thread> threadGroup;
	ReaderThread reader(*this);
	threadGroup.emplace_back(reader);
	
	for(unsigned i=0; i<_threadCount; ++i)
	{
		ProcessingThread function(*this, i);
		threadGroup.emplace_back(function);
	}
	for(std::thread& t : threadGroup)
		t.join();

	_loopIndex.reset();
	
	_writeThread.reset();

	if(_exceptionOccured)
		throw std::runtime_error("An exception occured in the parallel (multi-threaded) processing of the baselines: the RFI strategy will not continue.");
}

bool BaselineIterator::IsBaselineSelected(rfiStrategy::ImageSetIndex &index)
{
	rfiStrategy::IndexableSet* idImageSet = dynamic_cast<rfiStrategy::IndexableSet*>(_imageSet);
	size_t a1id, a2id;
	if(idImageSet != nullptr)
	{
		a1id = idImageSet->GetAntenna1(index);
		a2id = idImageSet->GetAntenna2(index);
		if(!_options.bands.empty() && _options.bands.count(idImageSet->GetBand(index))==0)
			return false;
		if(!_options.fields.empty() && _options.fields.count(idImageSet->GetField(index))==0)
			return false;
	} else {
		a1id = 0;
		a2id = 0;
	}
	if(_options.antennaeToSkip.count(a1id) != 0 || _options.antennaeToSkip.count(a2id) != 0)
		return false;
	if(!_options.antennaeToInclude.empty() && (_options.antennaeToInclude.count(a1id) == 0 && _options.antennaeToInclude.count(a2id) == 0))
		return false;
	
	// For SD/BHFits/QS/rfibl files, we want to select everything -- it's confusing
	// if the default option "only flag cross correlations" would also
	// hold for single-"baseline" files.
	if(!_imageSet->HasCrossCorrelations())
		return true;

	switch(_options.baselineSelection)
	{
		case rfiStrategy::All:
			return true;
		case rfiStrategy::CrossCorrelations:
		{
			return a1id != a2id;
		}
		case rfiStrategy::AutoCorrelations:
			return a1id == a2id;
		case rfiStrategy::Current:
		case rfiStrategy::EqualToCurrent:
		case rfiStrategy::AutoCorrelationsOfCurrentAntennae:
			throw BadUsageException("Not implemented");
	}
	return false;
}

std::unique_ptr<rfiStrategy::ImageSetIndex> BaselineIterator::GetNextIndex()
{
	std::lock_guard<std::mutex> lock(_mutex);
	while(_loopIndex->IsValid())
	{
		if(IsBaselineSelected(*_loopIndex))
		{
			std::unique_ptr<rfiStrategy::ImageSetIndex> newIndex = _loopIndex->Clone();
			_loopIndex->Next();

			return newIndex;
		}
		_loopIndex->Next();
	}
	return nullptr;
}

void BaselineIterator::SetExceptionOccured()
{
	std::lock_guard<std::mutex> lock(_mutex);
	_exceptionOccured = true;
}

void BaselineIterator::SetFinishedBaselines()
{
	std::lock_guard<std::mutex> lock(_mutex);
	_finishedBaselines = true;
}

void BaselineIterator::ProcessingThread::operator()()
{
	std::unique_lock<std::mutex> ioLock(*_parent._ioMutex);
	std::unique_ptr<rfiStrategy::ImageSet> privateImageSet = _parent._imageSet->Clone();
	ioLock.unlock();

	try {
		std::unique_ptr<rfiStrategy::BaselineData> baseline = _parent.GetNextBaseline();
		
		while(baseline != nullptr) {
			baseline->Index().Reattach(*privateImageSet);
			
			std::ostringstream progressStr;
			if(baseline->MetaData()->HasAntenna1() && baseline->MetaData()->HasAntenna2())
				progressStr << "Processing baseline " << baseline->MetaData()->Antenna1().name << " x " << baseline->MetaData()->Antenna2().name;
			else
				progressStr << "Processing next baseline";
			
			// TODO _parent.SetProgress(_progress, _parent.BaselineProgress(), _parent._baselineCount, progressStr.str(), _threadIndex);

			ScriptData scriptData;
			TimeFrequencyData data(baseline->Data());
			_parent._lua->Execute(_threadIndex, data, baseline->MetaData(), scriptData);
			
			_parent._writeThread->SaveFlags(data, baseline->Index());

			baseline = _parent.GetNextBaseline();
			_parent.IncBaselineProgress();
		}

	} catch(std::exception &e)
	{
		// TODO _progress.OnException(_parent, e);
		Logger::Error << e.what() << '\n';
		_parent.SetExceptionOccured();
	}
	Logger::Debug << "Processing thread finished.\n";
}

void BaselineIterator::ReaderThread::operator()()
{
	Stopwatch watch(true);
	bool finished = false;
	size_t threadCount = _parent._threadCount;
	size_t minRecommendedBufferSize, maxRecommendedBufferSize;
	rfiStrategy::MSImageSet* msImageSet = dynamic_cast<rfiStrategy::MSImageSet*>(_parent._imageSet);
	if(msImageSet != nullptr)
	{
		minRecommendedBufferSize = msImageSet->Reader()->GetMinRecommendedBufferSize(threadCount);
		maxRecommendedBufferSize = msImageSet->Reader()->GetMaxRecommendedBufferSize(threadCount) - _parent.GetBaselinesInBufferCount();
	} else {
		minRecommendedBufferSize = 1;
		maxRecommendedBufferSize = 2;
	}
	
	do {
		watch.Pause();
		_parent.WaitForReadBufferAvailable(minRecommendedBufferSize);
		
		size_t wantedCount = maxRecommendedBufferSize - _parent.GetBaselinesInBufferCount();
		size_t requestedCount = 0;
		
		std::unique_lock<std::mutex> lock(*_parent._ioMutex);
		watch.Start();
		
		for(size_t i=0;i<wantedCount;++i)
		{
			std::unique_ptr<rfiStrategy::ImageSetIndex> index = _parent.GetNextIndex();
			if(index != nullptr)
			{
				_parent._imageSet->AddReadRequest(*index);
				++requestedCount;
			} else {
				finished = true;
				break;
			}
		}
		
		if(requestedCount > 0)
		{
			_parent._imageSet->PerformReadRequests();
			watch.Pause();
			
			for(size_t i=0;i<requestedCount;++i)
			{
				std::unique_ptr<rfiStrategy::BaselineData> baseline = _parent._imageSet->GetNextRequested();
				
				std::lock_guard<std::mutex> bufferLock(_parent._mutex);
				_parent._baselineBuffer.emplace(std::move(baseline));
			}
		}
		
		lock.unlock();
		
		_parent._dataAvailable.notify_all();
		watch.Start();
	} while(!finished);
	_parent.SetFinishedBaselines();
	_parent._dataAvailable.notify_all();
	watch.Pause();
	Logger::Debug << "Time spent on reading: " << watch.ToString() << '\n';
}

std::string BaselineIterator::memToStr(double memSize)
{
	std::ostringstream str;
	if(memSize > 1024.0*1024.0*1024.0*1024.0)
		str << round(memSize*10.0 / (1024.0*1024.0*1024.0*1024.0))/10.0 << " TB";
	else if(memSize > 1024.0*1024.0*1024.0)
		str << round(memSize*10.0 / (1024.0*1024.0*1024.0))/10.0 << " GB";
	else if(memSize > 1024.0*1024.0)
		str << round(memSize*10.0 / (1024.0*1024.0))/10.0 << " MB";
	else if(memSize > 1024.0)
		str << round(memSize*10.0 / (1024.0))/10.0 << " KB";
	else
		str << memSize << " B";
	return str.str();
}
