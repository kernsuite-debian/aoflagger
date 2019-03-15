#ifndef IMAGECOMPARISONCONTROLLER_H
#define IMAGECOMPARISONCONTROLLER_H

#include <gtkmm/drawingarea.h>

#include <utility>
#include <vector>

#include "../../structures/image2d.h"
#include "../../structures/timefrequencydata.h"
#include "../../structures/timefrequencymetadata.h"
#include "../../structures/segmentedimage.h"

#include "../../plot/heatmapplot.h"

class ImageComparisonController {
	public:
		ImageComparisonController();
		void SetNewData(const class TimeFrequencyData &image, TimeFrequencyMetaDataCPtr metaData);

		TimeFrequencyData GetActiveData() const;

		TimeFrequencyData &OriginalData() { return _dataList.front().data; }
		const TimeFrequencyData &OriginalData() const { return _dataList.front().data; }

		size_t AddVisualization(const std::string& label, const TimeFrequencyData& data)
		{
			_dataList.emplace_back(label, data);
			size_t v = _dataList.size() - 1;
			_visualizationListChange.emit();
			return v;
		}
		
		template<typename T>
		void SetVisualizationData(size_t index, T&& data)
		{
			_dataList[index].data = std::forward<T>(data);
			if(index == 0 || _visualizedImage == index ||
				(_visualizedImage==0 && index==_dataList.size()-1) )
				updateVisualizedImageAndMask();
		}
		
		const TimeFrequencyData& VisualizedData() const {
			return _dataList[_visualizedImage].data;
		}
		void SetVisualizedData(const TimeFrequencyData &data)
		{
			_dataList[_visualizedImage].data = data;
			updateVisualizedImageAndMask();
		}
		
		void SetAltMaskData(TimeFrequencyData& data) {
			if(_visualizedImage == 0)
				_dataList.back().data = data;
			else
				_dataList[_visualizedImage].data = data;
			updateVisualizedImageAndMask();
		}
		
		const TimeFrequencyData& AltMaskData() const {
			if(_visualizedImage == 0)
				return _dataList.back().data;
			else
				return _dataList[_visualizedImage].data;
		}
		
		size_t VisualizationCount() const { return _dataList.size(); }
		size_t VisualizedIndex() const { return _visualizedImage; }
		
		const TimeFrequencyData& GetVisualizationData(size_t index) const { return _dataList[index].data; }
		const std::string& GetVisualizationLabel(size_t index) { return _dataList[index].label; }
		
		void SetVisualization(size_t visualizationIndex)
		{
			if(_visualizedImage != visualizationIndex && _visualizedImage < _dataList.size())
			{
				_visualizedImage = visualizationIndex;
				updateVisualizedImageAndMask();
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
				updateVisualizedImageAndMask();
			}
		}
		
		void ClearAllButOriginal();
		
		HeatMapPlot& Plot() { return _plot; }
		const HeatMapPlot& Plot() const { return _plot; }
		
		sigc::signal<void> &VisualizationListChange()
		{ return _visualizationListChange; }
	private:
		void getFirstAvailablePolarization(bool& pp, bool& pq, bool& qp, bool& qq) const;
		void updateVisualizedImageAndMask();
		void updateVisualizedImage();
		Mask2DCPtr getSelectedPolarizationMask(const TimeFrequencyData& data) const;
		void getActiveMask(TimeFrequencyData& data) const
		{
			bool orActive = _plot.ShowOriginalMask() && _dataList[0].data.MaskCount()!=0;
			size_t altMaskIndex = _visualizedImage;
			if(altMaskIndex == 0)
				altMaskIndex = _dataList.size()-1;
			bool altActive = _plot.ShowAlternativeMask() && _dataList[altMaskIndex].data.MaskCount()!=0;
			if(orActive && altActive)
			{
				data.SetMask(_dataList[0].data);
				data.JoinMask(_dataList[altMaskIndex].data);
			}
			else if(orActive)
				data.SetMask(_dataList[0].data);
			else if(altActive)
				data.SetMask(_dataList[altMaskIndex].data);
			else
				data.SetMasksToValue<false>();
		}
		HeatMapPlot _plot;
		bool _showPP, _showPQ, _showQP, _showQQ;
		size_t _visualizedImage;
		struct DataEntry
		{
			DataEntry(const std::string& _label, const TimeFrequencyData& _data)
			{ label = _label; data = _data; }
			std::string label;
			TimeFrequencyData data;
		};
		std::vector<DataEntry> _dataList;
		TimeFrequencyMetaDataCPtr _metaData;
		sigc::signal<void> _visualizationListChange;
};

#endif
