#ifndef RFIACTION_H
#define RFIACTION_H 

#include <string>

#include "../../util/types.h"

namespace rfiStrategy {

	enum ActionType
	{
		AbsThresholdActionType,
		ActionBlockType,
		BaselineSelectionActionType,
		CalibratePassbandActionType,
		ChangeResolutionActionType,
		CombineFlagResultsType,
		CutAreaActionType,
		EigenValueVerticalActionType,
		ForEachBaselineActionType,
		ForEachComplexComponentActionType,
		ForEachMSActionType,
		ForEachPolarisationBlockType,
		ForEachSimulatedBaselineActionType,
		FrequencyConvolutionActionType,
		FrequencySelectionActionType,
		FringeStopActionType,
		HighPassFilterActionType,
		ImagerActionType,
		IterationBlockType,
		NormalizeVarianceActionType,
		PlotActionType,
		QuickCalibrateActionType,
		ResamplingActionType,
		SaveHeatMapActionType,
		SetFlaggingActionType,
		SetImageActionType,
		SlidingWindowFitActionType,
		StatisticalFlagActionType,
		StrategyType,
		SumThresholdActionType,
		SVDActionType,
		TimeConvolutionActionType,
		TimeSelectionActionType,
		VisualizeActionType,
		WriteDataActionType,
		WriteFlagsActionType
	};

	class Action
	{
		friend class ActionContainer;

		public:
			Action() : _parent(0) { }
			virtual ~Action() { }
			virtual std::string Description() = 0;
			virtual void Initialize() { }
			virtual void Finish() { }
			/**
			 * Write any cached / delayed data to disk
			 */
			virtual void Sync() { }
			virtual void Perform(class ArtifactSet &artifacts, class ProgressListener &progress) = 0;
			class ActionContainer *Parent() const { return _parent; }
			virtual ActionType Type() const = 0;

			inline ActionContainer *GetRoot() const;
			virtual unsigned int Weight() const { return 1; }
		private:
			class ActionContainer *_parent;
	};
}

#include "../control/actioncontainer.h"

namespace rfiStrategy {

	ActionContainer *Action::GetRoot() const
	{
		if(_parent == nullptr)
			return _parent;
		else
		{
			ActionContainer *iter = _parent;
			while(iter->_parent != 0) iter = iter->_parent;
			return iter;
		}
	}

}

#endif // RFIACTION_H
