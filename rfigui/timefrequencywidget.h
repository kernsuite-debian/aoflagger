#ifndef TIME_FREQUENCY_WIDGET_H
#define TIME_FREQUENCY_WIDGET_H

#include "heatmapwidget.h"

#include "../plot/plotwidget.h"
#include "../plot/plot2d.h"

#include <gtkmm/box.h>

class TimeFrequencyWidget : public Gtk::VBox {
 public:
  TimeFrequencyWidget(HeatMapPlot* plot) : _heatMap(plot) {
    _timePlotWidget.SetPlot(_timePlot);
    _timePlotWidget.set_size_request(200, 60);
    _timePlot.SetShowXAxis(false);
    pack_start(_timePlotWidget, Gtk::PACK_EXPAND_WIDGET);
    EnableTimePlot();

    _heatMap.Plot().SetShowTitle(true);
    _heatMap.set_size_request(200, 200);
    pack_start(_heatMap, Gtk::PACK_EXPAND_WIDGET);
    _heatMap.show();
  }

  void Update() {
    _heatMap.Update();
    _timePlotWidget.Update();
  }

  bool HasImage() const { return _heatMap.Plot().HasImage(); }

  HeatMapWidget& HeatMap() { return _heatMap; }

  void EnableTimePlot() {
    _timePlot.LinkHorizontally(_heatMap.Plot());
    _timePlotWidget.show();
    Update();
  }

  void DisableTimePlot() {
    _timePlotWidget.hide();
    _timePlot.UnlinkHorizontally();
  }

  Plot2D& TimePlot() { return _timePlot; }

 private:
  HeatMapWidget _heatMap;
  PlotWidget _timePlotWidget;
  Plot2D _timePlot;
};

#endif
