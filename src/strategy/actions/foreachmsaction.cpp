#include "foreachmsaction.h"

#include <boost/filesystem.hpp>

#include "../../structures/msmetadata.h"

#include "strategy.h"

#include "../control/artifactset.h"
#include "../control/defaultstrategy.h"
#include "../control/strategywriter.h"

#include "../imagesets/imageset.h"
#include "../imagesets/joinedspwset.h"
#include "../imagesets/msimageset.h"

#include "../../util/logger.h"
#include "../../util/progresslistener.h"

#include <memory>

namespace rfiStrategy {

void ForEachMSAction::Initialize()
{ }

void ForEachMSAction::Perform(ArtifactSet& artifacts, ProgressListener& progress)
{
	unsigned taskIndex = 0;
	
	FinishAll();

	for(const std::string& filename : _filenames)
	{
		progress.OnStartTask(*this, taskIndex, _filenames.size(), std::string("Processing measurement set ") + filename);
		processMS(filename, artifacts, progress);
		progress.OnEndTask(*this);

		++taskIndex;
	}

	InitializeAll();
}

void ForEachMSAction::processMS(const std::string& filename, ArtifactSet& artifacts, ProgressListener& progress)
{
	bool skip = false;
	if(_skipIfAlreadyProcessed)
	{
		MSMetaData set(filename);
		if(set.HasAOFlaggerHistory())
		{
			skip = true;
			Logger::Info << "Skipping " << filename << ",\n"
				"because the set contains AOFlagger history and -skip-flagged was given.\n";
		}
	}

	if(!skip)
	{
		size_t nIntervals = 1;
		size_t intervalIndex = 0;
		size_t resolvedIntStart = 0, resolvedIntEnd = 0;
		bool isMS = false;
		
		while(intervalIndex < nIntervals)
		{
			std::unique_ptr<ImageSet> imageSet(ImageSet::Create(std::vector<std::string>{filename}, _baselineIOMode));
			isMS = dynamic_cast<MSImageSet*>(imageSet.get()) != nullptr;
			if(isMS)
			{ 
				MSImageSet* msImageSet = static_cast<MSImageSet*>(imageSet.get());
				msImageSet->SetDataColumnName(_dataColumnName);
				//msImageSet->SetSubtractModel(_subtractModel);
				msImageSet->SetReadUVW(_readUVW);
				// during the first iteration, the nr of intervals hasn't been calculated yet. Do that now.
				if(intervalIndex == 0)
				{
					if(_maxIntervalSize)
					{
						msImageSet->SetInterval(_intervalStart, _intervalEnd);
						const size_t obsTimesSize = msImageSet->MetaData().GetObservationTimesSet().size();
						nIntervals = (obsTimesSize + *_maxIntervalSize - 1) / *_maxIntervalSize;
						Logger::Info << "Maximum interval size of " << *_maxIntervalSize << " timesteps for total of " << obsTimesSize << " timesteps results in " << nIntervals << " intervals.\n";
						if(_intervalStart)
							resolvedIntStart = *_intervalStart;
						else
							resolvedIntStart = 0;
						if(_intervalEnd)
							resolvedIntEnd = *_intervalEnd;
						else
							resolvedIntEnd = obsTimesSize + resolvedIntStart;
					}
					else {
						nIntervals = 1;
					}
				}
				if(nIntervals == 1)
					msImageSet->SetInterval(_intervalStart, _intervalEnd);
				else {
					size_t nTimes = resolvedIntEnd - resolvedIntStart;
					size_t start = resolvedIntStart + intervalIndex*nTimes/nIntervals;
					size_t end = resolvedIntStart + (intervalIndex+1)*nTimes/nIntervals;
					Logger::Info << "Starting flagging of interval " << intervalIndex << ", timesteps " << start << " - " << end << '\n';
					msImageSet->SetInterval(start, end);
				}
				if(_combineSPWs)
				{
					msImageSet->Initialize();
					imageSet.release();
					std::unique_ptr<MSImageSet> msImageSetPtr(msImageSet);
					imageSet.reset(new JoinedSPWSet(std::move(msImageSetPtr)));
				}
			}
			imageSet->Initialize();
			
			if(_loadOptimizedStrategy)
			{
				rfiStrategy::DefaultStrategy::TelescopeId telescopeId;
				unsigned flags;
				double frequency, timeResolution, frequencyResolution;
				rfiStrategy::DefaultStrategy::DetermineSettings(*imageSet, telescopeId, flags, frequency, timeResolution, frequencyResolution);
				rfiStrategy::DefaultStrategy::StrategySetup setup = rfiStrategy::DefaultStrategy::DetermineSetup(telescopeId, flags, frequency, timeResolution, frequencyResolution);
				RemoveAll();
				rfiStrategy::DefaultStrategy::LoadFullStrategy(*this, setup);
			}
			
			if(_threadCount != 0)
				rfiStrategy::Strategy::SetThreadCount(*this, _threadCount);
			if(!_bandpassFilename.empty())
				rfiStrategy::Strategy::SetBandpassFilename(*this, _bandpassFilename);
			
			if(!_fields.empty() || !_bands.empty())
			{
				std::vector<Action*> fobActions = DefaultStrategy::FindActions(*this, ForEachBaselineActionType);
				for(std::vector<Action*>::iterator i=fobActions.begin(); i!=fobActions.end(); ++i)
				{
					ForEachBaselineAction* fobAction = static_cast<ForEachBaselineAction*>(*i);
					fobAction->Bands() = _bands;
					fobAction->Fields() = _fields;
				}
			}
			
			std::unique_ptr<ImageSetIndex> index(imageSet->StartIndex());
			artifacts.SetImageSet(std::move(imageSet));
			artifacts.SetImageSetIndex(std::move(index));

			InitializeAll();
			
			ActionBlock::Perform(artifacts, progress);
			
			FinishAll();
			
			artifacts.SetNoImageSet();
			
			++intervalIndex;
		}

		if(isMS)
			writeHistory(filename);
	}
}

void ForEachMSAction::AddDirectory(const std::string &name)
{
  // get all files ending in .MS
  boost::filesystem::path dir_path(name);
  boost::filesystem::directory_iterator end_it;

  for(boost::filesystem::directory_iterator it(dir_path); it != end_it; ++it) {
    if( is_directory(it->status()) && extension(it->path()) == ".MS" ) {
      _filenames.push_back( it->path().string() );
    }
  }
}

void ForEachMSAction::writeHistory(const std::string &filename)
{
	if(GetChildCount() != 0)
	{
		MSMetaData ms(filename);
		const Strategy *strategy = nullptr;
		if(GetChildCount() == 1 && dynamic_cast<const Strategy*>(&GetChild(0)) != nullptr)
		{
			strategy = static_cast<const Strategy*>(&GetChild(0));
		} else {
			const ActionContainer *root = GetRoot();
			if(dynamic_cast<const Strategy*>(root))
				strategy = static_cast<const Strategy*>(root);
		}
		Logger::Debug << "Adding strategy to history table of MS...\n";
		if(strategy != nullptr) {
			try {
				std::ostringstream ostr;
				rfiStrategy::StrategyWriter writer;
				writer.WriteToStream(*strategy, ostr);

				ms.AddAOFlaggerHistory(ostr.str(), _commandLineForHistory);
			} catch(std::exception &e)
			{
				Logger::Warn << "Failed to write history to MS: " << e.what() << '\n';
			}
		}
		else
			Logger::Error << "Could not find root strategy to write to Measurement Set history table!\n";
	}
}

}
