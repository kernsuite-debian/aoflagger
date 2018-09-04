#ifndef HORIZONTALPLOTSCALE_H
#define HORIZONTALPLOTSCALE_H

#include <memory>
#include <string>
#include <vector>

#include <gtkmm/drawingarea.h>

class HorizontalPlotScale {
	public:
		HorizontalPlotScale();
		~HorizontalPlotScale();
		void SetPlotDimensions(double plotWidth, double plotHeight, double fromLeft, double fromTop, bool isSecondAxis)
		{
			_plotWidth = plotWidth;
			_plotHeight = plotHeight;
			_fromLeft = fromLeft;
			_fromTop = fromTop;
			_isSecondAxis = isSecondAxis;
			_metricsAreInitialized = false;
		}
		double GetHeight(const Cairo::RefPtr<Cairo::Context>& cairo);
		double GetRightMargin(const Cairo::RefPtr<Cairo::Context>& cairo);
		void Draw(const Cairo::RefPtr<Cairo::Context>& cairo);
		void InitializeNumericTicks(double min, double max);
		void InitializeTimeTicks(double timeMin, double timeMax);
		void InitializeTextTicks(const std::vector<std::string> &labels);
		void InitializeLogarithmicTicks(double min, double max);
		void SetDrawWithDescription(bool drawWithDescription)
		{
			_drawWithDescription = drawWithDescription;
			_metricsAreInitialized = false;
		}
		void SetUnitsCaption(const std::string &caption)
		{
			_unitsCaption = caption;
			_metricsAreInitialized = false;
		}
		void SetDescriptionFontSize(double fontSize)
		{
			_tickValuesFontSize = fontSize;
			_metricsAreInitialized = false;
		}
		void SetTickValuesFontSize(double fontSize)
		{
			_tickValuesFontSize = fontSize;
			_metricsAreInitialized = false;
		}
		void SetRotateUnits(bool rotate)
		{
			_rotateUnits = rotate;
			_metricsAreInitialized = false;
		}
		double UnitToAxis(double unitValue) const;
		double AxisToUnit(double axisValue) const;
	private:
		void drawDescription(const Cairo::RefPtr<Cairo::Context>& cairo);
		bool ticksFit(const Cairo::RefPtr<Cairo::Context>& cairo);
		void initializeMetrics(const Cairo::RefPtr<Cairo::Context>& cairo); 

		double _plotWidth, _plotHeight, _fromLeft, _fromTop;
		bool _metricsAreInitialized;
		double _height, _rightMargin;
		std::unique_ptr<class TickSet> _tickSet;
		bool _drawWithDescription;
		std::string _unitsCaption;
		double _descriptionFontSize;
		double _tickValuesFontSize;
		bool _rotateUnits, _isLogarithmic, _isSecondAxis;
};

#endif
