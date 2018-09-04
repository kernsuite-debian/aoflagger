#ifndef LEGEND_H
#define LEGEND_H

#include <cairomm/context.h>

class Plot2D;

class Legend
{
public:
	Legend();
	
	void Initialize(Cairo::RefPtr<Cairo::Context>& cairo, const Plot2D& plot);
	
	double Height() const { return _height; }
	double Width() const { return _width; }
	
	void SetPosition(double x, double y)
	{
		_x = x; _y = y;
	}
	
	void Draw(Cairo::RefPtr<Cairo::Context>& cairo, const Plot2D& plot) const;
	
private:
	double _sizeOfM, _xBearing, _yBearing, _textAdvance;
	double _x, _y, _width, _height;
};

#endif

