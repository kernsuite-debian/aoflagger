#include "defaultstrategy.h"
#include "strategyiterator.h"

#include "../actions/baselineselectionaction.h"
#include "../actions/calibratepassbandaction.h"
#include "../actions/changeresolutionaction.h"
#include "../actions/combineflagresultsaction.h"
#include "../actions/foreachbaselineaction.h"
#include "../actions/foreachcomplexcomponentaction.h"
#include "../actions/foreachmsaction.h"
#include "../actions/foreachpolarisationaction.h"
#include "../actions/frequencyselectionaction.h"
#include "../actions/highpassfilteraction.h"
#include "../actions/iterationaction.h"
#include "../actions/plotaction.h"
#include "../actions/setflaggingaction.h"
#include "../actions/setimageaction.h"
#include "../actions/slidingwindowfitaction.h"
#include "../actions/morphologicalflagaction.h"
#include "../actions/strategy.h"
#include "../actions/sumthresholdaction.h"
#include "../actions/timeselectionaction.h"
#include "../actions/visualizeaction.h"
#include "../actions/writeflagsaction.h"

#include "../imagesets/bhfitsimageset.h"
#include "../imagesets/filterbankset.h"
#include "../imagesets/fitsimageset.h"
#include "../imagesets/imageset.h"
#include "../imagesets/msimageset.h"

#include "../../structures/measurementset.h"

#include <boost/algorithm/string/case_conv.hpp>

namespace rfiStrategy {

	const unsigned
		DefaultStrategy::FLAG_NONE                = aoflagger::StrategyFlags::NONE,
		DefaultStrategy::FLAG_LARGE_BANDWIDTH     = aoflagger::StrategyFlags::LARGE_BANDWIDTH,
		DefaultStrategy::FLAG_SMALL_BANDWIDTH     = aoflagger::StrategyFlags::SMALL_BANDWIDTH,
		DefaultStrategy::FLAG_TRANSIENTS          = aoflagger::StrategyFlags::TRANSIENTS,
		DefaultStrategy::FLAG_ROBUST              = aoflagger::StrategyFlags::ROBUST,
		DefaultStrategy::FLAG_FAST                = aoflagger::StrategyFlags::FAST,
		DefaultStrategy::FLAG_INSENSITIVE         = aoflagger::StrategyFlags::INSENSITIVE,
		DefaultStrategy::FLAG_SENSITIVE           = aoflagger::StrategyFlags::SENSITIVE,
		DefaultStrategy::FLAG_USE_ORIGINAL_FLAGS  = aoflagger::StrategyFlags::USE_ORIGINAL_FLAGS,
		DefaultStrategy::FLAG_AUTO_CORRELATION    = aoflagger::StrategyFlags::AUTO_CORRELATION,
		DefaultStrategy::FLAG_HIGH_TIME_RESOLUTION= aoflagger::StrategyFlags::HIGH_TIME_RESOLUTION;
	
	std::string DefaultStrategy::TelescopeName(DefaultStrategy::TelescopeId telescopeId)
	{
		switch(telescopeId)
		{
			default:
			case GENERIC_TELESCOPE: return "Generic";
			case AARTFAAC_TELESCOPE: return "Aartfaac";
			case ARECIBO_TELESCOPE: return "Arecibo";
			case BIGHORNS_TELESCOPE: return "Bighorns";
			case JVLA_TELESCOPE: return "JVLA";
			case LOFAR_TELESCOPE: return "LOFAR";
			case MWA_TELESCOPE: return "MWA";
			case PARKES_TELESCOPE: return "Parkes";
			case WSRT_TELESCOPE: return "WSRT";
		}
	}
		
	DefaultStrategy::TelescopeId DefaultStrategy::TelescopeIdFromName(const std::string &name)
	{
		const std::string nameUpper = boost::algorithm::to_upper_copy(name);
		if(nameUpper == "AARTFAAC")
			return AARTFAAC_TELESCOPE;
		else if(nameUpper == "ARECIBO" || nameUpper == "ARECIBO 305M")
			return ARECIBO_TELESCOPE;
		else if(nameUpper == "BIGHORNS")
			return BIGHORNS_TELESCOPE;
		else if(nameUpper == "EVLA" || nameUpper == "JVLA")
			return JVLA_TELESCOPE;
		else if(nameUpper == "LOFAR")
			return LOFAR_TELESCOPE;
		else if(nameUpper == "MWA")
			return MWA_TELESCOPE;
		else if(nameUpper == "PKS" || nameUpper == "ATPKSMB")
			return PARKES_TELESCOPE;
		else if(nameUpper == "WSRT")
			return WSRT_TELESCOPE;
		else
			return GENERIC_TELESCOPE;
	}
	
