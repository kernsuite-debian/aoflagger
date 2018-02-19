#ifndef STATISTICALFLAGACTION_H
#define STATISTICALFLAGACTION_H

#include "../../structures/types.h"

#include "action.h"

namespace rfiStrategy {

	class StatisticalFlagAction : public Action
	{
		public:
			StatisticalFlagAction()
			: _enlargeTimeSize(0), _enlargeFrequencySize(0),
			_minAvailableTimesRatio(0.0), _minAvailableFrequenciesRatio(0.0),
			_minAvailableTFRatio(0.0),
			_minimumGoodTimeRatio(0.2), _minimumGoodFrequencyRatio(0.2) { }
			virtual ~StatisticalFlagAction() { }
			virtual std::string Description() final override
			{
				return "Statistical flagging";
			}
			virtual void Perform(class ArtifactSet &artifacts, class ProgressListener &listener) final override;
			virtual ActionType Type() const final override { return StatisticalFlagActionType; }

			size_t EnlargeTimeSize() const { return _enlargeTimeSize; }
			void SetEnlargeTimeSize(size_t size) { _enlargeTimeSize = size; }
			size_t EnlargeFrequencySize() const { return _enlargeFrequencySize; }
			void SetEnlargeFrequencySize(size_t size) { _enlargeFrequencySize = size; }
			
			num_t MinAvailableTimesRatio() const { return _minAvailableTimesRatio; }
			void SetMinAvailableTimesRatio(num_t newValue) { _minAvailableTimesRatio = newValue; }
			num_t MinAvailableFrequenciesRatio() const { return _minAvailableFrequenciesRatio; }
			void SetMinAvailableFrequenciesRatio(num_t newValue) { _minAvailableFrequenciesRatio = newValue; }
			num_t MinAvailableTFRatio() const { return _minAvailableTFRatio; }
			void SetMinAvailableTFRatio(num_t newVal) { _minAvailableTFRatio = newVal; }
			
			num_t MinimumGoodTimeRatio() const { return _minimumGoodTimeRatio; }
			void SetMinimumGoodTimeRatio(num_t newValue) { _minimumGoodTimeRatio = newValue; }
			num_t MinimumGoodFrequencyRatio() const { return _minimumGoodFrequencyRatio; }
			void SetMinimumGoodFrequencyRatio(num_t newValue) { _minimumGoodFrequencyRatio = newValue; }
			
		private:
			size_t _enlargeTimeSize;
			size_t _enlargeFrequencySize;
			num_t _minAvailableTimesRatio;
			num_t _minAvailableFrequenciesRatio;
			num_t _minAvailableTFRatio;
			num_t _minimumGoodTimeRatio;
			num_t _minimumGoodFrequencyRatio;
	};

}

#endif // STATISTICALFLAGACTION_H
