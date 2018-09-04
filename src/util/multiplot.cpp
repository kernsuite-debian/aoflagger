#include "../util/multiplot.h"

MultiPlot::MultiPlot(Plot2D &plot, size_t plotCount)
	:
	_legends(plotCount),
	_points(plotCount),
	_plotCount(plotCount),
	_plot(plot)
{ }

void MultiPlot::Finish()
{
	for(size_t i=0;i<_plotCount;++i)
	{
		if(!_points[i].empty())
		{
			_plot.StartLine(_legends[i], _xAxisText, _yAxisText, false, Plot2DPointSet::DrawPoints);
			PointList& list = _points[i];
			for(PointList::const_iterator p=list.begin();p!=list.end();++p)
			{
				_plot.PushDataPoint(p->x, p->y);
			}
			list.clear();
		}
	}
}