	DefaultStrategy::StrategySetup DefaultStrategy::DetermineSetup(enum TelescopeId telescopeId, unsigned flags, double frequency, double timeRes, double frequencyRes)
	{
		StrategySetup setup;
		setup.calPassband =
			// Default MWA observations have strong frequency dependency
			(telescopeId==MWA_TELESCOPE && ((flags&FLAG_SMALL_BANDWIDTH) == 0)) ||
			// JVLA observation I saw (around 1100 MHz) have steep band edges
			(telescopeId==JVLA_TELESCOPE && ((flags&FLAG_SMALL_BANDWIDTH) == 0)) ||
			// Bighorns doesn't correct passband
			(telescopeId==BIGHORNS_TELESCOPE && ((flags&FLAG_SMALL_BANDWIDTH) == 0)) ||
			// Other cases with large bandwidth
			((flags&FLAG_LARGE_BANDWIDTH) != 0);
		setup.keepTransients = (flags&FLAG_TRANSIENTS) != 0;
		// Don't remove edges because of channel selection
		setup.channelSelection = (telescopeId != JVLA_TELESCOPE);
		setup.changeResVertically = true;
		setup.highTimeResolution = (timeRes <= 1e-3 && timeRes != 0.0) || ((flags&FLAG_HIGH_TIME_RESOLUTION)!=0);
		// WSRT has automatic gain control, which strongly affect autocorrelations
		if(((flags&FLAG_AUTO_CORRELATION) != 0) && telescopeId == WSRT_TELESCOPE)
		{
			setup.changeResVertically = false;
			setup.keepTransients = true;
		}
		// JVLA observations I saw (around 1100 MHz) have steep band edges, so smooth very little
		if(telescopeId == JVLA_TELESCOPE)
		{
			setup.changeResVertically = false;
		}
		setup.useOriginalFlags =
			((flags&FLAG_USE_ORIGINAL_FLAGS) != 0);
		setup.iterationCount = ((flags&FLAG_ROBUST)==0) ? 2 : 4;
		if(telescopeId == BIGHORNS_TELESCOPE)
			setup.iterationCount *= 2;
		setup.sumThresholdSensitivity = 1.0;
		if(telescopeId == PARKES_TELESCOPE || telescopeId == WSRT_TELESCOPE)
			setup.sumThresholdSensitivity = 1.4;
		else if(telescopeId == ARECIBO_TELESCOPE || telescopeId == BIGHORNS_TELESCOPE)
			setup.sumThresholdSensitivity = 1.2;
		if((flags&FLAG_AUTO_CORRELATION) != 0)
			setup.sumThresholdSensitivity *= 1.4;
		if((flags&FLAG_SENSITIVE) != 0)
			setup.sumThresholdSensitivity /= 1.2;
		if((flags&FLAG_INSENSITIVE) != 0)
			setup.sumThresholdSensitivity *= 1.2;
		setup.onStokesIQ = ((flags&FLAG_FAST) != 0);
		setup.includeStatistics =
			!(telescopeId==MWA_TELESCOPE || telescopeId==AARTFAAC_TELESCOPE);
		
		setup.verticalSmoothing = 5.0;
		if(telescopeId == JVLA_TELESCOPE)
			setup.verticalSmoothing = 1.0;
		
		setup.hasBaselines = telescopeId!=PARKES_TELESCOPE && telescopeId!=ARECIBO_TELESCOPE && telescopeId!=BIGHORNS_TELESCOPE && telescopeId!=GENERIC_TELESCOPE;
		return setup;
	}
	
