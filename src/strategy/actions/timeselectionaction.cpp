#include "timeselectionaction.h"

#include <map>

#include "../../structures/timefrequencydata.h"
#include "../../structures/image2d.h"
#include "../../structures/samplerow.h"

#include "../algorithms/medianwindow.h"

#include "../control/artifactset.h"

namespace rfiStrategy {

/**
 * Automatic selection selects all timesteps which RMS is higher than some value relative to the stddev of
 * all timesteps.
 */
void TimeSelectionAction::AutomaticSelection(ArtifactSet &artifacts)
{
	Image2DCPtr image = artifacts.ContaminatedData().GetSingleImage();
	SampleRow timesteps = SampleRow::MakeEmpty(image->Width());
	Mask2DPtr mask(new Mask2D(*artifacts.ContaminatedData().GetSingleMask()));
	for(size_t x=0;x<image->Width();++x)
	{
		SampleRow row = SampleRow::MakeFromColumnWithMissings(image.get(), mask.get(), x);
		timesteps.SetValue(x, row.RMSWithMissings());
	}
	bool change;
	MedianWindow<num_t>::SubtractMedian(timesteps, 512);
	do {
		num_t median = 0.0;
		num_t stddev = timesteps.StdDevWithMissings(0.0);
		change = false;
		for(size_t x=0;x<timesteps.Size();++x)
		{
			if(!timesteps.ValueIsMissing(x) && (timesteps.Value(x) - median > stddev * _threshold || median - timesteps.Value(x) > stddev * _threshold))
			{
				mask->SetAllVertically<true>(x);
				timesteps.SetValueMissing(x);
				change = true;
			}
		}
	} while(change);
	artifacts.ContaminatedData().SetGlobalMask(mask);
}

} // end of namespace
