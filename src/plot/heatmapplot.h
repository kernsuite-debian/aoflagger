#ifndef HEAT_MAP_PLOT_H
#define HEAT_MAP_PLOT_H

#include <cairomm/context.h>

#include <sigc++/signal.h>

#include "../structures/image2d.h"
#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"
#include "../structures/segmentedimage.h"

class HeatMapPlot
{
public:
	enum TFMap { BWMap, InvertedMap, HotColdMap, RedBlueMap, RedYellowBlueMap, FireMap, BlackRedMap, ViridisMap };
	enum Range { MinMax, Winsorized, Specified };
	enum ScaleOption { NormalScale, LogScale, ZeroSymmetricScale };
	
	HeatMapPlot();
	~HeatMapPlot();
	
	void Clear();
	
	bool ShowOriginalMask() const { return _showOriginalMask; }
	void SetShowOriginalMask(bool newValue) { _showOriginalMask = newValue; }

	bool ShowAlternativeMask() const { return _showAlternativeMask; }
	void SetShowAlternativeMask(bool newValue) { _showAlternativeMask = newValue; }

	TFMap GetColorMap() const { return _colorMap; }
	void SetColorMap(TFMap colorMap) { _colorMap = colorMap; }
	
	void Draw(const Cairo::RefPtr<Cairo::Context>& cairo, unsigned width, unsigned height, bool isInvalidated); 

	Image2DCPtr Image() const { return _image; }
	void SetImage(Image2DCPtr image) { _image = image; }

	Mask2DCPtr OriginalMask() const { return _originalMask; }
	void SetOriginalMask(Mask2DCPtr mask) { _originalMask = mask; }

	Mask2DCPtr AlternativeMask() const { return _alternativeMask; }
	void SetAlternativeMask(Mask2DCPtr mask) { _alternativeMask = mask; }

	Mask2DCPtr GetActiveMask() const;

	void SetRange(enum Range range)
	{
		_range = range;
	}
	enum Range Range() const
	{
		return _range;
	}
	void SetScaleOption(ScaleOption option)
	{
		_scaleOption = option;
	}
	enum ScaleOption ScaleOption() const { return _scaleOption; }
	
	void ZoomFit();
	void ZoomIn();
	void ZoomInOn(size_t x, size_t y);
	void ZoomOut();
	void ZoomTo(size_t x1, size_t y1, size_t x2, size_t y2);
	void Pan(int xDisplacement, int yDisplacement);
	
	double StartHorizontal() const { return _startHorizontal; }
	double EndHorizontal() const { return _endHorizontal; }
	double StartVertical() const { return _startVertical; }
	double EndVertical() const { return _endVertical; }
	void SetSegmentedImage(SegmentedImageCPtr segmentedImage) { _segmentedImage = segmentedImage; }
	
	void SetHighlighting(bool newValue) { _highlighting = newValue; }
	class ThresholdConfig &HighlightConfig() { return *_highlightConfig; }

	bool HasImage() const { return _image != nullptr; }

	TimeFrequencyMetaDataCPtr GetSelectedMetaData() const;
	const TimeFrequencyMetaDataCPtr& GetFullMetaData() const { return _metaData; }
	void SetMetaData(TimeFrequencyMetaDataCPtr metaData) { _metaData = metaData; }
	
	num_t Max() const { return _max; }
	num_t Min() const { return _min; }
	
	void SetMax(num_t max) { _max = max; }
	void SetMin(num_t min) { _min = min; }
	
	void SaveByExtension(const std::string& filename, unsigned width, unsigned height);
	void SavePdf(const std::string &filename, unsigned width, unsigned height);
	void SaveSvg(const std::string &filename, unsigned width, unsigned height);
	void SavePng(const std::string &filename, unsigned width, unsigned height);
	void SaveText(const std::string &filename);
	
	bool ShowTitle() const { return _showTitle; }
	void SetShowTitle(bool showTitle) {
		_showTitle = showTitle;
	}
	
	bool ShowXYAxes() const { return _showXYAxes; }
	void SetShowXYAxes(bool showXYAxes)
	{
		_showXYAxes = showXYAxes;
	}
	
	bool ShowColorScale() const { return _showColorScale; }
	void SetShowColorScale(bool showColorScale)
	{
		_showColorScale = showColorScale;
	}
	
	bool ShowXAxisDescription() const { return _showXAxisDescription; }
	void SetShowXAxisDescription(bool showXAxisDescription)
	{
		_showXAxisDescription = showXAxisDescription;
	}
	
	bool ShowYAxisDescription() const { return _showYAxisDescription; }
	void SetShowYAxisDescription(bool showYAxisDescription)
	{
		_showYAxisDescription = showYAxisDescription;
	}
	