	void DefaultStrategy::LoadSingleStrategy(ActionBlock &block, const StrategySetup& setup)
	{
		ActionBlock *current, *scratch;

		if(!setup.useOriginalFlags)
			block.Add(std::unique_ptr<Action>(new SetFlaggingAction()));
		
		current = &block;
		
		if(setup.highTimeResolution)
		{
			std::unique_ptr<ChangeResolutionAction> changeResAction(new ChangeResolutionAction());
			changeResAction->SetTimeDecreaseFactor(10);
			changeResAction->SetFrequencyDecreaseFactor(1);
			changeResAction->SetRestoreRevised(true);
			changeResAction->SetRestoreContaminated(false);
			changeResAction->SetRestoreMasks(true);
			scratch = changeResAction.get();
			current->Add(std::move(changeResAction));
			current = scratch;
		}

		std::unique_ptr<ForEachPolarisationBlock> fepBlock(new ForEachPolarisationBlock());
		if(setup.onStokesIQ)
		{
			fepBlock->SetOnPP(false);
			fepBlock->SetOnPQ(false);
			fepBlock->SetOnQP(false);
			fepBlock->SetOnQQ(false);
			fepBlock->SetOnStokesI(true);
			fepBlock->SetOnStokesQ(true);
		}
		scratch = fepBlock.get();
		current->Add(std::move(fepBlock));
		current = scratch;

		std::unique_ptr<ForEachComplexComponentAction> focAction(new ForEachComplexComponentAction());
		focAction->SetOnAmplitude(true);
		focAction->SetOnImaginary(false);
		focAction->SetOnReal(false);
		focAction->SetOnPhase(false);
		focAction->SetRestoreFromAmplitude(false);
		ForEachComplexComponentAction* focActionPtr = focAction.get();
		current->Add(std::move(focAction));
		current = focActionPtr;

		std::unique_ptr<IterationBlock> iteration(new IterationBlock());
		iteration->SetIterationCount(setup.iterationCount);
		iteration->SetSensitivityStart(2.0 * pow(2.0, setup.iterationCount/2.0));
		IterationBlock* iterationRoot = iteration.get();
		current->Add(std::move(iteration));
		
		std::unique_ptr<SumThresholdAction> t1(new SumThresholdAction());
		t1->SetTimeDirectionSensitivity(setup.sumThresholdSensitivity);
		t1->SetFrequencyDirectionSensitivity(setup.sumThresholdSensitivity);
		if(setup.keepTransients)
			t1->SetFrequencyDirectionFlagging(false);
		t1->SetExcludeOriginalFlags(setup.useOriginalFlags);
		iterationRoot->Add(std::move(t1));

		std::unique_ptr<CombineFlagResults> cfr1(new CombineFlagResults());
		if(setup.channelSelection)
			cfr1->Add(std::unique_ptr<FrequencySelectionAction>(new FrequencySelectionAction()));
		if(!setup.keepTransients)
			cfr1->Add(std::unique_ptr<TimeSelectionAction>(new TimeSelectionAction()));
		iterationRoot->Add(std::move(cfr1));
	
		iterationRoot->Add(std::unique_ptr<SetImageAction>(new SetImageAction()));
		if(setup.useOriginalFlags)
		{
			std::unique_ptr<SetFlaggingAction> orBeforeFilter(new SetFlaggingAction());
			orBeforeFilter->SetNewFlagging(SetFlaggingAction::OrOriginal);
			iterationRoot->Add(std::move(orBeforeFilter));
		}
		if(!setup.keepTransients || setup.changeResVertically)
		{
			std::unique_ptr<ChangeResolutionAction> changeResAction(new ChangeResolutionAction());
			if(setup.keepTransients)
				changeResAction->SetTimeDecreaseFactor(1);
			else
				changeResAction->SetTimeDecreaseFactor(3);
			if(setup.changeResVertically)
				changeResAction->SetFrequencyDecreaseFactor(3);
			else
				changeResAction->SetFrequencyDecreaseFactor(1);
			scratch = changeResAction.get();
			iterationRoot->Add(std::move(changeResAction));
			current = scratch;
		}
		else {
			current = iterationRoot;
		}

		std::unique_ptr<HighPassFilterAction> hpAction(new HighPassFilterAction());
		if(setup.keepTransients)
		{
			hpAction->SetWindowWidth(1);
		} else {
			hpAction->SetHKernelSigmaSq(2.5);
			hpAction->SetWindowWidth(21);
		}
		hpAction->SetVKernelSigmaSq(setup.verticalSmoothing);
		hpAction->SetWindowHeight(31);
		if(!setup.keepTransients || setup.changeResVertically)
			hpAction->SetMode(HighPassFilterAction::StoreRevised);
		else
			hpAction->SetMode(HighPassFilterAction::StoreContaminated);
		current->Add(std::move(hpAction));
		
		std::unique_ptr<VisualizeAction> visAction(new VisualizeAction());
		visAction->SetLabel("Iteration fit");
		visAction->SetSource(VisualizeAction::FromRevised);
		visAction->SetSortingIndex(0);
		iterationRoot->Add(std::move(visAction));
		
		visAction.reset(new VisualizeAction());
		visAction->SetLabel("Iteration residual");
		visAction->SetSource(VisualizeAction::FromContaminated);
		visAction->SetSortingIndex(1);
		iterationRoot->Add(std::move(visAction));
		//
		// End of strategy loop
		//
		current = focActionPtr;
		
		if(setup.calPassband)
			current->Add(std::unique_ptr<CalibratePassbandAction>(new CalibratePassbandAction()));
		
		std::unique_ptr<SumThresholdAction> t2(new SumThresholdAction());
		t2->SetTimeDirectionSensitivity(setup.sumThresholdSensitivity);
		t2->SetFrequencyDirectionSensitivity(setup.sumThresholdSensitivity);
		if(setup.keepTransients)
			t2->SetFrequencyDirectionFlagging(false);
		t2->SetExcludeOriginalFlags(setup.useOriginalFlags);
		current->Add(std::move(t2));
		
		visAction.reset(new VisualizeAction());
		visAction->SetLabel("Iteration residual");
		current->Add(std::move(visAction));
		
		if(setup.includeStatistics)
		{
			std::unique_ptr<PlotAction> plotPolarizationStatistics(new PlotAction());
			plotPolarizationStatistics->SetPlotKind(PlotAction::PolarizationStatisticsPlot);
			block.Add(std::move(plotPolarizationStatistics));
		}
		
		std::unique_ptr<SetFlaggingAction> setFlagsInAllPolarizations(new SetFlaggingAction());
		setFlagsInAllPolarizations->SetNewFlagging(SetFlaggingAction::PolarisationsEqual);
		block.Add(std::move(setFlagsInAllPolarizations));
		
		std::unique_ptr<MorphologicalFlagAction> morphAction(new MorphologicalFlagAction());
		morphAction->SetExcludeOriginalFlags(setup.useOriginalFlags);
		block.Add(std::move(morphAction));

		bool pedantic = false;
		if(pedantic)
		{
			std::unique_ptr<CombineFlagResults> cfr2(new CombineFlagResults());
			block.Add(std::move(cfr2));
			cfr2->Add(std::unique_ptr<FrequencySelectionAction>(new FrequencySelectionAction()));
			if(!setup.keepTransients) {
				std::unique_ptr<TimeSelectionAction> tsAction(new TimeSelectionAction());
				tsAction->SetThreshold(4.0);
				cfr2->Add(std::move(tsAction));
			}
		} else {
			if(!setup.keepTransients) {
				std::unique_ptr<TimeSelectionAction> tsAction(new TimeSelectionAction());
				tsAction->SetThreshold(4.0);
				block.Add(std::move(tsAction));
			}
		}

		if(setup.includeStatistics && setup.hasBaselines)
		{
			std::unique_ptr<BaselineSelectionAction> baselineSelection(new BaselineSelectionAction());
			baselineSelection->SetPreparationStep(true);
			block.Add(std::move(baselineSelection));
		}

		if(setup.useOriginalFlags)
		{
			std::unique_ptr<SetFlaggingAction> orWithOriginals(new SetFlaggingAction());
			orWithOriginals->SetNewFlagging(SetFlaggingAction::OrOriginal);
			block.Add(std::move(orWithOriginals));
		}
	}

