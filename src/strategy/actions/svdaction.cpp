#include "../../util/progresslistener.h"

#include "svdaction.h"

#include "../algorithms/svdmitigater.h"

#include "../control/artifactset.h"

namespace rfiStrategy {

	void SVDAction::Perform(ArtifactSet &artifacts, class ProgressListener &listener)
	{
		SVDMitigater mitigater;
		mitigater.Initialize(artifacts.ContaminatedData());
		mitigater.SetRemoveCount(_singularValueCount);
		mitigater.PerformFit();
		listener.OnProgress(*this, 1, 1);

		TimeFrequencyData newRevisedData = mitigater.Background();
		newRevisedData.SetMask(artifacts.RevisedData());

		TimeFrequencyData contaminatedData =
			TimeFrequencyData::MakeFromDiff(artifacts.ContaminatedData(), newRevisedData);
		contaminatedData.SetMask(artifacts.ContaminatedData());

		artifacts.SetRevisedData(newRevisedData);
		artifacts.SetContaminatedData(contaminatedData);
	}

} // namespace rfiStrategy
