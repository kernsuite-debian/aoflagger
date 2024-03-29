#ifndef GUI_QUALITY__GRAYSCALEPLOTPAGE_H
#define GUI_QUALITY__GRAYSCALEPLOTPAGE_H

#include <gtkmm/radiotoolbutton.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/separatortoolitem.h>

#include "../plot/plotwidget.h"

#include "../quality/qualitytablesformatter.h"

#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"

#include "plotsheet.h"

class GrayScalePlotPage : public PlotSheet {
 public:
  explicit GrayScalePlotPage(class HeatMapPageController* controller);
  virtual ~GrayScalePlotPage();

  void InitializeToolbar(Gtk::Toolbar& toolbar) override final;

  bool NormalizeXAxis() const { return _normalizeXAxisButton.get_active(); }
  bool NormalizeYAxis() const { return _normalizeYAxisButton.get_active(); }

  void Redraw();

 protected:
  QualityTablesFormatter::StatisticKind getSelectedStatisticKind() const {
    return _selectStatisticKind;
  }

  void updateImage();

  PlotWidget& grayScaleWidget() { return _imageWidget; }

 private:
  void initStatisticKinds(Gtk::Toolbar& toolbar);
  void initPolarizations(Gtk::Toolbar& toolbar);
  void initPhaseButtons(Gtk::Toolbar& toolbar);
  void initPlotOptions(Gtk::Toolbar& toolbar);

  aocommon::PolarizationEnum getSelectedPolarization() const;
  enum TimeFrequencyData::ComplexRepresentation getSelectedPhase() const;

  void onSelectCount() {
    _selectStatisticKind = QualityTablesFormatter::CountStatistic;
    updateImage();
  }
  void onSelectMean() {
    _selectStatisticKind = QualityTablesFormatter::MeanStatistic;
    updateImage();
  }
  void onSelectStdDev() {
    _selectStatisticKind = QualityTablesFormatter::StandardDeviationStatistic;
    updateImage();
  }
  void onSelectDCount() {
    _selectStatisticKind = QualityTablesFormatter::DCountStatistic;
    updateImage();
  }
  void onSelectDMean() {
    _selectStatisticKind = QualityTablesFormatter::DMeanStatistic;
    updateImage();
  }
  void onSelectDStdDev() {
    _selectStatisticKind = QualityTablesFormatter::DStandardDeviationStatistic;
    updateImage();
  }
  void onSelectRFIPercentage() {
    _selectStatisticKind = QualityTablesFormatter::RFIPercentageStatistic;
    updateImage();
  }
  void onSelectSNR() {
    _selectStatisticKind = QualityTablesFormatter::SignalToNoiseStatistic;
    updateImage();
  }
  void onPropertiesClicked();

  void onSelectMinMaxRange();
  void onSelectWinsorizedRange();
  void onSelectSpecifiedRange();
  void onLogarithmicScaleClicked();
  void onNormalizeAxesButtonClicked() { updateImage(); }
  void onChangeNormMethod() {
    if (_normalizeYAxisButton.get_active()) updateImage();
  }

  class HeatMapPageController* _controller;

  Gtk::SeparatorToolItem _separator1, _separator2, _separator3, _separator4,
      _separator5, _separator6;

  Gtk::RadioButtonGroup _statisticGroup;
  Gtk::RadioToolButton _countButton, _meanButton, _stdDevButton, _dMeanButton,
      _dStdDevButton, _rfiPercentageButton;

  Gtk::RadioButtonGroup _polGroup;
  Gtk::RadioToolButton _polPPButton, _polPQButton, _polQPButton, _polQQButton,
      _polIButton;

  Gtk::RadioButtonGroup _phaseGroup;
  Gtk::RadioToolButton _amplitudePhaseButton, _phasePhaseButton,
      _realPhaseButton, _imaginaryPhaseButton;

  Gtk::RadioButtonGroup _rangeGroup;
  Gtk::RadioToolButton _rangeMinMaxButton, _rangeWinsorizedButton,
      _rangeSpecified;
  Gtk::ToggleToolButton _logarithmicScaleButton, _normalizeXAxisButton,
      _normalizeYAxisButton;
  Gtk::RadioButtonGroup _rangeTypeGroup;
  Gtk::RadioToolButton _meanNormButton, _winsorNormButton, _medianNormButton;
  Gtk::ToolButton _plotPropertiesButton;

  QualityTablesFormatter::StatisticKind _selectStatisticKind;
  PlotWidget _imageWidget;

  class ImagePropertiesWindow* _imagePropertiesWindow;
};

#endif