	void DefaultStrategy::LoadFullStrategy(ActionBlock &destination, const StrategySetup& setup)
	{
		std::unique_ptr<ForEachBaselineAction> feBaseBlock(new ForEachBaselineAction());
		ForEachBaselineAction* feBaseBlockPtr = feBaseBlock.get();
		destination.Add(std::move(feBaseBlock));
		
		LoadSingleStrategy(*feBaseBlockPtr, setup);

		encapsulatePostOperations(destination, feBaseBlockPtr, setup);
	}
	
	void DefaultStrategy::EncapsulateSingleStrategy(ActionBlock& destination, std::unique_ptr<ActionBlock> singleStrategy, const StrategySetup& setup)
	{
		std::unique_ptr<ForEachBaselineAction> feBaseBlock(new ForEachBaselineAction());
		ForEachBaselineAction* feBaseBlockPtr = feBaseBlock.get();
		destination.Add(std::move(feBaseBlock));

		feBaseBlockPtr->Add(std::move(singleStrategy));
		
		encapsulatePostOperations(destination, feBaseBlockPtr, setup);
	}

	void DefaultStrategy::encapsulatePostOperations(ActionBlock& destination, ForEachBaselineAction* feBaseBlock, const StrategySetup& setup)
	{
		feBaseBlock->Add(std::unique_ptr<WriteFlagsAction>(new WriteFlagsAction()));

		if(setup.includeStatistics && setup.hasBaselines)
		{
			std::unique_ptr<PlotAction> antennaPlotAction(new PlotAction());
			antennaPlotAction->SetPlotKind(PlotAction::AntennaFlagCountPlot);
			feBaseBlock->Add(std::move(antennaPlotAction));
		}

		if(setup.includeStatistics)
		{
			std::unique_ptr<PlotAction> frequencyPlotAction(new PlotAction());
			frequencyPlotAction->SetPlotKind(PlotAction::FrequencyFlagCountPlot);
			feBaseBlock->Add(std::move(frequencyPlotAction));
		}

		if(setup.includeStatistics && setup.hasBaselines)
		{
			std::unique_ptr<BaselineSelectionAction> baselineSelection(new BaselineSelectionAction());
			baselineSelection->SetPreparationStep(false);
			destination.Add(std::move(baselineSelection));
		}
	}

