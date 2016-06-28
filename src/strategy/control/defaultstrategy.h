#ifndef DEFAULTSTRATEGYSET_H
#define DEFAULTSTRATEGYSET_H

#include <string>

#include "../actions/action.h"

#include "../../interface/aoflagger.h"

class MeasurementSet;

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
			FLAG_LOW_FREQUENCY,
			FLAG_HIGH_FREQUENCY,
			FLAG_LARGE_BANDWIDTH,
			FLAG_SMALL_BANDWIDTH,
			FLAG_TRANSIENTS,
			FLAG_ROBUST,
			FLAG_FAST,
			FLAG_OFF_AXIS_SOURCES,
			FLAG_UNSENSITIVE,
			FLAG_SENSITIVE,
			FLAG_GUI_FRIENDLY,
			FLAG_CLEAR_FLAGS,
			FLAG_AUTO_CORRELATION,
			FLAG_HIGH_TIME_RESOLUTION;
				
		/** @TODO Not all flags are implemented yet. */
		static Strategy *CreateStrategy(enum TelescopeId telescopeId, unsigned flags, double frequency=0.0, double timeRes=0.0, double frequencyRes=0.0);
		
		static void LoadStrategy(ActionBlock &strategy, enum TelescopeId telescopeId, unsigned flags, double frequency=0.0, double timeRes=0.0, double frequencyRes=0.0);
		
		static void LoadFullStrategy(ActionBlock &destination, enum TelescopeId telescopeId, unsigned flags, double frequency=0.0, double timeRes=0.0, double frequencyRes=0.0);
		
		static void EncapsulateSingleStrategy(ActionBlock& destination, ActionBlock* singleStrategy, enum TelescopeId telescopeId);
		
		static void LoadSingleStrategy(ActionBlock &destination, int iterationCount, bool keepTransients, bool changeResVertically, bool calPassband, bool channelSelection, bool clearFlags, bool resetContaminated, double sumThresholdSensitivity, bool onStokesIQ, bool includePolStatistics, double verticalSmoothing, bool hasBaselines, bool highTimeResolution);

		static std::string TelescopeName(DefaultStrategy::TelescopeId telescopeId);
		
		static DefaultStrategy::TelescopeId TelescopeIdFromName(const std::string &name);
		
		static void DetermineSettings(ImageSet &measurementSet, enum TelescopeId &telescopeId, unsigned &flags, double &frequency, double &timeRes, double &frequencyRes);
		
		static void DetermineSettings(MeasurementSet &measurementSet, enum TelescopeId &telescopeId, unsigned &flags, double &frequency, double &timeRes, double &frequencyRes);
		
		static bool StrategyContainsAction(Strategy& strategy, ActionType actionType);
		
		static std::vector<Action*> FindActions(ActionBlock& strategy, ActionType actionType);
		
	private:
		static void warnIfUnknownTelescope(enum TelescopeId &telescopeId, const std::string &telescopeName);
		
		static void encapsulatePostOperations(ActionBlock& destination, class ForEachBaselineAction* feBaseBlock, enum TelescopeId telescopeId);
	};


}

#endif
