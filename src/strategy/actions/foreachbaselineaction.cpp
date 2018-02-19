#include "foreachbaselineaction.h"

#include "../../structures/antennainfo.h"
#include "../../structures/system.h"

#include "../../util/logger.h"
#include "../../util/stopwatch.h"

#include "../imagesets/bhfitsimageset.h"
#include "../imagesets/fitsimageset.h"
#include "../imagesets/imageset.h"
#include "../imagesets/msimageset.h"
#include "../imagesets/filterbankset.h"
#include "../imagesets/qualitystatimageset.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

namespace rfiStrategy {
	
	void ForEachBaselineAction::Perform(ArtifactSet &artifacts, ProgressListener &progress)
	{
		if(!artifacts.HasImageSet())
		{
			progress.OnStartTask(*this, 0, 1, "For each baseline (no image set)");
			progress.OnEndTask(*this);
			Logger::Warn <<
				"I executed a ForEachBaselineAction without an active imageset: something is\n"
				"likely wrong. Check your strategy and the input files.\n";
		} else if(_selection == Current)
		{
			ActionBlock::Perform(artifacts, progress);
		} else
		{
			ImageSet& imageSet = artifacts.ImageSet();
			MSImageSet* msImageSet = dynamic_cast<MSImageSet*>(&imageSet);
			if(msImageSet != 0)
			{
				// Check memory usage
				std::unique_ptr<ImageSetIndex> tempIndex = msImageSet->StartIndex();
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
			if(dynamic_cast<FilterBankSet*>(&imageSet) != nullptr && _threadCount != 1)
			{
				Logger::Info << "This is a Filterbank set -- disabling multi-threading\n";
				_threadCount = 1;
			}
			if(!_antennaeToSkip.empty())
			{
				Logger::Debug << "The following antennas will be skipped: ";
				for(std::set<size_t>::const_iterator i=_antennaeToSkip.begin();i!=_antennaeToSkip.end(); ++i)
					Logger::Debug << (*i) << ' ';
				Logger::Debug <<'\n';
			}
			if(!_antennaeToInclude.empty())
			{
				Logger::Debug << "Only the following antennas will be included: ";
				for(std::set<size_t>::const_iterator i=_antennaeToInclude.begin();i!=_antennaeToInclude.end(); ++i)
					Logger::Debug << (*i) << ' ';
				Logger::Debug <<'\n';
			}

			if(artifacts.MetaData() != 0)
			{
				_hasInitAntennae = true;
				if(artifacts.MetaData()->HasAntenna1())
					_initAntenna1 = artifacts.MetaData()->Antenna1();
				else
					_hasInitAntennae = false;
				if(artifacts.MetaData()->HasAntenna2())
					_initAntenna2 = artifacts.MetaData()->Antenna2();
				else
					_hasInitAntennae = false;
			}
			_artifacts = &artifacts;
			
			_initPartIndex = 0;
			_finishedBaselines = false;
			_baselineCount = 0;
			_baselineProgress = 0;
			_nextIndex = 0;
			
			// Count the baselines that are to be processed
			std::unique_ptr<ImageSetIndex> iteratorIndex = imageSet.StartIndex();
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
			_progressTaskNo = new int[_threadCount];
			_progressTaskCount = new int[_threadCount];
			progress.OnStartTask(*this, 0, 1, "Initializing");

			std::vector<std::thread> threadGroup;
			ReaderFunction reader(*this);
			threadGroup.emplace_back(reader);
			
			size_t mathThreads = mathThreadCount();
			for(unsigned i=0;i<mathThreads;++i)
			{
				PerformFunction function(*this, progress, i);
				threadGroup.emplace_back(function);
			}
			for(std::thread& t : threadGroup)
				t.join();
			progress.OnEndTask(*this);

			if(_resultSet != 0)
			{
				artifacts = *_resultSet;
				delete _resultSet;
			}

			delete[] _progressTaskCount;
			delete[] _progressTaskNo;

			_loopIndex.reset();

			if(_exceptionOccured)
				throw std::runtime_error("An exception occured in one of the sub tasks of the (multi-threaded) \"For-each baseline\"-action: the RFI strategy will not continue.");
		}
	}

	bool ForEachBaselineAction::IsBaselineSelected(ImageSetIndex &index)
	{
		ImageSet& imageSet = _artifacts->ImageSet();
		IndexableSet* msImageSet = dynamic_cast<IndexableSet*>(&imageSet);
		size_t a1id, a2id;
		if(msImageSet != nullptr)
		{
			a1id = msImageSet->GetAntenna1(index);
			a2id = msImageSet->GetAntenna2(index);
			if(!_bands.empty() && _bands.count(msImageSet->GetBand(index))==0)
				return false;
			if(!_fields.empty() && _fields.count(msImageSet->GetField(index))==0)
				return false;
		} else {
			a1id = 0;
			a2id = 0;
		}
		if(_antennaeToSkip.count(a1id) != 0 || _antennaeToSkip.count(a2id) != 0)
			return false;
		if(!_antennaeToInclude.empty() && (_antennaeToInclude.count(a1id) == 0 && _antennaeToInclude.count(a2id) == 0))
			return false;
		
		// For SD/BHFits/QS files, we want to select everything -- it's confusing
		// if the default option "only flag cross correlations" would also
		// hold for sdfits files.
		if(dynamic_cast<FitsImageSet*>(&imageSet)!=0
			|| dynamic_cast<BHFitsImageSet*>(&imageSet)!=0
			|| dynamic_cast<FilterBankSet*>(&imageSet)!=0
			|| dynamic_cast<QualityStatImageSet*>(&imageSet)!=0)
			return true;

		switch(_selection)
		{
			case All:
				return true;
			case CrossCorrelations:
			{
				return a1id != a2id;
			}
			case AutoCorrelations:
				return a1id == a2id;
			case EqualToCurrent: {
				if(!_hasInitAntennae)
					throw BadUsageException("For each baseline over 'EqualToCurrent' with no current baseline");
				throw BadUsageException("Not implemented");
			}
			case AutoCorrelationsOfCurrentAntennae:
				if(!_hasInitAntennae)
					throw BadUsageException("For each baseline over 'AutoCorrelationsOfCurrentAntennae' with no current baseline");
				return a1id == a2id && (_initAntenna1.id == a1id || _initAntenna2.id == a1id);
			default:
				return false;
		}
	}

	std::unique_ptr<ImageSetIndex> ForEachBaselineAction::GetNextIndex()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		while(_loopIndex->IsValid())
		{
			if(IsBaselineSelected(*_loopIndex))
			{
				std::unique_ptr<ImageSetIndex> newIndex = _loopIndex->Clone();
				_loopIndex->Next();

				return newIndex;
			}
			_loopIndex->Next();
		}
		return nullptr;
	}

