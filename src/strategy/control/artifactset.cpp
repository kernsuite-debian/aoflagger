#include "artifactset.h"

#include "../algorithms/baselineselector.h"
#include "../algorithms/polarizationstatistics.h"

#include "../imagesets/imageset.h"

#include "../plots/antennaflagcountplot.h"
#include "../plots/frequencyflagcountplot.h"
#include "../plots/frequencypowerplot.h"
#include "../plots/iterationsplot.h"
#include "../plots/timeflagcountplot.h"

#include "../../imaging/model.h"
#include "../../imaging/observatorium.h"

namespace rfiStrategy {

ArtifactSet::~ArtifactSet()
{ }

ArtifactSet::Data::Data()
{ }

ArtifactSet::Data::~Data()
{ }

void ArtifactSet::SetImageSet(std::unique_ptr<class ImageSet> imageSet)
{
	_imageSet = std::move(imageSet);
}

void ArtifactSet::SetNoImageSet()
{ 
	_imageSet.reset();
	_imageSetIndex.reset(); 
}

void ArtifactSet::SetImageSetIndex(std::unique_ptr<class ImageSetIndex> imageSetIndex)
{
	_imageSetIndex = std::move(imageSetIndex);
}

void ArtifactSet::SetImager(class UVImager* imager)
{
	_imager = imager; 
}

void ArtifactSet::SetAntennaFlagCountPlot(std::unique_ptr<class AntennaFlagCountPlot> plot)
{
	_data->_antennaFlagCountPlot = std::move(plot);
}

void ArtifactSet::SetFrequencyFlagCountPlot(std::unique_ptr<class FrequencyFlagCountPlot> plot)
{
	_data->_frequencyFlagCountPlot = std::move(plot);
}

void ArtifactSet::SetFrequencyPowerPlot(std::unique_ptr<class FrequencyPowerPlot> plot)
{
	_data->_frequencyPowerPlot = std::move(plot);
}

void ArtifactSet::SetTimeFlagCountPlot(std::unique_ptr<class TimeFlagCountPlot> plot)
{
	_data->_timeFlagCountPlot = std::move(plot);
}

void ArtifactSet::SetPolarizationStatistics(std::unique_ptr<class PolarizationStatistics> statistics)
{
	_data->_polarizationStatistics = std::move(statistics);
}

void ArtifactSet::SetIterationsPlot(std::unique_ptr<class IterationsPlot> iterationsPlot)
{
	_data->_iterationsPlot = std::move(iterationsPlot);
}

void ArtifactSet::SetBaselineSelectionInfo(std::unique_ptr<class BaselineSelector> baselineSelectionInfo)
{
	_data->_baselineSelectionInfo = std::move(baselineSelectionInfo);
}

void ArtifactSet::SetObservatorium(std::unique_ptr<class Observatorium> observatorium)
{
	_data->_observatorium = std::move(observatorium);
}

void ArtifactSet::SetModel(std::unique_ptr<class Model> model)
{
	_data->_model = std::move(model);
}

}
