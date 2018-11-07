#include "visualizeaction.h"

#include "../control/artifactset.h"

namespace rfiStrategy {

void VisualizeAction::Perform(ArtifactSet& artifacts, ProgressListener&)
{
	std::ostringstream str;
	str << _label << " (T=" << artifacts.Sensitivity() << ")";
	switch(_source)
	{
		case FromOriginal:
			artifacts.AddVisualization(str.str(), artifacts.OriginalData(), _sortingIndex);
			break;
		case FromRevised:
			artifacts.AddVisualization(str.str(), artifacts.RevisedData(), _sortingIndex);
			break;
		case FromContaminated:
			artifacts.AddVisualization(str.str(), artifacts.ContaminatedData(), _sortingIndex);
			break;
	}
}

}
