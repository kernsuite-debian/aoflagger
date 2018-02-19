#include "baselineselectionaction.h"

#include <iostream>
#include <mutex>

#include "../../util/plot.h"

#include "../../structures/mask2d.h"
#include "../../msio/baselinereader.h"

#include "strategy.h"

#include "../control/artifactset.h"

#include "../imagesets/msimageset.h"

namespace rfiStrategy {
	
	void BaselineSelectionAction::prepare(class ArtifactSet &artifacts, class ProgressListener &)
	{
		if(artifacts.MetaData() == nullptr)
		{
			return;
		}
		if(!artifacts.MetaData()->HasBaseline())
		{
			return;
		}

		Mask2DCPtr mask = artifacts.ContaminatedData().GetSingleMask();

		BaselineSelector &info = artifacts.BaselineSelectionInfo();
		std::lock_guard<std::mutex> lock(info.Mutex());
		info.Add(mask, artifacts.MetaData());
	}
	
	void BaselineSelectionAction::mark(class ArtifactSet &artifacts, class ProgressListener &)
	{
		BaselineSelector& info = artifacts.BaselineSelectionInfo();
		if(info.BaselineCount() == 0)
		{
			Logger::Warn <<
			"BaselineSelectionAction wrongly used: trying to mark baselines, but baselines have\n"
			"not been prepared previously (you need to add a BaselineSelectionAction within a for\n"
			"each baseline block, that calculates the statistics and prepares selection)\n";
		} else {

			Logger::Debug << "Searching for bad baselines...\n";

			Strategy::SyncAll(*GetRoot());

			std::lock_guard<std::mutex> lock(info.Mutex());

			BaselineSelector& selector = artifacts.BaselineSelectionInfo();
			selector.SetAbsThreshold(_absThreshold);
			selector.SetSmoothingSigma(_smoothingSigma);
			selector.SetThreshold(_threshold);
			
			std::vector<BaselineSelector::SingleBaselineInfo> markedBaselines;
			selector.Search(markedBaselines);
			
			if(markedBaselines.size() > 0)
			{
				Logger::Info << "Found " << markedBaselines.size() << "/" << (markedBaselines.size()+selector.BaselineCount()) << " bad baselines: ";
				
				std::vector<BaselineSelector::SingleBaselineInfo>::const_iterator badBaselineIter = markedBaselines.begin();
				Logger::Info << badBaselineIter->antenna1Name << "x" << badBaselineIter->antenna2Name;
				++badBaselineIter;
				while(badBaselineIter!=markedBaselines.end())
				{
					Logger::Info << ", " << badBaselineIter->antenna1Name << "x" << badBaselineIter->antenna2Name;
					++badBaselineIter;
				}
				Logger::Info << '\n';
			} else {
				Logger::Info << "No bad baselines found.\n";
			}
			
			if(_flagBadBaselines)
			{
				flagBaselines(artifacts, markedBaselines);
			} else {
				if(markedBaselines.size() > 0)
					Logger::Info <<
						"Bad baseline finding is still experimental, please check the results.\n"
						"These baselines have therefore NOT been flagged yet. Writing flags to\n"
						"these baselines can be enabled by setting the flag-bad-baselines\n"
						"property of both BaselineSelectionAction's to '1' in your strategy\n"
						"file.\n";
			}
		}
	}

	void BaselineSelectionAction::flagBaselines(ArtifactSet &artifacts, const std::vector<BaselineSelector::SingleBaselineInfo> &baselines)
	{
		std::lock_guard<std::mutex> lock(artifacts.IOMutex());

		ImageSet& imageSet = artifacts.ImageSet();
		BaselineReaderPtr reader = dynamic_cast<MSImageSet&>(imageSet).Reader();

		size_t scans = reader->Set().GetObservationTimesSet().size();

		for(std::vector<BaselineSelector::SingleBaselineInfo>::const_iterator i=baselines.begin();
			i!=baselines.end();++i)
		{
			size_t frequencyCount = reader->Set().FrequencyCount(i->band);
			Mask2DPtr flaggedMask = Mask2D::CreateSetMaskPtr<true>(scans, frequencyCount);
			std::vector<Mask2DCPtr> masks;
			for(size_t p=0;p<reader->Polarizations().size();++p)
				masks.push_back(flaggedMask);
			reader->AddWriteTask(masks, i->antenna1, i->antenna2, i->band, i->sequenceId);
		}
		reader->PerformFlagWriteRequests();
	}
}