	void DefaultStrategy::warnIfUnknownTelescope(DefaultStrategy::TelescopeId& telescopeId, const string& telescopeName)
	{
		if(telescopeId == GENERIC_TELESCOPE)
		{
			Logger::Warn << 
				"**\n"
				"** Measurement set specified the following telescope name: '" << telescopeName << "'\n"
				"** No good strategy is known for this telescope!\n"
				"** A generic strategy will be used which might not be optimal.\n"
				"**\n";
		}
	}
	
	void DefaultStrategy::DetermineSettings(MeasurementSet& measurementSet, DefaultStrategy::TelescopeId& telescopeId, unsigned int& flags, double& frequency, double& timeRes, double& frequencyRes)
	{
		Logger::Debug << "Determining best known strategy for measurement set...\n";
		
		std::string telescopeName = measurementSet.TelescopeName();
		telescopeId = TelescopeIdFromName(telescopeName);
		warnIfUnknownTelescope(telescopeId, telescopeName);
		
		flags = 0;
		size_t bandCount = measurementSet.BandCount();
		double frequencySum = 0.0, freqResSum = 0.0;
		size_t resSumCount = 0;
		for(size_t bandIndex=0; bandIndex!=bandCount; ++bandIndex)
		{
			const BandInfo &band = measurementSet.GetBandInfo(bandIndex);
			frequencySum += band.CenterFrequencyHz();
			if(band.channels.size() > 1)
			{
				const double
					startFrequency = band.channels.begin()->frequencyHz,
					endFrequency = band.channels.rbegin()->frequencyHz;
				freqResSum += fabs((endFrequency - startFrequency) / (band.channels.size() - 1));
				++resSumCount;
			}
		}
		if(bandCount != 0)
			frequency = frequencySum / (double) bandCount;
		else
			frequency = 0.0;
		
		if(resSumCount != 0)
			frequencyRes = freqResSum / (double) resSumCount;
		else
			frequencyRes = 0.0;
		
		const std::set<double> &obsTimes = measurementSet.GetObservationTimesSet();
		if(obsTimes.size() > 1)
		{
			double
				startTime = *obsTimes.begin(),
				endTime = *obsTimes.rbegin();
			timeRes = (endTime - startTime) / (double) (obsTimes.size() - 1);
		}
		else
			timeRes = 0.0;
		
		Logger::Info <<
			"The strategy will be optimized for the following settings:\n"
			"Telescope=" << TelescopeName(telescopeId) << ", flags=NONE, frequency="
			<< Frequency::ToString(frequency) << ",\n"
			"time resolution=" << timeRes << " s, frequency resolution=" << Frequency::ToString(frequencyRes) << '\n';
	}
	
