#ifndef GUI_PLOT_TITLE
#define GUI_PLOT_TITLE

#include <gtkmm/drawingarea.h>

class Title
{
	public:
		Title() : _metricsAreInitialized(false), _plotWidth(0.0), _plotHeight(0.0), _topMargin(0.0), _fontSize(16), _height(0.0)
		{
		}
		void SetPlotDimensions(double plotWidth, double plotHeight, double topMargin)
		{
			_plotWidth = plotWidth;
			_plotHeight = plotHeight;
			_topMargin = topMargin;
		}
		double GetHeight(Cairo::RefPtr<Cairo::Context> &cairo)
		{
			initializeMetrics(cairo);
			return _height;
		}
		void Draw(Cairo::RefPtr<Cairo::Context> &cairo);
		
		void SetText(const std::string &text)
		{
			_metricsAreInitialized = false;
			_text = text;
		}
		const std::string &Text() const { return _text; }
		
	private:
		void initializeMetrics(Cairo::RefPtr<Cairo::Context> &cairo);
		
		bool _metricsAreInitialized;
		double _plotWidth, _plotHeight, _topMargin;
		double _fontSize, _height;
		std::string _text;
};

#endif
