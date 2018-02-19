#ifndef EIGENVALUEVERTICALACTION_H
#define EIGENVALUEVERTICALACTION_H

#include "../../structures/timefrequencydata.h"

#include "action.h"

#include "../algorithms/vertevd.h"

#include "../control/artifactset.h"

namespace rfiStrategy {
	
	class EigenValueVerticalAction : public Action {
		public:
			EigenValueVerticalAction() : _timeIntegrated(true)
			{
			}
			~EigenValueVerticalAction()
			{
			}
			virtual std::string Description() final override
			{
				return "Eigen value decomposition (vertical)";
			}

			virtual void Perform(class ArtifactSet &artifacts, class ProgressListener &) final override
			{
				TimeFrequencyData &data = artifacts.ContaminatedData();
				if(data.PolarizationCount()!=1)
				{
					throw std::runtime_error("Eigen value decompisition requires one polarization");
				}
				VertEVD::Perform(data, _timeIntegrated);
			}

			virtual ActionType Type() const final override { return EigenValueVerticalActionType; }
		private:
			bool _timeIntegrated;
	};

}
	
#endif // EIGENVALUEVERTICALACTION_H
