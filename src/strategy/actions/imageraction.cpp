#include "../../imaging/uvimager.h"

#include "../actions/imageraction.h"
#include "../algorithms/baselinetimeplaneimager.h"

#include "../../util/progresslistener.h"

#include <mutex>
#include <vector>

namespace rfiStrategy {
	void ImagerAction::Perform(ArtifactSet &artifacts, ProgressListener &progress)
	{
		std::lock_guard<std::mutex> lock(_imagerMutex);
		UVImager& imager = artifacts.Imager();
		if(!artifacts.HasImager())
			throw BadUsageException("No imager available to create image.");
		TimeFrequencyData& data = artifacts.ContaminatedData();
		TimeFrequencyMetaDataCPtr metaData = artifacts.MetaData();
		if(data.PolarizationCount() > 1)
		{
			data = data.Make(Polarization::StokesI);
		}
		
		bool btPlaneImager = true;
		if(btPlaneImager)
		{
			typedef double ImagerNumeric;
			BaselineTimePlaneImager<ImagerNumeric> btImager;
			BandInfo band = metaData->Band();
			Image2DCPtr
				inputReal = data.GetRealPart(),
				inputImag = data.GetImaginaryPart();
			Mask2DCPtr mask = data.GetSingleMask();
			size_t width = inputReal->Width();
			
			for(size_t t=0;t!=width;++t)
			{
				UVW uvw = metaData->UVW()[t];
				size_t channelCount = inputReal->Height();
				std::vector<std::complex<ImagerNumeric> > data(channelCount);
				for(size_t ch=0;ch!=channelCount;++ch) {
					if(mask->Value(t, ch))
						data[ch] = std::complex<ImagerNumeric>(0.0, 0.0);
					else
						data[ch] = std::complex<ImagerNumeric>(inputReal->Value(t, ch), inputImag->Value(t, ch));
				}
				
				btImager.Image(uvw.u, uvw.v, uvw.w, band.channels[0].frequencyHz, band.channels[1].frequencyHz-band.channels[0].frequencyHz, channelCount, &(data[0]), imager.FTReal());
			}
		} else {
			progress.OnStartTask(*this, 0, 1, "Imaging baseline");
			for(size_t y=0;y<data.ImageHeight();++y)
			{
				imager.Image(data, metaData, y);
				progress.OnProgress(*this, y, data.ImageHeight());
			}
			progress.OnEndTask(*this);
		}
	}
}