	bool ShowZAxisDescription() const { return _showZAxisDescription; }
	void SetShowZAxisDescription(bool showZAxisDescription)
	{
		_showZAxisDescription = showZAxisDescription;
	}
	
	void SetCairoFilter(Cairo::Filter filter)
	{
		_cairoFilter = filter;
	}
	Cairo::Filter CairoFilter() const { return _cairoFilter; }
	void SetTitleText(const std::string &title)
	{
		_titleText = title;
	}
	
	const std::string& XAxisDescription() const { return _xAxisDescription; }
	void SetXAxisDescription(const std::string &description)
	{
		_xAxisDescription = description;
	}
	const std::string& YAxisDescription() const { return _yAxisDescription; }
	void SetYAxisDescription(const std::string &description)
	{
		_yAxisDescription = description;
	}
	const std::string& ZAxisDescription() const { return _zAxisDescription; }
	void SetZAxisDescription(const std::string &description)
	{
		_zAxisDescription = description;
	}
	
	bool ManualTitle() const { return _manualTitle; }
	void SetManualTitle(bool manualTitle) { _manualTitle = manualTitle; }
	
	const std::string &ManualTitleText() {
		return _manualTitleText;
	}
	void SetManualTitleText(const std::string &manualTitle) {
		_manualTitleText = manualTitle;
	}
	
	bool ManualXAxisDescription() const { return _manualXAxisDescription; }
	void SetManualXAxisDescription(bool manualDesc)
	{
		_manualXAxisDescription = manualDesc;
	}
	bool ManualYAxisDescription() const { return _manualYAxisDescription; }
	void SetManualYAxisDescription(bool manualDesc)
	{
		_manualYAxisDescription = manualDesc;
	}
	bool ManualZAxisDescription() const { return _manualZAxisDescription; }
	void SetManualZAxisDescription(bool manualDesc)
	{
		_manualZAxisDescription = manualDesc;
	}
	
	bool IsZoomedOut() const {
		return
			_startHorizontal == 0.0 &&
			_endHorizontal == 1.0 &&
			_startVertical == 0.0 &&
			_endVertical == 1.0;
	}
	
	sigc::signal<void> &OnZoomChanged() { return _onZoomChanged; }

	bool ConvertToUnits(double mouseX, double mouseY, int &posX, int &posY) const;
	bool ConvertToScreen(int posX, int posY, double& mouseX, double& mouseY) const;
	
private:
	void redrawWithoutChanges(const Cairo::RefPtr<Cairo::Context>& cairo, unsigned width, unsigned height);

	bool _isInitialized;
	unsigned _initializedWidth, _initializedHeight;
	bool _showOriginalMask, _showAlternativeMask;
	enum TFMap _colorMap;
	TimeFrequencyMetaDataCPtr _metaData;
	Image2DCPtr _image;
	Mask2DCPtr _originalMask, _alternativeMask;
	bool _highlighting;
	double _leftBorderSize, _rightBorderSize, _topBorderSize, _bottomBorderSize;

	double _startHorizontal, _endHorizontal;
	double _startVertical, _endVertical;
	SegmentedImageCPtr _segmentedImage;
	std::unique_ptr<class HorizontalPlotScale> _horiScale;
	std::unique_ptr<class VerticalPlotScale> _vertScale;
	std::unique_ptr<class ColorScale> _colorScale;
	std::unique_ptr<class Title> _plotTitle;
	enum ScaleOption _scaleOption;
	bool _showXYAxes;
	bool _showColorScale;
	bool _showXAxisDescription;
	bool _showYAxisDescription;
	bool _showZAxisDescription;
	bool _showTitle;
	num_t _max, _min;
	std::string _titleText, _manualTitleText;
	enum Range _range;
	Cairo::Filter _cairoFilter;
	std::string _xAxisDescription, _yAxisDescription, _zAxisDescription;
	bool _manualTitle;
	bool _manualXAxisDescription;
	bool _manualYAxisDescription;
	bool _manualZAxisDescription;
	sigc::signal<void> _onZoomChanged;
	std::unique_ptr<class ThresholdConfig> _highlightConfig;
	
	void findMinMax(const Image2D* image, const Mask2D* mask, num_t& min, num_t& max);
	void update(const Cairo::RefPtr<Cairo::Context>& cairo, unsigned width, unsigned height);
	void downsampleImageBuffer(unsigned newWidth, unsigned newHeight);
	std::unique_ptr<class ColorMap> createColorMap();
	std::string actualTitleText() const
	{
		if(_manualTitle)
			return _manualTitleText;
		else
			return _titleText;
	}

	Cairo::RefPtr<Cairo::ImageSurface> _imageSurface;
};

#endif