	void ForEachBaselineAction::SetExceptionOccured()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_exceptionOccured = true;
	}
	
	void ForEachBaselineAction::SetFinishedBaselines()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_finishedBaselines = true;
	}
	
	void ForEachBaselineAction::PerformFunction::operator()()
	{
		std::unique_lock<std::mutex> ioLock(_action._artifacts->IOMutex());
		std::unique_ptr<ImageSet> privateImageSet = _action._artifacts->ImageSet().Clone();
		ioLock.unlock();

		try {
			std::unique_lock<std::mutex> lock(_action._mutex);
			ArtifactSet newArtifacts(*_action._artifacts);
			lock.unlock();
			
			std::unique_ptr<BaselineData> baseline = _action.GetNextBaseline();
			
			while(baseline != nullptr) {
				baseline->Index().Reattach(*privateImageSet);
				
				std::ostringstream progressStr;
				if(_action._hasInitAntennae)
					progressStr << "Processing baseline " << baseline->MetaData()->Antenna1().name << " x " << baseline->MetaData()->Antenna2().name;
				else
					progressStr << "Processing next baseline";
				_action.SetProgress(_progress, _action.BaselineProgress(), _action._baselineCount, progressStr.str(), _threadIndex);
	
				newArtifacts.SetOriginalData(baseline->Data());
				newArtifacts.SetContaminatedData(baseline->Data());
				TimeFrequencyData zero(baseline->Data());
				zero.SetImagesToZero();
				newArtifacts.SetRevisedData(zero);
				newArtifacts.SetImageSetIndex(baseline->Index().Clone());
				newArtifacts.SetMetaData(baseline->MetaData());

				_action.ActionBlock::Perform(newArtifacts, *this);
	
				baseline = _action.GetNextBaseline();
				_action.IncBaselineProgress();
			}
	
			if(_threadIndex == 0)
				_action._resultSet = new ArtifactSet(newArtifacts);

		} catch(std::exception &e)
		{
			_progress.OnException(_action, e);
			_action.SetExceptionOccured();
		}
	}

	void ForEachBaselineAction::PerformFunction::OnStartTask(const Action &/*action*/, size_t /*taskNo*/, size_t /*taskCount*/, const std::string &/*description*/, size_t /*weight*/)
	{
	}

	void ForEachBaselineAction::PerformFunction::OnEndTask(const Action &/*action*/)
	{
	}

	void ForEachBaselineAction::PerformFunction::OnProgress(const Action &/*action*/, size_t /*progres*/, size_t /*maxProgress*/)
	{
	}

	void ForEachBaselineAction::PerformFunction::OnException(const Action &action, std::exception &thrownException)
	{
		_progress.OnException(action, thrownException);
	}
	
	void ForEachBaselineAction::ReaderFunction::operator()()
	{
		Stopwatch watch(true);
		bool finished = false;
		size_t threadCount = _action.mathThreadCount();
		size_t minRecommendedBufferSize, maxRecommendedBufferSize;
		MSImageSet* msImageSet = dynamic_cast<MSImageSet*>(&_action._artifacts->ImageSet());
		if(msImageSet != nullptr)
		{
			minRecommendedBufferSize = msImageSet->Reader()->GetMinRecommendedBufferSize(threadCount);
			maxRecommendedBufferSize = msImageSet->Reader()->GetMaxRecommendedBufferSize(threadCount) - _action.GetBaselinesInBufferCount();
		} else {
			minRecommendedBufferSize = 1;
			maxRecommendedBufferSize = 2;
		}
		
		do {
			watch.Pause();
			_action.WaitForBufferAvailable(minRecommendedBufferSize);
			
			size_t wantedCount = maxRecommendedBufferSize - _action.GetBaselinesInBufferCount();
			size_t requestedCount = 0;
			
			std::unique_lock<std::mutex> lock(_action._artifacts->IOMutex());
			watch.Start();
			
			for(size_t i=0;i<wantedCount;++i)
			{
				std::unique_ptr<ImageSetIndex> index = _action.GetNextIndex();
				if(index != nullptr)
				{
					_action._artifacts->ImageSet().AddReadRequest(*index);
					++requestedCount;
				} else {
					finished = true;
					break;
				}
			}
			
			if(requestedCount > 0)
			{
				_action._artifacts->ImageSet().PerformReadRequests();
				watch.Pause();
				
				for(size_t i=0;i<requestedCount;++i)
				{
					std::unique_ptr<BaselineData> baseline = _action._artifacts->ImageSet().GetNextRequested();
					
					std::lock_guard<std::mutex> bufferLock(_action._mutex);
					_action._baselineBuffer.emplace(std::move(baseline));
				}
			}
			
			lock.unlock();
			
			_action._dataAvailable.notify_all();
			watch.Start();
		} while(!finished);
		_action.SetFinishedBaselines();
		_action._dataAvailable.notify_all();
		watch.Pause();
		Logger::Debug << "Time spent on reading: " << watch.ToString() << '\n';
	}

	void ForEachBaselineAction::SetProgress(ProgressListener &progress, int no, int count, const std::string& taskName, int threadId)
	{
	  std::lock_guard<std::mutex> lock(_mutex);
		_progressTaskNo[threadId] = no;
		_progressTaskCount[threadId] = count;
		size_t totalCount = 0, totalNo = 0;
		for(size_t i=0;i<_threadCount;++i)
		{
			totalCount += _progressTaskCount[threadId];
			totalNo += _progressTaskNo[threadId];
		}
		progress.OnEndTask(*this);
		std::stringstream str;
		str << "T" << threadId << ": " << taskName;
		progress.OnStartTask(*this, totalNo, totalCount, str.str());
	}
	
	std::string ForEachBaselineAction::memToStr(double memSize)
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
}

