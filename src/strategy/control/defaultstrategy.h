#ifndef DEFAULTSTRATEGYSET_H
#define DEFAULTSTRATEGYSET_H

#include <string>
#include <memory>

#include "../actions/action.h"

#include "../../interface/aoflagger.h"

class MSMetaData;

namespace rfiStrategy {
	
	class Strategy;
	class ActionBlock;
	class ImageSet;
	
	class DefaultStrategy
	{
		public:
		/**
		* The contents of this enum needs to be equal to aoflagger::StrategyId
		* defined in interfaces/aoflagger.h
		*/
		enum TelescopeId {
			GENERIC_TELESCOPE = aoflagger::GENERIC_TELESCOPE,
			AARTFAAC_TELESCOPE = aoflagger::AARTFAAC_TELESCOPE,
			ARECIBO_TELESCOPE = aoflagger::ARECIBO_TELESCOPE,
			BIGHORNS_TELESCOPE = aoflagger::BIGHORNS_TELESCOPE,
			JVLA_TELESCOPE = aoflagger::JVLA_TELESCOPE,
			LOFAR_TELESCOPE = aoflagger::LOFAR_TELESCOPE,
			MWA_TELESCOPE = aoflagger::MWA_TELESCOPE,
			PARKES_TELESCOPE = aoflagger::PARKES_TELESCOPE,
			WSRT_TELESCOPE = aoflagger::WSRT_TELESCOPE
		};

		/**
		* These flags need to be equal to aoflagger::StrategyFlags
		* defined in interfaces/aoflagger.h
		*/
		static const unsigned
			FLAG_NONE,
			FLAG_LARGE_BANDWIDTH,
			FLAG_SMALL_BANDWIDTH,
			FLAG_TRANSIENTS,
			FLAG_ROBUST,
			FLAG_FAST,
			FLAG_INSENSITIVE,
			FLAG_SENSITIVE,
			FLAG_USE_ORIGINAL_FLAGS,
			FLAG_AUTO_CORRELATION,
			FLAG_HIGH_TIME_RESOLUTION;
				
		struct StrategySetup {
			int iterationCount;
			double sumThresholdSensitivity;
			double verticalSmoothing;
			bool keepTransients;
			bool changeResVertically;
			bool calPassband;
			bool channelSelection;
			bool useOriginalFlags;
			bool onStokesIQ;
			bool includeStatistics;
			bool hasBaselines;
			bool highTimeResolution;
		};
		
		static void LoadFullStrategy(ActionBlock &destination, const StrategySetup& setup);
		
		static void EncapsulateSingleStrategy(ActionBlock& destination, std::unique_ptr<ActionBlock> singleStrategy, const StrategySetup& setup);
		
		static StrategySetup DetermineSetup(enum TelescopeId telescopeId, unsigned flags, double frequency, double timeRes, double frequencyRes);
		
		static void LoadSingleStrategy(ActionBlock &destination, const StrategySetup& setup);

		static std::string TelescopeName(DefaultStrategy::TelescopeId telescopeId);
		
		static DefaultStrategy::TelescopeId TelescopeIdFromName(const std::string &name);
		
		static void DetermineSettings(MSMetaData& msMetaData, enum TelescopeId &telescopeId, unsigned &flags, double &frequency, double &timeRes, double &frequencyRes);
		
		static void DetermineSettings(ImageSet& ms, enum TelescopeId &telescopeId, unsigned &flags, double &frequency, double &timeRes, double &frequencyRes);
		
		static bool StrategyContainsAction(Strategy& strategy, ActionType actionType);
		
		static std::vector<Action*> FindActions(ActionBlock& strategy, ActionType actionType);
		
	private:
		static void warnIfUnknownTelescope(enum TelescopeId &telescopeId, const std::string &telescopeName);
		
		static void encapsulatePostOperations(ActionBlock& destination, class ForEachBaselineAction* feBaseBlock, const StrategySetup& setup);
	};


}

#endif
