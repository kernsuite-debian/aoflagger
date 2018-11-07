#ifndef RFISTRATEGYREADER_H
#define RFISTRATEGYREADER_H

#include <memory>
#include <stdexcept>
#include <string>

#include <libxml/parser.h>
#include <libxml/tree.h>

namespace rfiStrategy {

class StrategyReaderError : public std::runtime_error
{
	public:
		explicit StrategyReaderError(const std::string &arg) : std::runtime_error(arg) { }
};

class StrategyReader {
	public:
		StrategyReader();
		~StrategyReader();

		std::unique_ptr<class Strategy> CreateStrategyFromFile(const std::string &filename);
		
	private:
		std::unique_ptr<class Action> parseChild(xmlNode* node);
		std::unique_ptr<class Strategy> parseStrategy(xmlNode* node);
		std::unique_ptr<class Strategy> parseRootChildren(xmlNode* rootNode);
		void parseChildren(xmlNode* node, class ActionContainer& parent);
		std::unique_ptr<Action> parseAction(xmlNode* node);

		xmlNode* getTextNode(xmlNode* node, const char *subNodeName, bool allowEmpty = false) const;
		int getInt(xmlNode* node, const char *name) const;
		int getIntOr(xmlNode* node, const char *name, int alternative) const;
		double getDouble(xmlNode* node, const char *name) const;
		double getDoubleOr(xmlNode* node, const char *name, double alternative) const;
		std::string getString(xmlNode* node, const char *name) const;
		bool getBool(xmlNode* node, const char *name) const { return getInt(node,name) != 0; }
		bool getBoolOr(xmlNode* node, const char *name, bool alternative) const
		{ return getIntOr(node, name, alternative ? 1 : 0) != 0; }

		std::unique_ptr<Action> parseAbsThresholdAction(xmlNode* node);
		std::unique_ptr<Action> parseApplyBandpassAction(xmlNode* node);
		std::unique_ptr<Action> parseBaselineSelectionAction(xmlNode* node);
		std::unique_ptr<Action> parseCalibratePassbandAction(xmlNode* node);
		std::unique_ptr<Action> parseChangeResolutionAction(xmlNode* node);
		std::unique_ptr<Action> parseCombineFlagResults(xmlNode* node);
		std::unique_ptr<Action> parseCutAreaAction(xmlNode* node);
		std::unique_ptr<Action> parseEigenValueVerticalAction(xmlNode* node);
		std::unique_ptr<Action> parseForEachBaselineAction(xmlNode* node);
		std::unique_ptr<Action> parseForEachComplexComponentAction(xmlNode* node);
		std::unique_ptr<Action> parseForEachMSAction(xmlNode* node);
		std::unique_ptr<Action> parseForEachPolarisationBlock(xmlNode* node);
		std::unique_ptr<Action> parseFrequencyConvolutionAction(xmlNode* node);
		std::unique_ptr<Action> parseFrequencySelectionAction(xmlNode* node);
		std::unique_ptr<Action> parseFringeStopAction(xmlNode* node);
		std::unique_ptr<Action> parseHighPassFilterAction(xmlNode* node);
		std::unique_ptr<Action> parseImagerAction(xmlNode* node);
		std::unique_ptr<Action> parseIterationBlock(xmlNode* node);
		std::unique_ptr<Action> parseNormalizeVarianceAction(xmlNode* node);
		std::unique_ptr<Action> parsePlotAction(xmlNode* node);
		std::unique_ptr<Action> parseQuickCalibrateAction(xmlNode* node);
		std::unique_ptr<Action> parseSetFlaggingAction(xmlNode* node);
		std::unique_ptr<Action> parseSetImageAction(xmlNode* node);
		std::unique_ptr<Action> parseSlidingWindowFitAction(xmlNode* node);
		std::unique_ptr<Action> parseMorphologicalFlagAction(xmlNode* node);
		std::unique_ptr<Action> parseSVDAction(xmlNode* node);
		std::unique_ptr<Action> parseSumThresholdAction(xmlNode* node);
		std::unique_ptr<Action> parseTimeConvolutionAction(xmlNode* node);
		std::unique_ptr<Action> parseTimeSelectionAction(xmlNode* node);
		std::unique_ptr<Action> parseVisualizeAction(xmlNode* node);
		std::unique_ptr<Action> parseWriteDataAction(xmlNode* node);
		std::unique_ptr<Action> parseWriteFlagsAction(xmlNode* node);

		xmlDocPtr _xmlDocument;

		static int useCount;
};

}

#endif