	void DefaultStrategy::DetermineSettings(ImageSet& imageSet, DefaultStrategy::TelescopeId& telescopeId, unsigned int& flags, double& frequency, double& timeRes, double& frequencyRes)
	{
		IndexableSet *indexableSet = dynamic_cast<IndexableSet*>(&imageSet);
		FitsImageSet *fitsImageSet = dynamic_cast<FitsImageSet*>(&imageSet);
		BHFitsImageSet *bhFitsImageSet = dynamic_cast<BHFitsImageSet*>(&imageSet);
		FilterBankSet *fbImageSet = dynamic_cast<FilterBankSet*>(&imageSet);

		if(indexableSet != 0)
		{
			DetermineSettings(
				indexableSet->Reader()->Set(),
				telescopeId,
				flags,
				frequency,
				timeRes,
				frequencyRes
			);
		}
		else if(fitsImageSet != 0) {
		  std::string telescopeName = fitsImageSet->ReadTelescopeName();
		  telescopeId = TelescopeIdFromName(telescopeName);
		  warnIfUnknownTelescope(telescopeId, telescopeName);
		  if(telescopeId != GENERIC_TELESCOPE)
		    Logger::Info <<
		      "The strategy will be optimized for telescope " << TelescopeName(telescopeId) << ". Telescope-specific\n"
		      "settings will be left to their defaults, which might not be optimal for all cases.\n";
		  flags = 0;
		  frequency = 0.0;
		  timeRes = 0.0;
		  frequencyRes = 0.0;
		}
		else if(bhFitsImageSet != 0) {
		  std::string telescopeName = bhFitsImageSet->GetTelescopeName();
		  telescopeId = TelescopeIdFromName(telescopeName);
		  warnIfUnknownTelescope(telescopeId, telescopeName);
		  if(telescopeId != GENERIC_TELESCOPE)
		    Logger::Info <<
		      "The strategy will be optimized for telescope " << TelescopeName(telescopeId) << ". Telescope-specific\n"
		      "settings will be left to their defaults, which might not be optimal for all cases.\n";
		  flags = 0;
		  frequency = 0.0;
		  timeRes = 0.0;
		  frequencyRes = 0.0;
		} else if(fbImageSet != 0) {
		  telescopeId = GENERIC_TELESCOPE;
			flags = 0;
			frequency = fbImageSet->CentreFrequency();
			timeRes = fbImageSet->TimeResolution();
			frequencyRes = fbImageSet->ChannelWidth();
			Logger::Info <<
				"The strategy will be optimized for the following settings specified by the FilterBankSet:\n"
				"Telescope=" << TelescopeName(telescopeId) << ", flags=NONE, frequency="
				<< Frequency::ToString(frequency) << ",\n"
				"time resolution=" << timeRes*1e6 << " µs, frequency resolution=" << Frequency::ToString(frequencyRes) << '\n';
			Logger::Warn <<
				"** Determined some settings from FilterBankSet, but telescope name cannot be determined.\n";
		} else {
		  telescopeId = GENERIC_TELESCOPE;
		  flags = 0;
		  frequency = 0.0;
		  timeRes = 0.0;
		  frequencyRes = 0.0;
		  Logger::Warn <<
		    "** Could not determine telescope name from set, because it has not\n"
		    "** been implemented for this file format. A generic strategy will be used!\n";
		}
	}

	bool DefaultStrategy::StrategyContainsAction(Strategy& strategy, ActionType actionType)
	{
		StrategyIterator iterator = StrategyIterator::NewStartIterator(strategy);
		while(!iterator.PastEnd())
		{
			const Action& action = *iterator;
			if(action.Type() == actionType)
				return true;
			++iterator;
		}
		return false;
	}
	
	std::vector<Action*> DefaultStrategy::FindActions(ActionBlock& strategy, ActionType actionType)
	{
		std::vector<Action*> foundActions;
		StrategyIterator iterator = StrategyIterator::NewStartIterator(strategy);
		while(!iterator.PastEnd())
		{
			const Action& action = *iterator;
			if(action.Type() == actionType)
				foundActions.push_back(&*iterator);
			++iterator;
		}
		return foundActions;
	}
}

