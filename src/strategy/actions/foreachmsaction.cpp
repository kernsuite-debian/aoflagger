#include "foreachmsaction.h"

#include <boost/filesystem.hpp>

#include "../../structures/measurementset.h"

#include "strategy.h"

#include "../control/artifactset.h"
#include "../control/defaultstrategy.h"

#include "../imagesets/imageset.h"
#include "../imagesets/joinedspwset.h"
#include "../imagesets/msimageset.h"

#include "../../util/logger.h"
#include "../../util/progresslistener.h"

#include <memory>

namespace rfiStrategy {

void ForEachMSAction::Initialize()
{
}

void ForEachMSAction::Perform(ArtifactSet &artifacts, ProgressListener &progress)
{
	unsigned taskIndex = 0;
	
	FinishAll();

	for(std::vector<std::string>::const_iterator i=_filenames.begin();i!=_filenames.end();++i)
	{
		std::string filename = *i;
		
		progress.OnStartTask(*this, taskIndex, _filenames.size(), std::string("Processing measurement set ") + filename);
		
		bool skip = false;
		if(_skipIfAlreadyProcessed)
		{
			MeasurementSet set(filename);
			if(set.HasAOFlaggerHistory())
			{
				skip = true;
				Logger::Info << "Skipping " << filename << ",\n"
					"because the set contains AOFlagger history and -skip-flagged was given.\n";
			}
		}
		
		if(!skip)
		{
			std::unique_ptr<ImageSet> imageSet(ImageSet::Create(std::vector<std::string>{filename}, _baselineIOMode));
			bool isMS = dynamic_cast<MSImageSet*>(&*imageSet) != 0;
			if(isMS)
			{ 
				MSImageSet* msImageSet = static_cast<MSImageSet*>(imageSet.get());
				msImageSet->SetDataColumnName(_dataColumnName);
				msImageSet->SetSubtractModel(_subtractModel);
				msImageSet->SetReadUVW(_readUVW);
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

			if(isMS)
				writeHistory(*i);
		}
	
		progress.OnEndTask(*this);

		
		++taskIndex;
	}

	InitializeAll();
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
		MeasurementSet ms(filename);
		const Strategy *strategy = 0;
		if(GetChildCount() == 1 && dynamic_cast<const Strategy*>(&GetChild(0)) != 0)
		{
			strategy = static_cast<const Strategy*>(&GetChild(0));
		} else {
			const ActionContainer *root = GetRoot();
			if(dynamic_cast<const Strategy*>(root) != 0)
				strategy = static_cast<const Strategy*>(root);
		}
		Logger::Debug << "Adding strategy to history table of MS...\n";
		if(strategy != 0) {
			try {
				ms.AddAOFlaggerHistory(*strategy, _commandLineForHistory);
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
