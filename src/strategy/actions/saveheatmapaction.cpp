#include "saveheatmapaction.h"

#ifdef HAVE_GTKMM
#include "../../plot/heatmapplot.h"

namespace rfiStrategy {

void SaveHeatMapAction::Perform(rfiStrategy::ArtifactSet& artifacts, ProgressListener&)
{
	const TimeFrequencyData& tfData = artifacts.ContaminatedData();
	std::stringstream filename;
	filename << "HeatMap";
	if(artifacts.HasMetaData())
	{
		TimeFrequencyMetaDataCPtr metaData = artifacts.MetaData();
		if(metaData->HasAntenna1())
			filename << '-' << metaData->Antenna1().name;
		if(metaData->HasAntenna2())
			filename << "x" << metaData->Antenna2().name;
		if(metaData->HasBand())
			filename << '-' << round(metaData->Band().CenterFrequencyHz()*1e-6) << "MHz";
	}
	filename << '-';
	auto pols = tfData.Polarizations();
	for(auto p : pols)
		filename << Polarization::TypeToShortString(p);
	size_t thisIndex;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		size_t& i = _prefixCounts[filename.str()];
		thisIndex = i;
		++i;
	}
	filename << '-' << thisIndex << ".png";
	HeatMapPlot plot;
	plot.SetImage(tfData.GetSingleImage());
	plot.SetAlternativeMask(tfData.GetSingleMask());
	plot.SaveByExtension(filename.str(), 800, 500);
}

}

#else // HAVE_GTKMM

namespace rfiStrategy {

void SaveHeatMapAction::Perform(rfiStrategy::ArtifactSet&, ProgressListener&)
{
	throw std::runtime_error("Not compiled with GTKMM -- graphical drawing is not available for SaveHeatMapAction");
}

}

#endif // HAVE_GTKMM 
