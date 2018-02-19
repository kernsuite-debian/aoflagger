#ifndef IMAGECOMPARISONCONTROLLER_H
#define IMAGECOMPARISONCONTROLLER_H

#include <gtkmm/drawingarea.h>

#include <vector>

#include "../../structures/image2d.h"
#include "../../structures/timefrequencydata.h"
#include "../../structures/timefrequencymetadata.h"
#include "../../structures/segmentedimage.h"

#include "../../plot/heatmapplot.h"

class ImageComparisonController {
	public:
		enum TFImage { TFOriginalImage, TFRevisedImage, TFContaminatedImage };
		ImageComparisonController();
		void SetNewData(const class TimeFrequencyData &image, TimeFrequencyMetaDataCPtr metaData);

		TimeFrequencyData GetActiveData() const;

		TimeFrequencyData &OriginalData() { return _original; }
		const TimeFrequencyData &OriginalData() const { return _original; }

		TimeFrequencyData &RevisedData() { return _revised; }
		const TimeFrequencyData &RevisedData() const { return _revised; }

		void SetRevisedData(const TimeFrequencyData &data)
		{
			_revised = data;
		  updateVisualizedImage();
		}
		const TimeFrequencyData &ContaminatedData() const { return _contaminated; }
		TimeFrequencyData &ContaminatedData() { return _contaminated; }
		void SetContaminatedData(const TimeFrequencyData &data)
		{
			_contaminated = data;
			_plot.SetAlternativeMask(data.GetSingleMask());
		  updateVisualizedImage();
		} 
		void SetVisualizedImage(TFImage visualizedImage)
		{
			if(_visualizedImage != visualizedImage)
			{
				_visualizedImage = visualizedImage;
				updateVisualizedImage();
			}
		}
		void TryVisualizePolarizations(bool& pp, bool& pq, bool& qp, bool& qq) const;
		void SetVisualizedPolarization(bool pp, bool pq, bool qp, bool qq)
		{
			if(_showPP != pp || _showPQ != pq || _showQP != qp || _showQQ != qq)
			{
				_showPP = pp;
				_showPQ = pq;
				_showQP = qp;
				_showQQ = qq;
				updateVisualizedImage();
			}
		}
		void ClearBackground();
		HeatMapPlot& Plot() { return _plot; }
		const HeatMapPlot& Plot() const { return _plot; }
	private:
		TimeFrequencyData* getSelectedData();
		void getFirstAvailablePolarization(bool& pp, bool& pq, bool& qp, bool& qq) const;
		const TimeFrequencyData* getSelectedData() const;
		void updateVisualizedImage();
		const TimeFrequencyData getActiveDataWithOriginalFlags() const
		{
			switch(_visualizedImage)
			{
				case TFOriginalImage:
				default:
					return _original;
				case TFRevisedImage:
					return _revised;
				case TFContaminatedImage:
					return _contaminated;
			}
		}
		void setActiveMask(TimeFrequencyData& data) const
		{
			bool orActive = _plot.ShowOriginalMask() && _original.MaskCount()!=0;
			bool altActive = _plot.ShowAlternativeMask() && _contaminated.MaskCount()!=0;
			if(orActive && altActive)
			{
				data.SetMask(_original);
				data.JoinMask(_contaminated);
			}
			else if(orActive)
				data.SetMask(_original);
			else if(altActive)
				data.SetMask(_contaminated);
			else
				data.SetMasksToValue<false>();
		}
		HeatMapPlot _plot;
		enum TFImage _visualizedImage;
		bool _showPP, _showPQ, _showQP, _showQQ;
		TimeFrequencyData _original, _revised, _contaminated;
		TimeFrequencyMetaDataCPtr _metaData;
};

#endif
