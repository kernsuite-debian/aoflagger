#ifndef IMAGECOMPARISONWIDGET_H
#define IMAGECOMPARISONWIDGET_H

#include <gtkmm/drawingarea.h>

#include <vector>

#include "../structures/image2d.h"
#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"
#include "../structures/segmentedimage.h"

#include "heatmapwidget.h"

class ImageComparisonWidget : public HeatMapWidget {
	public:
		ImageComparisonWidget(HeatMapPlot* plot);
	private:
};

#endif
