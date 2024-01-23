#ifndef TIME_FREQUENCY_WIDGET_H
#define TIME_FREQUENCY_WIDGET_H

#include "../rfigui/maskedheatmap.h"

#include "../plot/plotwidget.h"
#include "../plot/xyplot.h"

#include <gtkmm/box.h>

class TimeFrequencyWidget : public Gtk::VBox {
 public:
  explicit TimeFrequencyWidget(MaskedHeatMap& plot) : _heatMap() {
    _heatMap.SetPlot(plot);
    _timePlotWidget.SetPlot(_timePlot);
    _timePlotWidget.set_size_request(200, 60);
    _timePlot.XAxis().SetShow(false);
    pack_start(_timePlotWidget, Gtk::PACK_EXPAND_WIDGET);
    EnableTimePlot();

    GetMaskedHeatMap().SetShowTitle(true);
    _heatMap.set_size_request(200, 200);
    pack_start(_heatMap, Gtk::PACK_EXPAND_WIDGET);
    _heatMap.show();
  }

  void Update() {
    _heatMap.Update();
    _timePlotWidget.Update();
  }

  bool HasImage() const { return GetMaskedHeatMap().HasImage(); }

  PlotWidget& GetHeatMapWidget() { return _heatMap; }
  MaskedHeatMap& GetMaskedHeatMap() {
    return static_cast<MaskedHeatMap&>(_heatMap.Plot());
  }
  const MaskedHeatMap& GetMaskedHeatMap() const {
    return static_cast<const MaskedHeatMap&>(_heatMap.Plot());
  }

  void EnableTimePlot() {
    _timePlot.LinkHorizontally(_heatMap.Plot());
    _timePlotWidget.show();
    Update();
  }

  void DisableTimePlot() {
    _timePlotWidget.hide();
    _timePlot.UnlinkHorizontally();
  }

  XYPlot& TimePlot() { return _timePlot; }

 private:
  PlotWidget _heatMap;
  PlotWidget _timePlotWidget;
  XYPlot _timePlot;
};

#endif
